/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of a C port of Biogenesis (https://biogenesis.sourceforge.net/).
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

#include "ui/ui.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim/genetic_code.h"
#ifdef BIO_SYNOS
/* The SYN-OS interface draws the widgets in its theme (synos/skin.h); the
 * Heritage edition and the faithful build draw them as below. */
#include "synos/synos.h"
#define SKINNED synos_active()
#endif

#define MAX_LAYERS 32
#define MAX_KEYS 32
#define POPUP_MAX 32

typedef struct {
    uint32_t id;
    SDL_Rect r;
    bool modal;
} Layer;

static struct {
    int mx, my;
    bool down[4], pressed[4], released[4];
    bool dbl;
    int wheel_x, wheel_y;
#ifdef BIO_SYN_EDITION
    float wheel_fx, wheel_fy; /* with fractions, as touchpads scroll */
#endif
    SDL_Keycode keys[MAX_KEYS];
    int nkeys;
    char text[64];
    SDL_Keymod mods;

    Layer layers[MAX_LAYERS], prev[MAX_LAYERS];
    int nlayers, nprev;
    uint32_t stack[MAX_LAYERS];
    int nstack;
    uint32_t hover_layer;
    uint32_t pressed_layer;

    uint32_t hot, active, focus, capture;
    uint32_t last_focus_seen; /* focus survives only while its widget is drawn */
    bool focus_drawn;
    char edit[256];
    int cursor;
    bool commit_pending;
    uint32_t commit_id;

    /* window being dragged */
    UIWin *drag_win;
    int drag_dx, drag_dy;

    /* popup */
    bool popup_open;
    uint32_t popup_owner;
    int popup_x, popup_y;
    char popup_items[POPUP_MAX][64];
    bool popup_enabled[POPUP_MAX];
    int popup_n;
    bool popup_just_opened;
    uint32_t popup_result_owner;
    int popup_result;

    char tooltip[128];
    int tooltip_x, tooltip_y;
    uint32_t tooltip_hot;
    unsigned tooltip_since;
} U;

uint32_t ui_id(const char *s, uint32_t salt)
{
    uint32_t h = 2166136261u ^ salt * 0x9E3779B9u;
    for (; *s; s++)
        h = (h ^ (uint8_t)*s) * 16777619u;
    return h ? h : 1;
}

void ui_handle_event(const SDL_Event *e)
{
    switch (e->type) {
    case SDL_MOUSEMOTION:
        U.mx = e->motion.x;
        U.my = e->motion.y;
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (e->button.button <= 3) {
            U.down[e->button.button] = true;
            U.pressed[e->button.button] = true;
            if (e->button.clicks >= 2 && e->button.button == SDL_BUTTON_LEFT)
                U.dbl = true;
        }
        U.mx = e->button.x;
        U.my = e->button.y;
        break;
    case SDL_MOUSEBUTTONUP:
        if (e->button.button <= 3) {
            U.down[e->button.button] = false;
            U.released[e->button.button] = true;
        }
        U.mx = e->button.x;
        U.my = e->button.y;
        break;
    case SDL_MOUSEWHEEL:
        U.wheel_y += e->wheel.y;
        U.wheel_x += e->wheel.x;
#ifdef BIO_SYN_EDITION
        U.wheel_fy += e->wheel.preciseY;
        U.wheel_fx += e->wheel.preciseX;
#endif
        break;
    case SDL_KEYDOWN:
        if (U.nkeys < MAX_KEYS)
            U.keys[U.nkeys++] = e->key.keysym.sym;
        break;
    case SDL_TEXTINPUT:
        if (strlen(U.text) + strlen(e->text.text) < sizeof U.text)
            strcat(U.text, e->text.text);
        break;
    }
}

void ui_inject_text(const char *s)
{
    if (strlen(U.text) + strlen(s) < sizeof U.text)
        strcat(U.text, s);
}

void ui_begin_frame(void)
{
    U.mods = SDL_GetModState();
    /* hover layer from last frame's rectangles, topmost first */
    U.hover_layer = 0;
    int lowest = 0;
    for (int i = U.nprev - 1; i >= 0; i--)
        if (U.prev[i].modal) {
            lowest = i;
            break;
        }
    for (int i = U.nprev - 1; i >= lowest; i--) {
        if (rect_has(U.prev[i].r, U.mx, U.my)) {
            U.hover_layer = U.prev[i].id;
            break;
        }
    }
    if (U.popup_open && !U.popup_just_opened) {
        /* the popup is the top layer; decided in ui_end_frame */
    }
    U.pressed_layer = 0;
    if (U.pressed[1] || U.pressed[2] || U.pressed[3])
        U.pressed_layer = U.hover_layer;
    U.nlayers = 0;
    U.nstack = 0;
    U.hot = 0;
    U.focus_drawn = false;
    U.tooltip[0] = '\0';
}

static void consume_input_end(void)
{
    for (int i = 0; i < 4; i++) {
        U.pressed[i] = false;
        U.released[i] = false;
    }
    U.dbl = false;
    U.wheel_x = U.wheel_y = 0;
#ifdef BIO_SYN_EDITION
    U.wheel_fx = U.wheel_fy = 0;
#endif
    U.nkeys = 0;
    U.text[0] = '\0';
}

static uint32_t cur_layer(void)
{
    return U.nstack ? U.stack[U.nstack - 1] : UI_BASE_LAYER;
}

void ui_layer_begin(uint32_t id, SDL_Rect r, bool modal)
{
    if (U.nlayers < MAX_LAYERS)
        U.layers[U.nlayers++] = (Layer){id, r, modal};
    if (U.nstack < MAX_LAYERS)
        U.stack[U.nstack++] = id;
}

void ui_layer_end(void)
{
    if (U.nstack)
        U.nstack--;
}

bool ui_layer_is_hovered(uint32_t id)
{
    return U.hover_layer == id;
}

uint32_t ui_pressed_layer(void)
{
    return U.pressed_layer;
}

bool ui_any_modal(void)
{
    for (int i = 0; i < U.nprev; i++)
        if (U.prev[i].modal)
            return true;
    return false;
}

static bool layer_live(void)
{
    if (U.popup_open)
        return false;
    if (U.capture && U.capture != cur_layer())
        return false;
    return U.hover_layer == cur_layer();
}

bool ui_hover(SDL_Rect r)
{
    return layer_live() && rect_has(r, U.mx, U.my);
}

bool ui_pressed(SDL_Rect r, int button)
{
    return U.pressed[button] && ui_hover(r);
}

bool ui_double_clicked(void)
{
    return U.dbl;
}

void ui_mouse(int *x, int *y)
{
    *x = U.mx;
    *y = U.my;
}

bool ui_mouse_down(int b)
{
    return b >= 1 && b <= 3 && U.down[b];
}

int ui_wheel_y(void)
{
    return layer_live() ? U.wheel_y : 0;
}

int ui_wheel_x(void)
{
    return layer_live() ? U.wheel_x : 0;
}

#ifdef BIO_SYN_EDITION
float ui_wheel_fy(void)
{
    return layer_live() ? U.wheel_fy : 0;
}

float ui_wheel_fx(void)
{
    return layer_live() ? U.wheel_fx : 0;
}
#endif

bool ui_key_pressed(SDL_Keycode k)
{
    for (int i = 0; i < U.nkeys; i++)
        if (U.keys[i] == k)
            return true;
    return false;
}

SDL_Keymod ui_mods(void)
{
    return U.mods;
}

bool ui_text_focused(void)
{
    return U.focus != 0;
}

void ui_unfocus(void)
{
    if (U.focus) {
        U.commit_pending = true;
        U.commit_id = U.focus;
    }
    U.focus = 0;
    SDL_StopTextInput();
}

void ui_capture(uint32_t id)
{
    U.capture = id;
}

bool ui_captured(uint32_t id)
{
    return U.capture == id;
}

void ui_release(void)
{
    U.capture = 0;
}

/* ---- widgets ---- */

static bool clicked(uint32_t id, SDL_Rect r, bool enabled)
{
    bool over = enabled && ui_hover(r);
    if (over)
        U.hot = id;
    if (over && U.pressed[1])
        U.active = id;
    bool fire = false;
    if (U.active == id && U.released[1]) {
        fire = over;
        U.active = 0;
    }
    return fire;
}

bool ui_button(uint32_t id, SDL_Rect r, const char *label, bool enabled)
{
    bool fire = clicked(id, r, enabled);
#ifdef BIO_SYNOS
    if (SKINNED) {
        skin_button(synos_rect(r), synos_fonts()->ui, label,
                    (WidgetState){.hover = enabled && U.hot == id,
                                  .pressed = U.active == id && ui_hover(r),
                                  .disabled = !enabled},
                    false);
        return fire;
    }
#endif
    Col bg = C_PANEL2;
    if (!enabled)
        bg = C_PANEL;
    else if (U.active == id && ui_hover(r))
        bg = C_PRESS;
    else if (U.hot == id)
        bg = C_HOVER;
    gfx_fill(r, bg);
    gfx_frame(r, C_BORDER);
    gfx_text_in(r, label, enabled ? C_TEXT : C_DISABLED, true);
    return fire;
}

bool ui_icon_button(uint32_t id, SDL_Rect r, SDL_Texture *icon, const char *label, bool enabled,
                    const char *tooltip)
{
    bool fire = clicked(id, r, enabled);
#ifdef BIO_SYNOS
    if (SKINNED)
        skin_button(synos_rect(r), synos_fonts()->ui, "",
                    (WidgetState){.hover = enabled && U.hot == id,
                                  .pressed = U.active == id && ui_hover(r),
                                  .disabled = !enabled},
                    false);
    else
#endif
    if (enabled && (U.hot == id || (U.active == id && ui_hover(r)))) {
        gfx_fill(r, U.active == id ? C_PRESS : C_HOVER);
        gfx_frame(r, C_BORDER);
    }
    if (icon) {
        int s = r.h - 6;
        SDL_Rect d = {r.x + (r.w - s) / 2, r.y + 3, s, s};
        SDL_SetTextureAlphaMod(icon, enabled ? 255 : 70);
        SDL_RenderCopy(gfx_renderer(), icon, NULL, &d);
    } else {
        gfx_text_in(r, label, enabled ? C_TEXT : C_DISABLED, true);
    }
    if (tooltip && U.hot == id)
        ui_tooltip(tooltip);
    return fire;
}

void ui_label(int x, int y, const char *s)
{
    gfx_text(x, y, s, C_TEXT);
}

void ui_label_dim(int x, int y, const char *s)
{
    gfx_text(x, y, s, C_TEXT_DIM);
}

void ui_separator(int x, int y, int w)
{
    gfx_line(x, y, x + w, y, C_BORDER);
}

void ui_tooltip(const char *s)
{
    if (U.tooltip_hot != U.hot) {
        U.tooltip_hot = U.hot;
        U.tooltip_since = SDL_GetTicks();
    }
    snprintf(U.tooltip, sizeof U.tooltip, "%s", s);
    U.tooltip_x = U.mx + 12;
    U.tooltip_y = U.my + 18;
}

static void focus_widget(uint32_t id, const char *buf)
{
    if (U.focus && U.focus != id) {
        U.commit_pending = true;
        U.commit_id = U.focus;
    }
    U.focus = id;
    snprintf(U.edit, sizeof U.edit, "%s", buf);
    U.cursor = (int)strlen(U.edit);
    SDL_StartTextInput();
}

void ui_focus(uint32_t id, const char *current_text)
{
    focus_widget(id, current_text);
}

static int utf8_prev(const char *s, int i)
{
    if (i <= 0)
        return 0;
    i--;
    while (i > 0 && ((unsigned char)s[i] & 0xC0) == 0x80)
        i--;
    return i;
}

static int utf8_next(const char *s, int i)
{
    int n = (int)strlen(s);
    if (i >= n)
        return n;
    i++;
    while (i < n && ((unsigned char)s[i] & 0xC0) == 0x80)
        i++;
    return i;
}

bool ui_textfield(uint32_t id, SDL_Rect r, char *buf, size_t cap)
{
    bool committed = false;
    if (U.commit_pending && U.commit_id == id) {
        /* focus moved away last frame: keep what was typed */
        U.commit_pending = false;
        committed = true;
    }
    bool over = ui_hover(r);
    if (over && U.pressed[1] && U.focus != id)
        focus_widget(id, buf);
    if (U.focus == id) {
        U.focus_drawn = true;
        /* typed text */
        if (U.text[0]) {
            size_t el = strlen(U.edit), tl = strlen(U.text);
            if (el + tl < sizeof U.edit && el + tl < cap) {
                memmove(U.edit + U.cursor + tl, U.edit + U.cursor, el - (size_t)U.cursor + 1);
                memcpy(U.edit + U.cursor, U.text, tl);
                U.cursor += (int)tl;
            }
        }
        for (int i = 0; i < U.nkeys; i++) {
            SDL_Keycode k = U.keys[i];
            int len = (int)strlen(U.edit);
            if (k == SDLK_BACKSPACE && U.cursor > 0) {
                int p = utf8_prev(U.edit, U.cursor);
                memmove(U.edit + p, U.edit + U.cursor, (size_t)(len - U.cursor + 1));
                U.cursor = p;
            } else if (k == SDLK_DELETE && U.cursor < len) {
                int n = utf8_next(U.edit, U.cursor);
                memmove(U.edit + U.cursor, U.edit + n, (size_t)(len - n + 1));
            } else if (k == SDLK_LEFT) {
                U.cursor = utf8_prev(U.edit, U.cursor);
            } else if (k == SDLK_RIGHT) {
                U.cursor = utf8_next(U.edit, U.cursor);
            } else if (k == SDLK_HOME) {
                U.cursor = 0;
            } else if (k == SDLK_END) {
                U.cursor = len;
            } else if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_TAB) {
                snprintf(buf, cap, "%s", U.edit);
                U.focus = 0;
                SDL_StopTextInput();
                committed = true;
            } else if (k == SDLK_ESCAPE) {
                U.focus = 0;
                SDL_StopTextInput();
            }
        }
        if (U.focus == id) {
            snprintf(buf, cap, "%s", U.edit);
            /* a click elsewhere ends the edit */
            if ((U.pressed[1] || U.pressed[3]) && !over) {
                U.focus = 0;
                SDL_StopTextInput();
                committed = true;
            }
        }
    }
#ifdef BIO_SYNOS
    if (SKINNED) {
        skin_input(synos_rect(r), U.focus == id, ui_hover(r), false);
    } else {
#endif
    gfx_fill(r, C_FIELD);
    gfx_frame(r, U.focus == id ? C_ACCENT : C_BORDER);
#ifdef BIO_SYNOS
    }
#endif
    gfx_clip_push((SDL_Rect){r.x + 2, r.y, r.w - 4, r.h});
    int tx = r.x + 5, ty = r.y + (r.h - gfx_font_h()) / 2;
    gfx_text(tx, ty, buf, C_TEXT);
    if (U.focus == id && (SDL_GetTicks() / 500) % 2 == 0) {
        char tmp[256];
        snprintf(tmp, sizeof tmp, "%.*s", U.cursor, U.edit);
        int cx = tx + gfx_text_w(tmp);
        gfx_line(cx, ty + 2, cx, ty + gfx_font_h() - 2, C_TEXT);
    }
    gfx_clip_pop();
    return committed;
}

