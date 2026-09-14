#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <cstdint>
#include <chrono>
#include <cmath>

struct OptimizedBigInt {
    static const uint64_t BASE = 1000000000ULL; // 10^9
    static const int BASE_DIGITS = 9;

    bool negative = false;
    std::vector<uint64_t> limbs; // least significant limb at index 0

    OptimizedBigInt() : negative(false) {}
    OptimizedBigInt(int64_t v) {
        if (v < 0) { negative = true; v = -v; }
        if (v == 0) limbs.push_back(0);
        while (v > 0) {
            limbs.push_back(v % BASE);
            v /= BASE;
        }
    }
    OptimizedBigInt(const std::string& str) {
        std::string s = str;
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        s.erase(s.find_last_not_of(" \t\r\n") + 1);
        if (s.empty()) { limbs.push_back(0); return; }
        size_t start = 0;
        if (s[0] == '-') { negative = true; start = 1; }
        else if (s[0] == '+') { start = 1; }

        for (int i = (int)s.size(); i > (int)start; i -= BASE_DIGITS) {
            int l_start = std::max((int)start, i - BASE_DIGITS);
            std::string sub = s.substr(l_start, i - l_start);
            uint64_t v = std::stoull(sub);
            limbs.push_back(v);
        }
        trim();
    }

    void trim() {
        while (limbs.size() > 1 && limbs.back() == 0) limbs.pop_back();
        if (limbs.empty()) limbs.push_back(0);
        if (limbs.size() == 1 && limbs[0] == 0) negative = false;
    }

    bool is_zero() const {
        return limbs.empty() || (limbs.size() == 1 && limbs[0] == 0);
    }

    std::string to_string() const {
        if (limbs.empty() || (limbs.size() == 1 && limbs[0] == 0)) return "0";
        std::string s;
        if (negative) s += '-';
        s += std::to_string(limbs.back());
        for (int i = (int)limbs.size() - 2; i >= 0; --i) {
            std::string sub = std::to_string(limbs[i]);
            s.append(BASE_DIGITS - sub.size(), '0');
            s += sub;
        }
        return s;
    }

    int cmp_abs(const OptimizedBigInt& o) const {
        if (limbs.size() != o.limbs.size()) return limbs.size() < o.limbs.size() ? -1 : 1;
        for (int i = (int)limbs.size() - 1; i >= 0; --i) {
            if (limbs[i] != o.limbs[i]) return limbs[i] < o.limbs[i] ? -1 : 1;
        }
        return 0;
    }

    OptimizedBigInt add_abs(const OptimizedBigInt& b) const {
        OptimizedBigInt res;
        uint64_t carry = 0;
        size_t n = std::max(limbs.size(), b.limbs.size());
        for (size_t i = 0; i < n || carry; ++i) {
            uint64_t sum = carry;
            if (i < limbs.size()) sum += limbs[i];
            if (i < b.limbs.size()) sum += b.limbs[i];
            res.limbs.push_back(sum % BASE);
            carry = sum / BASE;
        }
        res.trim();
        return res;
    }

    OptimizedBigInt sub_abs(const OptimizedBigInt& b) const {
        OptimizedBigInt res;
        int64_t borrow = 0;
        for (size_t i = 0; i < limbs.size(); ++i) {
            int64_t diff = (int64_t)limbs[i] - borrow - (i < b.limbs.size() ? (int64_t)b.limbs[i] : 0);
            if (diff < 0) { diff += BASE; borrow = 1; }
            else borrow = 0;
            res.limbs.push_back((uint64_t)diff);
        }
        res.trim();
        return res;
    }

    OptimizedBigInt operator+(const OptimizedBigInt& b) const {
        if (negative == b.negative) {
            OptimizedBigInt res = add_abs(b);
            res.negative = negative;
            return res;
        }
        if (cmp_abs(b) >= 0) {
            OptimizedBigInt res = sub_abs(b);
            res.negative = negative;
            return res;
        } else {
            OptimizedBigInt res = b.sub_abs(*this);
            res.negative = b.negative;
            return res;
        }
    }

