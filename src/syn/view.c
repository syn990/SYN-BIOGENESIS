/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of the SYN edition of SYN-BIOGENESIS, a C port of Biogenesis
 * (https://biogenesis.sourceforge.net/).
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version. It is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License (LICENSE) for more details.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "syn/syn.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "compat.h"
#include "sim/params.h"
#include "syn/batch.h"
#include "syn/looks.h"
#include "syn/palette.h"
#include "synos/synos.h"
#include "ui/gfx.h"
#include "ui/ui.h"

#define ZOOM_MIN 0.03
#define ZOOM_MAX 48.0
#define DRAG_SLOP 4 /* pixels the mouse may move and still click */
#define MAX_SEG 1024
#define MINIMAP 200

static const char *const look_names[LOOK_COUNT] = {"Classic", "Biological", "Galaxy",
                                                   "Digital (SYN-OS)"};
static const char *const look_keys[LOOK_COUNT] = {"classic", "biological", "galaxy", "digital"};

static struct {
    SDL_Renderer *R;
    Look look;
    bool minimap;
    bool keep_settings; /* false when the look came from the environment */
    double alpha;
    uint32_t snap; /* simulation frames remembered */
    uint32_t tick; /* drawings */
    uint64_t last_counter;
    double t, dt; /* animation clock and the last frame's length, seconds */

    /* easing the zoom with a point held under the cursor */
    bool zooming;
    double target_zoom;
    int anchor_sx, anchor_sy;
    double anchor_wx, anchor_wy;
    /* gliding the view's centre somewhere */
    bool flying;
    double fly_x, fly_y;
    /* dragging with the left or middle button, and the fling after */
    int press_btn;
    int press_x, press_y, last_mx, last_my;
    bool dragging;
    double move_vx, move_vy, last_move_t;
    double vel_x, vel_y; /* screen pixels per second */
    bool mini_drag;
} S;

/* ---- per-organism drawing state, by organism id ---- */

typedef struct {
    const Organism *o; /* NULL: empty slot */
    int id;
    uint32_t snap; /* the simulation frame the pose below is from */
    double px, py, pth;
    double gs; /* growth scale as drawn, eased towards the real one */
    double born_t; /* display time it was first drawn as a newborn, or -1 */
    uint32_t seen;
} Entry;

static Entry *tab;
static int tab_cap, tab_n;

static uint32_t id_hash(int id)
{
    uint32_t h = (uint32_t)id * 0x9e3779b1u;
    return h ^ (h >> 16);
}

static void tab_rebuild(int live_hint)
{
    int cap = 1024;
    while (cap < live_hint * 4)
        cap *= 2;
    Entry *old = tab;
    int old_cap = tab_cap;
    tab = calloc((size_t)cap, sizeof *tab);
    if (!tab)
        abort();
    tab_cap = cap;
    tab_n = 0;
    for (int i = 0; i < old_cap; i++) {
        Entry *e = &old[i];
        /* drop what hasn't been seen for a while (dead and gone) */
        if (!e->o || e->seen + 600 < S.tick)
            continue;
        uint32_t j = id_hash(e->id) & (uint32_t)(cap - 1);
        while (tab[j].o)
            j = (j + 1) & (uint32_t)(cap - 1);
        tab[j] = *e;
        tab_n++;
    }
    free(old);
}

static Entry *entry(const Organism *o, bool create)
{
    if (!tab) {
        if (!create)
            return NULL;
        tab_rebuild(0);
    }
    uint32_t mask = (uint32_t)(tab_cap - 1);
    for (uint32_t j = id_hash(o->id) & mask;; j = (j + 1) & mask) {
        Entry *e = &tab[j];
        if (!e->o)
            break;
        if (e->id == o->id) {
            if (e->o == o)
                return e;
            /* same id, another organism: a world opened from a file */
            e->o = o;
            e->snap = 0;
            e->gs = 0;
            return e;
        }
    }
    if (!create)
        return NULL;
    if ((tab_n + 1) * 2 > tab_cap) {
        tab_rebuild(tab_n + 1);
        mask = (uint32_t)(tab_cap - 1);
    }
    uint32_t j = id_hash(o->id) & mask;
    while (tab[j].o)
        j = (j + 1) & mask;
    tab[j] = (Entry){.o = o, .id = o->id, .seen = S.tick};
    tab_n++;
    return &tab[j];
}

void syn_world_reset(void)
{
    free(tab);
    tab = NULL;
    tab_cap = tab_n = 0;
    S.snap++;
}

void syn_before_step(const World *w)
{
    S.snap++;
    for (int i = 0; i < w->norgs; i++) {
        const Organism *o = w->orgs[i];
        Entry *e = entry(o, true);
        e->px = o->dcenter_x;
        e->py = o->dcenter_y;
        e->pth = o->theta;
        e->snap = S.snap;
        e->seen = S.tick;
    }
}

void syn_set_alpha(double alpha)
{
    S.alpha = alpha < 0 ? 0 : alpha > 1 ? 1 : alpha;
}

/* Where to draw an organism: between where it was before the last
 * simulation frame and where it is now. */
static void pose(const Organism *o, Entry *e, double *x, double *y, double *th)
{
    *x = o->dcenter_x;
    *y = o->dcenter_y;
    *th = o->theta;
    if (!e || e->snap != S.snap || S.alpha >= 1)
        return;
    double dx = o->dcenter_x - e->px, dy = o->dcenter_y - e->py, dth = o->theta - e->pth;
    /* a jump (moved by hand, wrapped round) is shown as it is */
    if (fabs(dx) > 40 || fabs(dy) > 40 || fabs(dth) > M_PI)
        return;
    double k = 1 - S.alpha;
    *x -= dx * k;
    *y -= dy * k;
    *th -= dth * k;
}

/* ---- settings ---- */

static void settings_path(char *out, size_t n)
{
    snprintf(out, n, "%s/syn.cfg", params_config_dir());
}

static void settings_load(void)
{
    char path[1100];
    settings_path(path, sizeof path);
    FILE *f = fopen(path, "r");
    if (!f)
        return;
    char line[128];
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (!strncmp(line, "look=", 5)) {
            for (int i = 0; i < LOOK_COUNT; i++)
                if (!strcmp(line + 5, look_keys[i]))
                    S.look = (Look)i;
            /* what Classic was called before Heritage named the interface */
            if (!strcmp(line + 5, "heritage"))
                S.look = LOOK_CLASSIC;
        } else if (!strncmp(line, "minimap=", 8)) {
            S.minimap = line[8] == '1';
        }
    }
    fclose(f);
}

