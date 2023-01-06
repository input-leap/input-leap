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

#include "base/log_outputters.h"
#include "arch/Arch.h"
#include "base/String.h"
#include "io/filesystem.h"
#include <fstream>
#include <iostream>

enum EFileLogOutputter {
    kFileSizeLimit = 1024 // kb
};

//
// StopLogOutputter
//

StopLogOutputter::StopLogOutputter()
{
    // do nothing
}

StopLogOutputter::~StopLogOutputter()
{
    // do nothing
}

void
StopLogOutputter::open(const char*)
{
    // do nothing
}

void
StopLogOutputter::close()
{
    // do nothing
}

void
StopLogOutputter::show(bool)
{
    // do nothing
}

bool
StopLogOutputter::write(ELevel, const char*)
{
    return false;
}


//
// ConsoleLogOutputter
//

ConsoleLogOutputter::ConsoleLogOutputter()
{
}

ConsoleLogOutputter::~ConsoleLogOutputter()
{
}

void
ConsoleLogOutputter::open(const char* title)
{
}

void
ConsoleLogOutputter::close()
{
}

void
ConsoleLogOutputter::show(bool showIfEmpty)
{
}

bool
ConsoleLogOutputter::write(ELevel level, const char* msg)
{
    if ((level >= kFATAL) && (level <= kWARNING))
        std::cerr << msg << std::endl;
    else
        std::cout << msg << std::endl;

    std::cout.flush();
    return true;
}

void
ConsoleLogOutputter::flush()
{

}


//
// SystemLogOutputter
//

SystemLogOutputter::SystemLogOutputter()
{
    // do nothing
}

SystemLogOutputter::~SystemLogOutputter()
{
    // do nothing
}

void
SystemLogOutputter::open(const char* title)
{
    ARCH->openLog(title);
}

void
SystemLogOutputter::close()
{
    ARCH->closeLog();
}

void
SystemLogOutputter::show(bool showIfEmpty)
{
    ARCH->showLog(showIfEmpty);
}

bool
SystemLogOutputter::write(ELevel level, const char* msg)
{
    ARCH->writeLog(level, msg);
    return true;
}

//
// SystemLogger
//

SystemLogger::SystemLogger(const char* title, bool blockConsole) :
    m_stop(nullptr)
{
    // redirect log messages
    if (blockConsole) {
        m_stop = new StopLogOutputter;
        CLOG->insert(m_stop);
    }
    m_syslog = new SystemLogOutputter;
    m_syslog->open(title);
    CLOG->insert(m_syslog);
}

SystemLogger::~SystemLogger()
{
    CLOG->remove(m_syslog);
    delete m_syslog;
    if (m_stop != nullptr) {
        CLOG->remove(m_stop);
        delete m_stop;
    }
}


//
// BufferedLogOutputter
//

BufferedLogOutputter::BufferedLogOutputter(std::uint32_t maxBufferSize) :
    m_maxBufferSize(maxBufferSize)
{
    // do nothing
}

BufferedLogOutputter::~BufferedLogOutputter()
{
    // do nothing
}

BufferedLogOutputter::const_iterator
BufferedLogOutputter::begin() const
{
    return m_buffer.begin();
}

BufferedLogOutputter::const_iterator
BufferedLogOutputter::end() const
{
    return m_buffer.end();
}

void
BufferedLogOutputter::open(const char*)
{
    // do nothing
}

void
BufferedLogOutputter::close()
{
    // remove all elements from the buffer
    m_buffer.clear();
}

void
BufferedLogOutputter::show(bool)
{
    // do nothing
}

bool
BufferedLogOutputter::write(ELevel, const char* message)
{
    while (m_buffer.size() >= m_maxBufferSize) {
        m_buffer.pop_front();
    }
    m_buffer.push_back(std::string(message));
    return true;
}


//
// FileLogOutputter
//

FileLogOutputter::FileLogOutputter(const char* logFile)
{
    setLogFilename(logFile);
}

FileLogOutputter::~FileLogOutputter()
{
}

void
FileLogOutputter::setLogFilename(const char* logFile)
{
    assert(logFile != nullptr);
    m_fileName = logFile;
}

bool
FileLogOutputter::write(ELevel level, const char *message)
{
    (void) level;

    bool moveFile = false;

    std::ofstream m_handle;
    inputleap::open_utf8_path(m_handle, m_fileName, std::fstream::app);
    if (m_handle.is_open() && m_handle.fail() != true) {
        m_handle << message << std::endl;

        // when file size exceeds limits, move to 'old log' filename.
        size_t p = m_handle.tellp();
        if (p > (kFileSizeLimit * 1024)) {
            moveFile = true;
        }
    }
    m_handle.close();

    if (moveFile) {
        std::string oldLogFilename = inputleap::string::sprintf("%s.1", m_fileName.c_str());
        remove(oldLogFilename.c_str());
        rename(m_fileName.c_str(), oldLogFilename.c_str());
    }

    return true;
}

void FileLogOutputter::open(const char *title) { (void) title; }

void
FileLogOutputter::close() {}

void FileLogOutputter::show(bool showIfEmpty) { (void) showIfEmpty; }

//
// MesssageBoxLogOutputter
//

MesssageBoxLogOutputter::MesssageBoxLogOutputter()
{
    // do nothing
}

MesssageBoxLogOutputter::~MesssageBoxLogOutputter()
{
    // do nothing
}

void
MesssageBoxLogOutputter::open(const char* title)
{
    (void) title;
    // do nothing
}

void
MesssageBoxLogOutputter::close()
{
    // do nothing
}

void
MesssageBoxLogOutputter::show(bool showIfEmpty)
{
    (void) showIfEmpty;
    // do nothing
}

bool
MesssageBoxLogOutputter::write(ELevel level, const char* msg)
{
    (void) msg;
    // don't spam user with messages.
    if (level > kERROR) {
        return true;
    }

#if SYSAPI_WIN32
    MessageBox(nullptr, msg, CLOG->getFilterName(level), MB_OK);
#endif

    return true;
}
