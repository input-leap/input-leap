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

// From XFuncproto.h
/* Added in X11R6.9, so available in any version of modular xproto */
#if __has_attribute(__format__) \
  || defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 203)
# define _X_ATTRIBUTE_PRINTF(x,y) __attribute__((__format__(__printf__,x,y)))
#else /* not gcc >= 2.3 */
# define _X_ATTRIBUTE_PRINTF(x,y)
#endif
