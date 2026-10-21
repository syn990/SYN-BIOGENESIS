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

/* The SYN-OS interface's shell, over the world and every screen but the
 * main menu and options, made of SYN-OS's own desktop:
 *
 *   the bar        waybar's: the theme's glyph, which opens the launcher;
 *                  the screens as tabs (SYN-DEBT's); the world's figures
 *                  as modules, warning and critical as waybar has them;
 *                  MENU
 *   notifications  mako's, top right, 3px in the accent (urgent: its colour)
 *   the launcher   wmenu's and fuzzel's: every command under the root
 *                  menu's headings, filtered as it is typed (/)
 *   the console    a drop-down foot with syn-shell's command line (`):
 *                  every command by name, and the technical ones the
 *                  window adds
 *   the menu       labwc's, on a right click
 *   the hints      SYN-OS's TUIs' status line, along the bottom
 *   the card       the selected organism, SYN-TREK's target card
 *
 * Both simulations' windows drive it with their own commands (their
 * actions) and figures. Text the machine writes is in SYN Terminal
 * (Terminus), as on SYN-OS; the icons are Font Awesome's, as waybar's. */
#ifndef SYN_SHELL_H
#define SYN_SHELL_H

#include <SDL.h>
#include <stdbool.h>

#include "synos/skin.h"

/* Font Awesome's glyphs, as UTF-8 */
#define ICON_PLAY "\xef\x81\x8b"
#define ICON_PAUSE "\xef\x81\x8c"
#define ICON_STEP "\xef\x81\x91"
#define ICON_CLOCK "\xef\x80\x97"
#define ICON_USERS "\xef\x83\x80"
#define ICON_LEAF "\xef\x81\xac"
#define ICON_SMOG "\xef\x9d\x9f"
#define ICON_CLOUD "\xef\x83\x82"
#define ICON_FIRE "\xef\x81\xad"
#define ICON_GAUGE "\xef\x8f\xbd"
#define ICON_DISPLAY "\xee\x85\xa3"
#define ICON_SEARCH "\xef\x80\x82"
#define ICON_SAVE "\xef\x83\x87"
#define ICON_OPEN "\xef\x81\xbc"
#define ICON_FOLDER "\xef\x81\xbb"
#define ICON_FILE "\xef\x80\x96"
#define ICON_IMPORT "\xef\x95\xaf"
#define ICON_EXPORT "\xef\x95\xae"
#define ICON_PLUS "\xef\x81\xa7"
#define ICON_MINUS "\xef\x81\xa8"
#define ICON_SKULL "\xef\x95\x8c"
#define ICON_DNA "\xef\x91\xb1"
#define ICON_FLASK "\xef\x83\x83"
#define ICON_CHART "\xef\x88\x81"
#define ICON_BOOK "\xef\x80\xad"
#define ICON_TROPHY "\xef\x82\x91"
#define ICON_BINOCULARS "\xef\x87\xa5"
#define ICON_SLIDERS "\xef\x87\x9e"
#define ICON_TERMINAL "\xef\x84\xa0"
#define ICON_BARS "\xef\x83\x89"
#define ICON_CROSSHAIRS "\xef\x81\x9b"
#define ICON_SEEDLING "\xef\x93\x98"
#define ICON_WAND "\xee\x8b\x8a"
#define ICON_PASTE "\xef\x83\xaa"
#define ICON_COPY "\xef\x83\x85"
#define ICON_IMAGE "\xef\x80\xbe"
#define ICON_BOLT "\xef\x83\xa7"
#define ICON_HEART_CRACK "\xef\x9e\xa9"
#define ICON_HEART_PULSE "\xef\x88\x9e"
#define ICON_SITEMAP "\xef\x83\xa8"
#define ICON_BOOKMARK "\xef\x80\xae"
#define ICON_XMARK "\xef\x80\x8d"
#define ICON_CHECK "\xef\x80\x8c"
#define ICON_POWER "\xef\x80\x91"
#define ICON_GEAR "\xef\x80\x93"
#define ICON_MAP "\xef\x89\xb8"
#define ICON_EXPAND "\xef\x81\xa5"
#define ICON_EYE "\xef\x81\xae"
#define ICON_INFO "\xef\x81\x9a"
#define ICON_KEYBOARD "\xef\x84\x9c"
#define ICON_MICROSCOPE "\xef\x98\x90"
#define ICON_WIND "\xef\x9c\xae"
#define ICON_ROTATE "\xef\x83\xa2"
#define ICON_TRASH "\xef\x87\xb8"
#define ICON_DICE "\xef\x94\xa2"
#define ICON_GLOBE "\xef\x82\xac"
#define ICON_ZOOM_IN "\xef\x80\x8e"
#define ICON_ZOOM_OUT "\xef\x80\x90"
#define ICON_HOUSE "\xef\x80\x95"
#define ICON_PALETTE "\xef\x94\xbf"
#define ICON_BACKUP "\xef\x87\x9a"
#define ICON_VIRUS "\xee\x81\xb4"

