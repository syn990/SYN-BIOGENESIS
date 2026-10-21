/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of the SYN-OS interface SYN-BIOGENESIS shares with SYN-TREK and
 * SYN-DEBT; the main menu and options screens follow SYN-DEBT's
 * src/screens/main_menu.c and options.c.
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

#include "synos/synos.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "compat.h"
#include "sim/params.h"
#include "syn/syn.h"
#include "ui/gfx.h"
#include "ui/ui.h"

#define MENU_FADE_MS 1500

static struct {
    SDL_Renderer *R;
    SDL_Window *win;
    Fonts fonts;
    bool fonts_ok;
    ThemeList themes;
    int theme;
    bool theme_chosen; /* else it follows the theme SYN-OS has on */
    bool active;
    bool fullscreen;
    bool keep; /* save the settings (not when they came from the environment) */
    SynosScreen screen;
    Uint32 fade_start, fade_ms;
    float options_scroll;
    const char *title, *subtitle;
    Col heritage[11]; /* the classic palette, restored for the Heritage edition */
} G;

/* ---- settings ---- */

static void settings_path(char *out, size_t n)
{
    snprintf(out, n, "%s/synos.cfg", params_config_dir());
}

static void settings_save(void)
{
    if (!G.keep)
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
    fprintf(f, "# SYN-BIOGENESIS: the SYN-OS interface\ninterface=%s\nfullscreen=%d\n",
            G.active ? "synos" : "heritage", G.fullscreen ? 1 : 0);
    if (G.theme_chosen && G.themes.count)
        fprintf(f, "theme=%s\n", G.themes.items[G.theme].name);
    fclose(f);
}

/* ---- the classic widgets take the theme's colours ---- */

static GFX_PALETTE Col *const k_palette[11] = {&C_BG,     &C_PANEL, &C_PANEL2, &C_BORDER,
                                              &C_TEXT,   &C_TEXT_DIM, &C_ACCENT, &C_HOVER,
                                              &C_PRESS,  &C_FIELD, &C_DISABLED};

static Col col_of(SDL_Color c)
{
    return (Col){c.r, c.g, c.b, 255};
}

static Col rgb_col(Rgb c)
{
    return (Col){c.r, c.g, c.b, 255};
}

static void apply_palette(void)
{
    if (!G.active || !G.themes.count) {
        for (int i = 0; i < 11; i++)
            *k_palette[i] = G.heritage[i];
        return;
    }
    const Theme *t = &G.themes.items[G.theme];
    C_BG = rgb_col(t->bg);
    C_PANEL = rgb_col(t->panel);
    C_PANEL2 = rgb_col(t->bg_alt);
    C_BORDER = rgb_col(t->border);
    C_TEXT = col_of(skin_tone_color(TONE_TEXT));
    C_TEXT_DIM = col_of(skin_tone_color(TONE_MUTED));
    C_ACCENT = col_of(skin_tone_color(TONE_ACCENT));
    C_HOVER = rgb_col(t->panel_hover);
    C_PRESS = rgb_col(t->accent_dim);
    C_FIELD = rgb_col(t->light ? (Rgb){0xff, 0xff, 0xff} : t->bg);
    C_DISABLED = col_of(skin_tone_color(TONE_MUTED));
}

/* ---- set-up ---- */

