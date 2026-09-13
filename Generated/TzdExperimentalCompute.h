#pragma once

#include <string>
#include <vector>
#include <cstdint>

// --experimental-compute / --experimentalCompute flag:
// Enables the Tzd-Aether Cache-Aware SIMD 4-Step 3-Prime Montgomery NTT Engine on CPU.
extern bool g_experimentalCompute;

// Tzd-Aether CPU BigInt multiplication algorithm delivering 100% mathematical accuracy
std::string bigint_mul_experimental_cpu_str(const std::string& a, const std::string& b);
std::vector<uint64_t> bigint_mul_experimental_cpu_limbs(const std::vector<uint64_t>& la, const std::vector<uint64_t>& lb);