static void settings_save(void)
{
    if (!S.keep_settings)
        return;
    const char *dir = params_config_dir();
    char parent[1024];
    snprintf(parent, sizeof parent, "%s", dir);
    char *slash = strrchr(parent, '/');
    if (slash) {
        *slash = '\0';
        mkdir(parent, 0755);
    }
    mkdir(dir, 0755);
    char path[1100];
    settings_path(path, sizeof path);
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    fprintf(f, "# SYN-BIOGENESIS, SYN edition\nlook=%s\nminimap=%d\n", look_keys[S.look],
            S.minimap ? 1 : 0);
    fclose(f);
}

/* ---- set-up ---- */

bool syn_init(SDL_Renderer *r)
{
    S.R = r;
    S.minimap = true;
    S.keep_settings = true;
    S.alpha = 1;
    S.last_counter = SDL_GetPerformanceCounter();
    settings_load();
    /* SYN_BIOGENESIS_LOOK picks a look for this run only (screenshots) */
    const char *env = getenv("SYN_BIOGENESIS_LOOK");
    for (int i = 0; env && i < LOOK_COUNT; i++) {
        if (!strcmp(env, look_keys[i])) {
            S.look = (Look)i;
            S.keep_settings = false;
        }
    }
    return batch_textures_init(r);
}

void syn_shutdown(void)
{
    settings_save();
    look_free();
    batch_textures_free();
    free(tab);
    tab = NULL;
    tab_cap = tab_n = 0;
}

