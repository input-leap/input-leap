/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "config.h"

// define nullptr
#include <stddef.h>

// make assert available since we use it a lot
#include <assert.h>
#include <stdlib.h>
#include <string.h>

enum {
    kExitSuccess      = 0, // successful completion
    kExitFailed       = 1, // general failure
    kExitTerminated   = 2, // killed by signal
    kExitArgs         = 3, // bad arguments
    kExitConfig       = 4, // cannot read configuration
    kExitSubscription = 5  // subscription error
};

#if defined(__GNUC__)
    #define INPUTLEAP_DEFINE_ATTRIBUTE_PRINTF 1
#elif defined(__has_attribute)
    #if __has_attribute(__format__)
        #define INPUTLEAP_DEFINE_ATTRIBUTE_PRINTF 1
    #endif
#endif

#ifdef INPUTLEAP_DEFINE_ATTRIBUTE_PRINTF
    #define INPUTLEAP_ATTRIBUTE_PRINTF(x,y) __attribute__((__format__(__printf__,x,y)))
#else
    #define INPUTLEAP_ATTRIBUTE_PRINTF(x,y)
#endif
