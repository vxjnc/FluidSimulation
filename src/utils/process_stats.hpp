#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

#if defined(__linux__)
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <unistd.h>

#elif defined(_WIN32)
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on

#elif defined(__APPLE__)
#include <mach/mach.h>
#include <sys/resource.h>
#endif

namespace ProcessStats {
    struct Stats {
        size_t rssBytes = 0;
        double cpuPercent = 0.0;
    };

    namespace detail {
        inline bool hasPrev_ = false;
        inline std::chrono::steady_clock::time_point prevWall_;

#if defined(__linux__)
        inline long long prevCpuNs_ = 0;

        inline size_t getRssBytes() {
            FILE* f = std::fopen("/proc/self/statm", "r");
            if (!f) {
                return 0;
            }
            long pages = 0;
            if (std::fscanf(f, "%*d %ld", &pages) != 1) {
                std::fclose(f);
                return 0;
            }
            std::fclose(f);
            return static_cast<size_t>(pages) * static_cast<size_t>(sysconf(_SC_PAGESIZE));
        }

        inline double getCpuPercent() {
            struct timespec ts;
            if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0) {
                return 0.0;
            }
            long long cpuNs = static_cast<long long>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
            auto now = std::chrono::steady_clock::now();
            if (!hasPrev_) {
                hasPrev_ = true;
                prevWall_ = now;
                prevCpuNs_ = cpuNs;
                return 0.0;
            }
            double cpuDeltaSec = static_cast<double>(cpuNs - prevCpuNs_) / 1e9;
            double wallDeltaSec = std::chrono::duration<double>(now - prevWall_).count();
            prevWall_ = now;
            prevCpuNs_ = cpuNs;
            if (wallDeltaSec <= 0.0) {
                return 0.0;
            }
            return (cpuDeltaSec / wallDeltaSec) * 100.0;
        }

#elif defined(_WIN32)
        inline ULARGE_INTEGER prevKernel_{}, prevUser_{};

        inline size_t getRssBytes() {
            PROCESS_MEMORY_COUNTERS info{};
            if (!GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info))) {
                return 0;
            }
            return static_cast<size_t>(info.WorkingSetSize);
        }

        inline double getCpuPercent() {
            FILETIME createTime, exitTime, kernelTime, userTime;
            if (!GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &kernelTime, &userTime)) {
                return 0.0;
            }
            ULARGE_INTEGER kernel, user;
            kernel.LowPart = kernelTime.dwLowDateTime;
            kernel.HighPart = kernelTime.dwHighDateTime;
            user.LowPart = userTime.dwLowDateTime;
            user.HighPart = userTime.dwHighDateTime;
            auto now = std::chrono::steady_clock::now();
            if (!hasPrev_) {
                hasPrev_ = true;
                prevWall_ = now;
                prevKernel_ = kernel;
                prevUser_ = user;
                return 0.0;
            }
            double cpuDeltaSec = static_cast<double>((kernel.QuadPart - prevKernel_.QuadPart) +
                                                     (user.QuadPart - prevUser_.QuadPart)) /
                                 1e7;
            double wallDeltaSec = std::chrono::duration<double>(now - prevWall_).count();
            prevWall_ = now;
            prevKernel_ = kernel;
            prevUser_ = user;
            if (wallDeltaSec <= 0.0) {
                return 0.0;
            }
            return (cpuDeltaSec / wallDeltaSec) * 100.0;
        }

#elif defined(__APPLE__)
        inline double prevCpuSec_ = 0.0;

        inline size_t getRssBytes() {
            mach_task_basic_info info{};
            mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
            if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info),
                          &count) != KERN_SUCCESS) {
                return 0;
            }
            return static_cast<size_t>(info.resident_size);
        }

        inline double getCpuPercent() {
            struct rusage usage{};
            if (getrusage(RUSAGE_SELF, &usage) != 0) {
                return 0.0;
            }
            double cpuSec = static_cast<double>(usage.ru_utime.tv_sec) +
                            static_cast<double>(usage.ru_utime.tv_usec) / 1e6 +
                            static_cast<double>(usage.ru_stime.tv_sec) +
                            static_cast<double>(usage.ru_stime.tv_usec) / 1e6;
            auto now = std::chrono::steady_clock::now();
            if (!hasPrev_) {
                hasPrev_ = true;
                prevWall_ = now;
                prevCpuSec_ = cpuSec;
                return 0.0;
            }
            double cpuDeltaSec = cpuSec - prevCpuSec_;
            double wallDeltaSec = std::chrono::duration<double>(now - prevWall_).count();
            prevWall_ = now;
            prevCpuSec_ = cpuSec;
            if (wallDeltaSec <= 0.0) {
                return 0.0;
            }
            return (cpuDeltaSec / wallDeltaSec) * 100.0;
        }

#else
        inline size_t getRssBytes() { return 0; }
        inline double getCpuPercent() { return 0.0; }
#endif
    }

    inline Stats sample() {
        return {.rssBytes = detail::getRssBytes(), .cpuPercent = detail::getCpuPercent()};
    }

    namespace Monitor {
        namespace detail {
            inline std::atomic<bool> running{false};
            inline std::thread worker;
            inline std::mutex mutex;
            inline Stats current_stats;

            inline void stop() {
                if (!running.exchange(false)) {
                    return;
                }
                if (worker.joinable()) {
                    worker.join();
                }
            }

            struct ThreadGuard {
                ~ThreadGuard() { stop(); }
            };
            inline ThreadGuard guard;
        }

        inline void start(std::chrono::milliseconds interval = std::chrono::seconds(1)) {
            if (detail::running.exchange(true)) {
                return;
            }

            detail::worker = std::thread([interval]() {
                while (detail::running) {
                    auto next_wakeup = std::chrono::steady_clock::now() + interval;

                    Stats new_stats = sample();

                    {
                        std::lock_guard<std::mutex> lock(detail::mutex);
                        detail::current_stats = new_stats;
                    }

                    std::this_thread::sleep_until(next_wakeup);
                }
            });
        }

        inline void stop() { detail::stop(); }

        inline Stats sample() {
            std::lock_guard<std::mutex> lock(detail::mutex);
            return detail::current_stats;
        }
    }
}