Look syn_look(void)
{
    return S.look;
}

void syn_set_look(Look l)
{
    if (l < 0 || l >= LOOK_COUNT)
        return;
    S.look = l;
    S.keep_settings = true;
    settings_save();
}

const char *syn_look_name(Look l)
{
    return l >= 0 && l < LOOK_COUNT ? look_names[l] : "";
}

bool syn_minimap(void)
{
    return S.minimap;
}

void syn_set_minimap(bool on)
{
    S.minimap = on;
    settings_save();
}

bool syn_animating(void)
{
    return S.look != LOOK_CLASSIC || S.zooming || S.flying || S.press_btn || S.mini_drag ||
           S.vel_x != 0 || S.vel_y != 0;
}

/* ---- navigation ---- */

static double clamp_zoom(double z)
{
    z = z < ZOOM_MIN ? ZOOM_MIN : z > ZOOM_MAX ? ZOOM_MAX : z;
    /* settle on 1:1, where Classic's pixels are the original's */
    return fabs(z - 1.0) < 0.06 ? 1.0 : z;
}

static void view_centre(const WorldView *v, double *x, double *y)
{
    *x = v->cam_x + v->view.w / v->zoom / 2;
    *y = v->cam_y + v->view.h / v->zoom / 2;
}

/* Zoom towards target, holding the world point under (sx, sy) there. */
static void zoom_towards(WorldView *v, double target, int sx, int sy)
{
    S.target_zoom = clamp_zoom(target);
    S.anchor_sx = sx;
    S.anchor_sy = sy;
    wv_screen_to_world(v, sx, sy, &S.anchor_wx, &S.anchor_wy);
    S.zooming = S.target_zoom != v->zoom;
}

void syn_view_zoom(WorldView *v, const World *w, double factor)
{
    (void)w;
    double base = S.zooming ? S.target_zoom : v->zoom;
    zoom_towards(v, base * factor, v->view.x + v->view.w / 2, v->view.y + v->view.h / 2);
}

void syn_view_actual_size(WorldView *v, const World *w)
{
    (void)w;
    zoom_towards(v, 1.0, v->view.x + v->view.w / 2, v->view.y + v->view.h / 2);
}

void syn_view_fit(WorldView *v, const World *w)
{
    double z = fmin((double)v->view.w / w->width, (double)v->view.h / w->height) * 0.97;
    zoom_towards(v, z, v->view.x + v->view.w / 2, v->view.y + v->view.h / 2);
    S.target_zoom = z < ZOOM_MIN ? ZOOM_MIN : z > ZOOM_MAX ? ZOOM_MAX : z; /* no 1:1 snap */
    S.zooming = true;
    S.flying = true;
    S.fly_x = w->width / 2.0;
    S.fly_y = w->height / 2.0;
    S.vel_x = S.vel_y = 0;
}

static bool g_hud_off;
static int g_hud_bottom;

void syn_view_hud(bool minimap, int bottom)
{
    g_hud_off = !minimap;
    g_hud_bottom = bottom;
}

/* its title bar, over it */
#define MINIMAP_TITLE 22

static bool minimap_rect(const WorldView *v, const World *w, SDL_Rect *r)
{
    if (!S.minimap || g_hud_off || w->width <= 0 || w->height <= 0)
        return false;
    /* only when part of the world is out of view */
    if (v->view.w / v->zoom >= w->width && v->view.h / v->zoom >= w->height)
        return false;
    double k = (double)MINIMAP / (w->width > w->height ? w->width : w->height);
    int mw = (int)lround(w->width * k), mh = (int)lround(w->height * k);
    if (mw < 30)
        mw = 30;
    if (mh < 30)
        mh = 30;
    if (mw + 20 > v->view.w || mh + 20 + g_hud_bottom + MINIMAP_TITLE > v->view.h)
        return false;
    *r = (SDL_Rect){v->view.x + v->view.w - mw - 16, v->view.y + v->view.h - mh - 12 - g_hud_bottom,
                    mw, mh};
    return true;
}

