/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#ifndef INPUTLEAP_LIB_BASE_UNIQUE_PTR_CONTAINER
#define INPUTLEAP_LIB_BASE_UNIQUE_PTR_CONTAINER

#include <algorithm>
#include <memory>
#include <vector>

template<class T>
class UniquePtrContainer {
public:
    using Container = std::vector<std::unique_ptr<T>>;

    void clear()
    {
        data_.clear();
    }

    void insert(std::unique_ptr<T>&& d)
    {
        data_.push_back(std::move(d));
    }

    std::unique_ptr<T> erase(T* p)
    {
        for (auto it = data_.begin(); it != data_.end(); ++it) {
            if (it->get() == p) {
                std::unique_ptr<T> ret = std::move(*it);
                data_.erase(it);
                return ret;
            }
        }
        return {};
    }

    typename Container::iterator begin() { return data_.begin(); }
    typename Container::const_iterator begin() const { return data_.begin(); }
    typename Container::const_iterator cbegin() const { return data_.cbegin(); }

    typename Container::iterator end() { return data_.end(); }
    typename Container::const_iterator end() const { return data_.end(); }
    typename Container::const_iterator cend() const { return data_.cend(); }
private:
    std::vector<std::unique_ptr<T>> data_;
};

#endif // INPUTLEAP_LIB_BASE_UNIQUE_PTR_CONTAINER
