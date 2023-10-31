#include "checker_utils.h"

std::pair<int, std::string> readInt(std::ifstream &in, int l, int r, const std::string& name) {
    int x; in >> x;
    if (in.fail()) {
        return std::make_pair(0, "Can't read any further, blocked at " + name + ".");
    }

    if (x < l || x > r) {
        return std::make_pair(0, name + " is not in the interval [" + std::to_string(l) + ", " + std::to_string(r) + "].");
    }

    return std::make_pair(x, "");
}

