#pragma once

#include <algorithm>
#include <iostream>
#include <optional>
#include <fstream>
#include <utility>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>
#include <cassert>
#include <cstdlib>
#include <cmath>
#include <unistd.h>
#include <boost/tokenizer.hpp>
#include <sys/resource.h>

#define aaa system("read -r -p \"Press enter to continue...\" key");

std::pair<int, std::string> readInt(std::ifstream &in, int l, int r, const std::string& name);