bool synos_init(SDL_Renderer *r, SDL_Window *w, const char *title, const char *subtitle)
{
    G.R = r;
    G.win = w;
    G.title = title;
    G.subtitle = subtitle;
    G.keep = true;
    G.screen = SYNOS_MENU;
    G.fade_start = SDL_GetTicks();
    G.fade_ms = MENU_FADE_MS;
    for (int i = 0; i < 11; i++)
        G.heritage[i] = *k_palette[i];
    G.fonts_ok = skin_init(r, gfx_asset("fonts/NotoSans-Regular.ttf"), &G.fonts);
    const char *dir = gfx_asset("themes");
    if (dir)
        theme_load_dir(dir, &G.themes);

    /* the saved choices, then the environment for this run only */
    char want[160] = "";
    char path[1100];
    settings_path(path, sizeof path);
    FILE *f = fopen(path, "r");
    bool have_interface = false;
    if (f) {
        char line[160];
        while (fgets(line, sizeof line, f)) {
            line[strcspn(line, "\r\n")] = '\0';
            if (!strncmp(line, "interface=", 10)) {
                G.active = !strcmp(line + 10, "synos");
                have_interface = true;
            } else if (!strncmp(line, "theme=", 6)) {
                snprintf(want, sizeof want, "%s", line + 6);
                G.theme_chosen = true;
            } else if (!strncmp(line, "fullscreen=", 11)) {
                G.fullscreen = line[11] == '1';
            }
        }
        fclose(f);
    }
    if (!have_interface)
        G.active = false; /* the first run is the classic window */
    const char *env = getenv("SYN_BIOGENESIS_INTERFACE");
    if (env && (!strcmp(env, "synos") || !strcmp(env, "heritage"))) {
        G.active = !strcmp(env, "synos");
        G.keep = false;
    }
    env = getenv("SYN_BIOGENESIS_THEME");
    if (env && *env) {
        snprintf(want, sizeof want, "%s", env);
        G.keep = false;
    }
    /* as SYN-DEBT: the setting, else the theme SYN-OS has on, else SYN-OS-RED */
    if (!want[0] && !theme_system_current(want, (int)sizeof want))
        snprintf(want, sizeof want, "SYN-OS-RED");
    int i = theme_find(&G.themes, want);
    if (i < 0) {
        if (G.themes.count)
            fprintf(stderr, "SYN-OS: no theme called %s; using SYN-OS-RED\n", want);
        i = theme_find(&G.themes, "SYN-OS-RED");
    }
    G.theme = i < 0 ? 0 : i;
    if (G.themes.count)
        skin_set_theme(&G.themes.items[G.theme]);
    apply_palette();
    if (G.fullscreen && G.win)
        SDL_SetWindowFullscreen(G.win, SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!G.active)
        G.screen = SYNOS_WORLD;
    return G.fonts_ok && G.themes.count > 0;
}

/* (the settings are saved as they change, so a theme never chosen goes on
 * following SYN-OS's) */
void synos_shutdown(void)
{
    skin_shutdown(&G.fonts);
    theme_list_free(&G.themes);
}

/* ---- the simulation's speed ---- */

/* milliseconds a frame, slowest first: 1 to 500 frames a second, then as
 * fast as it goes */
static const int k_delays[] = {1000, 500, 200, 100, 50, 33, 20, 10, 5, 2, 1};
#define N_DELAYS ((int)(sizeof k_delays / sizeof k_delays[0]))

/* the step nearest a delay */
static int speed_step(int delay)
{
    double d = log(delay > 1 ? (double)delay : 1.0);
    int best = 0;
    for (int i = 1; i < N_DELAYS; i++)
        if (fabs(log((double)k_delays[i]) - d) < fabs(log((double)k_delays[best]) - d))
            best = i;
    return best;
}

