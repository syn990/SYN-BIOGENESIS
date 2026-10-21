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

#include "syn/telescope_screen.h"

/* Color Mod has no telescope yet (syn/telescope.h) */
#ifndef BIO_COLOUR

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io/bgw.h"
#include "sim/params.h"
#include "syn/arena_os.h"
#include "synos/synos.h"
#include "ui/app.h"
#include "ui/gfx.h"
#include "ui/ui.h"

#define PAD 24.0f
#define LOG 400
#define LINE 240

static struct {
    char path[1200];  /* the world searched, or to be */
    char shown[1200]; /* how it is named here */
    bool defaults;    /* with the default parameters, not the saved ones */
    bool running, done, found;
    ArenaProc proc;
    char part[4096]; /* a line not yet ended */
    size_t npart;
    char log[LOG][LINE];
    int nlog, first; /* a ring of the last LOG lines */
    char head[LINE];  /* what the search says of the world first */
    int lengths, replays, turned;
    int64_t seed;
    uint64_t state;
    double started, took;
    char status[400];
    Uint32 status_at;
    float scroll;
    bool follow; /* the log kept at its end */
} T = {.follow = true};

static SDL_Rect sdl(UIRect r)
{
    return (SDL_Rect){(int)floorf(r.x), (int)floorf(r.y), (int)ceilf(r.w), (int)ceilf(r.h)};
}

static bool over(UIRect r)
{
    return ui_hover(sdl(r));
}

static bool button(UIRect r, const char *label, bool active, bool enabled)
{
    bool h = enabled && over(r);
    skin_button(r, synos_fonts()->bold, label,
                (WidgetState){.hover = h, .active = active, .disabled = !enabled}, true);
    return h && ui_pressed(sdl(r), 1);
}

/* Text wrapped to width w from (x, y); returns the y after it. */
static float para(TTF_Font *f, const char *text, float x, float y, float w, Tone t)
{
    char line[400];
    size_t n = 0;
    float lh = skin_text_height(f) + 4;
    const char *p = text;
    while (*p) {
        const char *q = p;
        while (*q == ' ')
            q++;
        while (*q && *q != ' ')
            q++;
        char cand[400];
        snprintf(cand, sizeof cand, "%.*s%.*s", (int)n, line, (int)(q - p), n ? p : p + strspn(p, " "));
        if (n && skin_text_width(f, cand) > w) {
            line[n] = '\0';
            skin_text(f, line, x, y, t, ALIGN_LEFT);
            y += lh;
            n = 0;
            p += strspn(p, " ");
            continue;
        }
        n = strlen(cand) < sizeof line ? strlen(cand) : sizeof line - 1;
        memcpy(line, cand, n);
        p = q;
    }
    if (n) {
        line[n] = '\0';
        skin_text(f, line, x, y, t, ALIGN_LEFT);
        y += lh;
    }
    return y;
}

static void say(const char *s)
{
    snprintf(T.status, sizeof T.status, "%s", s);
    T.status_at = SDL_GetTicks();
}

static const char *log_line(int i)
{
    return T.log[(T.first + i) % LOG];
}

/* as much of src as fits */
static void copy(char *dst, size_t n, const char *src)
{
    size_t k = strlen(src);
    if (k >= n)
        k = n - 1;
    memcpy(dst, src, k);
    dst[k] = '\0';
}

static void log_add(const char *s)
{
    if (T.nlog < LOG) {
        copy(T.log[(T.first + T.nlog) % LOG], LINE, s);
        T.nlog++;
    } else {
        copy(T.log[T.first], LINE, s);
        T.first = (T.first + 1) % LOG;
    }
}

/* ---- the search ---- */

static void stop(void)
{
    if (T.running)
        arena_os_end(&T.proc);
    T.running = false;
}

static void begin(void)
{
    stop();
    T.nlog = T.first = 0;
    T.npart = 0;
    T.head[0] = '\0';
    T.lengths = T.replays = T.turned = 0;
    T.done = T.found = false;
    T.scroll = 0;
    T.follow = true;
    const char *args[] = {"--telescope", "world", T.path, "--progress", "--defaults"};
    char err[300];
    if (!arena_os_spawn(&T.proc, args, T.defaults ? 5 : 4, err, sizeof err)) {
        char msg[400];
        snprintf(msg, sizeof msg, "The search didn't start: %s", err);
        say(msg);
        return;
    }
    T.running = true;
    T.started = arena_os_now();
}

