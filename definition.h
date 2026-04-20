#pragma once
#include "interface.h"
#include <cmath>

namespace oj {

struct Task {
    time_t      launch_time;
    time_t      deadline;
    time_t      execution_time;
    priority_t  priority;
};

template <typename _Tp>
struct Range {
    _Tp min;
    _Tp max;
};

struct Description {
    // Count of CPUs
    cpu_id_t    cpu_count;
    // Number of tasks that you should generate
    task_id_t   task_count;
    // Range of deadline time
    Range <time_t> deadline_time;
    // Range of a single execution time
    Range <time_t> execution_time_single;
    // Range of the sum of execution time
    Range <time_t> execution_time_sum;
    // Range of priority of one
    Range <priority_t> priority_single;
    // Range of priority of sum
    Range <priority_t> priority_sum;
};

struct Launch {
    cpu_id_t    cpu_cnt;
    task_id_t   task_id;
};

struct Saving {
    task_id_t   task_id;
};

struct Cancel {
    task_id_t   task_id;
};

struct PublicInformation {
    static constexpr time_t   kMaxTime  = 1e8;
    static constexpr cpu_id_t kCPUCount = 114;
    static constexpr time_t   kStartUp  = 2;
    static constexpr time_t   kSaving   = 2;
    static constexpr double   kAccel    = 0.75;
};

/**
 * @brief Suppose a task first launched at x, and start saving at y,
 * then the duration should be y - x.
 * (Note that the saving is completed at y + kSaving)
 * 
 * If duration < kStartUp, the task can't be saved,
 * and we should not count the contribution.
 * 
 * Otherwise, we count the contribution by the multiplication
 * of effective core count and the effective duration.
 * 
 * @return The contribution (time) of the task within duration.
 */
inline auto time_policy(time_t duration, cpu_id_t cpu_cnt) -> double {
    if (duration < PublicInformation::kStartUp) return 0;
    const auto effective_core = std::pow(cpu_cnt, PublicInformation::kAccel);
    const auto effective_time = duration - PublicInformation::kStartUp;
    return effective_core * effective_time;
}

/* The description of each oj sub-task. */

inline constexpr Description senpai = {
    .cpu_count              = PublicInformation::kCPUCount,
    .task_count             = 114514,
    .deadline_time          = { .min = 1,   .max = int(1e6) },
    .execution_time_single  = { .min = 1,   .max = int(1e4) },
    .execution_time_sum     = { .min = int(2e5), .max = 1919810  },
    .priority_single        = { .min = 1,   .max = 114514   },
    .priority_sum           = { .min = 10,   .max = 1919810  },
};

constexpr Description small = {
    .cpu_count              = PublicInformation::kCPUCount,
    .task_count             = 1000,
    .deadline_time          = { .min = 1, .max = 900        }, 
    .execution_time_single  = { .min = 1, .max = 100        },
    .execution_time_sum     = { .min = 1, .max = 10000      },
    .priority_single        = { .min = 10, .max = 20         },
    .priority_sum           = { .min = 10, .max = 10000      },
};

constexpr Description middle = {
    .cpu_count              = PublicInformation::kCPUCount,
    .task_count             = 100000,
    .deadline_time          = { .min = 1,   .max = 300000   },
    .execution_time_single  = { .min = 100, .max = 10000    },
    .execution_time_sum     = { .min = 50000000, .max = 99999999 },
    .priority_single        = { .min = 1,   .max = 1000     },
    .priority_sum           = { .min = 1,   .max = 10000000 },
};

constexpr Description huge = {
    .cpu_count              = PublicInformation::kCPUCount,
    .task_count             = 100000,
    .deadline_time          = { .min = 1,   .max = 3000000  },
    .execution_time_single  = { .min = 999, .max = 100000   },
    .execution_time_sum     = { .min = 1,   .max = 999999999 },
    .priority_single        = { .min = 1,   .max = 1000     },
    .priority_sum           = { .min = 1,   .max = 10000000 },
};

constexpr oj::Description testcase_array[] = {
    small,
    middle,
    senpai,
    huge,
};

} // namespace oj