/* Animated zoom, glide and fling, once a frame. */
static void animate(WorldView *v, const World *w)
{
    double k = 1 - exp(-S.dt * 14);
    if (S.flying) {
        double cx, cy;
        view_centre(v, &cx, &cy);
        cx += (S.fly_x - cx) * k;
        cy += (S.fly_y - cy) * k;
        if (fabs(S.fly_x - cx) * v->zoom < 0.5 && fabs(S.fly_y - cy) * v->zoom < 0.5) {
            cx = S.fly_x;
            cy = S.fly_y;
            S.flying = false;
        }
        S.anchor_sx = v->view.x + v->view.w / 2;
        S.anchor_sy = v->view.y + v->view.h / 2;
        S.anchor_wx = cx;
        S.anchor_wy = cy;
        if (!S.zooming)
            wv_center_on(v, w, cx, cy);
    }
    if (S.zooming) {
        double lz = log(v->zoom), lt = log(S.target_zoom);
        double z = fabs(lt - lz) < 0.003 ? S.target_zoom : exp(lz + (lt - lz) * k);
        v->zoom = z;
        v->cam_x = S.anchor_wx - (S.anchor_sx - v->view.x) / z;
        v->cam_y = S.anchor_wy - (S.anchor_sy - v->view.y) / z;
        if (z == S.target_zoom)
            S.zooming = false;
    }
    if (!S.press_btn && (S.vel_x != 0 || S.vel_y != 0)) {
        v->cam_x -= S.vel_x * S.dt / v->zoom;
        v->cam_y -= S.vel_y * S.dt / v->zoom;
        double f = exp(-S.dt * 4.5);
        S.vel_x *= f;
        S.vel_y *= f;
        if (hypot(S.vel_x, S.vel_y) < 8)
            S.vel_x = S.vel_y = 0;
    }
    double ox = v->cam_x, oy = v->cam_y;
    wv_clamp(v, w);
    /* a fling stops at the edge it runs into */
    if (v->cam_x != ox)
        S.vel_x = 0;
    if (v->cam_y != oy)
        S.vel_y = 0;
}

static void tick(void)
{
    uint64_t now = SDL_GetPerformanceCounter();
    S.dt = (double)(now - S.last_counter) / SDL_GetPerformanceFrequency();
    if (S.dt > 0.1)
        S.dt = 0.1;
    S.last_counter = now;
    /* screenshots: a steady clock, so the same scene draws the same */
    static int shot = -1;
    if (shot < 0)
        shot = getenv("SYN_BIOGENESIS_SHOT") != NULL;
    if (shot)
        S.dt = 1.0 / 60;
    S.t += S.dt;
}

void syn_view_idle(WorldView *v, const World *w)
{
    tick();
    animate(v, w);
}