static void draw_drop_arrow(SDL_Rect r)
{
    int cx = r.x + r.w - 12, cy = r.y + r.h / 2 - 2;
    for (int i = 0; i < 4; i++)
        gfx_line(cx - 4 + i, cy + i, cx + 4 - i, cy + i, C_TEXT_DIM);
}

bool ui_combo(uint32_t id, SDL_Rect r, const char *const *items, int n, int *sel)
{
    bool changed = false;
    int got = ui_popup_take(id);
    if (got >= 0 && got < n && got != *sel) {
        *sel = got;
        changed = true;
    }
    if (clicked(id, r, true))
        ui_popup_open(id, r.x, r.y + r.h, items, NULL, n);
#ifdef BIO_SYNOS
    if (SKINNED) {
        skin_input(synos_rect(r), ui_popup_open_for(id), U.hot == id, false);
    } else {
#endif
    gfx_fill(r, U.hot == id ? C_HOVER : C_PANEL2);
    gfx_frame(r, C_BORDER);
#ifdef BIO_SYNOS
    }
#endif
    if (*sel >= 0 && *sel < n)
        gfx_text_in((SDL_Rect){r.x, r.y, r.w - 18, r.h}, items[*sel], C_TEXT, false);
    draw_drop_arrow(r);
    if (ui_hover(r) && U.wheel_y) {
        int ns = *sel - (U.wheel_y > 0 ? 1 : -1);
        if (ns >= 0 && ns < n) {
            *sel = ns;
            changed = true;
        }
    }
    return changed;
}

