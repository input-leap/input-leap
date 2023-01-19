/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2011 Nick Bolton
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

#include "io/IStream.h"

#include <gmock/gmock.h>

class MockStream : public inputleap::IStream
{
public:
    MockStream() { }
    MOCK_METHOD0(close, void());
    MOCK_METHOD2(read, std::uint32_t(void*, std::uint32_t));
    MOCK_METHOD2(write, void(const void*, std::uint32_t));
    MOCK_METHOD0(flush, void());
    MOCK_METHOD0(shutdownInput, void());
    MOCK_METHOD0(shutdownOutput, void());
    MOCK_METHOD0(getInputReadyEvent, EventType());
    MOCK_METHOD0(getOutputErrorEvent, EventType());
    MOCK_METHOD0(getInputShutdownEvent, EventType());
    MOCK_METHOD0(getOutputShutdownEvent, EventType());
    MOCK_CONST_METHOD0(get_event_target, void*());
    MOCK_CONST_METHOD0(isReady, bool());
    MOCK_CONST_METHOD0(getSize, std::uint32_t());
};