bool syn_view_input(WorldView *v, const World *w, bool keys_allowed, int *cx, int *cy,
                    bool *dragged)
{
    tick();

    bool clicked = false;
    *dragged = false;
    int mx, my;
    ui_mouse(&mx, &my);
    SDL_Rect in = v->view;
    uint32_t cap = ui_id("synview", 0);
    SDL_Rect mm;
    bool mm_on = minimap_rect(v, w, &mm);

    if (mm_on && ui_pressed(mm, 1)) {
        S.mini_drag = true;
        S.flying = false;
        S.vel_x = S.vel_y = 0;
        ui_capture(cap);
    } else if (ui_pressed(in, 1) || ui_pressed(in, 2)) {
        S.press_btn = ui_pressed(in, 1) ? 1 : 2;
        S.press_x = S.last_mx = mx;
        S.press_y = S.last_my = my;
        S.dragging = false;
        S.move_vx = S.move_vy = 0;
        S.vel_x = S.vel_y = 0;
        S.flying = false;
        ui_capture(cap);
    }

    if (S.mini_drag) {
        if (ui_mouse_down(1)) {
            double wx = (mx - mm.x) * (double)w->width / mm.w;
            double wy = (my - mm.y) * (double)w->height / mm.h;
            wv_center_on(v, w, wx, wy);
            if (S.zooming)
                wv_screen_to_world(v, S.anchor_sx, S.anchor_sy, &S.anchor_wx, &S.anchor_wy);
            *dragged = true;
        } else {
            S.mini_drag = false;
        }
    }

    if (S.press_btn) {
        if (ui_mouse_down(S.press_btn)) {
            if (!S.dragging &&
                (abs(mx - S.press_x) > DRAG_SLOP || abs(my - S.press_y) > DRAG_SLOP))
                S.dragging = true;
            if (S.dragging) {
                int dx = mx - S.last_mx, dy = my - S.last_my;
                v->cam_x -= dx / v->zoom;
                v->cam_y -= dy / v->zoom;
                if (S.zooming)
                    wv_screen_to_world(v, S.anchor_sx, S.anchor_sy, &S.anchor_wx, &S.anchor_wy);
                if ((dx || dy) && S.dt > 0) {
                    S.move_vx = S.move_vx * 0.5 + dx / S.dt * 0.5;
                    S.move_vy = S.move_vy * 0.5 + dy / S.dt * 0.5;
                    S.last_move_t = S.t;
                }
                S.last_mx = mx;
                S.last_my = my;
                *dragged = true;
            }
        } else {
            if (!S.dragging && S.press_btn == 1) {
                clicked = true;
                *cx = S.press_x;
                *cy = S.press_y;
            }
            /* let go while moving: the view keeps sliding */
            if (S.dragging && S.t - S.last_move_t < 0.08) {
                S.vel_x = S.move_vx;
                S.vel_y = S.move_vy;
            }
            S.press_btn = 0;
            S.dragging = false;
        }
    }

    if (ui_hover(in)) {
        float wy = ui_wheel_fy(), wx = ui_wheel_fx();
        if (wy != 0 && (ui_mods() & KMOD_SHIFT)) {
            v->cam_x -= wy * 60 / v->zoom;
        } else if (wy != 0) {
            double base = S.zooming ? S.target_zoom : v->zoom;
            zoom_towards(v, base * pow(1.2, wy), mx, my);
            S.flying = false;
        }
        if (wx != 0)
            v->cam_x += wx * 60 / v->zoom;
    }

    if (keys_allowed) {
        const Uint8 *ks = SDL_GetKeyboardState(NULL);
        double step = 700 * S.dt / v->zoom;
        double kx = (ks[SDL_SCANCODE_RIGHT] ? 1 : 0) - (ks[SDL_SCANCODE_LEFT] ? 1 : 0);
        double ky = (ks[SDL_SCANCODE_DOWN] ? 1 : 0) - (ks[SDL_SCANCODE_UP] ? 1 : 0);
        if (kx || ky) {
            v->cam_x += kx * step;
            v->cam_y += ky * step;
            S.flying = false;
            *dragged = true;
        }
        if (ui_key_pressed(SDLK_PLUS) || ui_key_pressed(SDLK_EQUALS) ||
            ui_key_pressed(SDLK_KP_PLUS))
            syn_view_zoom(v, w, 1.25);
        if (ui_key_pressed(SDLK_MINUS) || ui_key_pressed(SDLK_KP_MINUS))
            syn_view_zoom(v, w, 0.8);
        if (ui_key_pressed(SDLK_0) || ui_key_pressed(SDLK_KP_0))
            syn_view_actual_size(v, w);
        if (ui_key_pressed(SDLK_HOME))
            syn_view_fit(v, w);
        if (ui_key_pressed(SDLK_m))
            syn_set_minimap(!S.minimap);
        if (ui_key_pressed(SDLK_l))
            syn_set_look((Look)((S.look + 1) % LOOK_COUNT));
    }
    animate(v, w);
    return clicked;
}

void syn_view_follow(WorldView *v, const World *w, const Organism *o)
{
    double x, y, th;
    pose(o, entry(o, false), &x, &y, &th);
    S.flying = false;
    S.vel_x = S.vel_y = 0;
    if (S.zooming) {
        /* zoom about the panel's centre, where the organism is */
        S.anchor_sx = v->view.x + v->view.w / 2;
        S.anchor_sy = v->view.y + v->view.h / 2;
        S.anchor_wx = x;
        S.anchor_wy = y;
    }
    wv_center_on(v, w, x, y);
}