static void line_in(const char *s)
{
    if (!strncmp(s, "@found ", 7)) {
        long long seed;
        unsigned long long state;
        if (sscanf(s + 7, "%lld %llx", &seed, &state) == 2) {
            T.found = true;
            T.seed = seed;
            T.state = state;
        }
        T.done = true;
        return;
    }
    if (!strcmp(s, "@none")) {
        T.done = true;
        return;
    }
    if (!T.head[0] && strstr(s, " organisms ("))
        copy(T.head, sizeof T.head, s);
    if (!strncmp(s, "gene length ", 12))
        T.lengths++;
    if (strstr(s, "turned away at tick")) {
        T.turned++;
        T.replays++;
    } else if (strstr(s, "replayed to tick")) {
        T.replays++;
    }
    log_add(s);
}

static void poll_search(void)
{
    if (!T.running)
        return;
    char buf[4096];
    for (;;) {
        long m = arena_os_read(&T.proc, buf, sizeof buf);
        if (m == 0)
            break;
        if (m < 0) {
            /* it has ended */
            arena_os_end(&T.proc);
            T.running = false;
            T.took = arena_os_now() - T.started;
            if (T.npart) {
                T.part[T.npart] = '\0';
                line_in(T.part);
                T.npart = 0;
            }
            if (!T.done)
                say("The search stopped before it could say");
            break;
        }
        for (long i = 0; i < m; i++) {
            if (buf[i] == '\n' || T.npart + 1 >= sizeof T.part) {
                T.part[T.npart] = '\0';
                line_in(T.part);
                T.npart = 0;
            } else if (buf[i] != '\r') {
                T.part[T.npart++] = buf[i];
            }
        }
    }
}

static void choose(const char *path, const char *shown)
{
    stop();
    snprintf(T.path, sizeof T.path, "%s", path);
    snprintf(T.shown, sizeof T.shown, "%s", shown);
    T.done = T.found = false;
    T.nlog = T.first = 0;
    T.head[0] = '\0';
}

/* the world on screen, as it is now, saved where the search can read it */
static void this_world(App *a)
{
    char path[1200], err[256];
    snprintf(path, sizeof path, "%s/telescope-world.bgw", params_config_dir());
    if (!bgw_save(&a->world, path, err, sizeof err)) {
        char msg[400];
        snprintf(msg, sizeof msg, "This world can't be saved for the search: %s", err);
        say(msg);
        return;
    }
    choose(path, "this world, as it is now");
    T.defaults = false;
}

/* the saved world opened with the generator where it was when it was
 * saved: from here on it is the world it was */
static void open_on_thread(App *a)
{
    app_file_chosen(a, FD_OPEN_WORLD, T.path);
    g_rng.seed = T.state & ((1ULL << 48) - 1);
    if (T.defaults) {
        int w = a->world.width, h = a->world.height;
        params_set_defaults(&P);
        P.world_width = w;
        P.world_height = h;
    }
    char msg[400];
    snprintf(msg, sizeof msg, "Opened on its own thread: the world of seed %" PRId64 "%s", T.seed,
             T.defaults ? ", with the default parameters it was made with (not saved)" : "");
    app_set_status(a, msg);
}

/* ---- the screen ---- */

