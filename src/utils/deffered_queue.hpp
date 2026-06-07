#pragma once

#include <functional>
#include <vector>

class DeferredQueue {
public:
    void push(std::function<void()> action) { actions.push_back(std::move(action)); }
    void flush() {
        for (auto& a : actions) {
            a();
        }
        actions.clear();
    }

private:
    std::vector<std::function<void()>> actions;
};
