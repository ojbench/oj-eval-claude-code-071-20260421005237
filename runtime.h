#pragma once
#include "interface.h"
#include "definition.h"
#include <bit>
#include <span>
#include <cmath>
#include <array>
#include <ranges>
#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <variant>
#include <optional>
#include <algorithm>
#include <stdexcept>
#include <filesystem>
#include <unordered_set>

namespace oj::detail::runtime {

struct OJException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct UserException : public OJException {
    using OJException::OJException;
};

struct SystemException : public OJException {
    using OJException::OJException;
};

template <typename _Error = UserException>
[[noreturn]]
static void panic(const std::string &message) {
    throw UserException { message };
}

struct ServiceInfo {
    priority_t complete;
    priority_t total;
};

struct RuntimeManager : public PublicInformation {
private:
    struct TaskFree {
        /* Nothing. */
    };
    struct TaskLaunch {
        cpu_id_t cpu_cnt;
        time_t   start;
    };
    struct TaskSaving {
        cpu_id_t cpu_cnt;
        time_t   finish;
        double   time_passed;
    };

    struct TaskStatus {
        using WorkLoad = std::variant <TaskFree, TaskLaunch, TaskSaving>;
        WorkLoad workload;
        double time_passed; // Total time passed.
        const time_t deadline;
    };

    // Return time from when the task have done.
    auto time_policy(const TaskLaunch &launch) const -> double {
        const auto distance = get_time() - launch.start;
        return oj::time_policy(distance, launch.cpu_cnt);
    }

    void launch_check(const Launch &command) const {
        const auto [cpu_cnt, task_id] = command;
        if (cpu_cnt == 0)
            panic("Launch: CPU count should not be zero.");
        if (cpu_cnt > kCPUCount)
            panic("Launch: CPU count exceeds the kMaxCPU limit.");
        if (task_id >= global_tasks)
            panic("Launch: Task ID out of range.");
        const auto &workload = task_state[task_id].workload;
        if (!holds_alternative <TaskFree> (workload))
            panic("Launch: Task is not free.");
    }

    // From free -> launch.
    void launch_commit(const Launch &command) {
        const auto [cpu_cnt, task_id] = command;
        auto &workload = task_state[task_id].workload;

        this->cpu_usage += cpu_cnt;

        workload = TaskLaunch {
            .cpu_cnt    = cpu_cnt,
            .start      = get_time(),
        };
    }

    void saving_check(const Saving &command) const {
        const auto [task_id] = command;
        if (task_id >= global_tasks)
            panic("Saving: Task ID out of range.");

        const auto &workload = task_state[task_id].workload;
        if (!holds_alternative <TaskLaunch> (workload))
            panic("Saving: Task is not launched.");
    }

    // From launch -> saving.
    void saving_commit(const Saving &command) {
        const auto [task_id] = command;

        auto &task = task_state[task_id];
        auto &workload = task.workload;
        const auto &launch = get <TaskLaunch> (workload);
        const auto time_sum = this->time_policy(launch);

        task_saving.insert(&task);

        auto [cpu_cnt, start] = launch;
        workload = TaskSaving {
            .cpu_cnt    = cpu_cnt,
            .finish     = get_time() + kSaving,
            .time_passed = time_sum,
        };
    }

    void cancel_check(const Cancel &command) const {
        const auto [task_id] = command;
        if (task_id >= global_tasks)
            panic("Cancel: Task ID out of range.");
    }

    void cancel_commit(const Cancel &command) {
        const auto [task_id] = command;
        auto &task = task_state[task_id];
        auto &workload = task.workload;

        if (holds_alternative <TaskLaunch> (workload)) {
            // From launch -> free.
            auto &launch = get <TaskLaunch> (workload);
            this->cpu_usage -= launch.cpu_cnt;
        } else if (holds_alternative <TaskSaving> (workload)) {
            // From saving -> free.
            auto &saving = get <TaskSaving> (workload);
            this->cpu_usage -= saving.cpu_cnt;
            this->task_saving.erase(&task);
        }

        workload = TaskFree {};
    }

