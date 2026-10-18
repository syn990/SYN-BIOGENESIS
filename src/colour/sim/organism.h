/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 by tools/colour/j2c.py from Organism.java of
 * Biogenesis Color Mod 2.01 (https://sourceforge.net/projects/biogenesis/),
 * Copyright (C) 2006-2010 Joan Queralt Molina,
 * Color Mod Copyright (C) 2012-2022 MarcoDBAA.
 * Generated: edit the translator or the Java, not this file.
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

/* Organism.java of Color Mod: a body of coloured segments that
 * moves, feeds, breathes, grows, collides and reproduces, and the rules of
 * what each colour does when it touches another.
 */
#ifndef COLOUR_ORGANISM_H
#define COLOUR_ORGANISM_H

#include <stdbool.h>
#include <stdint.h>

#include "sim/genetic_code.h"
#include "sim/jrt.h"

typedef struct World World;

/* What a colour does (Organism.getTypeColor) */
enum {
    TY_NOCOLOR = -1,
    TY_GREEN = 0,
    TY_FOREST = 1,
    TY_SPRING = 2,
    TY_SUMMER = 3,
    TY_LIME = 4,
    TY_LEAF = 5,
    TY_C4 = 6,
    TY_JADE = 7,
    TY_GRASS = 8,
    TY_BARK = 9,
    TY_PURPLE = 10,
    TY_RED = 11,
    TY_FIRE = 12,
    TY_ORANGE = 13,
    TY_MAROON = 14,
    TY_PINK = 15,
    TY_CREAM = 16,
    TY_SILVER = 17,
    TY_SPIKE = 18,
    TY_LILAC = 19,
    TY_GRAY = 20,
    TY_VIOLET = 21,
    TY_OLIVE = 22,
    TY_SKY = 23,
    TY_BLUE = 24,
    TY_OCHRE = 25,
    TY_FALLOW = 26,
    TY_SPORE = 27,
    TY_WHITE = 28,
    TY_PLAGUE = 29,
    TY_CORAL = 30,
    TY_MINT = 31,
    TY_LAVENDER = 32,
    TY_MAGENTA = 33,
    TY_ROSE = 34,
    TY_CYAN = 35,
    TY_TEAL = 36,
    TY_YELLOW = 37,
    TY_AUBURN = 38,
    TY_INDIGO = 39,
    TY_BLOND = 40,
    TY_FLOWER = 41,
    TY_DARKGRAY = 42,
    TY_GOLD = 43,
    TY_DARK = 44,
    TY_EYE = 45,
    TY_WINTER = 46,
    TY_OLDBARK = 47,
    TY_DARKJADE = 48,
    TY_DARKGREEN = 49,
    TY_DARKFIRE = 50,
    TY_DARKLILAC = 51,
    TY_DEEPSKY = 52,
    TY_DARKOLIVE = 53,
    TY_SPIKEPOINT = 54,
    TY_FRUIT = 55,
    TY_VISION = 56,
    TY_ICE = 57,
    TY_LIGHT_BLUE = 58,
    TY_LIGHTBROWN = 59,
    TY_GREENBROWN = 60,
    TY_BROKEN = 61,
    TY_DEADBARK = 62,
    TY_BROWN = 63,
};

typedef struct Organism {
    JRect r; /* the java.awt.Rectangle it extends: the bounding box */
    GeneticCode *gc; /* A reference to the genetic code of this organism */
    /* If this organism has been infected by a white segment, here we have the */
    GeneticCode *infected_gc;
    /* A reference to the saved genetic code of this organisms child, if the organism */
    GeneticCode *saved_gc;
    int n_children; /* Number of children that this organism will produce at once. This */
    int n_virus_children; /* Number of viruses that this organism is forced to produce at once. */
    int indigo; /* Amount of indigo an organism has */
    int antiviral; /* Amount of mint an organism has */
    int lavender; /* Amount of lavender shielding an organism has */
    int createlavender; /* Amount of lavender shielding an organism can create per frame */
    int healing; /* Amount of magenta an organism has */
    int sporetime; /* Amount of time an organism will spend as spore */
    int sporeversion; /* Spore version */
    int skyversion; /* Sky/Freezer version */
    int fallowversion; /* Fallow version */
    int plagueversion; /* Plague version */
    int summerinactivity; /* Summer inactivity version */
    int yellow_counter; /* Total length of all yellow segments */
    int reproducelate; /* Total length of all flower segments */
    int gold; /* Total length of all golden segments */
    double maintenance; /* Segments maintenance costs */
    double methanotrophy; /* Segments methanotrophy */
    double photosynthesis; /* Segments photosynthesis */
    double colony_photosynthesis; /* Active Forest colony photosynthesis */
    double forestphoto; /* Forest segments photosynthesis */
    double leafphoto; /* Leaf segments reduced photosynthesis */
    int spin; /* Spin of spring segments */
    /* Indicates if this organism has jade segments and also the individual jade delay */
    int jadefactor;
    int lengthfriend; /* If rose segments of two organisms have the same length they are friends */
    int thetafriend; /* If rose segments of two organisms have the same theta they are friends */
    bool siblingbattle; /* Indicates if siblings battle */
    bool generationbattle; /* Indicates if generations battle */
    bool clockwise; /* Indicates if this organism spins clockwise */
    bool altruist; /* Indicates if this organism is an altruist */
    int lowersymmetric; /* Indicates if this organism has lower symmetry (1-3) */
    bool isaplant; /* Indicates if this organism is a plant */
    bool isaconsumer; /* Indicates if this organism is a consumer */
    bool isafungus; /* Indicates if this organism is a fungus (unmodified pink) */
    bool isakiller; /* Indicates if this organism is a killer */
    bool isgray; /* Indicates if this organism has gray segments */
    bool isspike; /* Indicates if this organism has spike segments */
    bool canmove; /* Indicates if this organism has movement segments */
    bool canreact; /* Indicates if this organism has teal segments */
    bool isfrozen; /* Indicates if this organism is frozen */
    /* Indicates if all plant, sky and movement segments of this organism are frozen */
    bool allfrozen;
    bool isinjured; /* Indicates if this organism has destroyed segments. */
    bool isreproductive; /* Indicates if this organism has reproductive segments. */
    bool isinfectious; /* Indicates if this organism has infectious segments. */
    bool iscoral; /* Indicates if this organism has coral segments. */
    bool isauburn; /* Indicates if this organism has auburn segments. */
    bool isblond; /* Indicates if this organism has blond segments. */
    bool issilver; /* Indicates if this organism has silver segments. */
    bool isenhanced; /* Indicates if this organism has dark gray segments. */
    bool islime; /* Indicates if this organism has lime segments. */
    bool isonlyc4; /* Indicates if this organism is a plant with only C4 segments. */
    bool transfersenergy; /* Indicates if this organism has rose segments. */
    bool crowded; /* Indicates if this organism is near other organisms. */
    bool candodge; /* Indicates if an organism can dodge an attack in general */
    bool dodge; /* Indicates if an organism can dodge an attack this frame */
    bool hasdodged; /* Indicates if an organism already dodged an attack */
    bool haseyes; /* Indicates if an organism has eyes. */
    bool hasgoodvision; /* Indicates if an organism has good vision; */
    int segmentpretoucheffects; /* Indicates if an organism uses spike or dark */
    bool useframemovement; /* Indicates if an organism uses frame movement */
    bool useextraeffects; /* Indicates if an organism uses extra frame effects */
    bool usepretoucheffects; /* Indicates if an organism uses pretouch effects */
    int usefriendeffects; /* Indicates if an organism uses touchfriend effects */
    int update_effects; /* Takes a note that the static segment effects needs to be updated */
    World *world; /* Reference to the world where the organism lives. */
    void *visible_world; /* Reference to the visual part of the world where the organism lives. */
    int parent_id; /* Identification number of this organism's parent. */
    int id; /* Identification number of this organism. */
    char *n_position; /* String to check already used positions for a child */
    int total_children; /* Number of children it has produced. */
    int total_kills; /* Number of organism that has killed */
    int total_infected; /* Number of organism that has infected */
    int *start_x; /* X coordinates of the starting point of each organism's segments. */
    int *start_y; /* Y coordinates of the starting point of each organism's segments. */
    int *end_x; /* X coordinates of the ending point of each organism's segments. */
    int *end_y; /* Y coordinates of the ending point of each organism's segments. */
    double *m1; /* Precalculated distance from the origin to the starting point of each segment. */
    double *m2; /* Precalculated distance from the origin to the ending point of each segment. */
    double *m; /* Precalculated modulus of each segment. */
    /* Precalculated modulus of each segment, divided by symmetry, used for photosynthesis. */
    double *mphoto;
    int center_x; /* X coordinate of this organim's center of gravity. */
    int center_y; /* Y coordinate of this organim's center of gravity. */
    /* Like _centerX but with double precision to be able to make movements slower than a pixel. */
    double dcenter_x;
    /* Like _centerY but with double precision to be able to make movements slower than a pixel. */
    double dcenter_y;
    /* Effective segment colors, taken from the genetic code if alive or brown if dead. */
    JColor *seg_color;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segred_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *seggreen_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segblue_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segplague_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segscourge_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segwhite_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *seggray_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segdefault_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segconsumer_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segplant_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segmagenta_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segpink_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segcoral_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segorange_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segbark_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segviolet_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segvirus_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segmaroon_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segolive_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segmint_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segcream_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segspike_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segfallow_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *seglightblue_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segochre_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segsky_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *seglilac_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segsilver_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segfire_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *seglightbrown_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *seggreenbrown_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segbrown_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segice_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segbroken_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segsick_reaction;
    /* Effective segment reactions, taken from the genetic code if alive or brown if dead. */
    int *segfriend_reaction;
    int segments; /* The total number of segments of the organism */
    /* Growth ratio of the organism. Used to calculate segments when the organism is not */
    int growth_ratio;
    /* Total mass of this organism. The mass is calculated as the sum of all segment lengths. */
    double mass;
    /* Moment of inertia of this organism, used to calculate the effect of collisions. */
    double inertia;
    double energy; /* Chemical energy stored by this organism */
    JRect size_rect; /* Organism size independent on its position in the world. */
    double theta; /* Rotation angle that this organism has at a given moment. */
    /* Last frame angle, used to avoid calculating point rotations when the angle doesn't */
    double last_theta;
    int *x1; /* Rotated segments of the last frame, to use when _theta == _lastTheta */
    int *y1; /* Rotated segments of the last frame, to use when _theta == _lastTheta */
    int *x2; /* Rotated segments of the last frame, to use when _theta == _lastTheta */
    int *y2; /* Rotated segments of the last frame, to use when _theta == _lastTheta */
    double dx; /* Speed. Variation applied to organism coordinates at every frame. */
    double dy; /* Speed. Variation applied to organism coordinates at every frame. */
    double dtheta; /* Angular speed. Organism angle variation at every frame. */
    /* Minimum required energy to reproduce this genetic code of an organism with blond segments. */
    int early_reproduce_energy;
    int reproduce_energy; /* Minimum required energy to reproduce this genetic code. */
    /* Minimum required energy to reproduce this infected genetic code. */
    int infected_reproduce_energy;
    int age; /* Number of frames of life of this organism */
    int max_age; /* The maximum time that the organism can be alive. */
    /* Color used to draw the organism when a collision occurs. We save the color that */
    JColor flash_color;
    int frames_color; /* Number of frames in which the organism will be drawn in _color. */
    /* Number of frame that need to pass between two reproductions, even they are not */
    int time_to_reproduce;
    /* Maximum number of frames that need to pass between two reproductions, directly */
    int time_to_reproduce_max;
    /* Indicates if the organism has grown at the last frame. If it has grown it is */
    int has_grown;
    /* Indicates if it has moved at the last frame. If it has moved it is necessary */
    bool has_moved;
    JRect last_frame; /* The place that this organism occupies at the last frame. If the organism */
    bool alive; /* Indicates if the organism is alive. */
    bool active; /* Indicates if the organism is active. */
    /* world bookkeeping, not part of the Java organism */
    bool removed;  /* out of energy this frame; freed when the next one starts */
    double env[4]; /* frame-start envelope: minx, maxx, miny, maxy */
} Organism;

/* A zeroed object for the world reader: deserialization runs no
 * constructor and no field initialiser. */
Organism *org_alloc_blank(void);
void org_free(Organism *o);
/* Marks the collected objects it refers to (jgc_mark). */
void org_mark_refs(void *obj);
bool org_is_alive(Organism *o);
double org_get_energy(Organism *o);
int org_get_id(Organism *o);
int org_get_parent_id(Organism *o);
int org_get_reproduce_energy(Organism *o);
int org_get_age(Organism *o);
int org_get_max_age(Organism *o);
int org_get_total_children(Organism *o);
int org_get_total_kills(Organism *o);
int org_get_total_infected(Organism *o);
GeneticCode *org_get_genetic_code(Organism *o);
double org_get_mass(Organism *o);
void org_create(Organism *o);
void org_segments_check_plant(Organism *o);
void org_segments_restore_effects(Organism *o);
void org_segments_create_effects(Organism *o);
bool org_random_create(Organism *o);
bool org_inherit(Organism *o, Organism *parent, bool first);
bool org_inherit_transformation(Organism *o, Organism *victim, bool first);
bool org_paste_organism(Organism *o, int posx, int posy);
void org_symmetric(Organism *o);
void org_transform(Organism *o);
void org_calculate_bounds(Organism *o, bool force);
void org_reproduce(Organism *o);
void org_reproduce_early(Organism *o);
void org_reproduce_early_plant(Organism *o);
void org_reproduce_early_virus(Organism *o);
void org_reproduce_spore(Organism *o);
void org_reproduce_yellow(Organism *o);
void org_reproduce_virus(Organism *o);
void org_reproduceforeign_virus(Organism *o);
bool org_move(Organism *o);
bool org_use_decay_energy(Organism *o, double q);
bool org_use_energy(Organism *o, double q);
void org_breath(Organism *o);
void org_decay(Organism *o);
void org_blue_spore(Organism *o);
void org_cyan_spore(Organism *o);
void org_hatch(Organism *o);
void org_die(Organism *o, Organism *killingOrganism);
void org_survive(Organism *o, Organism *grayOrganism);
void org_inhibited(Organism *o, Organism *fallowOrganism);
void org_frozen(Organism *o, Organism *freezingOrganism);
void org_paralyzed(Organism *o, Organism *freezingOrganism);
void org_neutralized(Organism *o, Organism *mintOrganism);
void org_infected_by_genetic_code(Organism *o, GeneticCode *infectingCode);
void org_infected_by_organism(Organism *o, Organism *infectingOrganism);
void org_infected_by_fruit(Organism *o, Organism *infectingOrganism);
void org_infected_by_fruit_weak(Organism *o, Organism *infectingOrganism);
bool org_contact(Organism *o, Organism *org);
Organism *org_new(World *world);
Organism *org_new_with_code(World *world, GeneticCode *geneticCode);

#endif