void syn_telescope_frame(App *a, int w, int h)
{
    const Fonts *fo = synos_fonts();
    poll_search();
    bool keys = !ui_any_modal() && !ui_text_focused() && !ui_popup_showing();
    if (keys && ui_key_pressed(SDLK_ESCAPE)) {
        synos_set_screen(SYNOS_MENU);
        return;
    }
    skin_fill(ui_rect(0, 0, (float)w, (float)h), (Rgb){0, 0, 0}, 0.5f);
    float pw = fminf((float)w - 48, 1500), ph = (float)h - 48;
    UIRect panel = {((float)w - pw) / 2, 24, pw, ph};
    skin_panel(panel, true);
    float x = panel.x + PAD, y = panel.y + 14, right = panel.x + panel.w - PAD;
    x += skin_text(fo->big, "TELESCOPE", x, y, TONE_TEXT, ALIGN_LEFT) + 14;
    skin_text(fo->body, "Biogenesis 0.9", x, y + 9, TONE_MUTED, ALIGN_LEFT);
    UIRect back = {right - 110, y, 110, 36};
    if (button(back, "BACK", false, true)) {
        synos_set_screen(SYNOS_MENU);
        return;
    }
    y += 54;
    float tw = right - (panel.x + PAD);
    x = panel.x + PAD;
    y = para(fo->body,
             "A world's seed, read back from its save: the genes its founders drew at tick 0 still "
             "in it point to the seed, and replaying that seed from tick 0 to the saved tick proves it.",
             x, y, tw, TONE_MUTED) +
        14;

    /* which world, with which parameters */
    skin_text(fo->small_bold, "WORLD", x, y, TONE_MUTED, ALIGN_LEFT);
    y += skin_text_height(fo->small_bold) + 6;
    UIRect bthis = {x, y, 170, 34}, bsaved = {x + 178, y, 210, 34};
    if (button(bthis, "THIS WORLD", false, !T.running))
        this_world(a);
    if (over(bthis))
        ui_tooltip("The world on screen, saved as it is now for the search");
    if (button(bsaved, "A SAVED WORLD...", false, !T.running))
        filedlg_open(a, FD_TELESCOPE_WORLD, false, "bgw|sbw", a->last_dir_world);
    if (over(bsaved))
        ui_tooltip("A .bgw or .sbw file: made here or by the original Java program");
    char fit[1200];
    skin_text(fo->body, T.path[0] ? skin_fit(fo->body, T.shown, right - bsaved.x - bsaved.w - 20, fit, sizeof fit)
                                  : "none chosen",
              bsaved.x + bsaved.w + 20, y + 7, T.path[0] ? TONE_TEXT : TONE_MUTED, ALIGN_LEFT);
    y += 34 + 14;
    skin_text(fo->small_bold, "PARAMETERS", x, y, TONE_MUTED, ALIGN_LEFT);
    y += skin_text_height(fo->small_bold) + 6;
    UIRect psaved = {x, y, 170, 34}, pdef = {x + 178, y, 210, 34};
    if (button(psaved, "AS SAVED HERE", !T.defaults, !T.running))
        T.defaults = false;
    if (over(psaved))
        ui_tooltip("The parameters this program keeps (World > Parameters)");
    if (button(pdef, "THE DEFAULTS", T.defaults, !T.running))
        T.defaults = true;
    if (over(pdef))
        ui_tooltip("Biogenesis 0.9's defaults: for worlds the Java program made with its preferences empty");
    skin_text(fo->small, "The world's size is always the save's.", pdef.x + pdef.w + 20, y + 9, TONE_MUTED,
              ALIGN_LEFT);
    y += 34 + 18;

    UIRect go = {x, y, 200, 40};
    if (button(go, T.running ? "STOP" : "FIND ITS SEED", T.running, T.running || T.path[0])) {
        if (T.running) {
            stop();
            say("Stopped");
        } else {
            begin();
        }
    }
    char line[400];
    double secs = T.running ? arena_os_now() - T.started : T.took;
    if (T.running || T.nlog) {
        snprintf(line, sizeof line, "%d gene length%s tried  ·  %d replay%s, %d of them turned away early  ·  %.0f s%s",
                 T.lengths, T.lengths == 1 ? "" : "s", T.replays, T.replays == 1 ? "" : "s", T.turned, secs,
                 T.running ? "  ·  searching" : "");
        skin_text(fo->body, line, go.x + go.w + 20, y + 10, T.running ? TONE_ACCENT : TONE_MUTED, ALIGN_LEFT);
    }
    y += 40 + 16;

    /* the answer */
    if (T.done && T.found) {
        float cy = y + 12;
        float h1 = skin_text_height(fo->big), h2 = skin_text_height(fo->body), h3 = skin_text_height(fo->small);
        UIRect card = {x, y, tw, 12 + h1 + 6 + h2 + 4 + h3 + 12 + 38 + 14};
        skin_fill(card, skin_theme()->accent, 0.14f);
        char big[80];
        snprintf(big, sizeof big, "SEED %" PRId64, T.seed);
        skin_text(fo->big, big, card.x + 16, cy, TONE_TEXT, ALIGN_LEFT);
        cy += h1 + 6;
        skin_text(fo->body, "Replayed from tick 0, it is the saved world. So is every seed equal to it mod 2^48.",
                  card.x + 16, cy, TONE_GOOD, ALIGN_LEFT);
        cy += h2 + 4;
        snprintf(line, sizeof line, "The generator at the saved tick: 0x%012" PRIx64 "%s", T.state,
                 T.defaults ? "  ·  with the default parameters" : "");
        skin_text(fo->small, line, card.x + 16, cy, TONE_MUTED, ALIGN_LEFT);
        cy += h3 + 12;
        UIRect bopen = {card.x + 16, cy, 300, 38};
        UIRect bcopy = {bopen.x + bopen.w + 8, cy, 150, 38};
        if (button(bopen, "OPEN ON ITS OWN THREAD", false, true))
            open_on_thread(a);
        if (over(bopen))
            ui_tooltip("Open the saved world with the random generator where it was: it goes on as the "
                       "world it was, not a new path from there");
        if (button(bcopy, "COPY SEED", false, true)) {
            char sd[32];
            snprintf(sd, sizeof sd, "%" PRId64, T.seed);
            SDL_SetClipboardText(sd);
            say("The seed is on the clipboard");
        }
        y += card.h + 14;
    } else if (T.done) {
        skin_text(fo->heading, "No seed found", x, y, TONE_WARN, ALIGN_LEFT);
        skin_text(fo->small,
                  "Its founders' genes are gone, it was changed from the menus, or it was made with other "
                  "parameters. The log says what was tried.",
                  x, y + skin_text_height(fo->heading) + 4, TONE_MUTED, ALIGN_LEFT);
        y += skin_text_height(fo->heading) + skin_text_height(fo->small) + 18;
    }
    if (T.head[0]) {
        skin_text(fo->small, skin_fit(fo->small, T.head, tw, fit, sizeof fit), x, y, TONE_MUTED, ALIGN_LEFT);
        y += skin_text_height(fo->small) + 8;
    }

    /* what the search says, the newest at the end */
    float bottom = panel.y + panel.h - PAD - skin_text_height(fo->small) - 10;
    UIRect logr = {x, y, tw, bottom - y};
    if (logr.h > 40 && T.nlog) {
        skin_fill(logr, (Rgb){0, 0, 0}, 0.35f);
        float lh = skin_text_height(fo->small) + 3;
        float maxs = fmaxf(0, (float)T.nlog * lh + 16 - logr.h);
        if (over(logr)) {
            float dy = ui_wheel_fy();
            if (dy != 0) {
                T.scroll -= dy * lh * 3;
                T.follow = false;
            }
        }
        if (T.follow)
            T.scroll = maxs;
        T.scroll = fminf(fmaxf(T.scroll, 0), maxs);
        if (T.scroll >= maxs - 1)
            T.follow = true;
        gfx_clip_push(sdl(logr));
        int i0 = (int)(T.scroll / lh);
        for (int i = i0; i < T.nlog; i++) {
            float ly = logr.y + 8 + (float)i * lh - T.scroll;
            if (ly > logr.y + logr.h)
                break;
            const char *s = log_line(i);
            Tone t = strstr(s, "identical to the saved world") ? TONE_GOOD
                     : strstr(s, "turned away") || strstr(s, "not this world") ? TONE_MUTED
                                                                                : TONE_TEXT;
            skin_text(fo->small, s, logr.x + 10, ly, t, ALIGN_LEFT);
        }
        gfx_clip_pop();
    }
    if (T.status[0] && SDL_GetTicks() - T.status_at < 8000)
        skin_text(fo->small, T.status, x, panel.y + panel.h - PAD - skin_text_height(fo->small), TONE_ACCENT,
                  ALIGN_LEFT);
}

void syn_telescope_chosen(App *a, const char *path)
{
    (void)a;
    choose(path, path);
}

void syn_telescope_shot_prepare(App *a)
{
    (void)a;
    const char *f = getenv("SYN_BIOGENESIS_SHOT_TELESCOPE");
    if (!f)
        return;
    choose(f, f);
    T.defaults = getenv("SYN_BIOGENESIS_SHOT_TELESCOPE_DEFAULTS") != NULL;
    /* the search to its end before the picture */
    begin();
    while (T.running) {
        arena_os_wait(&T.proc.from, 1, 200);
        poll_search();
    }
}

void syn_telescope_screen_free(void)
{
    stop();
    memset(&T, 0, sizeof T);
    T.follow = true;
}

#else

void syn_telescope_frame(struct App *a, int w, int h)
{
    (void)a;
    (void)w;
    (void)h;
}

void syn_telescope_chosen(struct App *a, const char *path)
{
    (void)a;
    (void)path;
}

void syn_telescope_shot_prepare(struct App *a)
{
    (void)a;
}

void syn_telescope_screen_free(void)
{
}

#endif