bool ui_color_combo(uint32_t id, SDL_Rect r, const int *colors, const char *const *names, int n,
                    int *sel)
{
    bool changed = ui_combo(id, r, names, n, sel);
    if (*sel >= 0 && *sel < n) {
        uint8_t cr, cg, cb;
        bio_color_rgb(colors[*sel], &cr, &cg, &cb);
        SDL_Rect sw = {r.x + r.w - 38, r.y + 5, 14, r.h - 10};
        gfx_fill(sw, COL(cr, cg, cb));
        gfx_frame(sw, C_BORDER);
    }
    return changed;
}

bool ui_spinner(uint32_t id, SDL_Rect r, double *v, double min, double max, double step,
                int decimals, bool wrap)
{
    bool changed = false;
    int bw = 18;
    SDL_Rect field = {r.x, r.y, r.w - bw, r.h};
    SDL_Rect up = {r.x + r.w - bw, r.y, bw, r.h / 2};
    SDL_Rect dn = {r.x + r.w - bw, r.y + r.h / 2, bw, r.h - r.h / 2};
    double nv = *v;
    if (clicked(id ^ 0x55u, up, true))
        nv += step;
    if (clicked(id ^ 0xAAu, dn, true))
        nv -= step;
    if (ui_hover(r) && U.wheel_y)
        nv += step * (U.wheel_y > 0 ? 1 : -1);
    static char buf[256];
    char shown[64];
    snprintf(shown, sizeof shown, "%.*f", decimals, *v);
    if (U.focus == id || (U.commit_pending && U.commit_id == id)) {
        snprintf(buf, sizeof buf, "%s", U.focus == id ? U.edit : shown);
        if (ui_textfield(id, field, buf, sizeof buf)) {
            char *end;
            double t = strtod(buf, &end);
            if (end != buf && isfinite(t))
                nv = t;
        }
    } else {
        snprintf(buf, sizeof buf, "%s", shown);
        ui_textfield(id, field, buf, sizeof buf);
    }
    if (nv != *v) {
        if (wrap) {
            double span = max - min + step;
            while (nv > max + 1e-9)
                nv -= span;
            while (nv < min - 1e-9)
                nv += span;
        }
        if (nv < min)
            nv = min;
        if (nv > max)
            nv = max;
        double p = pow(10, decimals);
        nv = round(nv * p) / p;
        if (nv != *v) {
            *v = nv;
            changed = true;
        }
    }
#ifdef BIO_SYNOS
    if (SKINNED) {
        skin_button(synos_rect(up), synos_fonts()->ui, "",
                    (WidgetState){.hover = U.hot == (id ^ 0x55u)}, false);
        skin_button(synos_rect(dn), synos_fonts()->ui, "",
                    (WidgetState){.hover = U.hot == (id ^ 0xAAu)}, false);
    } else {
#endif
    gfx_fill(up, U.hot == (id ^ 0x55u) ? C_HOVER : C_PANEL2);
    gfx_fill(dn, U.hot == (id ^ 0xAAu) ? C_HOVER : C_PANEL2);
    gfx_frame(up, C_BORDER);
    gfx_frame(dn, C_BORDER);
#ifdef BIO_SYNOS
    }
#endif
    int cx = up.x + bw / 2;
    for (int i = 0; i < 3; i++) {
        gfx_line(cx - i, up.y + up.h / 2 - 1 + i, cx + i, up.y + up.h / 2 - 1 + i, C_TEXT_DIM);
        gfx_line(cx - i, dn.y + dn.h / 2 + 1 - i, cx + i, dn.y + dn.h / 2 + 1 - i, C_TEXT_DIM);
    }
    return changed;
}

