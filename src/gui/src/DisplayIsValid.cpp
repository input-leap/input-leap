#if (defined(WINAPI_XWINDOWS) && (!defined(HAVE_LIBPORTAL_INPUTCAPTURE) || !defined(HAVE_LIBPORTAL_SESSION_CONNECT_TO_EIS)))

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
