#include "utils.h"

#include <cctype>

using namespace std;

string_view Strip(string_view line) {
    while (!line.empty() && isspace(line.front())) {
        line.remove_prefix(1);
    }
    while (!line.empty() && isspace(line.back())) {
        line.remove_suffix(1);
    }
    return line;
}

bool EqualWithAccuracy(double lhs, double rhs) {
    if (rhs != 0) {
        return abs(lhs - rhs) / rhs <= 0.0001;
    } else {
        return lhs == 0;
    }
}
