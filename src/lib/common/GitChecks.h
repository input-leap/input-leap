#pragma once

#ifndef INPUTLEAP_LIB_COMMON_GIT_CHECKS_H
#define INPUTLEAP_LIB_COMMON_GIT_CHECKS_H

#include "git.h"

namespace inputleap {
    class GitChecks {
    public:
        const char* get_current_sha();
        static bool is_dirty();
    };
}

#endif
