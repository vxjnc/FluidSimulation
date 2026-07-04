#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

enum class NotifyLevel : uint8_t {
    Info,
    Warning,
    Error,
};

struct Notification {
    NotifyLevel level;
    std::string message;
    float shown_at;
};

namespace {
    float now_seconds() {
        using namespace std::chrono;
        return duration<float>(steady_clock::now().time_since_epoch()).count();
    }
}

class NotificationManager {
public:
    void notify(NotifyLevel level, std::string message) {
        toasts_.push_back({level, std::move(message), now_seconds()});
    }
    void info(std::string message) { notify(NotifyLevel::Info, std::move(message)); }
    void warning(std::string message) { notify(NotifyLevel::Warning, std::move(message)); }
    void error(std::string message) { notify(NotifyLevel::Error, std::move(message)); }

    const std::vector<Notification>& active_toasts() {
        float current = now_seconds();
        std::erase_if(toasts_, [current](const Notification& n) {
            return current - n.shown_at > kToastDurationSeconds;
        });
        return toasts_;
    }

private:
    std::vector<Notification> toasts_;
    static constexpr float kToastDurationSeconds = 3.0f;
};
