#pragma once

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

class NotificationManager {
public:
    void notify(NotifyLevel level, std::string message);
    void info(std::string message) { notify(NotifyLevel::Info, std::move(message)); }
    void warning(std::string message) { notify(NotifyLevel::Warning, std::move(message)); }
    void error(std::string message) { notify(NotifyLevel::Error, std::move(message)); }

    const std::vector<Notification>& active_toasts();

private:
    std::vector<Notification> toasts_;
};
