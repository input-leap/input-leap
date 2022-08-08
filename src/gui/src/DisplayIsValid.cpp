#ifdef WINAPI_XWINDOWS

#include "DisplayIsValid.h"
#include <X11/Xlib.h>

bool display_is_valid()
{
    auto dsp = XOpenDisplay(nullptr);
    if (dsp != nullptr)
        XCloseDisplay(dsp);
    return dsp != nullptr;
}

#endif
