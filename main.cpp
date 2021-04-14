#include <deque>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <system_error>
#include <iostream>

#include "global_alignment.hpp"

int main() {
    std::cout << GlobalAlignment::GetDiff("1.txt", "2.txt");
}