    OptimizedBigInt operator-(const OptimizedBigInt& b) const {
        OptimizedBigInt neg_b = b;
        neg_b.negative = !b.negative;
        return *this + neg_b;
    }

    OptimizedBigInt mul_small(uint64_t v) const {
        if (v == 0 || is_zero()) return OptimizedBigInt(0);
        OptimizedBigInt res;
        res.negative = negative;
        uint64_t carry = 0;
        for (size_t i = 0; i < limbs.size() || carry; ++i) {
            uint64_t cur = carry + (i < limbs.size() ? limbs[i] * v : 0);
            res.limbs.push_back(cur % BASE);
            carry = cur / BASE;
        }
        res.trim();
        return res;
    }

    // Karatsuba multiplication
    static std::vector<uint64_t> karatsuba_mul(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b) {
        size_t n = std::max(a.size(), b.size());
        if (n <= 32) {
            std::vector<uint64_t> res(a.size() + b.size(), 0);
            for (size_t i = 0; i < a.size(); ++i) {
                uint64_t carry = 0;
                for (size_t j = 0; j < b.size() || carry; ++j) {
                    uint64_t cur = res[i + j] + carry + (j < b.size() ? a[i] * b[j] : 0);
                    res[i + j] = cur % BASE;
                    carry = cur / BASE;
                }
            }
            return res;
        }

        size_t k = (n + 1) / 2;
        std::vector<uint64_t> a0(a.begin(), a.begin() + std::min(k, a.size()));
        std::vector<uint64_t> a1(a.size() > k ? a.begin() + k : a.end(), a.end());
        std::vector<uint64_t> b0(b.begin(), b.begin() + std::min(k, b.size()));
        std::vector<uint64_t> b1(b.size() > k ? b.begin() + k : b.end(), b.end());

        auto z0 = karatsuba_mul(a0, b0);
        auto z2 = karatsuba_mul(a1, b1);

        // a0 + a1
        std::vector<uint64_t> a01 = a0;
        uint64_t carry = 0;
        for (size_t i = 0; i < std::max(a0.size(), a1.size()) || carry; ++i) {
            uint64_t sum = carry + (i < a0.size() ? a0[i] : 0) + (i < a1.size() ? a1[i] : 0);
            if (i < a01.size()) a01[i] = sum % BASE;
            else a01.push_back(sum % BASE);
            carry = sum / BASE;
        }
        // b0 + b1
        std::vector<uint64_t> b01 = b0;
        carry = 0;
        for (size_t i = 0; i < std::max(b0.size(), b1.size()) || carry; ++i) {
            uint64_t sum = carry + (i < b0.size() ? b0[i] : 0) + (i < b1.size() ? b1[i] : 0);
            if (i < b01.size()) b01[i] = sum % BASE;
            else b01.push_back(sum % BASE);
            carry = sum / BASE;
        }

        auto z1 = karatsuba_mul(a01, b01);

        // z1 = z1 - z0 - z2
        int64_t borrow = 0;
        for (size_t i = 0; i < z1.size(); ++i) {
            int64_t diff = (int64_t)z1[i] - borrow - (i < z0.size() ? (int64_t)z0[i] : 0) - (i < z2.size() ? (int64_t)z2[i] : 0);
            if (diff < 0) {
                int64_t b_cnt = (-diff + BASE - 1) / BASE;
                diff += b_cnt * BASE;
                borrow = b_cnt;
            } else borrow = 0;
            z1[i] = (uint64_t)diff;
        }

        // Combine result: z2 * BASE^(2k) + z1 * BASE^k + z0
        size_t total_sz = std::max({z0.size(), z1.size() + k, z2.size() + 2 * k}) + 2;
        std::vector<uint64_t> res(total_sz, 0);
        carry = 0;
        for (size_t i = 0; i < total_sz; ++i) {
            uint64_t sum = carry;
            if (i < z0.size()) sum += z0[i];
            if (i >= k && (i - k) < z1.size()) sum += z1[i - k];
            if (i >= 2 * k && (i - 2 * k) < z2.size()) sum += z2[i - 2 * k];
            res[i] = sum % BASE;
            carry = sum / BASE;
        }
        while (res.size() > 1 && res.back() == 0) res.pop_back();
        return res;
    }

