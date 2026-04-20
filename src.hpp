#pragma once
#include "interface.h"
#include "definition.h"
#include <random>
#include <algorithm>
#include <map>
#include <queue>
#include <cmath>

namespace oj {

auto generate_tasks(const Description &desc) -> std::vector<Task> {
    std::vector<Task> tasks;
    tasks.reserve(desc.task_count);

    std::mt19937_64 rng(42);

    // Calculate max throughput with all CPUs
    double max_throughput = std::pow(PublicInformation::kCPUCount, PublicInformation::kAccel);

    // Target sums
    time_t target_exec_sum = (desc.execution_time_sum.min + desc.execution_time_sum.max) / 2;
    priority_t target_prio_sum = (desc.priority_sum.min + desc.priority_sum.max) / 2;

    time_t total_exec_time = 0;
    priority_t total_priority = 0;

    for (size_t i = 0; i < desc.task_count; ++i) {
        Task task;

        // Distribute execution time evenly
        time_t base_exec = target_exec_sum / desc.task_count;
        time_t var_range = std::max((time_t)1, base_exec / 4);
        std::uniform_int_distribution<time_t> exec_var(0, var_range);
        time_t exec_candidate = base_exec + exec_var(rng) - var_range / 2;
        task.execution_time = std::max(desc.execution_time_single.min,
                                       std::min(desc.execution_time_single.max, exec_candidate));
        total_exec_time += task.execution_time;

        // Distribute priority evenly
        priority_t base_prio = target_prio_sum / desc.task_count;
        priority_t prio_var_range = std::max((priority_t)1, base_prio / 4);
        std::uniform_int_distribution<priority_t> prio_var(0, prio_var_range);
        priority_t prio_candidate = base_prio + prio_var(rng) - prio_var_range / 2;
        task.priority = std::max(desc.priority_single.min,
                                std::min(desc.priority_single.max, prio_candidate));
        total_priority += task.priority;

        // Choose deadline with enough space for the task
        double min_time_for_task = PublicInformation::kStartUp + PublicInformation::kSaving +
                                   task.execution_time / max_throughput + 1;
        time_t min_deadline = std::max(desc.deadline_time.min, (time_t)std::ceil(min_time_for_task));
        std::uniform_int_distribution<time_t> deadline_dist(min_deadline, desc.deadline_time.max);
        task.deadline = deadline_dist(rng);

        // Choose launch time with safe margin
        double exec_duration = task.execution_time / max_throughput;
        double required_time = PublicInformation::kStartUp + PublicInformation::kSaving + exec_duration;
        time_t max_launch = std::max((time_t)0, (time_t)(task.deadline - required_time - 1));

        std::uniform_int_distribution<time_t> launch_dist(0, max_launch);
        task.launch_time = launch_dist(rng);

        tasks.push_back(task);
    }

    // Adjust totals to match constraints
    if (total_exec_time < desc.execution_time_sum.min) {
        time_t deficit = desc.execution_time_sum.min - total_exec_time;
        tasks.back().execution_time += deficit;
    } else if (total_exec_time > desc.execution_time_sum.max) {
        time_t excess = total_exec_time - desc.execution_time_sum.max;
        tasks.back().execution_time -= excess;
    }

    if (total_priority < desc.priority_sum.min) {
        priority_t deficit = desc.priority_sum.min - total_priority;
        tasks.back().priority += deficit;
    } else if (total_priority > desc.priority_sum.max) {
        priority_t excess = total_priority - desc.priority_sum.max;
        tasks.back().priority -= excess;
    }

    // Fix last task's launch time if needed after adjustment
    auto &last = tasks.back();
    double exec_duration = last.execution_time / max_throughput;
    double required_time = PublicInformation::kStartUp + PublicInformation::kSaving + exec_duration;
    time_t max_launch = std::max((time_t)0, (time_t)(last.deadline - required_time - 1));
    last.launch_time = std::min(last.launch_time, max_launch);

    // Sort by launch time
    std::sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b) {
        return a.launch_time < b.launch_time;
    });

    return tasks;
}

} // namespace oj