bool ui_tabs(uint32_t id, SDL_Rect r, const char *const *names, int n, int *sel)
{
    bool changed = false;
    int x = r.x;
    for (int i = 0; i < n; i++) {
        int w = gfx_text_w(names[i]) + 24;
        SDL_Rect t = {x, r.y, w, r.h};
        if (clicked(id + (uint32_t)i, t, true) && *sel != i) {
            *sel = i;
            changed = true;
        }
#ifdef BIO_SYNOS
        if (SKINNED) {
            skin_tab(synos_rect(t), synos_fonts()->ui, names[i],
                     (WidgetState){.hover = U.hot == id + (uint32_t)i, .active = i == *sel});
            x += w + 2;
            continue;
        }
#endif
        gfx_fill(t, i == *sel ? C_PANEL2 : (U.hot == id + (uint32_t)i ? C_HOVER : C_PANEL));
        gfx_frame(t, C_BORDER);
        if (i == *sel)
            gfx_line(t.x + 1, t.y + t.h - 1, t.x + t.w - 2, t.y + t.h - 1, C_PANEL2);
        gfx_text_in(t, names[i], i == *sel ? C_TEXT : C_TEXT_DIM, true);
        x += w - 1;
    }
    gfx_line(x, r.y + r.h - 1, r.x + r.w, r.y + r.h - 1, C_BORDER);
    return changed;
}

