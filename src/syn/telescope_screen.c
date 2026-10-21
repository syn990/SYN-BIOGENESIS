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

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "compat.h"
#include "io/bgg.h"
#include "io/bgw.h"
#include "sim/params.h"
#include "syn/arena.h"
#include "syn/arena_os.h"
#include "syn/batch.h"
#include "syn/crack.h"
#include "syn/family.h"
#include "syn/library_screen.h"
#include "syn/palette.h"
#include "syn/shell.h"
#include "synos/synos.h"
#include "ui/app.h"
#include "ui/gfx.h"
#include "ui/ui.h"

#define PAD 24.0f
#define LOG 400
#define LINE 240
#define HITS 400

enum { M_SEED, M_BEING, M_FORGE };

/* A scene the finder found: a frame of a world where the being, or one of
 * its genes, is. */
typedef struct {
    bool round;    /* an arena round of the record, else a seed's world */
    int set;       /* the seed's: which parameters (F.set) */
    int index;     /* the round's place in the record */
    int cycle;     /* the round's number */
    int64_t seed;
    int64_t frame;
    char kind[12];
    int n, d;
} Hit;

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
    int mode;    /* M_SEED, M_BEING */
} T = {.follow = true};

/* where a being lives: the search, its scenes, and the jump into one */
static struct {
    char name[80];
    char path[1200]; /* the being as a .bgg file, for the search */
    GeneticCode *gc;
    bool running, done;
    ArenaProc proc;
    char part[4096];
    size_t npart;
    char log[8][LINE];
    int nlog;
    struct {
        int w, h, n;
    } set[4];
    int nsets;
    Hit hit[HITS];
    int nhits;
    int cur_round, cur_cycle; /* the last "@round" told */
    int prog_k, prog_n;       /* the seed being run, of how many */
    int64_t cur_seed;
    /* what the engine read of it (syn/crack.h) */
    bool anc_told;
    uint64_t anc_start;
    int anc_proof, anc_kept, anc_changes;
    GeneticCode *anc; /* NULL once told: none, its seed has faded */
    struct {
        int gene;
        int64_t place;
        char kind; /* l: its length drawn again, w: a gene added whole, a: its angle */
    } draw[24];
    int ndraws;
    int basin[4]; /* the seeds that make the ancestor, per set */
    double started, took;
    float scroll;
    /* the jump: a scene being made, then opened */
    bool jumping;
    ArenaProc jproc;
    char jpart[256];
    size_t njpart;
    int jhit; /* its row among the scenes (-1: a forged winner's) */
    Hit jh;   /* the scene being made */
    int64_t jframe_now;
    char jpath[1200];
    Batch b;
} F;

/* a winner from the dice: seeds weighed by their founders alone, the
 * likeliest run until one bloodline is left (syn/crack.h) */
#define FORGED 32
static struct {
    bool running, done;
    ArenaProc proc;
    char part[4096];
    size_t npart;
    int64_t scan, from, next_from;
    int top;
    int64_t scanned;
    int prog_k, prog_n;
    int w, h, n; /* the world forged in: the arena's */
    char dir[1200];
    struct {
        int64_t seed;
        int win, lone, t, copies, changes;
        double chance;
        GeneticCode *gc;
    } r[FORGED];
    int nr;
    char last[LINE];
    double started, took;
    float scroll;
} G = {.scan = 20000, .top = 8, .next_from = 1};

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

/* the configuration folder, made if it isn't there yet (a first run):
 * the searches' files go in it */