namespace oj {

// Track task state
struct TaskInfo {
    Task task;
    task_id_t id;
    double work_done;
    bool is_running;
    bool is_saving;
    time_t save_end_time;
    time_t current_launch_time;
    cpu_id_t current_cpu_cnt;
};

auto schedule_tasks(time_t time, std::vector<Task> list, const Description &desc) -> std::vector<Policy> {
    static task_id_t task_id = 0;
    const task_id_t first_id = task_id;
    task_id += list.size();

    static std::map<task_id_t, TaskInfo> all_tasks;
    static cpu_id_t cpu_used = 0;

    // Add new tasks
    for (size_t i = 0; i < list.size(); ++i) {
        TaskInfo info;
        info.task = list[i];
        info.id = first_id + i;
        info.work_done = 0.0;
        info.is_running = false;
        info.is_saving = false;
        info.save_end_time = 0;
        info.current_launch_time = 0;
        info.current_cpu_cnt = 0;
        all_tasks[info.id] = info;
    }

    // Update saving tasks
    std::vector<task_id_t> to_free;
    for (auto &[tid, info] : all_tasks) {
        if (info.is_saving && time >= info.save_end_time) {
            cpu_used -= info.current_cpu_cnt;
            info.is_saving = false;
            info.is_running = false;

            // Check if task is complete
            if (info.work_done >= info.task.execution_time) {
                to_free.push_back(tid);
            }
        }
    }

    for (auto tid : to_free) {
        all_tasks.erase(tid);
    }

    std::vector<Policy> policies;

    // Priority queue: sort by deadline (earliest first), then priority (highest first)
    std::vector<task_id_t> ready_tasks;
    for (const auto &[tid, info] : all_tasks) {
        if (!info.is_running && !info.is_saving && info.work_done < info.task.execution_time) {
            if (time <= info.task.deadline) {
                ready_tasks.push_back(tid);
            }
        }
    }

    std::sort(ready_tasks.begin(), ready_tasks.end(), [&](task_id_t a, task_id_t b) {
        const auto &ta = all_tasks[a];
        const auto &tb = all_tasks[b];

        time_t slack_a = ta.task.deadline - time;
        time_t slack_b = tb.task.deadline - time;

        // Prioritize by urgency
        if (slack_a != slack_b) return slack_a < slack_b;

        // Then by priority
        return ta.task.priority > tb.task.priority;
    });

    // Try to save running tasks that are near completion
    for (auto &[tid, info] : all_tasks) {
        if (info.is_running && !info.is_saving) {
            double work_remaining = info.task.execution_time - info.work_done;

            // Save if we're done or nearly done
            if (work_remaining <= 0) {
                policies.push_back(Saving{tid});
                info.is_saving = true;
                info.save_end_time = time + PublicInformation::kSaving;
            }
        }
    }

    // Launch new tasks
    const cpu_id_t cpu_available = desc.cpu_count;
    for (task_id_t tid : ready_tasks) {
        auto &info = all_tasks[tid];

        double work_remaining = info.task.execution_time - info.work_done;
        time_t time_remaining = info.task.deadline - time;

        if (time_remaining <= PublicInformation::kStartUp + PublicInformation::kSaving) {
            continue; // Not enough time
        }

        // Determine optimal CPU count
        cpu_id_t optimal_cpu = 1;
        time_t effective_time = time_remaining - PublicInformation::kStartUp - PublicInformation::kSaving;

        for (cpu_id_t k = 1; k <= std::min(cpu_available - cpu_used, cpu_available); ++k) {
            double throughput = std::pow(k, PublicInformation::kAccel) * effective_time;
            if (throughput >= work_remaining) {
                optimal_cpu = k;
                break;
            }
        }

        // Cap at reasonable amount
        optimal_cpu = std::min(optimal_cpu, (cpu_id_t)20);
        optimal_cpu = std::max(optimal_cpu, (cpu_id_t)1);

        if (cpu_used + optimal_cpu <= cpu_available) {
            policies.push_back(Launch{optimal_cpu, tid});
            info.is_running = true;
            info.current_launch_time = time;
            info.current_cpu_cnt = optimal_cpu;
            cpu_used += optimal_cpu;
        }
    }

    // Update work done for running tasks
    for (auto &[tid, info] : all_tasks) {
        if (info.is_running && !info.is_saving) {
            time_t duration = time - info.current_launch_time + 1;
            if (duration > PublicInformation::kStartUp) {
                double effective_time = duration - PublicInformation::kStartUp;
                double work = std::pow(info.current_cpu_cnt, PublicInformation::kAccel) * effective_time;
                info.work_done = work;
            }
        }
    }

    return policies;
}

} // namespace oj