/* ---- popups ---- */

void ui_popup_open(uint32_t owner, int x, int y, const char *const *items, const bool *enabled,
                   int n)
{
    if (n > POPUP_MAX)
        n = POPUP_MAX;
    U.popup_open = true;
    U.popup_owner = owner;
    U.popup_x = x;
    U.popup_y = y;
    U.popup_n = n;
    for (int i = 0; i < n; i++) {
        snprintf(U.popup_items[i], sizeof U.popup_items[i], "%s", items[i]);
        U.popup_enabled[i] = enabled ? enabled[i] : true;
    }
    U.popup_just_opened = true;
    U.active = 0;
}

bool ui_popup_open_for(uint32_t owner)
{
    return U.popup_open && U.popup_owner == owner;
}

#ifdef BIO_SYN_EDITION
bool ui_popup_showing(void)
{
    return U.popup_open;
}
#endif

int ui_popup_take(uint32_t owner)
{
    if (U.popup_result_owner == owner && U.popup_result >= 0) {
        int r = U.popup_result;
        U.popup_result = -1;
        U.popup_result_owner = 0;
        return r;
    }
    return -1;
}

void ui_popup_close(void)
{
    U.popup_open = false;
}

static void popup_frame(void)
{
    if (!U.popup_open)
        return;
    int fh = gfx_font_h();
    int ih = fh + 8, sh = 9;
    int w = 120, h = 4;
    for (int i = 0; i < U.popup_n; i++) {
        if (U.popup_items[i][0] == '-') {
            h += sh;
        } else {
            int tw = gfx_text_w(U.popup_items[i]) + 36;
            w = tw > w ? tw : w;
            h += ih;
        }
    }
    int sw, sh_;
    SDL_GetRendererOutputSize(gfx_renderer(), &sw, &sh_);
    int x = U.popup_x, y = U.popup_y;
    if (x + w > sw)
        x = sw - w;
    if (y + h > sh_)
        y = y - h > 0 ? y - h : sh_ - h;
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    SDL_Rect box = {x, y, w, h};
#ifdef BIO_SYNOS
    if (SKINNED) {
        skin_panel(synos_rect(box), false);
    } else {
#endif
    gfx_fill((SDL_Rect){x + 3, y + 3, w, h}, (Col){0, 0, 0, 90});
    gfx_fill(box, C_PANEL);
    gfx_frame(box, C_BORDER);
#ifdef BIO_SYNOS
    }
#endif
    int cy = y + 2;
    int chosen = -1;
    for (int i = 0; i < U.popup_n; i++) {
        if (U.popup_items[i][0] == '-') {
            gfx_line(x + 6, cy + sh / 2, x + w - 7, cy + sh / 2, C_BORDER);
            cy += sh;
            continue;
        }
        SDL_Rect it = {x + 2, cy, w - 4, ih};
        bool over = rect_has(it, U.mx, U.my) && U.popup_enabled[i];
#ifdef BIO_SYNOS
        if (SKINNED) {
            if (over)
                skin_row_hover(synos_rect(it));
            gfx_text(it.x + 12, it.y + (ih - fh) / 2, U.popup_items[i],
                     U.popup_enabled[i] ? C_TEXT : C_DISABLED);
            if (over && !U.popup_just_opened && (U.released[1] || U.released[3]))
                chosen = i;
            cy += ih;
            continue;
        }
#endif
        if (over)
            gfx_fill(it, C_ACCENT);
        gfx_text(it.x + 12, it.y + (ih - fh) / 2, U.popup_items[i],
                 U.popup_enabled[i] ? (over ? COL(255, 255, 255) : C_TEXT) : C_DISABLED);
        if (over && !U.popup_just_opened && (U.released[1] || U.released[3]))
            chosen = i;
        cy += ih;
    }
    if (chosen >= 0) {
        U.popup_result_owner = U.popup_owner;
        U.popup_result = chosen;
        U.popup_open = false;
    } else if (!U.popup_just_opened && (U.pressed[1] || U.pressed[3]) &&
               !rect_has(box, U.mx, U.my)) {
        U.popup_open = false;
    } else if (ui_key_pressed(SDLK_ESCAPE)) {
        U.popup_open = false;
    }
    /* releasing the button that opened it does not pick an item */
    if (U.popup_just_opened && !U.down[1] && !U.down[3])
        U.popup_just_opened = false;
    else if (U.popup_just_opened && (U.pressed[1] || U.pressed[3]))
        U.popup_just_opened = false;
}