/* ---- drawing ---- */

static float gx0[MAX_SEG], gy0[MAX_SEG], gx1[MAX_SEG], gy1[MAX_SEG];
static float sel_x0[MAX_SEG], sel_y0[MAX_SEG], sel_x1[MAX_SEG], sel_y1[MAX_SEG];
static uint8_t gcol[MAX_SEG], sel_col[MAX_SEG]; /* palette slots (syn/palette.h) */

/* The body as the genes draw it, unrounded (Organism.symmetric without
 * Math.round), scaled by gs, turned by th and placed at (cx, cy) on screen. */
static void body(const LookCtx *c, const Organism *o, double gs, double wx, double wy, double th,
                 DOrg *d)
{
    const GeneticCode *gc = o->gc;
    int n = o->segments < MAX_SEG ? o->segments : MAX_SEG;
    int sym = gc->symmetry > 0 ? gc->symmetry : 1, seq = o->segments / sym;
    double left = 0, right = 0, top = 0, bottom = 0, vth = 0;
    int s = 0;
#ifdef BIO_COLOUR
    /* Color Mod's: a gene starts at the centre, at the end of the segment
     * before it (branch -1), or at the end of segment `branch` of its arm,
     * turning from the direction of the segment before; eyes are drawn
     * longer. Its eye test reads the first arm's colours, which it turns
     * from EYE to VISION as it goes: arm 0's eyes with a segment after them
     * are lengthened, the other arms' not. */
    int ng = syn_genes(gc);
    for (int i = 0; i < sym && s < n; i++) {
        bool mirrored = !(gc->mirror == 0 || i % 2 == 0);
        for (int j = 0; j < seq && s < n; j++, s++) {
            const Gene *g = gc->genes[j < ng ? j : ng - 1];
            double vmod = g->length * gs;
            if (o->haseyes && (o->seg_color[j] == CM_EYE || (i == 0 && o->seg_color[j] == CM_VISION)))
                vmod *= o->hasgoodvision ? 2 : 1.5;
            double px = 0, py = 0;
            if (j == 0 || g->branch == 0) {
                vth = mirrored ? M_PI - (g->theta + (i - 1) * 2 * M_PI / sym)
                               : g->theta + i * 2 * M_PI / sym;
            } else {
                int from = g->branch == -1 ? s - 1 : i * seq + g->branch - 1;
                if (from < 0 || from >= s)
                    from = s - 1;
                px = gx1[from];
                py = gy1[from];
                vth += mirrored ? -g->theta : g->theta;
            }
            gx0[s] = (float)px;
            gy0[s] = (float)py;
            px += vmod * cos(vth);
            py += vmod * sin(vth);
            gx1[s] = (float)px;
            gy1[s] = (float)py;
            left = fmin(left, px);
            right = fmax(right, px);
            top = fmin(top, py);
            bottom = fmax(bottom, py);
        }
    }
#else
    for (int i = 0; i < sym && s < n; i++) {
        double px = 0, py = 0;
        for (int j = 0; j < seq && s < n; j++, s++) {
            const Gene *g = &gc->genes[j < gc->ngenes ? j : gc->ngenes - 1];
            if (j == 0) {
                if (gc->mirror == 0 || i % 2 == 0)
                    vth = g->theta + i * 2 * M_PI / sym;
                else
                    vth = M_PI - (g->theta + (i - 1) * 2 * M_PI / sym);
            } else {
                vth += (gc->mirror == 0 || i % 2 == 0) ? g->theta : -g->theta;
            }
            double vmod = g->length * gs;
            gx0[s] = (float)px;
            gy0[s] = (float)py;
            px += vmod * cos(vth);
            py += vmod * sin(vth);
            gx1[s] = (float)px;
            gy1[s] = (float)py;
            left = fmin(left, px);
            right = fmax(right, px);
            top = fmin(top, py);
            bottom = fmax(bottom, py);
        }
    }
#endif
    double ox = (left + right) / 2, oy = (top + bottom) / 2;
    double cs = cos(th), sn = sin(th), z = c->zoom;
    float scx = (float)((wx - c->cam_x) * z + c->in.x), scy = (float)((wy - c->cam_y) * z + c->in.y);
    double r2 = 0;
    int count[SYN_NCOL] = {0};
    for (int i = 0; i < n; i++) {
        double ax = gx0[i] - ox, ay = gy0[i] - oy, bx = gx1[i] - ox, by = gy1[i] - oy;
        r2 = fmax(r2, fmax(ax * ax + ay * ay, bx * bx + by * by));
        gx0[i] = scx + (float)((ax * cs - ay * sn) * z);
        gy0[i] = scy + (float)((ax * sn + ay * cs) * z);
        gx1[i] = scx + (float)((bx * cs - by * sn) * z);
        gy1[i] = scy + (float)((bx * sn + by * cs) * z);
        int slot = syn_seg(o, i);
        gcol[i] = (uint8_t)slot;
        if (slot >= 0 && slot < SYN_NCOL)
            count[slot]++;
    }
    int best = 0;
    for (int k = 0; k < SYN_NCOL; k++)
        if (count[k] > count[best])
            best = k;
    *d = (DOrg){o, scx, scy, (float)(sqrt(r2) * z), n, gx0, gy0, gx1, gy1, gcol, (uint8_t)best, 0};
}