float synos_speed(float right, float y, bool draw)
{
    const Fonts *fo = &G.fonts;
    const float bar_w = 84, gap = 10;
    float tw = fmaxf(skin_text_width(fo->small, "SPEED"), skin_text_width(fo->bold, "500/s"));
    float w = tw + gap + bar_w;
    if (!draw)
        return w;
    float x = right - w, th = skin_text_height(fo->small) - 1 + skin_text_height(fo->bold);
    int step = speed_step(P.delay);
    char value[16];
    if (P.delay <= 1)
        snprintf(value, sizeof value, "MAX");
    else
        snprintf(value, sizeof value, "%d/s", (int)lround(1000.0 / P.delay));
    skin_text(fo->small, "SPEED", x, y, TONE_MUTED, ALIGN_LEFT);
    skin_text(fo->bold, value, x, y + skin_text_height(fo->small) - 1, TONE_TEXT, ALIGN_LEFT);

    /* the bar: slowest at the left; scrolled over, clicked or dragged */
    UIRect bar = {x + tw + gap, y + th / 2 - 3, bar_w, 6};
    SDL_Rect all = {(int)x, (int)y - 4, (int)w, (int)th + 8};
    SDL_Rect grip = {(int)bar.x - 4, (int)y - 4, (int)bar_w + 8, (int)th + 8};
    uint32_t id = ui_id("synspeed", 0);
    int to = step;
    /* a step a notch; a touchpad's fractions add up to one */
    static float wheel;
    if (ui_hover(all)) {
        wheel += ui_wheel_fy();
        for (; wheel >= 1; wheel -= 1)
            to++;
        for (; wheel <= -1; wheel += 1)
            to--;
    } else {
        wheel = 0;
    }
    if (ui_pressed(grip, 1))
        ui_capture(id);
    if (ui_captured(id)) {
        if (ui_mouse_down(1)) {
            int mx, my;
            ui_mouse(&mx, &my);
            to = (int)lround(((float)mx - bar.x) / bar.w * (float)(N_DELAYS - 1));
        } else {
            ui_release();
        }
    }
    to = to < 0 ? 0 : to >= N_DELAYS ? N_DELAYS - 1 : to;
    if (to != step) {
        P.delay = k_delays[to];
        step = to;
    }
    skin_meter(bar, (double)step / (N_DELAYS - 1), TONE_ACCENT);
    Rgb ac = skin_theme()->accent;
    skin_fill(ui_rect(bar.x + bar.w * (float)step / (N_DELAYS - 1) - 2, bar.y - 4, 4, bar.h + 8), ac, 1);
    if (ui_hover(all)) {
        char tip[200];
        if (P.delay <= 1)
            snprintf(tip, sizeof tip, "As fast as the computer allows. Scroll or drag to slow it down");
        else
            snprintf(tip, sizeof tip, "Up to %d frames a second (%d ms each). Scroll or drag to change it",
                     (int)lround(1000.0 / P.delay), P.delay);
        ui_tooltip(tip);
    }
    return w;
}

void synos_end_frame(void)
{
    skin_end_frame();
}

bool synos_active(void)
{
    return G.active && G.fonts_ok && G.themes.count;
}

void synos_set_active(bool on)
{
    G.active = on;
    G.keep = true;
    apply_palette();
    settings_save();
    if (!on)
        G.screen = SYNOS_WORLD;
}

const Fonts *synos_fonts(void)
{
    return &G.fonts;
}

int synos_theme_count(void)
{
    return G.themes.count;
}

const Theme *synos_theme_at(int i)
{
    return i >= 0 && i < G.themes.count ? &G.themes.items[i] : NULL;
}

int synos_theme_index(void)
{
    return G.theme;
}

void synos_set_theme(int i)
{
    if (i < 0 || i >= G.themes.count)
        return;
    G.theme = i;
    G.theme_chosen = true;
    G.keep = true;
    skin_set_theme(&G.themes.items[i]);
    apply_palette();
    settings_save();
}

bool synos_fullscreen(void)
{
    return G.fullscreen;
}