/* ---- windows ---- */

void ui_window_centre(UIWin *win, int w, int h)
{
    int sw, sh;
    SDL_GetRendererOutputSize(gfx_renderer(), &sw, &sh);
    win->w = w;
    win->h = h;
    win->x = (sw - w) / 2;
    win->y = (sh - h) / 2;
    if (win->x < 0)
        win->x = 0;
    if (win->y < 0)
        win->y = 0;
    win->centred = true;
}

SDL_Rect ui_window_begin(UIWin *win, uint32_t id, const char *title, bool modal)
{
    int th = gfx_font_h() + 10;
    int sw, sh;
    SDL_GetRendererOutputSize(gfx_renderer(), &sw, &sh);
    /* keep the title bar reachable */
    if (win->x > sw - 60)
        win->x = sw - 60;
    if (win->y > sh - th)
        win->y = sh - th;
    if (win->x + win->w < 60)
        win->x = 60 - win->w;
    if (win->y < 0)
        win->y = 0;
    SDL_Rect frame = {win->x, win->y, win->w, win->h};
    if (modal) {
        SDL_Rect all = {0, 0, sw, sh};
        gfx_fill(all, (Col){0, 0, 0, 110});
        ui_layer_begin(id, all, true);
    } else {
        ui_layer_begin(id, frame, false);
    }
#ifdef BIO_SYNOS
    SDL_Rect bar;
    if (SKINNED) {
        bar = (SDL_Rect){frame.x + 1, frame.y + 1, frame.w - 2, th};
        skin_panel(synos_rect(frame), false);
        skin_bar(synos_rect(bar));
    } else {
        gfx_fill((SDL_Rect){frame.x + 4, frame.y + 4, frame.w, frame.h}, (Col){0, 0, 0, 80});
        gfx_fill(frame, C_PANEL);
        gfx_frame(frame, C_BORDER);
        bar = (SDL_Rect){frame.x + 1, frame.y + 1, frame.w - 2, th};
        gfx_fill(bar, C_PANEL2);
    }
#else
    gfx_fill((SDL_Rect){frame.x + 4, frame.y + 4, frame.w, frame.h}, (Col){0, 0, 0, 80});
    gfx_fill(frame, C_PANEL);
    gfx_frame(frame, C_BORDER);
    SDL_Rect bar = {frame.x + 1, frame.y + 1, frame.w - 2, th};
    gfx_fill(bar, C_PANEL2);
#endif
    gfx_text(bar.x + 8, bar.y + 5, title, C_TEXT);
    SDL_Rect close = {bar.x + bar.w - th, bar.y, th, th};
    bool close_hot = ui_hover(close);
    if (close_hot)
        gfx_fill(close, (Col){170, 60, 60, 255});
    int c = th / 2, m = 5;
    gfx_line(close.x + c - m, close.y + c - m, close.x + c + m, close.y + c + m, C_TEXT);
    gfx_line(close.x + c - m, close.y + c + m, close.x + c + m, close.y + c - m, C_TEXT);
    if (close_hot && U.pressed[1]) {
        win->open = false;
    } else if (ui_hover(bar) && U.pressed[1]) {
        U.drag_win = win;
        U.drag_dx = U.mx - win->x;
        U.drag_dy = U.my - win->y;
    }
    if (U.drag_win == win) {
        if (U.down[1]) {
            win->x = U.mx - U.drag_dx;
            win->y = U.my - U.drag_dy;
        } else {
            U.drag_win = NULL;
        }
    }
    return (SDL_Rect){frame.x + 10, frame.y + th + 10, frame.w - 20, frame.h - th - 20};
}