/* Something the window can do: a menu item once, now a command. */
typedef struct {
    int id;              /* the window's own action */
    const char *section; /* its heading in the launcher: "WORLD", "VIEW" */
    const char *label;
    const char *key;     /* the key that does it ("P", "Ctrl+S"), or NULL */
    const char *word;    /* its name in the console, or NULL */
    const char *icon;    /* ICON_*, or NULL */
    bool disabled;
    bool on;             /* a choice that is on: the look in use, the minimap */
    const char *tip;     /* what it does, in a sentence, or NULL */
} ShellCommand;

/* A screen, in the bar */
typedef struct {
    const char *label;
    const char *icon;
    int id; /* the action that opens it */
    bool active, disabled;
} ShellTab;

/* A figure, in the bar */
typedef struct {
    const char *icon;
    char text[48];
    const char *tip;
    int level; /* 0; 1 waybar's .warning; 2 .critical */
    int id;    /* what a click does, or 0 */
    /* or a module that draws itself (the speed): its width; drawn at
     * right with its middle at cy when draw */
    float (*custom)(float right, float cy, bool draw);
} ShellModule;

#define SHELL_BAR_H 44
#define SHELL_HINTS_H 26

/* ---- the bar ---- */

/* Across the top of w: name beside the glyph, then the tabs, then the
 * modules (dropped from the last when the window is too narrow) and MENU.
 * Returns the id clicked (a tab's, a module's, menu_id), or 0; the glyph
 * and name open the launcher themselves. */
int syn_shell_bar(int w, const char *name, const ShellTab *tabs, int ntabs,
                  const ShellModule *mods, int nmods, int menu_id);

/* ---- the hints ---- */

/* keys: pairs of a key and what it does; right: written at the right */
void syn_shell_hints(int w, int h, const char *const *keys, int nkeys, const char *right);

/* ---- notifications ---- */

/* What the window says: shown at the top right, and kept in the console. */
void syn_shell_notify(const char *text, bool urgent);
/* Under the bar (top), drawn over everything else */
void syn_shell_notifications(int w, int top);
/* none shown any more (a world put in place without being opened) */
void syn_shell_notify_clear(void);

/* ---- the launcher ---- */

void syn_shell_launcher_open(void);
bool syn_shell_launcher_showing(void);
/* Drawn last; the command chosen (and it closes), or 0 */
int syn_shell_launcher(int w, int h, const ShellCommand *c, int n);

/* ---- the console ---- */

typedef enum { SHELL_OUT, SHELL_ECHO, SHELL_ERROR, SHELL_NOTE, SHELL_HEAD, SHELL_GOOD } ShellKind;

void syn_shell_console_toggle(void);
bool syn_shell_console_showing(void);
/* Under the bar (top): the line typed when Enter is pressed (to be run),
 * or NULL. words complete the first word typed on Tab. */
const char *syn_shell_console(int w, int top, int h, const char *const *words, int nwords);
void syn_shell_print(ShellKind kind, const char *fmt, ...);
void syn_shell_clear(void);
/* The line split into words, in place ("quoted words" kept whole) */
int syn_shell_split(char *line, char **argv, int max);
/* The command named word, or NULL */
const ShellCommand *syn_shell_lookup(const ShellCommand *c, int n, const char *word);
/* Every named command, under its heading */
void syn_shell_list(const ShellCommand *c, int n);

/* ---- the right-click menu ---- */

/* Items with id 0 are headings (labwc's labelled separators), "-" a line. */
void syn_shell_menu_open(int x, int y, const char *title, const ShellCommand *items, int n);
bool syn_shell_menu_showing(void);
void syn_shell_menu_close(void);
/* The command chosen (and it closes), or 0 */
int syn_shell_menu(int w, int h);

/* ---- the selected thing's card ---- */

typedef struct {
    const char *label;
    char value[32];
    Tone tone;
} ShellReadout;

typedef struct {
    char title[64];       /* "ORGANISM 6" */
    const char *state;    /* "ALIVE" */
    Tone state_tone;
    char subtitle[96];    /* its species */
    ShellReadout cells[10];
    int ncells;
    /* two bars under the figures: what it has of what it needs, 0..1 */
    const char *bar_label[2];
    double bar[2];
    Tone bar_tone[2];
    ShellCommand actions[12];
    int nactions;
    /* its body, drawn in r */
    void (*draw_body)(void *ud, SDL_Rect r);
    void *ud;
} ShellCard;

/* Bottom left, above the hints; the action clicked, or 0 */
int syn_shell_card(int w, int h, const ShellCard *c);
/* The card's rectangle (to keep the minimap clear of it) */
SDL_Rect syn_shell_card_rect(int w, int h);

/* ---- dialogs ---- */

/* A question in the middle of the screen, as SYN-DEBT asks one: its title,
 * the text (lines split on \n and wrapped), and buttons right to left from
 * the first, which Enter presses (Esc the last). The button pressed, or -1. */
int syn_shell_dialog(int w, int h, const char *title, const char *text, const char *const *buttons, int n);

/* The launcher, the menu or the console's line has the keyboard */
bool syn_shell_modal(void);
/* Something of the shell's moves (a notification, a caret): keep drawing */
bool syn_shell_animating(void);

#endif
