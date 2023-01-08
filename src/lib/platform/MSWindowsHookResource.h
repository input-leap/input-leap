#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace inputleap {

class WindowsHookResource
{
public:
    explicit WindowsHookResource();
    ~WindowsHookResource();

    bool set(int idHook, HOOKPROC lpfn, HINSTANCE hmod, DWORD dwThreadId);
    bool unset();

    bool is_set() const;
    operator HHOOK() const;

private:
    HHOOK _hook;
};

} // namespace inputleap
