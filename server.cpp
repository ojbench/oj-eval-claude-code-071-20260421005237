#include "runtime.h"
#include "src.hpp"

namespace oj::detail::runtime {

static auto grading_policy(priority_t std_ans, priority_t usr_ans) -> double {
    // Mapping [0, infty] -> [0, 1]

    if (std_ans == 0) return 1;

    double ratio = double(usr_ans) / std_ans;

    if (ratio >= 2) return 1;

    return ratio / 2;
}

static auto judge() {
    using enum JudgeResult;
    std::string error_message;
    try {
        auto [header, tasks] = deserialize(std::cin);
        const auto &std_info = header.service_info;
        const auto &desc     = header.description;
        auto info = schedule_work(desc, tasks);

        if (info.total != std_info.total)
            panic <SystemException> ("Total service priority mismatch!");

        const auto std_ans = std_info.complete;
        const auto usr_ans = info.complete;

        std::cout << grading_policy(std_ans, usr_ans) << std::endl;
        return;
    } catch (const OJException &e) {
        if (dynamic_cast <const UserException *> (&e)) {
            error_message = "Schedule failed: ";
        } else { // Unknown system error.
            error_message = "System error: ";
        }
        error_message += e.what();
    } catch (const std::exception &e) {
        error_message = "System error: Unexpected std::exception(): ";
        error_message += e.what();
    } catch (...) {
        error_message = "System error: An unknown error occurred!";
    }

    // Error handling here.
    std::cout << -1 << std::endl;
    std::cout << error_message << std::endl;

    std::cerr << error_message << std::endl;
}

} // namespace oj::detail::runtime

signed main() {
    using oj::detail::runtime::judge;
    judge();
    return 0;
}
