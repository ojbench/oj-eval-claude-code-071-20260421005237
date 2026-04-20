#include "runtime.h"
#include <iostream>
#include <fstream>

static void check_error(std::istream &output, std::ostream &score, std::ostream &message) {
    double result;
    output >> result;
    if (result < 0) {
        std::string error_message;
        output >> error_message;
        score << 0;
        message << error_message;
    } else {
        if (result >= 0 && result <= 1) {
            score << result;
        } else {
            score << 0;
            message << "System Error: Server grading crashed";   
        }
    }
}

namespace oj {

auto generate_tasks(const Description &) -> std::vector <Task> {
    return {};
}

auto schedule_tasks(time_t, std::vector <Task>, const Description &) -> std::vector<Policy> {
    return {};
}

} // namespace oj

signed main(int argc, char *argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <input> <output> <answer> <score> <message>\n";
        return 1;
    } else {
        std::ifstream input(argv[1]);
        std::ifstream output(argv[2]);
        std::ifstream answer(argv[3]);
        std::ofstream score(argv[4]);
        std::ofstream message(argv[5]);
        check_error(output, score, message);
        return 0;
    }
}
