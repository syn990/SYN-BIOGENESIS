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

/* The SYN-OS interface's arena screen (main menu > ARENA), over the ladder
 * syn/arena.h runs:
 *
 * - the setup, each choice with what it means: the world, a round's
 *   length, sudden death, how many rounds, how much of the computer, how
 *   many winners meet in a round and whether newcomers join them; START
 *   and STOP;
 * - a card for each battle: a qualifier or its level, its bloodlines'
 *   shares of the living stacked over the round so far (the fighters' first,
 *   in their colours), the being ahead, how long it has left;
 * - the ladder: a rung for each level, the beings in the running on it,
 *   and the qualifiers under them; the climb: each round's winner by the
 *   level it went up to;
 * - the beings named most recently, as they are;
 * - the leaderboard for these settings, the highest level first, and the
 *   reigning champion.
 *
 * A being shown opens in the laboratory on a click; a right click copies
 * its code. PRINT WINNERS writes the ladder into a folder: every being's
 * .bgg, winners.txt and winners.png. The rounds go on with the screen
 * closed, until stopped. Esc or BACK returns to the main menu. */
#ifndef SYN_ARENA_SCREEN_H
#define SYN_ARENA_SCREEN_H

struct App;

void syn_arena_frame(struct App *a, int w, int h);
/* Test hooks for screenshots: SYN_BIOGENESIS_SHOT_ARENA=rounds[,units[,seed]]
 * runs the arena (with a ladder of its own, not kept) until that many
 * rounds have ended and every lane is half way through its next;
 * SYN_BIOGENESIS_SHOT_ARENA_VIEW = setup | battles | tree shows that. */
void syn_arena_shot_prepare(void);
void syn_arena_screen_free(void);

#endif
