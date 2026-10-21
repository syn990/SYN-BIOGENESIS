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

/* A small immediate-mode UI on SDL2: the menus, toolbar, dialogs and
 * forms that Swing provided in the Java version.
 *
 * Input goes to one layer at a time. Every window registers a layer (its
 * rectangle) while it is drawn; the layer under the mouse is decided from
 * the previous frame's rectangles, topmost first, and a modal layer blocks
 * everything beneath it. Popups (menus, combo lists) are the top layer and
 * report their choice to their owner on the next frame.
 */
#ifndef UI_UI_H
#define UI_UI_H

#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>

#include "ui/gfx.h"

#define UI_BASE_LAYER 1u

uint32_t ui_id(const char *s, uint32_t salt);

void ui_handle_event(const SDL_Event *e);
/* Typed text from somewhere other than SDL (the test script). */
void ui_inject_text(const char *s);
void ui_begin_frame(void);
/* Draws the open popup and tooltip; call last, before presenting. */
void ui_end_frame(void);

/* Layers */
void ui_layer_begin(uint32_t id, SDL_Rect r, bool modal);
void ui_layer_end(void);
bool ui_layer_is_hovered(uint32_t id);
/* The mouse is over this layer (and nothing above it) right now. */
bool ui_hover(SDL_Rect r);
/* A button went down this frame inside r on the current layer. */
bool ui_pressed(SDL_Rect r, int button);
bool ui_double_clicked(void);
/* Layer the mouse went down on this frame (0 if none). */
uint32_t ui_pressed_layer(void);
bool ui_any_modal(void);

/* Mouse and keyboard state for custom widgets (the world view). */
void ui_mouse(int *x, int *y);
bool ui_mouse_down(int button);
int ui_wheel_y(void);
int ui_wheel_x(void);
#ifdef BIO_SYN_EDITION
/* The wheel with its fractions (touchpads; sdl2-compat's pushed events). */
float ui_wheel_fy(void);
float ui_wheel_fx(void);
#endif
bool ui_key_pressed(SDL_Keycode k);
SDL_Keymod ui_mods(void);
bool ui_text_focused(void);
void ui_unfocus(void);
/* Give a text field the keyboard focus (it must be drawn this frame). */
void ui_focus(uint32_t id, const char *current_text);
/* Claim the mouse for a drag that started in this layer. */
void ui_capture(uint32_t id);
bool ui_captured(uint32_t id);
void ui_release(void);

/* Widgets. ids must be unique per frame among visible widgets. */
bool ui_button(uint32_t id, SDL_Rect r, const char *label, bool enabled);
bool ui_icon_button(uint32_t id, SDL_Rect r, SDL_Texture *icon, const char *label, bool enabled,
                    const char *tooltip);
void ui_label(int x, int y, const char *s);
void ui_label_dim(int x, int y, const char *s);
/* Returns true when the edit is committed (Enter, Tab or focus lost). */
bool ui_textfield(uint32_t id, SDL_Rect r, char *buf, size_t cap);
bool ui_combo(uint32_t id, SDL_Rect r, const char *const *items, int n, int *sel);
/* A combo whose items are colour swatches with names. */
bool ui_color_combo(uint32_t id, SDL_Rect r, const int *colors, const char *const *names, int n,
                    int *sel);
bool ui_spinner(uint32_t id, SDL_Rect r, double *v, double min, double max, double step,
                int decimals, bool wrap);
bool ui_tabs(uint32_t id, SDL_Rect r, const char *const *names, int n, int *sel);
void ui_separator(int x, int y, int w);
void ui_tooltip(const char *s);

/* Popups: menus, context menus, combo lists. Item text starting with '-'
 * is a separator. */
void ui_popup_open(uint32_t owner, int x, int y, const char *const *items, const bool *enabled,
                   int n);
bool ui_popup_open_for(uint32_t owner);
#ifdef BIO_SYN_EDITION
/* Any popup menu is open (it takes Esc for itself). */
bool ui_popup_showing(void);
/* After a stretch in which events went elsewhere (a world being made):
 * the mouse buttons held and the pointer as they are now. */
void ui_input_resync(void);

/* A line edited without being drawn, for the SYN-OS shell's own fields:
 * called each frame it is shown, it holds the keyboard, puts in what is
 * typed (not on the frame it took the keyboard, so the key that opened it
 * isn't typed), and edits as a terminal's line does (Ctrl+U, Ctrl+W,
 * Ctrl+A and Ctrl+E too). The caller draws buf, the cursor at *cursor.
 * Returns the key that asks something of the caller, or UI_LINE_NONE. */
typedef enum {
    UI_LINE_NONE,
    UI_LINE_ENTER,
    UI_LINE_ESCAPE,
    UI_LINE_TAB,
    UI_LINE_UP,
    UI_LINE_DOWN,
    UI_LINE_PAGE_UP,
    UI_LINE_PAGE_DOWN,
} UiLineKey;
UiLineKey ui_line_edit(uint32_t id, char *buf, size_t cap, int *cursor);
#endif
/* The chosen item for owner, once (or -1). */
int ui_popup_take(uint32_t owner);
void ui_popup_close(void);

/* Floating windows */
typedef struct {
    int x, y, w, h;
    bool open;
    bool centred; /* placed on first show */
#ifdef BIO_SYN_EDITION
    /* SYN-OS: a screen, not a window: where the caller puts it, with no
     * close box and no dragging, under a labwc title bar */
    bool docked;
#endif
} UIWin;

/* Draws frame and title bar, handles dragging and the close box. Returns
 * the content rectangle. Sets win->open = false when closed. */
SDL_Rect ui_window_begin(UIWin *win, uint32_t id, const char *title, bool modal);
void ui_window_end(void);
void ui_window_centre(UIWin *win, int w, int h);

#endif
