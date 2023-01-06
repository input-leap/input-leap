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

#include "inputleap/PortableTaskBarReceiver.h"
#include "base/IEventQueue.h"
#include "base/String.h"
#include "arch/Arch.h"
#include "common/Version.h"

//
// PortableTaskBarReceiver
//

PortableTaskBarReceiver::PortableTaskBarReceiver(IEventQueue* events) :
    m_state(kNotRunning),
    m_events(events)
{
    // do nothing
}

PortableTaskBarReceiver::~PortableTaskBarReceiver()
{
    // do nothing
}

void PortableTaskBarReceiver::updateStatus(INode* node, const std::string& errorMsg)
{
    {
        // update our status
        m_errorMessage = errorMsg;
        if (node == nullptr) {
            if (m_errorMessage.empty()) {
                m_state = kNotRunning;
            }
            else {
                m_state = kNotWorking;
            }
        }
        else {
            m_state = kNotConnected;
        }

        // let subclasses have a go
        onStatusChanged(node);
    }

    // tell task bar
    ARCH->updateReceiver(this);
}

PortableTaskBarReceiver::EState
PortableTaskBarReceiver::getStatus() const
{
    return m_state;
}

const std::string& PortableTaskBarReceiver::getErrorMessage() const
{
    return m_errorMessage;
}

void
PortableTaskBarReceiver::quit()
{
    m_events->addEvent(Event(Event::kQuit));
}

void
PortableTaskBarReceiver::onStatusChanged(INode*)
{
    // do nothing
}

void
PortableTaskBarReceiver::lock() const
{
    // do nothing
}

void
PortableTaskBarReceiver::unlock() const
{
    // do nothing
}

std::string
PortableTaskBarReceiver::getToolTip() const
{
    switch (m_state) {
    case kNotRunning:
        return inputleap::string::sprintf("%s:  Not running", kAppVersion);

    case kNotWorking:
        return inputleap::string::sprintf("%s:  %s",
                                kAppVersion, m_errorMessage.c_str());

    case kNotConnected:
        return inputleap::string::sprintf("%s:  Unknown", kAppVersion);

    default:
        return "";
    }
}