void ui_window_end(void)
{
    ui_layer_end();
}

void ui_end_frame(void)
{
    if (U.popup_open) {
        int sw, sh;
        SDL_GetRendererOutputSize(gfx_renderer(), &sw, &sh);
        /* everything is the popup's while it is open */
        ui_layer_begin(0xFFFFFFF0u, (SDL_Rect){0, 0, sw, sh}, true);
        popup_frame();
        ui_layer_end();
    }
    if (U.tooltip[0] && SDL_GetTicks() - U.tooltip_since > 450) {
        int w = gfx_text_w(U.tooltip) + 12, h = gfx_font_h() + 6;
        int sw, sh;
        SDL_GetRendererOutputSize(gfx_renderer(), &sw, &sh);
        int x = U.tooltip_x, y = U.tooltip_y;
        if (x + w > sw)
            x = sw - w;
        if (y + h > sh)
            y = U.my - h - 4;
        SDL_Rect r = {x, y, w, h};
#ifdef BIO_SYNOS
        if (SKINNED) {
            skin_panel(synos_rect(r), false);
            gfx_text(x + 6, y + 3, U.tooltip, C_TEXT);
        } else {
#endif
        gfx_fill(r, (Col){250, 250, 225, 255});
        gfx_frame(r, (Col){90, 90, 70, 255});
        gfx_text(x + 6, y + 3, U.tooltip, (Col){20, 20, 20, 255});
#ifdef BIO_SYNOS
        }
#endif
    } else if (!U.tooltip[0]) {
        U.tooltip_hot = 0;
    }
    /* a focused field that is no longer drawn loses focus */
    if (U.focus && !U.focus_drawn) {
        U.focus = 0;
        SDL_StopTextInput();
    }
    if (!U.down[1])
        U.active = U.active && U.released[1] ? 0 : U.active;
    if (!U.down[1] && !U.down[2] && !U.down[3])
        U.capture = 0;
    memcpy(U.prev, U.layers, sizeof(Layer) * (size_t)U.nlayers);
    U.nprev = U.nlayers;
    consume_input_end();
}
