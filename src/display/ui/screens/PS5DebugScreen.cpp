#include "display/ui/screens/PS5DebugScreen.h"

std::deque<std::string> PS5DebugScreen::messages;

void PS5DebugScreen::init() {
    if (messages.empty()) {
        messages.push_back("PS5 Debug Log");
        messages.push_back("Waiting...");
    }
}

int8_t PS5DebugScreen::update() {
    return -1;
}

void PS5DebugScreen::shutdown() {
}

void PS5DebugScreen::drawScreen() {
    getRenderer()->clearScreen();

    int line = 0;
    for (auto it = messages.begin(); it != messages.end() && line < 8; ++it, ++line) {
        getRenderer()->drawText(0, line, it->c_str());
    }
}

void PS5DebugScreen::addMessage(const std::string& msg) {
    messages.push_back(msg);
    if (messages.size() > MAX_MESSAGES) {
        messages.pop_front();
    }
}

void PS5DebugScreen::clearMessages() {
    messages.clear();
}
