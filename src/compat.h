/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
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

/* What Windows (MinGW-w64) lacks or does its own way, under the POSIX names
 * the rest of the source uses. Elsewhere it adds only the path helpers,
 * which there change nothing.
 *
 * Paths are kept with '/' between their parts, which Windows takes too, and
 * the code finds a path's parts by '/'; a path Windows hands over (the
 * working folder, a resolved path, the user's folders) goes through
 * path_slashes first. The program runs in the UTF-8 code page
 * (src/win/syn-biogenesis.manifest), so these narrow names take UTF-8, as
 * SDL gives it. */
#ifndef COMPAT_H
#define COMPAT_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

/* '\' to '/' on Windows; elsewhere '\' is part of a name and stays */
static inline char *path_slashes(char *p)
{
#ifdef _WIN32
    for (char *c = p; *c; c++)
        if (*c == '\\')
            *c = '/';
#endif
    return p;
}

/* from the root: "/x", and on Windows "C:/x" */
static inline bool path_absolute(const char *p)
{
#ifdef _WIN32
    if (((p[0] >= 'A' && p[0] <= 'Z') || (p[0] >= 'a' && p[0] <= 'z')) && p[1] == ':')
        return true;
#endif
    return p[0] == '/';
}

/* the user's own folder: on Windows their profile (C:/Users/name), not the
 * $HOME a Unix-like shell there may set */
static inline const char *home_dir(void)
{
#ifdef _WIN32
    static char home[1024];
    const char *p = getenv("USERPROFILE");
    if (p && *p) {
        snprintf(home, sizeof home, "%s", p);
        return path_slashes(home);
    }
#endif
    return getenv("HOME");
}

#ifdef _WIN32
#include <direct.h>
#include <io.h>

#define mkdir(path, mode) _mkdir(path)

static inline char *realpath(const char *path, char *resolved)
{
    return _fullpath(resolved, path, 4096) ? path_slashes(resolved) : NULL;
}

static inline int setenv(const char *name, const char *value, int overwrite)
{
    if (!overwrite && getenv(name))
        return 0;
    return _putenv_s(name, value) ? -1 : 0;
}

static inline int unsetenv(const char *name)
{
    return _putenv_s(name, "") ? -1 : 0;
}
#endif

#endif
