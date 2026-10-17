/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Based on StatisticsWindow.java
 * of Biogenesis 0.9 (https://biogenesis.sourceforge.net/),
 * Copyright (C) 2006-2010 Joan Queralt Molina;
 * rewritten for SDL2 in 2026.
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

/* World statistics (StatisticsWindow.java). Unlike the original's
 * snapshot-plus-Update-button, it stays live while open. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ui/app.h"

void stats_open(App *a)
{
    StatsWindow *s = &a->stats;
    if (!s->win.open)
        ui_window_centre(&s->win, 900, 640);
    s->win.open = true;
    stats_find_best_alive(&a->world.stats, &a->world);
}

/* GraphPanel: a 100-sample history scaled between min and max */
static void graph(SDL_Rect r, const StatsSeries *ser, double max, double min, Col col)
{
    if (ser->n < 2 || max - min <= 0)
        return;
    double sx = (double)r.w / 100.0;
    int px = 0, py = 0;
    for (int i = 0; i < ser->n; i++) {
        int x = r.x + (int)lround(i * sx);
        int y = r.y + (int)(r.h - (ser->v[i] - min) * r.h / (max - min));
        if (i > 0)
            gfx_line(px, py, x, y, col);
        px = x;
        py = y;
    }
}

static void legend(int x, int y, const char *const *names, const Col *cols, int n)
{
    for (int i = 0; i < n; i++)
        gfx_text(x, y + i * (gfx_font_h() + 2), names[i], cols[i]);
}

static void line(int x, int *y, const char *label, double v)
{
    char num[48], buf[160];
    fmt_num(num, sizeof num, v);
    snprintf(buf, sizeof buf, "%s%s", label, num);
    ui_label(x, *y, buf);
    *y += gfx_font_h() + 3;
}

typedef struct {
    int count;
    int color;
} ColorCount;

static int cmp_count(const void *a, const void *b)
{
    const ColorCount *x = a, *y = b;
    return (y->count > x->count) - (y->count < x->count);
}

void stats_draw(App *a)
{
    StatsWindow *s = &a->stats;
    if (!s->win.open)
        return;
    World *w = &a->world;
    WorldStats *st = &w->stats;
    if (SDL_GetTicks() - s->last_refresh > 500) {
        stats_find_best_alive(st, w);
        s->last_refresh = SDL_GetTicks();
    }
    SDL_Rect c = ui_window_begin(&s->win, ui_id("stats", 0), "Statistics", false);
    int fh = gfx_font_h(), lh = fh + 3;

    /* left: current state */
    int x = c.x, y = c.y;
    ui_label_dim(x, y, "Current state");
    y += lh + 4;
    char buf[160];
    snprintf(buf, sizeof buf, "Time: %lld", (long long)st->time);
    ui_label(x, y, buf);
    line(x + 200, &y, "Oxygen: ", w->o2);
    snprintf(buf, sizeof buf, "Population: %d", w->population);
    ui_label(x, y, buf);
    line(x + 200, &y, "Carbon dioxide: ", w->co2);
    snprintf(buf, sizeof buf, "Remains of beings: %d", world_n_corpses(w));
    ui_label(x, y, buf);
    y += lh;
    ui_label(x, y + 2, "Color proportion:");
    /* ColorPanel: gene colours of every organism, largest first */
    ColorCount cc[GENE_COLORS] = {{0, COL_GREEN}, {0, COL_RED},  {0, COL_BLUE},  {0, COL_CYAN},
                                  {0, COL_WHITE}, {0, COL_GRAY}, {0, COL_YELLOW}};
    long total = 0;
    for (int i = 0; i < w->norgs; i++) {
        const GeneticCode *gc = w->orgs[i]->gc;
        for (int g = 0; g < gc->ngenes; g++)
            for (int k = 0; k < GENE_COLORS; k++)
                if (cc[k].color == gc->genes[g].color)
                    cc[k].count++;
    }
    qsort(cc, GENE_COLORS, sizeof cc[0], cmp_count);
    for (int k = 0; k < GENE_COLORS; k++)
        total += cc[k].count;
    SDL_Rect bar = {x + 140, y, 260, fh + 4};
    gfx_fill(bar, COL(0, 0, 0));
    if (total > 0) {
        int bx = bar.x;
        for (int k = 0; k < GENE_COLORS; k++) {
            int bw = (int)(bar.w * (long)cc[k].count / total);
            uint8_t r, g, b;
            bio_color_rgb(cc[k].color, &r, &g, &b);
            gfx_fill((SDL_Rect){bx, bar.y, bw, bar.h}, COL(r, g, b));
            bx += bw;
        }
    }
    y += lh + 16;

    /* remarkable organisms */
    ui_label_dim(x, y, "Remarkable organisms");
    y += lh + 4;
    struct {
        const char *alive_label, *ever_label, *count_label;
        GeneticCode *alive_gc, *ever_gc;
        int alive_n, ever_n;
        int64_t ever_time;
    } rows[3] = {
        {"Alive, most children", "Ever, most children", "Children: ", st->alive_most_children,
         st->most_children, st->alive_most_children_n, st->most_children_n,
         st->most_children_time},
        {"Alive, most victims", "Ever, most victims", "Victims: ", st->alive_most_kills,
         st->most_kills, st->alive_most_kills_n, st->most_kills_n, st->most_kills_time},
        {"Alive, most infected", "Ever, most infected", "Infected: ", st->alive_most_infections,
         st->most_infections, st->alive_most_infections_n, st->most_infections_n,
         st->most_infections_time},
    };
    int ps = 100;
    for (int r = 0; r < 3; r++) {
        int col2 = x + 220;
        ui_label(x, y, rows[r].alive_label);
        ui_label(col2, y, rows[r].ever_label);
        int py = y + lh;
        gc_panel(a, ui_id("st_alive", (uint32_t)r), (SDL_Rect){x, py, ps, ps}, rows[r].alive_gc);
        gc_panel(a, ui_id("st_ever", (uint32_t)r), (SDL_Rect){col2, py, ps, ps}, rows[r].ever_gc);
        int ty = py + 4;
        snprintf(buf, sizeof buf, "%s%d", rows[r].count_label, rows[r].alive_n);
        ui_label(x + ps + 8, ty, buf);
        snprintf(buf, sizeof buf, "%s%d", rows[r].count_label, rows[r].ever_n);
        ui_label(col2 + ps + 8, ty, buf);
        snprintf(buf, sizeof buf, "Time: %lld", (long long)rows[r].ever_time);
        ui_label(col2 + ps + 8, ty + lh, buf);
        y = py + ps + 10;
    }
    ui_label_dim(x, y, "Right-click a body to copy its genetic code.");

    /* right: world history */
    int rx = c.x + 470, ry = c.y;
    ui_label_dim(rx, ry, "World history - population");
    ry += lh + 4;
    SDL_Rect g1 = {rx, ry, 300, 150};
    gfx_fill(g1, COL(0, 0, 0));
    double avg = stats_average_population(st);
    graph(g1, &st->deaths, fmax(avg, st->max_deaths), 0, COL(255, 0, 0));
    graph(g1, &st->births, fmax(avg, st->max_births), 0, COL(0, 255, 0));
    graph(g1, &st->population, st->max_population, 0, COL(255, 255, 255));
    static const char *const n1[] = {"Deaths", "Births", "Population"};
    static const Col c1[] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {255, 255, 255, 255}};
    legend(g1.x + g1.w + 8, g1.y, n1, c1, 3);
    ui_label_dim(g1.x, g1.y + g1.h + 2, "0");
    ui_label_dim(g1.x + g1.w - gfx_text_w("100"), g1.y + g1.h + 2, "100");
    ry += g1.h + lh + 6;
    line(rx, &ry, "Average population: ", avg);
    line(rx, &ry, "Average birth rate: ", stats_average_births(st));
    line(rx, &ry, "Average mortality rate: ", stats_average_deaths(st));
    line(rx, &ry, "Average infections rate: ", stats_average_infections(st));
    line(rx, &ry, "Generated organisms: ", st->created_organisms);
    snprintf(buf, sizeof buf, "Maximum population: %d  at time: %lld", st->max_population,
             (long long)st->max_population_time);
    ui_label(rx, ry, buf);
    ry += lh;
    snprintf(buf, sizeof buf, "Minimum population: %d  at time: %lld", st->min_population,
             (long long)st->min_population_time);
    ui_label(rx, ry, buf);
    ry += lh;
    line(rx, &ry, "Mass extinctions: ", st->mass_extinctions);
    ry += 8;

    ui_label_dim(rx, ry, "World history - atmosphere");
    ry += lh + 4;
    SDL_Rect g2 = {rx, ry, 300, 110};
    gfx_fill(g2, COL(0, 0, 0));
    graph(g2, &st->oxygen, st->max_oxygen, st->min_oxygen, COL(80, 120, 255));
    graph(g2, &st->co2, st->max_co2, st->min_co2, COL(255, 255, 255));
    static const char *const n2[] = {"Oxygen", "Carbon dioxide"};
    static const Col c2[] = {{80, 120, 255, 255}, {255, 255, 255, 255}};
    legend(g2.x + g2.w + 8, g2.y, n2, c2, 2);
    ry += g2.h + 8;
    char n_a[32], n_b[32];
    fmt_num(n_a, sizeof n_a, st->max_co2);
    fmt_num(n_b, sizeof n_b, st->min_co2);
    snprintf(buf, sizeof buf, "Carbon dioxide: max %s (t %lld), min %s (t %lld)", n_a,
             (long long)st->max_co2_time, n_b, (long long)st->min_co2_time);
    ui_label(rx, ry, buf);
    ry += lh;
    fmt_num(n_a, sizeof n_a, st->max_oxygen);
    fmt_num(n_b, sizeof n_b, st->min_oxygen);
    snprintf(buf, sizeof buf, "Oxygen: max %s (t %lld), min %s (t %lld)", n_a,
             (long long)st->max_oxygen_time, n_b, (long long)st->min_oxygen_time);
    ui_label(rx, ry, buf);

    int rh = fh + 10;
    if (ui_button(ui_id("st_close", 0), (SDL_Rect){c.x + c.w - 100, c.y + c.h - rh, 100, rh},
                  "Close", true))
        s->win.open = false;
    ui_window_end();
}
