
// exceptional Chapter 2, P25

#include <iomanip>
#include <sstream>

#include <cstdio>
#include <cstring>
#include <cassert>

void RunTinyTests();

void test_sprintf_caveat() {
    char smallBuf[5] = {0, 0, 0, 0, 0};

    // given a char array of length 5
    // conversion is successful only for:
    // 1, 10, 100, 1000, 10000
    int value = 10000;
    assert(5 == std::sprintf(smallBuf, "%d", value));
    assert(0 == std::strcmp("10000", smallBuf));

    // P25
    // (increase the number of digits) we will start scribbling past
    // the end of smallBuf, which might be into the bytes of value
    // itself if the compiler choose a memory layout that put value
    // immediate;y after smallBuf in memory
}

void test_stringstream() {
    std::stringstream ss;
    ss << std::setw(4) << 1000000;
    assert(std::string("1000000") == ss.str());
    ss.str("");
    ss << std::setw(4) << 1;
    assert(std::string("   1") == ss.str());

}

int main() {
    RunTinyTests();
    return 0;
}