    OptimizedBigInt operator*(const OptimizedBigInt& b) const {
        if (is_zero() || b.is_zero()) return OptimizedBigInt(0);
        OptimizedBigInt res;
        res.negative = negative ^ b.negative;
        res.limbs = karatsuba_mul(limbs, b.limbs);
        res.trim();
        return res;
    }

    std::pair<OptimizedBigInt, OptimizedBigInt> divmod(const OptimizedBigInt& b) const {
        if (b.is_zero()) return {OptimizedBigInt(0), OptimizedBigInt(0)};
        OptimizedBigInt q, rem;
        q.limbs.assign(limbs.size(), 0);
        for (int i = (int)limbs.size() - 1; i >= 0; --i) {
            rem.limbs.insert(rem.limbs.begin(), limbs[i]);
            rem.trim();
            uint64_t l = 0, r = BASE - 1, best = 0;
            while (l <= r) {
                uint64_t mid = (l + r) / 2;
                if (b.mul_small(mid).cmp_abs(rem) <= 0) {
                    best = mid;
                    l = mid + 1;
                } else {
                    if (mid == 0) break;
                    r = mid - 1;
                }
            }
            q.limbs[i] = best;
            rem = rem.sub_abs(b.mul_small(best));
        }
        q.negative = negative ^ b.negative;
        rem.negative = negative;
        q.trim();
        rem.trim();
        return {q, rem};
    }

    OptimizedBigInt operator/(const OptimizedBigInt& b) const { return divmod(b).first; }
    OptimizedBigInt operator%(const OptimizedBigInt& b) const { return divmod(b).second; }

    static OptimizedBigInt pow(OptimizedBigInt base, uint64_t exp) {
        OptimizedBigInt res(1);
        while (exp > 0) {
            if (exp & 1) res = res * base;
            base = base * base;
            exp >>= 1;
        }
        return res;
    }

    static OptimizedBigInt powmod(OptimizedBigInt base, OptimizedBigInt exp, const OptimizedBigInt& mod) {
        OptimizedBigInt res(1);
        base = base % mod;
        while (!exp.is_zero()) {
            if (exp.limbs[0] & 1) res = (res * base) % mod;
            base = (base * base) % mod;
            exp = exp / OptimizedBigInt(2);
        }
        return res;
    }

    static OptimizedBigInt gcd(OptimizedBigInt a, OptimizedBigInt b) {
        a.negative = false;
        b.negative = false;
        while (!b.is_zero()) {
            OptimizedBigInt r = a % b;
            a = b;
            b = r;
        }
        return a;
    }

    static OptimizedBigInt factorial(int64_t n) {
        if (n <= 1) return OptimizedBigInt(1);
        OptimizedBigInt res(1);
        for (int64_t i = 2; i <= n; ++i) {
            res = res.mul_small(i);
        }
        return res;
    }
};

int main() {
    auto t0 = std::chrono::high_resolution_clock::now();
    OptimizedBigInt f1000 = OptimizedBigInt::factorial(1000);
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "1000! digits: " << f1000.to_string().size() << " (computed in " << ms << " ms)\n";

    // Big multiplication benchmark (e.g. two 2000-digit numbers)
    std::string s1(1000, '9');
    std::string s2(1000, '8');
    OptimizedBigInt b1(s1), b2(s2);
    auto t2 = std::chrono::high_resolution_clock::now();
    OptimizedBigInt b3 = b1 * b2;
    auto t3 = std::chrono::high_resolution_clock::now();
    double mul_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();
    std::cout << "1000-digit * 1000-digit Karatsuba: result digits " << b3.to_string().size() << " (computed in " << mul_ms << " ms)\n";

    // Power test
    OptimizedBigInt p2_100 = OptimizedBigInt::pow(OptimizedBigInt(2), 100);
    std::cout << "2^100 = " << p2_100.to_string() << "\n";
    return 0;
}
