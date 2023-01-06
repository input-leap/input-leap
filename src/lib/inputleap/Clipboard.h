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

#include "inputleap/IClipboard.h"

//! Memory buffer clipboard
/*!
This class implements a clipboard that stores data in memory.
*/
class Clipboard : public IClipboard {
public:
    Clipboard();
    virtual ~Clipboard();

    //! @name manipulators
    //@{

    //! Unmarshall clipboard data
    /*!
    Extract marshalled clipboard data and store it in this clipboard.
    Sets the clipboard time to \c time.
    */
    void unmarshall(const std::string& data, Time time);

    //@}
    //! @name accessors
    //@{

    //! Marshall clipboard data
    /*!
    Merge this clipboard's data into a single buffer that can be later
    unmarshalled to restore the clipboard and return the buffer.
    */
    std::string marshall() const;

    //@}

    // IClipboard overrides
    bool empty() override;
    void add(EFormat, const std::string& data) override;
    bool open(Time) const override;
    void close() const override;
    Time getTime() const override;
    bool has(EFormat) const override;
    std::string get(EFormat) const override;

private:
    mutable bool m_open;
    mutable Time m_time;
    bool m_owner;
    Time m_timeOwned;
    bool m_added[kNumFormats];
    std::string m_data[kNumFormats];
};
