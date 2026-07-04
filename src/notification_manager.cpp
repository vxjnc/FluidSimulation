#include "notification_manager.hpp"

#include <chrono>

namespace {
    constexpr float kToastDurationSeconds = 3.0f;

    float now_seconds() {
        using namespace std::chrono;
        return duration<float>(steady_clock::now().time_since_epoch()).count();
    }
}

void NotificationManager::notify(NotifyLevel level, std::string message) {
    toasts_.push_back({level, std::move(message), now_seconds()});
}

const std::vector<Notification>& NotificationManager::active_toasts() {
    float current = now_seconds();
    std::erase_if(toasts_,
                  [current](const Notification& n) { return current - n.shown_at > kToastDurationSeconds; });
    return toasts_;
}
