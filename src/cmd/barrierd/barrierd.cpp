/*  InputLeap -- mouse and keyboard sharing utility

    InputLeap is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Copyright (C) InputLeap developers.
*/

#include "inputleap/win32/DaemonApp.h"

#include <iostream>

#ifdef SYSAPI_UNIX

int
main(int argc, char** argv)
{
    DaemonApp app;
    return app.run(argc, argv);
}

#elif SYSAPI_WIN32

#include "common/win32/winapi.h"

int WINAPI
WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    DaemonApp app;
    return app.run(__argc, __argv);
}

#endif