static void draw_minimap(const WorldView *v, const World *w)
{
    SDL_Rect mm;
    if (!minimap_rect(v, w, &mm))
        return;
    SDL_Renderer *R = S.R;
    /* a window of its own, as labwc draws one: its title bar, then the map */
    UIRect frame = {(float)mm.x - 4, (float)mm.y - 4 - MINIMAP_TITLE, (float)mm.w + 8,
                    (float)mm.h + 8 + MINIMAP_TITLE};
    skin_panel(frame, true);
    UIRect tb = {frame.x + 1, frame.y + 1, frame.w - 2, MINIMAP_TITLE - 1};
    Rgb tc = skin_titlebar(tb);
    const Fonts *fo = synos_fonts();
    char size[48];
    snprintf(size, sizeof size, "%d x %d", w->width, w->height);
    float ty = tb.y + (tb.h - skin_text_height(fo->mono_small_bold)) * 0.5f;
    skin_text_rgb(fo->mono_small_bold, "MAP", tb.x + 8, ty, tc.r, tc.g, tc.b, ALIGN_LEFT);
    skin_text_rgb(fo->mono_small, size, tb.x + tb.w - 8, ty, tc.r, tc.g, tc.b, ALIGN_RIGHT);
    gfx_fill((SDL_Rect){mm.x, mm.y, mm.w, mm.h}, (Col){0, 0, 0, 225});
    static SDL_FPoint *pts[SYN_NCOL];
    static int cap[SYN_NCOL];
    int np[SYN_NCOL] = {0};
    double kx = (double)mm.w / w->width, ky = (double)mm.h / w->height;
    for (int i = 0; i < w->norgs; i++) {
        const Organism *o = w->orgs[i];
        int c = SYN_DEAD;
        if (o->alive) {
            int count[SYN_NCOL] = {0};
            for (int s = 0; s < o->segments; s++) {
                int slot = syn_seg(o, s);
                if (slot >= 0 && slot < SYN_NCOL)
                    count[slot]++;
            }
            c = 0;
            for (int k = 0; k < SYN_NCOL; k++)
                if (count[k] > count[c])
                    c = k;
        }
        if (np[c] == cap[c]) {
            cap[c] = cap[c] ? cap[c] * 2 : 256;
            SDL_FPoint *p = realloc(pts[c], (size_t)cap[c] * sizeof *p);
            if (!p)
                abort();
            pts[c] = p;
        }
        pts[c][np[c]++] = (SDL_FPoint){(float)(mm.x + o->dcenter_x * kx), (float)(mm.y + o->dcenter_y * ky)};
    }
    for (int c = 0; c < SYN_NCOL; c++) {
        if (!np[c])
            continue;
        SDL_Color k = look_color(S.look, c);
        SDL_SetRenderDrawColor(R, k.r, k.g, k.b, c == SYN_DEAD ? 120 : 255);
        SDL_RenderDrawPointsF(R, pts[c], np[c]);
    }
    /* the view */
    double x0 = fmax(v->cam_x, 0), y0 = fmax(v->cam_y, 0);
    double x1 = fmin(v->cam_x + v->view.w / v->zoom, w->width);
    double y1 = fmin(v->cam_y + v->view.h / v->zoom, w->height);
    SDL_Rect r = {mm.x + (int)floor(x0 * kx), mm.y + (int)floor(y0 * ky),
                  (int)ceil((x1 - x0) * kx), (int)ceil((y1 - y0) * ky)};
    if (r.w < 2)
        r.w = 2;
    if (r.h < 2)
        r.h = 2;
    gfx_frame(r, C_ACCENT);
}