void synos_set_fullscreen(bool on)
{
    G.fullscreen = on;
    G.keep = true;
    if (G.win)
        SDL_SetWindowFullscreen(G.win, on ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    settings_save();
}

SynosScreen synos_screen(void)
{
    return G.screen;
}

void synos_set_screen(SynosScreen s)
{
    /* the slow fade is the arrival; coming back from the world is quick */
    if (s == SYNOS_MENU && G.screen != SYNOS_MENU) {
        G.fade_start = SDL_GetTicks();
        G.fade_ms = G.screen == SYNOS_WORLD ? 200 : 0;
    }
    if (s == SYNOS_OPTIONS)
        G.options_scroll = 0;
    G.screen = s;
}

/* ---- the main menu ---- */

static void shade(int w, int h, float a)
{
    skin_fill(ui_rect(0, 0, (float)w, (float)h), (Rgb){0, 0, 0}, a);
}

SynosAction synos_menu_frame(int w, int h, bool can_continue)
{
    const Fonts *fo = &G.fonts;
    int mx, my;
    ui_mouse(&mx, &my);

    /* SYN-TREK's grid: 260px buttons, 48 tall, 30 across and 12 down,
     * starting 40% of the way down; ARENA and LIBRARY, then TELESCOPE
     * (Biogenesis 0.9's only) and QUIT */
    float btn_h = 48.0f, gap_x = 30.0f, gap_y = 12.0f;
    float total_w = 260.0f * 2.0f + gap_x;
    int rows = 4;
    float total_h = btn_h * (float)rows + gap_y * (float)(rows - 1);
    UIRect area = ui_rect(((float)w - total_w) * 0.5f, (float)h * 0.40f, total_w, total_h);
    struct {
        UIRect r;
        const char *label;
        SynosAction act;
        bool enabled;
    } b[] = {
        {ui_grid_cell(area, 2, rows, 0, 0, gap_x, gap_y), "CONTINUE", SYNOS_CONTINUE, can_continue},
        {ui_grid_cell(area, 2, rows, 1, 0, gap_x, gap_y), "NEW WORLD", SYNOS_NEW_WORLD, true},
        {ui_grid_cell(area, 2, rows, 0, 1, gap_x, gap_y), "LOAD WORLD", SYNOS_LOAD_WORLD, true},
        {ui_grid_cell(area, 2, rows, 1, 1, gap_x, gap_y), "OPTIONS", SYNOS_OPEN_OPTIONS, true},
        {ui_grid_cell(area, 2, rows, 0, 2, gap_x, gap_y), "ARENA", SYNOS_OPEN_ARENA, true},
        {ui_grid_cell(area, 2, rows, 1, 2, gap_x, gap_y), "LIBRARY", SYNOS_OPEN_LIBRARY, true},
#ifdef BIO_COLOUR
        {ui_grid_cell(area, 2, rows, 0, 3, gap_x, gap_y), "TELESCOPE", SYNOS_OPEN_TELESCOPE, false},
#else
        {ui_grid_cell(area, 2, rows, 0, 3, gap_x, gap_y), "TELESCOPE", SYNOS_OPEN_TELESCOPE, true},
#endif
        {ui_grid_cell(area, 2, rows, 1, 3, gap_x, gap_y), "QUIT", SYNOS_QUIT, true},
    };
    int nb = (int)(sizeof b / sizeof b[0]);

    /* a light shade over the living world: the point is to see it */
    shade(w, h, 0.34f);
    /* the arrival: everything fades up from black (not in screenshots) */
    float fade = G.fade_ms ? (float)(SDL_GetTicks() - G.fade_start) / (float)G.fade_ms : 1.0f;
    if (fade > 1.0f || getenv("SYN_BIOGENESIS_SHOT"))
        fade = 1.0f;
    if (fade < 1.0f)
        shade(w, h, 1.0f - fade);
    skin_set_alpha(fade);

    float y = (float)h * 0.14f;
    skin_text_rgb(fo->title, G.title, (float)w * 0.5f, y, 235, 235, 235, ALIGN_CENTER);
    y += skin_text_height(fo->title) + 10.0f;
    skin_text_rgb(fo->button, G.subtitle, (float)w * 0.5f, y, 170, 170, 170, ALIGN_CENTER);

    SynosAction act = SYNOS_NONE;
    for (int i = 0; i < nb; i++) {
        bool hov = b[i].enabled && skin_hit(b[i].r, mx, my);
        skin_button(b[i].r, fo->button, b[i].label,
                    (WidgetState){.hover = hov, .disabled = !b[i].enabled}, true);
        if (hov && ui_pressed((SDL_Rect){(int)b[i].r.x, (int)b[i].r.y, (int)b[i].r.w, (int)b[i].r.h}, 1))
            act = b[i].act;
    }
    bool keys = !ui_any_modal() && !ui_text_focused();
    if (keys && (ui_key_pressed(SDLK_RETURN) || ui_key_pressed(SDLK_KP_ENTER)))
        act = can_continue ? SYNOS_CONTINUE : SYNOS_NEW_WORLD;
    if (keys && ui_key_pressed(SDLK_ESCAPE) && can_continue)
        act = SYNOS_CONTINUE;

    /* which theme is on, bottom left, quietly */
    const Theme *t = skin_theme();
    skin_text_rgb(fo->small, t->name, 16.0f, (float)h - skin_text_height(fo->small) - 12.0f, 150,
                  150, 150, ALIGN_LEFT);
    skin_set_alpha(1.0f);
    return act;
}

/* ---- options ---- */

static const struct {
    const char *group;
    const char *title;
    const char *blurb;
} FAMILIES[] = {
    {"vanilla", "VANILLA", "Flat panels, an accent bar"},
    {"bevel", "BEVEL", "Gradients and lit edges"},
    {"flatline", "FLATLINE", "No boxes, just text"},
    {"halo", "HALO", "Outlines that glow"},
    {"slab", "SLAB", "Heavy borders"},
};
#define FAMILY_COUNT (int)(sizeof FAMILIES / sizeof FAMILIES[0])

static bool clicked_in(UIRect r, int mx, int my)
{
    return skin_hit(r, mx, my) &&
           ui_pressed((SDL_Rect){(int)r.x, (int)r.y, (int)ceilf(r.w), (int)ceilf(r.h)}, 1);
}

SynosAction synos_options_frame(int w, int h)
{
    const Fonts *fo = &G.fonts;
    int mx, my;
    ui_mouse(&mx, &my);
    if (!ui_any_modal() && ui_key_pressed(SDLK_ESCAPE)) {
        synos_set_screen(SYNOS_MENU);
        return SYNOS_NONE;
    }

    /* a panel over the world, like SYN-TREK's Legends board */
    float pw = (float)w * 0.86f, ph = (float)h * 0.84f;
    if (pw > 1500)
        pw = 1500;
    /* small windows: all the room there is, so the names fit */
    if (w < 1400)
        pw = (float)w - 48.0f;
    if (h < 900)
        ph = (float)h - 48.0f;
    UIRect panel = {((float)w - pw) * 0.5f, ((float)h - ph) * 0.5f, pw, ph};
    float pad = 24.0f;
    UIRect back = {panel.x + panel.w - pad - 140.0f, panel.y + 18.0f, 140.0f, 40.0f};
    UIRect full = {back.x - 12.0f - 220.0f, back.y, 220.0f, 40.0f};

    shade(w, h, 0.45f);
    skin_panel(panel, true);
    skin_text(fo->title, "OPTIONS", panel.x + pad, panel.y + 8.0f, TONE_TEXT, ALIGN_LEFT);
    skin_button(back, fo->bold, "BACK", (WidgetState){.hover = skin_hit(back, mx, my)}, true);
    skin_button(full, fo->bold, G.fullscreen ? "FULLSCREEN: ON" : "FULLSCREEN: OFF",
                (WidgetState){.hover = skin_hit(full, mx, my), .active = G.fullscreen}, true);

    float y = panel.y + 8.0f + skin_text_height(fo->title) + 10.0f;
    /* the interface, the look of the world, the minimap */
    float bw = 200.0f, bh = 38.0f, gap = 10.0f;
    skin_text(fo->heading, "INTERFACE", panel.x + pad, y, TONE_TEXT, ALIGN_LEFT);
    UIRect r_synos = {panel.x + pad + 160.0f, y - 6.0f, bw, bh};
    UIRect r_her = {r_synos.x + bw + gap, r_synos.y, bw, bh};
    skin_button(r_synos, fo->bold, "SYN-OS",
                (WidgetState){.hover = skin_hit(r_synos, mx, my), .active = G.active}, true);
    skin_button(r_her, fo->bold, "HERITAGE EDITION",
                (WidgetState){.hover = skin_hit(r_her, mx, my), .active = !G.active}, true);
    UIRect r_mini = {r_her.x + bw + gap * 3, r_her.y, bw * 0.8f, bh};
    skin_button(r_mini, fo->bold, syn_minimap() ? "MINIMAP: ON" : "MINIMAP: OFF",
                (WidgetState){.hover = skin_hit(r_mini, mx, my), .active = syn_minimap()}, true);
    y += bh + 8.0f;
    /* the simulation: the other is another program in this one */
#ifdef BIO_COLOUR
    bool colour = true;
#else
    bool colour = false;
#endif
    skin_text(fo->heading, "SIMULATION", panel.x + pad, y, TONE_TEXT, ALIGN_LEFT);
    UIRect r_bio = {panel.x + pad + 160.0f, y - 6.0f, bw, bh};
    UIRect r_col = {r_bio.x + bw + gap, r_bio.y, bw, bh};
    skin_button(r_bio, fo->bold, "BIOGENESIS 0.9",
                (WidgetState){.hover = skin_hit(r_bio, mx, my), .active = !colour}, true);
    skin_button(r_col, fo->bold, "COLOR MOD 2.01",
                (WidgetState){.hover = skin_hit(r_col, mx, my), .active = colour}, true);
    y += bh + 8.0f;
    skin_text(fo->heading, "LOOK", panel.x + pad, y, TONE_TEXT, ALIGN_LEFT);
    UIRect r_look[LOOK_COUNT];
    float lx = panel.x + pad + 160.0f;
    float lw = (panel.x + panel.w - pad - lx - gap * (float)(LOOK_COUNT - 1)) / (float)LOOK_COUNT;
    if (lw > bw * 0.85f)
        lw = bw * 0.85f;
    for (int i = 0; i < LOOK_COUNT; i++) {
        r_look[i] = (UIRect){lx + (float)i * (lw + gap), y - 6.0f, lw, bh};
        skin_button(r_look[i], fo->bold, syn_look_name((Look)i),
                    (WidgetState){.hover = skin_hit(r_look[i], mx, my),
                                  .active = syn_look() == (Look)i},
                    true);
    }
    y += bh + 14.0f;

    const Theme *cur = skin_theme();
    char line[256];
    snprintf(line, sizeof line, "Theme: %s  (%s, %s)", cur->name, theme_style_name(cur->style),
             cur->light ? "light" : "dark");
    skin_text(fo->bold, line, panel.x + pad, y, TONE_ACCENT, ALIGN_LEFT);
    y += skin_text_height(fo->bold) + 2.0f;
    skin_text(fo->small,
              "The same palettes SYN-OS uses for labwc, waybar, foot and mako. Click one and "
              "everything changes to it.",
              panel.x + pad, y, TONE_MUTED, ALIGN_LEFT);
    y += skin_text_height(fo->small) + 16.0f;

    /* one column per family, each theme a row with its colours beside its name */
    UIRect list = {panel.x + pad, y, panel.w - pad * 2, panel.y + panel.h - pad - y};
    float col_gap = 16.0f;
    float col_w = (list.w - col_gap * (float)(FAMILY_COUNT - 1)) / (float)FAMILY_COUNT;
    float row_h = 30.0f;
    int tallest = 0;
    for (int f = 0; f < FAMILY_COUNT; f++) {
        int n = 0;
        for (int i = 0; i < G.themes.count; i++)
            if (!strcmp(G.themes.items[i].group, FAMILIES[f].group))
                n++;
        if (n > tallest)
            tallest = n;
    }
    float content_h = 56.0f + row_h * (float)tallest;
    float max_scroll = content_h - list.h;
    if (max_scroll < 0)
        max_scroll = 0;
    if (skin_hit(list, mx, my))
        G.options_scroll -= ui_wheel_y() * 40.0f;
    if (G.options_scroll < 0)
        G.options_scroll = 0;
    if (G.options_scroll > max_scroll)
        G.options_scroll = max_scroll;

    int clicked_theme = -1;
    gfx_clip_push((SDL_Rect){(int)list.x, (int)list.y, (int)list.w, (int)list.h});
    for (int f = 0; f < FAMILY_COUNT; f++) {
        float cx = list.x + (col_w + col_gap) * (float)f;
        float cy = list.y - G.options_scroll;
        skin_text(fo->heading, FAMILIES[f].title, cx, cy, TONE_TEXT, ALIGN_LEFT);
        skin_text(fo->small, FAMILIES[f].blurb, cx, cy + skin_text_height(fo->heading), TONE_MUTED,
                  ALIGN_LEFT);
        cy += 50.0f;
        skin_separator(cx, cy - 4.0f, col_w);
        for (int i = 0; i < G.themes.count; i++) {
            const Theme *t = &G.themes.items[i];
            if (strcmp(t->group, FAMILIES[f].group) != 0)
                continue;
            UIRect row = {cx, cy, col_w, row_h - 2.0f};
            bool selected = i == G.theme;
            bool hover = skin_hit(list, mx, my) && skin_hit(row, mx, my);
            if (hover)
                skin_row_hover(row);
            if (selected)
                skin_fill(ui_rect(row.x, row.y, 4.0f, row.h), cur->accent, 1);
            skin_swatches((UIRect){row.x + 10.0f, row.y + 7.0f, 44.0f, row.h - 14.0f}, t);
            /* the column already says the family: HALO-LIGHT-AZURE reads AZURE,
             * with LIGHT beside it when there is room */
            const char *short_name = theme_short_name(t);
            size_t fl = strlen(FAMILIES[f].title);
            if (!strncmp(short_name, FAMILIES[f].title, fl) && short_name[fl] == '-')
                short_name += fl + 1;
            bool light = !strncmp(short_name, "LIGHT-", 6);
            if (light)
                short_name += 6;
            TTF_Font *nf = selected ? fo->bold : fo->body;
            float nx = row.x + 64.0f, room = row.x + row.w - 4.0f - nx;
            char buf[96];
            const char *name = skin_fit(nf, short_name, room, buf, sizeof buf);
            float ny = row.y + (row.h - skin_text_height(nf)) * 0.5f;
            float nw = skin_text(nf, name, nx, ny, selected ? TONE_ACCENT : TONE_TEXT, ALIGN_LEFT);
            if (light && nw + 8.0f + skin_text_width(fo->small, "LIGHT") <= room)
                skin_text(fo->small, "LIGHT", nx + nw + 8.0f,
                          row.y + (row.h - skin_text_height(fo->small)) * 0.5f, TONE_MUTED,
                          ALIGN_LEFT);
            if (hover && clicked_in(row, mx, my))
                clicked_theme = i;
            cy += row_h;
        }
    }
    gfx_clip_pop();

    if (clicked_in(colour ? r_bio : r_col, mx, my))
        return SYNOS_SWITCH_SIM;
    if (clicked_in(back, mx, my))
        synos_set_screen(SYNOS_MENU);
    else if (clicked_in(full, mx, my))
        synos_set_fullscreen(!G.fullscreen);
    else if (clicked_in(r_synos, mx, my) && !G.active)
        synos_set_active(true);
    else if (clicked_in(r_her, mx, my) && G.active)
        synos_set_active(false);
    else if (clicked_in(r_mini, mx, my))
        syn_set_minimap(!syn_minimap());
    else if (clicked_theme >= 0)
        synos_set_theme(clicked_theme);
    for (int i = 0; i < LOOK_COUNT; i++)
        if (clicked_in(r_look[i], mx, my))
            syn_set_look((Look)i);
    return SYNOS_NONE;
}