    /* Counting all the tasks in this cycle. */
    auto get_new_tasks() -> std::vector <Task> {
        auto &which = this->global_tasks;
        const auto start = which;

        while (which < task_list.size()
        && task_list[which].launch_time == get_time())
            which += 1;

        auto begin = task_list.begin();
        return std::vector <Task> (begin + start, begin + which);
    }

    void work(const Launch &command) {
        this->launch_check(command);
        this->launch_commit(command);
    }

    void work(const Saving &command) {
        this->saving_check(command);
        this->saving_commit(command);
    }

    void work(const Cancel &command) {
        this->cancel_check(command);
        this->cancel_commit(command);
    }

    /* Remove those outdated saving file within.  */
    void complete_this_cycle() {
        auto begin = task_saving.begin();
        auto finish = task_saving.end();

        while (begin != finish) {
            auto &task = **begin;
            auto &workload = task.workload;
            auto &saving = get <TaskSaving> (workload);

            if (saving.finish != this->get_time()) {
                ++begin; continue;
            }

            // From saving -> free.

            cpu_usage -= saving.cpu_cnt;

            if (this->get_time() <= task.deadline) {
                task.time_passed += saving.time_passed;
                workload = TaskFree {};
            }

            begin = task_saving.erase(begin);
        }
    }

public:
    explicit RuntimeManager(std::vector <Task> task_list)
        : global_clock(-1), global_tasks(0), cpu_usage(0), task_list(std::move(task_list)) {
        if (!std::ranges::is_sorted(this->task_list, {}, &Task::launch_time))
            panic <SystemException> ("Task list is not sorted.");
        task_state.reserve(this->task_list.size());
        for (auto &task : this->task_list)
            task_state.push_back(TaskStatus {
                .workload       = TaskFree {},
                .time_passed    = 0,
                .deadline       = task.deadline,
            });
    }

    auto synchronize() -> std::vector <Task> {
        this->complete_this_cycle();

        if (this->cpu_usage > kCPUCount)
            panic("CPU usage exceeds the limit.");

        global_clock += 1;
        return this->get_new_tasks();
    }

    void work(std::vector <Policy> p) {
        for (const auto &policy : p) {
            std::visit([this](const auto &command) { this->work(command); }, policy);
        }
    }

    auto get_time() const -> time_t {
        return global_clock;
    }

    auto get_service_info() const -> ServiceInfo {
        ServiceInfo result { .complete = 0, .total = 0 };
        for (task_id_t id = 0; id < global_tasks; ++id) {
            auto &state   = task_state[id];
            auto &task    = task_list[id];
            if (time_t(state.time_passed) >= task.execution_time) {
                result.complete += task.priority;
            }
            result.total += task.priority;
        }
        return result;
    }

private:
    time_t      global_clock;   // A global clock to record the current time
    task_id_t   global_tasks;   // A global task ID counter.
    cpu_id_t    cpu_usage;      // The current CPU usage

    const std::vector <Task> task_list;         // A list of tasks
    std::vector <TaskStatus> task_state;        // A list of task status
    std::unordered_set <TaskStatus *> task_saving; // A list of free tasks
};

} // oj::detail::runtime

