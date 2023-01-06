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

#ifndef CCLIENTTASKBARRECEIVER_H
#define CCLIENTTASKBARRECEIVER_H

#include "arch/IArchTaskBarReceiver.h"
#include "base/log_outputters.h"
#include "client/Client.h"

class IEventQueue;

//! Implementation of IArchTaskBarReceiver for the barrier server
class ClientTaskBarReceiver : public IArchTaskBarReceiver {
public:
    ClientTaskBarReceiver(IEventQueue* events);
    ~ClientTaskBarReceiver() override;

    //! @name manipulators
    //@{

    //! Update status
    /*!
    Determine the status and query required information from the client.
    */
    void updateStatus(Client*, const std::string& errorMsg);

    void updateStatus(INode* n, const std::string& errorMsg)  override
        { updateStatus(static_cast<Client*>(n), errorMsg); }

    //@}

    // IArchTaskBarReceiver overrides
    void lock() const override;
    void unlock() const override;
    std::string getToolTip() const override;
    void cleanup() override {}

protected:
    enum EState {
        kNotRunning,
        kNotWorking,
        kNotConnected,
        kConnecting,
        kConnected,
        kMaxState
    };

    //! Get status
    EState getStatus() const;

    //! Get error message
    const std::string& getErrorMessage() const;

    //! Quit app
    /*!
    Causes the application to quit gracefully
    */
    void quit();

    //! Status change notification
    /*!
    Called when status changes.  The default implementation does nothing.
    */
    virtual void onStatusChanged(Client* client);

private:
    EState m_state;
    std::string m_errorMessage;
    std::string m_server;
    IEventQueue* m_events;
};

IArchTaskBarReceiver* createTaskBarReceiver(const BufferedLogOutputter* logBuffer, IEventQueue* events);

#endif
