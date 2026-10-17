# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

CC = gcc
# -fwrapv: the java.awt geometry checks in src/sim/jgeom.c rely on Java's
# wrapping int overflow. -ffp-contract=off: no fused multiply-adds, so the
# floating point matches the Java original operation for operation (the
# javaref comparison depends on it).
CFLAGS = -Wall -Wextra -std=c11 -O2 -g -Isrc -fwrapv -ffp-contract=off -D_DEFAULT_SOURCE
LDLIBS = -lm

PKG_CONFIG ?= pkg-config
SDL_CFLAGS = $(shell $(PKG_CONFIG) --cflags sdl2 SDL2_ttf SDL2_image)
SDL_LIBS = $(shell $(PKG_CONFIG) --libs sdl2 SDL2_ttf SDL2_image)

SIM_SRC = src/sim/jrandom.c src/sim/jgeom.c src/sim/params.c src/sim/genetic_code.c \
          src/sim/organism.c src/sim/world.c src/sim/stats.c \
          src/io/bgg.c src/io/worldfile.c
CORE_SRC = $(SIM_SRC) src/headless.c src/main.c
UI_SRC = $(wildcard src/ui/*.c)

OBJ = $(patsubst src/%.c,obj/%.o,$(CORE_SRC) $(UI_SRC))
HEADLESS_OBJ = $(patsubst src/%.c,obj/headless/%.o,$(CORE_SRC))

.PHONY: all headless strict test clean

all: syn-biogenesis

syn-biogenesis: $(OBJ)
	$(CC) -o $@ $^ $(SDL_LIBS) $(LDLIBS)

# No SDL needed: the simulation alone, for benchmarks and the javaref check.
headless: syn-biogenesis-headless

syn-biogenesis-headless: $(HEADLESS_OBJ)
	$(CC) -o $@ $^ $(LDLIBS)

# Verification build: the headless simulation on fdlibm 5.3, the library
# Java's StrictMath is ported from (see tools/javaref/fdlibm/README.md).
FDLIBM_SRC = $(filter-out %/fd_shim.c,$(wildcard tools/javaref/fdlibm/*.c))
FDLIBM_RENAME = -D__LITTLE_ENDIAN -Dsin=fd_sin -Dcos=fd_cos -Datan=fd_atan -Dfabs=fd_fabs \
	-Dscalbn=fd_scalbn -Dcopysign=fd_copysign -Dfloor=fd_floor -Dsqrt=fd_sqrt \
	-D__ieee754_atan2=fd_atan2 -D__ieee754_pow=fd_pow -D__ieee754_rem_pio2=fd_rem_pio2 \
	-D__kernel_sin=fd_kernel_sin -D__kernel_cos=fd_kernel_cos \
	-D__kernel_rem_pio2=fd_kernel_rem_pio2
STRICT_OBJ = $(patsubst src/%.c,obj/strict/%.o,$(CORE_SRC)) \
	$(patsubst tools/javaref/fdlibm/%.c,obj/fdlibm/%.o,$(FDLIBM_SRC)) obj/fdlibm/fd_shim.o

strict: syn-biogenesis-strict

syn-biogenesis-strict: $(STRICT_OBJ)
	$(CC) -o $@ $^ $(LDLIBS)

obj/strict/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DBIO_NO_GUI -DBIO_FDLIBM -MMD -c -o $@ $<

obj/fdlibm/fd_shim.o: tools/javaref/fdlibm/fd_shim.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -ffp-contract=off -c -o $@ $<

# K&R-era code: type punning through int pointers, so no strict aliasing.
obj/fdlibm/%.o: tools/javaref/fdlibm/%.c
	@mkdir -p $(dir $@)
	$(CC) -std=gnu89 -O1 -fno-strict-aliasing -fwrapv -ffp-contract=off -w $(FDLIBM_RENAME) -c -o $@ $<

obj/ui/%.o: src/ui/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -MMD -c -o $@ $<

obj/main.o: src/main.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -MMD -c -o $@ $<

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -c -o $@ $<

obj/headless/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DBIO_NO_GUI -MMD -c -o $@ $<

tests/geom_check: tests/geom_check.c src/sim/jgeom.c src/sim/jrandom.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

tests/bgg_tool: tests/bgg_tool.c $(SIM_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

tests/sim_check: tests/sim_check.c $(SIM_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# File format checks, the java.awt geometry against the JVM, and the
# frame-by-frame comparison with the Java original (the last two need a
# JDK; see tools/javaref/README.md).
test: syn-biogenesis-headless syn-biogenesis-strict tests/geom_check tests/sim_check tests/bgg_tool
	tests/run.sh

clean:
	rm -rf obj syn-biogenesis syn-biogenesis-headless syn-biogenesis-strict tests/geom_check \
		tests/sim_check tests/bgg_tool

-include $(OBJ:.o=.d) $(HEADLESS_OBJ:.o=.d) $(STRICT_OBJ:.o=.d)
