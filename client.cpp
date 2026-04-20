#include "runtime.h"
#include "src.hpp"

namespace oj::detail::runtime {

template <JudgeResult _Default_Result>
[[noreturn]]
static void handle_exception(const OJException &e) {
    std::string error_message;
    if (dynamic_cast <const UserException *> (&e)) {
        if constexpr (_Default_Result == JudgeResult::GenerateFailed) {
            error_message = "Generate failed: ";
        }
        if constexpr (_Default_Result == JudgeResult::ScheduleFailed) {
            error_message = "Schedule failed: ";
        }
    } else { // Unknown system error.
        error_message = "System error: ";
    }

    error_message += e.what();

    std::cerr << error_message << std::endl;
    serialize_error(std::cout, error_message);

    std::exit(EXIT_SUCCESS);
}

static auto judge(const Description &desc) {
    using enum JudgeResult;
    std::vector <Task> tasks;

    try {
        tasks = generate_work(desc);
    } catch (const OJException &e) {
        handle_exception <GenerateFailed> (e);
    }

    try {
        auto info = schedule_work(desc, tasks);

        std::cerr << "Complete rate: "
            << std::setprecision(2)
            << std::fixed
            << 100 * double(info.complete) / info.total
            << "% ("
            << info.complete
            << "/"
            << info.total
            << ")"
            << std::endl;

        serialize(std::cout, tasks, desc, info);
    } catch (const OJException &e) {
        handle_exception <ScheduleFailed> (e);
    }
}

} // namespace oj::detail::runtime

signed main() {
    try {
        unsigned x;
        std::cin >> x;
        if (x > 3) return 1;
        using oj::detail::runtime::serialize;
        using oj::detail::runtime::judge;
        judge(oj::testcase_array[x]);
    } catch (const std::exception &e) {
        std::string msg = "System Error: Unexpected std::exception(): ";
        msg += e.what();
        oj::detail::runtime::serialize_error(std::cout, std::move(msg));
    } catch (...) {
        oj::detail::runtime::serialize_error(std::cout, "System Error: An unknown error occurred!");
    }
    return 0;
}
