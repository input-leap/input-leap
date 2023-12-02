#pragma once

#if (defined(WINAPI_XWINDOWS) && (!defined(HAVE_LIBPORTAL_INPUTCAPTURE) || !defined(HAVE_LIBPORTAL_SESSION_CONNECT_TO_EIS)))
bool display_is_valid();
#endif