static void config_dir_make(void)
{
    const char *dir = params_config_dir();
    char parent[1024];
    snprintf(parent, sizeof parent, "%s", dir);
    char *slash = strrchr(parent, '/');
    if (slash) {
        *slash = '\0';
        mkdir(parent, 0755);
    }
    mkdir(dir, 0755);
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
    config_dir_make();
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

/* ---- where a being lives ---- */

static void find_log(const char *s)
{
    if (F.nlog == 8) {
        memmove(F.log[0], F.log[1], sizeof F.log - sizeof F.log[0]);
        F.nlog--;
    }
    copy(F.log[F.nlog++], LINE, s);
}

static void find_stop(void)
{
    if (F.running)
        arena_os_end(&F.proc);
    F.running = false;
}

static void jump_stop(void)
{
    if (F.jumping)
        arena_os_end(&F.jproc);
    F.jumping = false;
}

static void find_begin(void);

void syn_telescope_find(const GeneticCode *gc, const char *name)
{
    find_stop();
    jump_stop();
    T.mode = M_BEING;
    snprintf(F.name, sizeof F.name, "%s", name && name[0] ? name : "this being");
    config_dir_make();
    snprintf(F.path, sizeof F.path, "%s/telescope-being.bgg", params_config_dir());
    FILE *f = fopen(F.path, "wb");
    if (f) {
        bgg_write_file(f, gc);
        fclose(f);
    }
    char err[200];
    gc_unref(F.gc);
    F.gc = f ? bgg_read(F.path, err, sizeof err) : NULL;
    F.nhits = F.nlog = F.nsets = 0;
    F.done = false;
    F.scroll = 0;
    F.anc_told = false;
    gc_unref(F.anc);
    F.anc = NULL;
    F.ndraws = 0;
    if (!F.gc)
        say("The being couldn't be kept for the search");
    else
        find_begin();
}

static void find_begin(void)
{
    find_stop();
    F.nhits = F.nlog = F.nsets = 0;
    F.done = false;
    F.scroll = 0;
    F.npart = 0;
    F.cur_round = -1;
    F.prog_k = F.prog_n = 0;
    F.anc_told = false;
    gc_unref(F.anc);
    F.anc = NULL;
    F.ndraws = 0;
    for (int i = 0; i < 4; i++)
        F.basin[i] = -1;
    /* the parameters as saved, and the arena's world if it differs */
    int w, h, n;
    syn_arena_init();
    syn_arena_world(&w, &h, &n);
    char ws[16], hs[16], ns[16];
    snprintf(ws, sizeof ws, "%d", w);
    snprintf(hs, sizeof hs, "%d", h);
    snprintf(ns, sizeof ns, "%d", n);
    bool other = w != P.world_width || h != P.world_height || n != P.initial_organisms;
    const char *args[] = {"--telescope", "find", F.path, "--progress", "--saved", "--world", ws, hs, ns};
    char err[300];
    if (!arena_os_spawn(&F.proc, args, other ? 9 : 5, err, sizeof err)) {
        char msg[400];
        snprintf(msg, sizeof msg, "The search didn't start: %s", err);
        say(msg);
        return;
    }
    F.running = true;
    F.started = arena_os_now();
}

static void find_line(const char *s)
{
    Hit h;
    memset(&h, 0, sizeof h);
    long long seed, frame;
    int a, b, c, d;
    char kind[12];
    if (sscanf(s, "@progress %d %d %d", &a, &b, &c) == 3) {
        F.prog_k = b;
        F.prog_n = c;
        return;
    }
    unsigned long long st;
    if (!strcmp(s, "@ancestor none")) {
        F.anc_told = true;
        return;
    }
    if (sscanf(s, "@ancestor %llx %d %d %d", &st, &a, &b, &c) == 4) {
        F.anc_told = true;
        F.anc_start = st;
        F.anc_proof = a;
        F.anc_kept = b;
        F.anc_changes = c;
        gc_unref(F.anc);
        F.anc = crack_founder_at(st);
        return;
    }
    char dk;
    if (sscanf(s, "@draw %d %lld %c", &a, &frame, &dk) == 3) {
        if (F.ndraws < 24) {
            F.draw[F.ndraws].gene = a;
            F.draw[F.ndraws].place = frame;
            F.draw[F.ndraws++].kind = dk;
        }
        return;
    }
    if (sscanf(s, "@basin %d %d", &a, &b) == 2) {
        if (a >= 0 && a < 4)
            F.basin[a] = b;
        return;
    }
    if (!strncmp(s, "@seed ", 6))
        return;
    if (sscanf(s, "@set %d %d %d %d", &a, &b, &c, &d) == 4) {
        if (a >= 0 && a < 4) {
            F.set[a].w = b;
            F.set[a].h = c;
            F.set[a].n = d;
            F.nsets = a + 1 > F.nsets ? a + 1 : F.nsets;
        }
        return;
    }
    if (sscanf(s, "@scene %d %lld %lld %11s %d %d", &a, &seed, &frame, kind, &c, &d) == 6) {
        h.set = a;
        h.seed = seed;
    } else if (sscanf(s, "@round %d %d %lld", &a, &b, &seed) == 3) {
        F.cur_round = a;
        F.cur_cycle = b;
        F.cur_seed = seed;
        return;
    } else if (sscanf(s, "@rscene %d %lld %11s %d %d", &a, &frame, kind, &c, &d) == 5) {
        h.round = true;
        h.index = a;
        h.cycle = a == F.cur_round ? F.cur_cycle : 0;
        h.seed = a == F.cur_round ? F.cur_seed : 0;
    } else {
        if (!strcmp(s, "@done") || !strcmp(s, "@none"))
            F.done = true;
        else if (s[0] != '@')
            find_log(s);
        return;
    }
    h.frame = frame;
    snprintf(h.kind, sizeof h.kind, "%s", kind);
    h.n = c;
    h.d = d;
    if (F.nhits == HITS)
        return;
    /* a world's scenes come together: in it, by tick */
    int at = F.nhits;
    while (at > 0 && F.hit[at - 1].round == h.round && F.hit[at - 1].seed == h.seed &&
           F.hit[at - 1].index == h.index && F.hit[at - 1].frame > h.frame)
        at--;
    memmove(&F.hit[at + 1], &F.hit[at], (size_t)(F.nhits - at) * sizeof *F.hit);
    F.hit[at] = h;
    F.nhits++;
}

/* lines from a child, a whole line at a time; false when it has ended */
static bool child_lines(ArenaProc *p, char *part, size_t cap, size_t *np, void (*line)(const char *))
{
    char buf[4096];
    for (;;) {
        long m = arena_os_read(p, buf, sizeof buf);
        if (m == 0)
            return true;
        if (m < 0) {
            if (*np) {
                part[*np] = '\0';
                line(part);
                *np = 0;
            }
            return false;
        }
        for (long i = 0; i < m; i++) {
            if (buf[i] == '\n' || *np + 1 >= cap) {
                part[*np] = '\0';
                line(part);
                *np = 0;
            } else if (buf[i] != '\r') {
                part[(*np)++] = buf[i];
            }
        }
    }
}

static void poll_find(void)
{
    if (F.running && !child_lines(&F.proc, F.part, sizeof F.part, &F.npart, find_line)) {
        arena_os_end(&F.proc);
        F.running = false;
        F.took = arena_os_now() - F.started;
        if (!F.done)
            say("The search stopped before it could say");
    }
}

static App *g_app; /* for the jump's end */
static uint64_t g_jstate;
static bool g_jat;

static void jump_line(const char *s)
{
    unsigned long long st;
    long long fr;
    if (sscanf(s, "@at %llx", &st) == 1) {
        g_jstate = st;
        g_jat = true;
    } else if (sscanf(s, "@frame %lld", &fr) == 1) {
        F.jframe_now = fr;
    }
}

/* The scene made by this program again, then opened: a round of the
 * record, or a seed's world (whn: its size and first organisms, NULL: the
 * parameters as saved). */
static void jump_spawn(const Hit *h, const int *whn, int row)
{
    jump_stop();
    config_dir_make();
    snprintf(F.jpath, sizeof F.jpath, "%s/telescope-scene.bgw", params_config_dir());
    char a1[32], a2[32], ws[16], hs[16], ns[16];
    const char *args[12];
    int n = 0;
    args[n++] = "--telescope";
    args[n++] = "scene";
    if (h->round) {
        args[n++] = "--round";
        snprintf(a1, sizeof a1, "%d", h->index);
    } else {
        snprintf(a1, sizeof a1, "%" PRId64, h->seed);
    }
    snprintf(a2, sizeof a2, "%" PRId64, h->frame);
    args[n++] = a1;
    args[n++] = a2;
    args[n++] = F.jpath;
    args[n++] = "--progress";
    if (!h->round && whn) {
        snprintf(ws, sizeof ws, "%d", whn[0]);
        snprintf(hs, sizeof hs, "%d", whn[1]);
        snprintf(ns, sizeof ns, "%d", whn[2]);
        args[n++] = "--world";
        args[n++] = ws;
        args[n++] = hs;
        args[n++] = ns;
    }
    char err[300];
    g_jat = false;
    if (!arena_os_spawn(&F.jproc, args, n, err, sizeof err)) {
        char msg[400];
        snprintf(msg, sizeof msg, "The scene couldn't be made: %s", err);
        say(msg);
        return;
    }
    F.jumping = true;
    F.jhit = row;
    F.jh = *h;
    F.jframe_now = 0;
}

static void jump_begin(int i)
{
    const Hit *h = &F.hit[i];
    int whn[3];
    bool other = !h->round && h->set > 0 && h->set < F.nsets;
    if (other) {
        whn[0] = F.set[h->set].w;
        whn[1] = F.set[h->set].h;
        whn[2] = F.set[h->set].n;
    }
    jump_spawn(h, other ? whn : NULL, i);
}

static void poll_jump(App *a)
{
    if (!F.jumping || child_lines(&F.jproc, F.jpart, sizeof F.jpart, &F.njpart, jump_line))
        return;
    arena_os_end(&F.jproc);
    F.jumping = false;
    if (!g_jat) {
        say("The scene couldn't be made");
        return;
    }
    /* opened with the generator where it was there: the world goes on as
     * it went on */
    const Hit *h = &F.jh;
    app_file_chosen(a, FD_OPEN_WORLD, F.jpath);
    g_rng.seed = g_jstate & ((1ULL << 48) - 1);
    char msg[400];
    if (h->round)
        snprintf(msg, sizeof msg, "Arena round %d (seed %" PRId64 ") at tick %" PRId64 ", on its own thread", h->cycle,
                 h->seed, h->frame);
    else
        snprintf(msg, sizeof msg, "Seed %" PRId64 " at tick %" PRId64 ", on its own thread", h->seed, h->frame);
    app_set_status(a, msg);
    synos_set_screen(SYNOS_WORLD);
}

static void hit_text(const Hit *h, char *out, size_t n)
{
    if (!strcmp(h->kind, "founder"))
        snprintf(out, n, "its gene %d, in a founder at the world's start", h->n);
    else if (!strcmp(h->kind, "gene"))
        snprintf(out, n, "its gene %d drawn here", h->n);
    else if (!strcmp(h->kind, "born"))
        snprintf(out, n, "born: its newest gene drawn, %d alive just after", h->n);
    else if (!strcmp(h->kind, "alive"))
        snprintf(out, n, "alive: %d of it", h->n);
    else if (!strcmp(h->kind, "most"))
        snprintf(out, n, "the most of it: %d", h->n);
    else if (!strcmp(h->kind, "nearest"))
        snprintf(out, n, "its nearest relative: %d change%s", h->d, h->d == 1 ? "" : "s");
    else if (!strcmp(h->kind, "final"))
        snprintf(out, n, "the round ended with it (%d living)", h->n);
    else if (!strcmp(h->kind, "genesis"))
        snprintf(out, n, "its bloodline began the round as it");
    else if (!strcmp(h->kind, "ancestor"))
        snprintf(out, n, "its genesis ancestor, founder %d of the world", h->n);
    else if (!strcmp(h->kind, "lone"))
        snprintf(out, n, "its ancestor's bloodline alone: all %d living descend from it", h->n);
    else if (!strcmp(h->kind, "reigns"))
        snprintf(out, n, "it reigns: %d of it alive, its bloodline the only one", h->n);
    else if (!strcmp(h->kind, "gone"))
        snprintf(out, n, "its ancestor's bloodline dies out");
    else
        snprintf(out, n, "%s", h->kind);
}

static SDL_Color raw(int c, Uint8 a)
{
    uint8_t r, g, b;
    bio_color_rgb(c, &r, &g, &b);
    return (SDL_Color){r, g, b, a};
}

/* A code's body in r, on black, as the laboratory draws it. */
static void body(UIRect r, const GeneticCode *gc)
{
    skin_fill(r, (Rgb){0, 0, 0}, 1);
    if (!gc)
        return;
    int n = syn_genes(gc) * gc->symmetry;
    GcLine stackbuf[256];
    GcLine *lines = n <= 256 ? stackbuf : malloc((size_t)n * sizeof *lines);
    if (!lines)
        return;
    int w, h;
    int m = gc_layout(gc, lines, n, &w, &h);
    float scale = fminf(r.w * 0.86f / (float)(w > 0 ? w : 1), r.h * 0.86f / (float)(h > 0 ? h : 1));
    if (scale > 4)
        scale = 4;
    float cx = r.x + r.w / 2, cy = r.y + r.h / 2, hw = fminf(1.6f, 0.7f + 0.25f * scale);
    batch_begin(&F.b, TX_DISC, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < m; i++)
        batch_capsule(&F.b, cx + (float)lines[i].x0 * scale, cy + (float)lines[i].y0 * scale,
                      cx + (float)lines[i].x1 * scale, cy + (float)lines[i].y1 * scale, hw,
                      raw(lines[i].color, 255));
    batch_flush(&F.b, gfx_renderer());
    if (lines != stackbuf)
        free(lines);
}

static void being_view(App *a, float x, float y, float tw, float bottom)
{
    const Fonts *fo = synos_fonts();
    char line[400], fit[1200];
    y = para(fo->body,
             "Its genes read back to where they were drawn: every gene length is one draw of the world's dice, "
             "so it points to a place in a seed's stream. A gene its founders had gives the seed; the arena's "
             "rounds, whose seeds are in the record, are searched for the rest. Each scene opens that world at "
             "that tick, on its own thread.",
             x, y, tw, TONE_MUTED) +
        12;
    UIRect pic = {x, y, 150, 110};
    body(pic, F.gc);
    float bx = pic.x + pic.w + 18;
    skin_text(fo->heading, F.gc ? F.name : "No being chosen", bx, y + 2, F.gc ? TONE_TEXT : TONE_MUTED, ALIGN_LEFT);
    if (F.gc) {
        char plan[200];
        syn_family_plan(F.gc, plan, sizeof plan);
        skin_text(fo->small, skin_fit(fo->small, plan, x + tw - bx, fit, sizeof fit), bx,
                  y + skin_text_height(fo->heading) + 8, TONE_MUTED, ALIGN_LEFT);
    } else {
        skin_text(fo->small, "Pick one in the library and choose WHERE IT LIVES.", bx,
                  y + skin_text_height(fo->heading) + 8, TONE_MUTED, ALIGN_LEFT);
    }
    float by = y + 110 - 38;
    UIRect go = {bx, by, 250, 38}, lib = {go.x + go.w + 8, by, 130, 38};
    if (button(go, F.running ? "STOP" : "FIND WHERE IT LIVES", F.running, F.running || F.gc)) {
        if (F.running) {
            find_stop();
            say("Stopped");
        } else {
            find_begin();
        }
    }
    if (button(lib, "LIBRARY", false, true)) {
        syn_library_open_from(SYNOS_TELESCOPE);
        synos_set_screen(SYNOS_LIBRARY);
        return;
    }
    double secs = F.running ? arena_os_now() - F.started : F.took;
    if (F.running || F.done) {
        char now[80] = "";
        if (F.running && F.prog_n)
            snprintf(now, sizeof now, "  ·  running seed %d of %d", F.prog_k, F.prog_n);
        else if (F.running)
            snprintf(now, sizeof now, "  ·  searching");
        snprintf(line, sizeof line, "%d scene%s  ·  %.0f s%s", F.nhits, F.nhits == 1 ? "" : "s", secs, now);
        skin_text(fo->body, line, lib.x + lib.w + 20, by + 10, F.running ? TONE_ACCENT : TONE_MUTED, ALIGN_LEFT);
    }
    y += 110 + 16;

    /* what the engine read of it: its genesis ancestor and its later draws */
    if (F.anc_told && F.anc) {
        UIRect ap = {x, y, 150, 78};
        body(ap, F.anc);
        float ax = ap.x + ap.w + 18, aw = x + tw - ax;
        float lh2 = skin_text_height(fo->small) + 4;
        skin_text(fo->small_bold, "ITS GENESIS ANCESTOR", ax, y, TONE_ACCENT, ALIGN_LEFT);
        float ty = y + skin_text_height(fo->small_bold) + 5;
        char plan[200];
        syn_family_plan(F.anc, plan, sizeof plan);
        skin_text(fo->small, skin_fit(fo->small, plan, aw, fit, sizeof fit), ax, ty, TONE_TEXT, ALIGN_LEFT);
        ty += lh2;
        snprintf(line, sizeof line, "drawn again from state 0x%012" PRIx64 "  ·  %d of its %d genes kept, %d mutation%s at"
                 " the fewest", F.anc_start, F.anc_kept, F.gc ? F.gc->ngenes : 0, F.anc_changes,
                 F.anc_changes == 1 ? "" : "s");
        for (int i = 0; i < F.nsets && i < 4; i++)
            if (F.basin[i] >= 0) {
                size_t k = strlen(line);
                snprintf(line + k, sizeof line - k, "%sa founder in %d seed%s with %d x %d and %d first organisms",
                         k ? "  ·  " : "", F.basin[i], F.basin[i] == 1 ? "" : "s", F.set[i].w, F.set[i].h, F.set[i].n);
            }
        if (line[0])
            skin_text(fo->small, skin_fit(fo->small, line, aw, fit, sizeof fit), ax, ty, TONE_MUTED, ALIGN_LEFT);
        ty += lh2;
        line[0] = '\0';
        for (int i = 0; i < F.ndraws; i++) {
            size_t k = strlen(line);
            const char *what = F.draw[i].kind == 'a' ? "angle" : F.draw[i].kind == 'w' ? "gene, added" : "length";
            if (F.draw[i].place >= 0)
                snprintf(line + k, sizeof line - k, "%sgene %d's %s at draw %" PRId64, k ? "  ·  " : "Drawn since: ",
                         F.draw[i].gene, what, F.draw[i].place);
            else
                snprintf(line + k, sizeof line - k, "%sgene %d's %s in another stream", k ? "  ·  " : "Drawn since: ",
                         F.draw[i].gene, what);
        }
        if (!F.ndraws)
            snprintf(line, sizeof line, "None of its genes drawn since: it is in every one of those worlds from tick 0"
                                        " or comes of it by what can't be placed (colours, arms, genes dropped)");
        skin_text(fo->small, skin_fit(fo->small, line, aw, fit, sizeof fit), ax, ty, TONE_MUTED, ALIGN_LEFT);
        y += ap.h + 14;
    } else if (F.anc_told) {
        y = para(fo->small,
                 "No genesis ancestor: none of its genes is still a founder's with another of its draws to prove it. "
                 "Its seed has faded from it; only the arena's rounds can place it.",
                 x, y, tw, TONE_MUTED) +
            10;
    }

    /* the search's last words: fewer once there are scenes to show */
    float lh = skin_text_height(fo->small) + 3;
    int nlog = F.nhits && F.nlog > 3 ? 3 : F.nlog;
    float logh = (float)nlog * lh;
    /* the scenes, by world */
    UIRect list = {x, y, tw, bottom - y - logh - 10};
    if (list.h > 60 && F.nhits) {
        float rh = 40, hh = skin_text_height(fo->small_bold) + 14;
        float total = 0;
        for (int i = 0; i < F.nhits; i++) {
            bool head = !i || F.hit[i].round != F.hit[i - 1].round || F.hit[i].seed != F.hit[i - 1].seed ||
                        F.hit[i].index != F.hit[i - 1].index;
            total += rh + (head ? hh : 0);
        }
        float maxs = fmaxf(0, total - list.h);
        if (over(list))
            F.scroll -= ui_wheel_fy() * rh * 2;
        F.scroll = fminf(fmaxf(F.scroll, 0), maxs);
        gfx_clip_push(sdl(list));
        float ry = list.y - F.scroll;
        for (int i = 0; i < F.nhits; i++) {
            const Hit *hh_ = &F.hit[i];
            bool head = !i || hh_->round != F.hit[i - 1].round || hh_->seed != F.hit[i - 1].seed ||
                        hh_->index != F.hit[i - 1].index;
            if (head) {
                if (hh_->round)
                    snprintf(line, sizeof line, "ARENA ROUND %d  ·  seed %" PRId64 "  ·  as the round began: its seed and fighters",
                             hh_->cycle, hh_->seed);
                else if (hh_->set >= 0 && hh_->set < F.nsets)
                    snprintf(line, sizeof line, "SEED %" PRId64 "  ·  %d x %d, %d first organisms", hh_->seed,
                             F.set[hh_->set].w, F.set[hh_->set].h, F.set[hh_->set].n);
                else
                    snprintf(line, sizeof line, "SEED %" PRId64, hh_->seed);
                if (ry + hh > list.y && ry < list.y + list.h)
                    skin_text(fo->small_bold, line, list.x, ry + 8, TONE_ACCENT, ALIGN_LEFT);
                ry += hh;
            }
            if (ry + rh > list.y && ry < list.y + list.h) {
                UIRect row = {list.x, ry, list.w, rh - 4};
                skin_fill(row, skin_theme()->accent, over(row) ? 0.12f : 0.05f);
                char what[200];
                hit_text(hh_, what, sizeof what);
                skin_text(fo->body, what, row.x + 12, row.y + 9,
                          !strcmp(hh_->kind, "reigns") ? TONE_GOOD : !strcmp(hh_->kind, "gone") ? TONE_MUTED : TONE_TEXT,
                          ALIGN_LEFT);
                snprintf(line, sizeof line, "tick %" PRId64 "  ·  time unit %" PRId64, hh_->frame, hh_->frame / 256);
                skin_text(fo->small, line, row.x + row.w - 150, row.y + 11, TONE_MUTED, ALIGN_RIGHT);
                UIRect jb = {row.x + row.w - 130, row.y + 3, 120, row.h - 6};
                bool busy = F.jumping && F.jhit == i;
                if (button(jb, busy ? "MAKING..." : "JUMP", busy, !F.jumping))
                    jump_begin(i);
                if (over(jb) && !F.jumping)
                    ui_tooltip("Make this world at this tick and open it, the generator where it was there");
            }
            ry += rh;
        }
        gfx_clip_pop();
    } else if (F.done && !F.nhits) {
        para(fo->body,
             "Nowhere found. Every gene it has was drawn after some world's start, in no round of the record: "
             "a gene drawn later says where in a stream it was drawn, not which stream. Its seed has faded.",
             x, y, tw, TONE_WARN);
    }
    float ly = bottom - logh;
    for (int i = 0; i < nlog; i++)
        skin_text(fo->small, skin_fit(fo->small, F.log[F.nlog - nlog + i], tw, fit, sizeof fit), x,
                  ly + (float)i * lh, TONE_MUTED, ALIGN_LEFT);
    if (F.jumping) {
        snprintf(line, sizeof line, "Making the scene: tick %" PRId64 " of %" PRId64, F.jframe_now, F.jh.frame);
        say(line);
    }
    (void)a;
}

/* ---- a winner from the dice ---- */

static void forge_stop(void)
{
    if (G.running)
        arena_os_end(&G.proc);
    G.running = false;
}

static void forge_clear(void)
{
    for (int i = 0; i < G.nr; i++)
        gc_unref(G.r[i].gc);
    G.nr = 0;
}

static void forge_begin(void)
{
    forge_stop();
    forge_clear();
    G.done = false;
    G.scanned = 0;
    G.prog_k = G.prog_n = 0;
    G.npart = 0;
    G.last[0] = '\0';
    G.scroll = 0;
    /* each forging goes on where the last left off: the seeds in order */
    G.from = G.next_from;
    syn_arena_init();
    syn_arena_world(&G.w, &G.h, &G.n);
    config_dir_make();
    snprintf(G.dir, sizeof G.dir, "%s/telescope-forge", params_config_dir());
    if (mkdir(G.dir, 0755) != 0 && errno != EEXIST) {
        say("The forge has nowhere to put its winners");
        return;
    }
    char from[32], scan[32], top[16], ws[16], hs[16], ns[16];
    snprintf(from, sizeof from, "%" PRId64, G.from);
    snprintf(scan, sizeof scan, "%" PRId64, G.scan);
    snprintf(top, sizeof top, "%d", G.top);
    snprintf(ws, sizeof ws, "%d", G.w);
    snprintf(hs, sizeof hs, "%d", G.h);
    snprintf(ns, sizeof ns, "%d", G.n);
    const char *args[] = {"--telescope", "forge", "--progress", "--from", from, "--scan", scan, "--top",
                          top, "--out", G.dir, "--world", ws, hs, ns};
    char err[300];
    if (!arena_os_spawn(&G.proc, args, 15, err, sizeof err)) {
        char msg[400];
        snprintf(msg, sizeof msg, "The forge didn't start: %s", err);
        say(msg);
        return;
    }
    G.running = true;
    G.started = arena_os_now();
    G.next_from = G.from + G.scan;
}

static void forge_line(const char *s)
{
    long long seed, scanned, total;
    int win, lone, t, copies, changes, a, b, c;
    double chance;
    char path[1200];
    if (sscanf(s, "@scan %lld %lld", &scanned, &total) == 2) {
        G.scanned = scanned;
    } else if (sscanf(s, "@progress %d %d %d", &a, &b, &c) == 3) {
        G.prog_k = b;
        G.prog_n = c;
    } else if (sscanf(s, "@forge %lld %d %d %d %lf %d %d", &seed, &win, &lone, &t, &chance, &copies, &changes) == 7) {
        if (G.nr < FORGED) {
            G.r[G.nr].seed = seed;
            G.r[G.nr].win = win;
            G.r[G.nr].lone = lone;
            G.r[G.nr].t = t;
            G.r[G.nr].copies = copies;
            G.r[G.nr].changes = changes;
            G.r[G.nr].chance = chance;
            G.r[G.nr++].gc = NULL;
        }
    } else if (sscanf(s, "@file %lld %1199[^\n]", &seed, path) == 2) {
        for (int i = 0; i < G.nr; i++)
            if (G.r[i].seed == seed && !G.r[i].gc) {
                char err[200];
                G.r[i].gc = bgg_read(path, err, sizeof err);
            }
    } else if (!strcmp(s, "@done")) {
        G.done = true;
    } else if (s[0] != '@') {
        copy(G.last, sizeof G.last, s);
    }
}

static void poll_forge(void)
{
    if (G.running && !child_lines(&G.proc, G.part, sizeof G.part, &G.npart, forge_line)) {
        arena_os_end(&G.proc);
        G.running = false;
        G.took = arena_os_now() - G.started;
        if (!G.done)
            say("The forge stopped before it could say");
    }
}

static void forge_view(App *a, float x, float y, float tw, float bottom)
{
    const Fonts *fo = synos_fonts();
    char line[400], fit[1200];
    y = para(fo->body,
             "A winner from the dice. Each seed's founders are made without running its world and weighed by how "
             "fast each turns light into children. Weighed so, the fittest founder's bloodline was the one left in "
             "45% of 240 worlds run to the end, and 46% of 240 it wasn't fitted on, against 7% by chance. The seeds "
             "whose likeliest bloodline is likeliest are run until one bloodline is left, and the being it is left "
             "as is the winner. Each forging goes on through the seeds where the last stopped.",
             x, y, tw, TONE_MUTED) +
        12;
    skin_text(fo->small_bold, "SEEDS TO WEIGH", x, y, TONE_MUTED, ALIGN_LEFT);
    skin_text(fo->small_bold, "TO RUN", x + 520, y, TONE_MUTED, ALIGN_LEFT);
    y += skin_text_height(fo->small_bold) + 6;
    static const int64_t scans[] = {20000, 200000, 2000000};
    static const char *scan_s[] = {"20,000", "200,000", "2,000,000"};
    for (int i = 0; i < 3; i++)
        if (button((UIRect){x + (float)i * 166, y, 160, 34}, scan_s[i], G.scan == scans[i], !G.running))
            G.scan = scans[i];
    static const int tops[] = {4, 8, 16};
    for (int i = 0; i < 3; i++) {
        char t[8];
        snprintf(t, sizeof t, "%d", tops[i]);
        if (button((UIRect){x + 520 + (float)i * 76, y, 70, 34}, t, G.top == tops[i], !G.running))
            G.top = tops[i];
    }
    y += 34 + 14;
    UIRect go = {x, y, 200, 40};
    if (button(go, G.running ? "STOP" : "FORGE", G.running, true)) {
        if (G.running) {
            forge_stop();
            say("Stopped");
        } else {
            forge_begin();
        }
    }
    if (over(go) && !G.running)
        ui_tooltip("Weigh the next seeds and run the likeliest, in the arena's world");
    double secs = G.running ? arena_os_now() - G.started : G.took;
    if (G.running && G.prog_n)
        snprintf(line, sizeof line, "seeds %" PRId64 " to %" PRId64 " weighed  ·  running %d of %d  ·  %.0f s", G.from,
                 G.from + G.scan - 1, G.prog_k, G.prog_n, secs);
    else if (G.running)
        snprintf(line, sizeof line, "weighing seeds from %" PRId64 ": %" PRId64 " of %" PRId64 "  ·  %.0f s", G.from,
                 G.scanned, G.scan, secs);
    else if (G.done)
        snprintf(line, sizeof line, "seeds %" PRId64 " to %" PRId64 ", %d x %d with %d first organisms  ·  %.0f s",
                 G.from, G.from + G.scan - 1, G.w, G.h, G.n, secs);
    else
        snprintf(line, sizeof line, "next: from seed %" PRId64, G.next_from);
    skin_text(fo->body, line, go.x + go.w + 20, y + 10, G.running ? TONE_ACCENT : TONE_MUTED, ALIGN_LEFT);
    y += 40 + 16;

    float lh = skin_text_height(fo->small) + 3;
    UIRect list = {x, y, tw, bottom - y - (G.last[0] ? lh + 6 : 0)};
    float rh = 74;
    float maxs = fmaxf(0, (float)G.nr * rh - list.h);
    if (over(list))
        G.scroll -= ui_wheel_fy() * rh;
    G.scroll = fminf(fmaxf(G.scroll, 0), maxs);
    gfx_clip_push(sdl(list));
    for (int i = 0; i < G.nr; i++) {
        float ry = list.y + (float)i * rh - G.scroll;
        if (ry + rh < list.y || ry > list.y + list.h)
            continue;
        UIRect row = {list.x, ry, list.w, rh - 6};
        skin_fill(row, skin_theme()->accent, over(row) ? 0.12f : 0.05f);
        body((UIRect){row.x + 6, row.y + 6, 96, row.h - 12}, G.r[i].gc);
        float tx = row.x + 116;
        bool acts = G.r[i].gc && G.r[i].lone >= 0;
        UIRect bj = {row.x + row.w - 126, row.y + (row.h - 34) / 2, 118, 34};
        UIRect bk = {bj.x - 104, bj.y, 98, 34}, bw = {bk.x - 180, bj.y, 174, 34};
        float txw = (acts ? bw.x - 12 : row.x + row.w - 8) - tx;
        snprintf(line, sizeof line, "SEED %" PRId64 "  ·  founder %d expected, %.0f%%", G.r[i].seed, G.r[i].win + 1,
                 100 * G.r[i].chance);
        skin_text(fo->body, skin_fit(fo->body, line, txw, fit, sizeof fit), tx, row.y + 10, TONE_TEXT, ALIGN_LEFT);
        Tone tone = TONE_MUTED;
        if (G.r[i].lone == -1)
            snprintf(line, sizeof line, "all died by time %d", G.r[i].t);
        else if (G.r[i].lone == -2)
            snprintf(line, sizeof line, "more than one bloodline still at time %d", G.r[i].t);
        else {
            snprintf(line, sizeof line, "founder %d's bloodline alone at time %d, as %d of this being (%d mutation%s from "
                     "that founder)%s", G.r[i].lone + 1, G.r[i].t, G.r[i].copies, G.r[i].changes,
                     G.r[i].changes == 1 ? "" : "s", G.r[i].lone == G.r[i].win ? "  ·  as expected" : "");
            tone = G.r[i].lone == G.r[i].win ? TONE_GOOD : TONE_TEXT;
        }
        skin_text(fo->small, skin_fit(fo->small, line, txw, fit, sizeof fit), tx,
                  row.y + 12 + skin_text_height(fo->body), tone, ALIGN_LEFT);
        if (!acts)
            continue;
        char name[64];
        snprintf(name, sizeof name, "seed %" PRId64, G.r[i].seed);
        bool busy = F.jumping && F.jhit == -1 && F.jh.seed == G.r[i].seed;
        if (button(bj, busy ? "MAKING..." : "JUMP", busy, !F.jumping)) {
            Hit h;
            memset(&h, 0, sizeof h);
            h.seed = G.r[i].seed;
            h.frame = (int64_t)G.r[i].t * 256;
            snprintf(h.kind, sizeof h.kind, "lone");
            int whn[3] = {G.w, G.h, G.n};
            jump_spawn(&h, whn, -1);
        }
        if (over(bj) && !F.jumping)
            ui_tooltip("Make this world at the time its bloodline was left alone, and open it");
        if (button(bk, "KEEP", false, true)) {
            char origin[200];
            snprintf(origin, sizeof origin, "forged: seed %" PRId64 ", founder %d's bloodline alone at time %d",
                     G.r[i].seed, G.r[i].lone + 1, G.r[i].t);
            syn_library_keep(a, G.r[i].gc, "Forged", name, origin);
        }
        if (over(bk))
            ui_tooltip("Keep it in the being library, in Forged");
        if (button(bw, "WHERE IT LIVES", false, true))
            syn_telescope_find(G.r[i].gc, name);
    }
    gfx_clip_pop();
    if (G.last[0])
        skin_text(fo->small, skin_fit(fo->small, G.last, tw, fit, sizeof fit), x, bottom - lh, TONE_MUTED, ALIGN_LEFT);
    if (F.jumping && F.jhit == -1) {
        snprintf(line, sizeof line, "Making the scene: tick %" PRId64 " of %" PRId64, F.jframe_now, F.jh.frame);
        say(line);
    }
}

/* ---- the screen ---- */

void syn_telescope_frame(App *a, int w, int h)
{
    const Fonts *fo = synos_fonts();
    poll_search();
    poll_find();
    poll_forge();
    g_app = a;
    poll_jump(a);
    bool keys = !ui_any_modal() && !ui_text_focused() && !ui_popup_showing();
    if (keys && ui_key_pressed(SDLK_ESCAPE)) {
        synos_set_screen(SYNOS_MENU);
        return;
    }
    skin_fill(ui_rect(0, 0, (float)w, (float)h), (Rgb){0, 0, 0}, 0.5f);
    float top = SHELL_BAR_H + 10;
    float pw = fminf((float)w - 48, 1500), ph = (float)h - top - 24;
    UIRect panel = {((float)w - pw) / 2, top, pw, ph};
    skin_panel(panel, true);
    float x = panel.x + PAD, y = panel.y + 14, right = panel.x + panel.w - PAD;
    x += skin_text(fo->big, "TELESCOPE", x, y, TONE_TEXT, ALIGN_LEFT) + 14;
    skin_text(fo->body, "Biogenesis 0.9", x, y + 9, TONE_MUTED, ALIGN_LEFT);
    y += 50;
    float tw = right - (panel.x + PAD);
    x = panel.x + PAD;
    UIRect t1 = {x, y, 200, 34}, t2 = {x + 206, y, 250, 34}, t3 = {t2.x + t2.w + 6, y, 220, 34};
    if (button(t1, "A WORLD'S SEED", T.mode == M_SEED, true))
        T.mode = M_SEED;
    if (button(t2, "WHERE A BEING LIVES", T.mode == M_BEING, true))
        T.mode = M_BEING;
    if (button(t3, "FORGE A WINNER", T.mode == M_FORGE, true))
        T.mode = M_FORGE;
    y += 34 + 16;
    if (T.mode == M_FORGE) {
        forge_view(a, x, y, tw, panel.y + panel.h - PAD - skin_text_height(fo->small) - 10);
        if (T.status[0] && SDL_GetTicks() - T.status_at < 8000)
            skin_text(fo->small, T.status, x, panel.y + panel.h - PAD - skin_text_height(fo->small), TONE_ACCENT,
                      ALIGN_LEFT);
        return;
    }
    if (T.mode == M_BEING) {
        being_view(a, x, y, tw, panel.y + panel.h - PAD - skin_text_height(fo->small) - 10);
        if (T.status[0] && SDL_GetTicks() - T.status_at < 8000)
            skin_text(fo->small, T.status, x, panel.y + panel.h - PAD - skin_text_height(fo->small), TONE_ACCENT,
                      ALIGN_LEFT);
        return;
    }
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
    /* SYN_BIOGENESIS_SHOT_FIND = a .bgg: where it lives, searched to the end */
    const char *b = getenv("SYN_BIOGENESIS_SHOT_FIND");
    if (b) {
        char err[200];
        GeneticCode *gc = bgg_read(b, err, sizeof err);
        if (!gc) {
            fprintf(stderr, "%s: %s\n", b, err);
            return;
        }
        const char *base = strrchr(b, '/');
        syn_telescope_find(gc, base ? base + 1 : b);
        gc_unref(gc);
        while (F.running) {
            arena_os_wait(&F.proc.from, 1, 200);
            poll_find();
        }
        return;
    }
    /* SYN_BIOGENESIS_SHOT_FORGE = how many seeds to weigh: a forging to its end */
    const char *fg = getenv("SYN_BIOGENESIS_SHOT_FORGE");
    if (fg) {
        T.mode = M_FORGE;
        G.scan = atoll(fg) > 0 ? atoll(fg) : G.scan;
        G.top = 4;
        forge_begin();
        while (G.running) {
            arena_os_wait(&G.proc.from, 1, 200);
            poll_forge();
        }
        return;
    }
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
    find_stop();
    jump_stop();
    forge_stop();
    forge_clear();
    gc_unref(F.gc);
    gc_unref(F.anc);
    batch_free(&F.b);
    memset(&F, 0, sizeof F);
    memset(&T, 0, sizeof T);
    T.follow = true;
    int64_t next = G.next_from;
    memset(&G, 0, sizeof G);
    G.scan = 20000;
    G.top = 8;
    G.next_from = next;
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

void syn_telescope_find(const struct GeneticCode *gc, const char *name)
{
    (void)gc;
    (void)name;
}

#endif