/* Some other functions. */
namespace oj::detail::runtime {

struct Header {
    std::size_t task_count;
    Description description;
    ServiceInfo service_info;
    const bool        error_occur   = false;
    const std::size_t error_length  = 0;
    const std::size_t magic         = kMagic;
    // 'D' 'A' 'R' 'K'
    // 'W' 'H' 'A' 'T'
    static constexpr std::size_t kMagic =
        std::size_t(0x4B524144) << 32 | std::size_t(0x54414857);
};

inline void serialize_error(std::ostream &os, const std::string &msg) {
    const auto header = Header {
        .task_count   = 0,
        .description  = {},
        .service_info = {},
        .error_occur  = true,
        .error_length = msg.size(),
    };

    os.write(std::bit_cast <const char *> (&header), sizeof(Header));
    os.write(msg.data(), msg.size());

    if (!os.good())
        std::exit(EXIT_FAILURE);
}

inline void serialize(
    std::ostream &os,
    std::span <const Task> vec,
    Description description,
    ServiceInfo service_info) {
    const auto header = Header {
        .task_count   = vec.size(),
        .description  = description,
        .service_info = service_info,
    };
    os.write(std::bit_cast <const char *> (&header), sizeof(Header));

    const auto size = vec.size() * sizeof(Task);
    os.write(std::bit_cast <const char *> (vec.data()), size);

    if (!os.good())
        panic <SystemException> ("File write failed.");
}

inline auto deserialize_error(std::istream &is) -> std::string {
    Header header;
    is.read(std::bit_cast <char *> (&header), sizeof(Header));

    if (header.magic != header.kMagic) {
        return "User Error: What the fuck did you output?";
    }

    if (header.error_occur) {
        std::string message(header.error_length, '\0');
        is.read(message.data(), header.error_length);
        return message;
    }

    std::vector <Task> vec(header.task_count);
    const auto size = vec.size() * sizeof(Task);
    is.read(std::bit_cast <char *> (vec.data()), size);

    if (!is.good()) {
        return "System Error: File incomplete.";
    }

    return {};
}


inline auto deserialize(std::istream &is) -> std::pair <Header, std::vector <Task>> {
    Header header;

    is.read(std::bit_cast <char *> (&header), sizeof(Header));

    if (header.magic != header.kMagic)
        panic <SystemException> ("System Error: Not handled in the spj!");

    if (header.error_occur)
        panic <SystemException> ("System Error: Not handled in the spj!");

    std::vector <Task> vec(header.task_count);
    const auto size = vec.size() * sizeof(Task);
    is.read(std::bit_cast <char *> (vec.data()), size);

    if (!is.good())
        panic <SystemException> ("System Error: Not handled in the spj!");

    return { std::move(header), std::move(vec) };
}

template <typename _Tp>
static bool within(_Tp x, Range <_Tp> range) {
    return range.min <= x && x <= range.max;
}

static void check_tasks(std::span <const Task> tasks, const Description &desc) {
    if (tasks.size() != desc.task_count)
        panic("The number of tasks is not equal to the number of tasks required.");

    time_t execution_time_sum   = 0;
    priority_t priority_sum     = 0;
    for (const auto &task : tasks) {
        if (task.launch_time +
            oj::PublicInformation::kSaving +
            oj::PublicInformation::kStartUp +
            (double)task.execution_time /
            pow(oj::PublicInformation::kCPUCount, oj::PublicInformation::kAccel)
            >= task.deadline)
            panic("The task is impossible to finish.");

        if (task.launch_time >= task.deadline)
            panic("The launch time is no earlier to the deadline.");

        if (!within(task.deadline, desc.deadline_time))
            panic("The deadline time is out of range.");

        if (!within(task.execution_time, desc.execution_time_single))
            panic("The execution time is out of range.");

        if (!within(task.priority, desc.priority_single))
            panic("The priority is out of range.");

        execution_time_sum += task.execution_time;
        priority_sum += task.priority;
    }

    if (!within(execution_time_sum, desc.execution_time_sum))
        panic("The total execution time is out of range.");

    if (!within(priority_sum, desc.priority_sum))
        panic("The total priority is out of range.");
}

[[maybe_unused]]
static auto generate_work(const Description &desc) -> std::vector <Task> {
    auto tasks = generate_tasks(desc);

    std::ranges::sort(tasks, {}, &Task::launch_time);

    /* Check the tasks. */
    check_tasks(tasks, desc);

    return tasks;
}

[[maybe_unused]]
static auto schedule_work(const Description &desc, std::vector <Task> tasks)
-> ServiceInfo {
    RuntimeManager manager { std::move(tasks) };

    for (std::size_t i = 0; i <= desc.deadline_time.max; ++i) {
        auto new_tasks = manager.synchronize();
        if (i != manager.get_time())
            panic <SystemException> ("Time is not synchronized");
        manager.work(schedule_tasks(i, std::move(new_tasks), desc));
    }

    manager.synchronize();

    return manager.get_service_info();
}

enum class JudgeResult {
    GenerateFailed,
    ScheduleFailed,
};

} // namespace oj::detail::runtime
