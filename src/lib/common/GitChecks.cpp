#include "GitChecks.h"

namespace inputleap {
    const char *GitChecks::get_current_sha() {
        return git_CommitSHA1();
    }

    bool GitChecks::is_dirty() {
        return git_AnyUncommittedChanges();
    }
}