void syn_view_draw(WorldView *v, const World *w, const Organism *selected)
{
    S.tick++;
    if (S.look == LOOK_CLASSIC) {
        wv_draw(v, w, selected);
        gfx_clip_push(v->view);
        draw_minimap(v, w);
        gfx_clip_pop();
        return;
    }

    LookCtx c = {S.R, v->view, v->cam_x, v->cam_y, v->zoom, S.t, w};
    gfx_clip_push(v->view);
    look_begin(S.look, &c);

    /* what is in view, with room for glows */
    double m = 8 + 24 / v->zoom;
    double vx0 = v->cam_x - m, vy0 = v->cam_y - m;
    double vx1 = v->cam_x + v->view.w / v->zoom + m, vy1 = v->cam_y + v->view.h / v->zoom + m;
    double ease = 1 - exp(-S.dt * 7);
    DOrg sel = {0};
    bool have_sel = false;
    for (int i = 0; i < w->norgs; i++) {
        Organism *o = w->orgs[i];
        if (o->r.x > vx1 || o->r.y > vy1 || o->r.x + o->r.width < vx0 || o->r.y + o->r.height < vy0)
            continue;
        if (!o->gc || syn_genes(o->gc) <= 0 || o->segments <= 0)
            continue;
        Entry *e = entry(o, true);
        e->seen = S.tick;
        double target = syn_growth_scale(o);
        if (e->gs <= 0) {
            /* newborns swell out from a point; everything else appears as it is */
            bool newborn = o->alive && o->age < 2;
            e->gs = newborn ? target * 0.25 : target;
            e->born_t = newborn ? S.t : -1;
        } else {
            e->gs += (target - e->gs) * ease;
        }
        double x, y, th;
        pose(o, e, &x, &y, &th);
        DOrg d;
        body(&c, o, e->gs, x, y, th, &d);
        d.fresh = e->born_t >= 0 ? (float)fmax(0, 1 - (S.t - e->born_t) / 0.6) : 0;
        look_org(S.look, &c, &d);
        if (o == selected) {
            memcpy(sel_x0, d.x0, sizeof(float) * (size_t)d.n);
            memcpy(sel_y0, d.y0, sizeof(float) * (size_t)d.n);
            memcpy(sel_x1, d.x1, sizeof(float) * (size_t)d.n);
            memcpy(sel_y1, d.y1, sizeof(float) * (size_t)d.n);
            memcpy(sel_col, d.col, (size_t)d.n);
            sel = d;
            sel.col = sel_col;
            sel.x0 = sel_x0;
            sel.y0 = sel_y0;
            sel.x1 = sel_x1;
            sel.y1 = sel_y1;
            have_sel = true;
        }
#ifndef BIO_COLOUR
        /* as Organism.draw: the flash lasts ten drawings (Color Mod counts
         * it down as it moves) */
        if (o->frames_color > 0)
            o->frames_color--;
#endif
    }
    look_end(S.look, &c, have_sel ? &sel : NULL);
    draw_minimap(v, w);
    gfx_clip_pop();
}
