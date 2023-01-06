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

#pragma once

#include "server/Server.h"
#include "inputleap/ServerApp.h"
#include "arch/IArchTaskBarReceiver.h"
#include "base/EventTypes.h"
#include "base/String.h"
#include "base/Event.h"
#include "common/stdvector.h"

class IEventQueue;

//! Implementation of IArchTaskBarReceiver for the barrier server
class ServerTaskBarReceiver : public IArchTaskBarReceiver {
public:
    ServerTaskBarReceiver(IEventQueue* events);
    ~ServerTaskBarReceiver() override;

    //! @name manipulators
    //@{

    //! Update status
    /*!
    Determine the status and query required information from the server.
    */
    void updateStatus(Server*, const std::string& errorMsg);

    void updateStatus(INode* n, const std::string& errorMsg) override
        { updateStatus(static_cast<Server*>(n), errorMsg); }

    //@}

    // IArchTaskBarReceiver overrides
    void lock() const override;
    void unlock() const override;
    std::string getToolTip() const override;

protected:
    typedef std::vector<std::string> Clients;
    enum EState {
        kNotRunning,
        kNotWorking,
        kNotConnected,
        kConnected,
        kMaxState
    };

    //! Get status
    EState getStatus() const;

    //! Get error message
    const std::string& getErrorMessage() const;

    //! Get connected clients
    const Clients& getClients() const;

    //! Quit app
    /*!
    Causes the application to quit gracefully
    */
    void quit();

    //! Status change notification
    /*!
    Called when status changes.  The default implementation does
    nothing.
    */
    virtual void onStatusChanged(Server* server);

private:
    EState m_state;
    std::string m_errorMessage;
    Clients m_clients;
    IEventQueue* m_events;
};

IArchTaskBarReceiver* createTaskBarReceiver(const BufferedLogOutputter* logBuffer, IEventQueue* events);
