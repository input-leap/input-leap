#pragma once

#include "base/Log.h"
#include <gtest/gtest.h>

#include <X11/Xlib.h>

namespace inputleap {

class XDisplayOpenUtil {
public:

    ~XDisplayOpenUtil()
    {
        if (display_ != nullptr) {
            LOG_DEBUG("closing display");
            XCloseDisplay(display_);
        }
    }

    void open()
    {
        if (display_) {
            // open the display only once for the entire test suite
            return;
        }

        for (unsigned i = 0; i < 100 && display_ == nullptr; ++i) {
            LOG_DEBUG("opening display");

            display_ = XOpenDisplay(nullptr);

            // failed to open the display and DISPLAY is null? probably
            // running in a CI, let's skip
            if (display_ == nullptr && std::getenv("DISPLAY") == nullptr)
                GTEST_SKIP() << "DISPLAY environment variable not set, skipping test";

            if (display_) {
                return;
            }
        }

        ASSERT_TRUE(display_ != nullptr) << "unable to open display: " << errno;
    }

    Display* display() { return display_; }
private:
    Display* display_ = nullptr;
};

} // namespace inputleap
