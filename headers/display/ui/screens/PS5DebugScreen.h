#ifndef _PS5DEBUGSCREEN_H_
#define _PS5DEBUGSCREEN_H_

#include "gpscreen.h"
#include <string>
#include <deque>

class PS5DebugScreen : public GPScreen {
    public:
        PS5DebugScreen(GPGFX* renderer) { setRenderer(renderer); }
        void init();
        int8_t update();
        void shutdown();
        void drawScreen();

        static void addMessage(const std::string& msg);
        static void clearMessages();

    private:
        static std::deque<std::string> messages;
        static const size_t MAX_MESSAGES = 7; // Max lines on display
};

#endif
