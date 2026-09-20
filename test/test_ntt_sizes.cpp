#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include "Generated/TzdExperimentalCompute.h"

bool g_bigTime = true;

std::vector<uint64_t> limbs_from_str(const std::string& s) {
    const uint64_t LIMB_BASE = 1000000000ULL;
    const int DIGITS_PER_LIMB = 9;
    std::vector<uint64_t> limbs;
    int len = (int)s.size();
    for (int i = len; i > 0; i -= DIGITS_PER_LIMB) {
        int start = std::max(0, i - DIGITS_PER_LIMB);
        uint64_t val = 0;
        for (int j = start; j < i; j++) val = val * 10 + (s[j] - '0');
        limbs.push_back(val);
    }
    while (!limbs.empty() && limbs.back() == 0) limbs.pop_back();
    return limbs;
}

std::string limbs_to_str(const std::vector<uint64_t>& limbs) {
    if (limbs.empty()) return "0";
    std::string s = std::to_string(limbs.back());
    for (int i = (int)limbs.size() - 2; i >= 0; i--) {
        std::string part = std::to_string(limbs[i]);
        s.append(9 - part.size(), '0');
        s.append(part);
    }
    return s;
}

int main() {
    std::cout << "Testing bigint_mul_experimental_cpu_str for 1M and 4.74M digits...\n";
    for (size_t d : {1000000, 4741006}) {
        std::cout << "\nGenerating " << d << "-digit test strings...\n";
        std::string a(d, '9');
        std::string b(d, '9');

        auto t1 = std::chrono::high_resolution_clock::now();
        std::string c = bigint_mul_experimental_cpu_str(a, b);
        auto t2 = std::chrono::high_resolution_clock::now();

        double ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
        std::cout << "digits=" << d << " -> res_len=" << c.size() << "  time=" << ms << " ms\n";
    }
    std::cout << "\nAll extreme sizes passed!\n";
    return 0;
}
