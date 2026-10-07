#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <cstdint>

static const uint64_t BASE_10_9 = 1000000000ULL;

static void karatsuba_mul_rec(uint64_t* res, const uint64_t* a, size_t na, const uint64_t* b, size_t nb, uint64_t* ws) {
    if (na < nb) { karatsuba_mul_rec(res, b, nb, a, na, ws); return; }
    if (nb <= 32) {
        std::memset(res, 0, (na + nb + 2) * sizeof(uint64_t));
        for (size_t i = 0; i < na; i++) {
            uint64_t ai = a[i];
            if (ai == 0) continue;
            uint64_t carry = 0;
            for (size_t j = 0; j < nb; j++) {
                uint64_t cur = res[i + j] + ai * b[j] + carry;
                res[i + j] = cur % BASE_10_9;
                carry = cur / BASE_10_9;
            }
            res[i + nb] += carry;
        }
        return;
    }

    // Unbalanced operands: split longer operand in half
    if (na > 2 * nb) {
        size_t k = na / 2;
        uint64_t* r0 = ws;
        uint64_t* r1 = r0 + (k + nb + 4);
        uint64_t* next_ws = r1 + (na - k + nb + 4);

        karatsuba_mul_rec(r0, a, k, b, nb, next_ws);
        karatsuba_mul_rec(r1, a + k, na - k, b, nb, next_ws);

        std::memset(res, 0, (na + nb + 4) * sizeof(uint64_t));
        for (size_t i = 0; i < k + nb; i++) res[i] = r0[i];

        uint64_t c = 0;
        for (size_t i = 0; i < (na - k + nb) || c; i++) {
            uint64_t v = res[i + k] + (i < (na - k + nb) ? r1[i] : 0) + c;
            res[i + k] = v % BASE_10_9;
            c = v / BASE_10_9;
        }
        return;
    }

    size_t k = (na + 1) / 2;
    size_t a0_len = (k < na) ? k : na;
    size_t a1_len = (na > k) ? (na - k) : 0;
    size_t b0_len = (k < nb) ? k : nb;
    size_t b1_len = (nb > k) ? (nb - k) : 0;

    const uint64_t* a0 = a;
    const uint64_t* a1 = a + k;
    const uint64_t* b0 = b;
    const uint64_t* b1 = b + k;

    uint64_t* z0 = ws;
    uint64_t* z2 = z0 + 2 * k + 4;
    uint64_t* sa = z2 + 2 * k + 4;
    uint64_t* sb = sa + k + 4;
    uint64_t* z1 = sb + k + 4;
    uint64_t* next_ws = z1 + 2 * k + 8;

    karatsuba_mul_rec(z0, a0, a0_len, b0, b0_len, next_ws);

    if (a1_len > 0 && b1_len > 0) {
        karatsuba_mul_rec(z2, a1, a1_len, b1, b1_len, next_ws);
    } else {
        std::memset(z2, 0, (a1_len + b1_len + 4) * sizeof(uint64_t));
    }

    size_t sa_len = (a0_len > a1_len ? a0_len : a1_len);
    uint64_t c = 0;
    for (size_t i = 0; i < sa_len; i++) {
        uint64_t v = (i < a0_len ? a0[i] : 0) + (i < a1_len ? a1[i] : 0) + c;
        sa[i] = v % BASE_10_9;
        c = v / BASE_10_9;
    }
    if (c) { sa[sa_len++] = c; }

    size_t sb_len = (b0_len > b1_len ? b0_len : b1_len);
    c = 0;
    for (size_t i = 0; i < sb_len; i++) {
        uint64_t v = (i < b0_len ? b0[i] : 0) + (i < b1_len ? b1[i] : 0) + c;
        sb[i] = v % BASE_10_9;
        c = v / BASE_10_9;
    }
    if (c) { sb[sb_len++] = c; }

    karatsuba_mul_rec(z1, sa, sa_len, sb, sb_len, next_ws);

    size_t z0_len = a0_len + b0_len;
    size_t z2_len = a1_len + b1_len;
    size_t z1_len = sa_len + sb_len;

    int64_t borrow = 0;
    for (size_t i = 0; i < z1_len; i++) {
        int64_t sub = (i < z0_len ? (int64_t)z0[i] : 0) + borrow;
        int64_t cur = (int64_t)z1[i] - sub;
        if (cur < 0) { cur += BASE_10_9; borrow = 1; } else { borrow = 0; }
        z1[i] = (uint64_t)cur;
    }

    borrow = 0;
    for (size_t i = 0; i < z1_len; i++) {
        int64_t sub = (i < z2_len ? (int64_t)z2[i] : 0) + borrow;
        int64_t cur = (int64_t)z1[i] - sub;
        if (cur < 0) { cur += BASE_10_9; borrow = 1; } else { borrow = 0; }
        z1[i] = (uint64_t)cur;
    }

    std::memset(res, 0, (na + nb + 4) * sizeof(uint64_t));
    for (size_t i = 0; i < z0_len; i++) res[i] = z0[i];

    c = 0;
    for (size_t i = 0; i < z1_len || c; i++) {
        uint64_t v = res[i + k] + (i < z1_len ? z1[i] : 0) + c;
        res[i + k] = v % BASE_10_9;
        c = v / BASE_10_9;
    }

    c = 0;
    for (size_t i = 0; i < z2_len || c; i++) {
        uint64_t v = res[i + 2 * k] + (i < z2_len ? z2[i] : 0) + c;
        res[i + 2 * k] = v % BASE_10_9;
        c = v / BASE_10_9;
    }
}

static std::vector<uint64_t> schoolbook_mul(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b) {
    if (a.empty() || b.empty()) return {};
    std::vector<uint64_t> r(a.size() + b.size() + 2, 0);
    for (size_t i = 0; i < a.size(); i++) {
        uint64_t carry = 0;
        for (size_t j = 0; j < b.size(); j++) {
            uint64_t cur = r[i + j] + a[i] * b[j] + carry;
            r[i + j] = cur % BASE_10_9;
            carry = cur / BASE_10_9;
        }
        r[i + b.size()] += carry;
    }
    while (!r.empty() && r.back() == 0) r.pop_back();
    return r;
}

int main() {
    std::cout << "Testing Balanced and Unbalanced Karatsuba...\n";
    std::vector<std::pair<size_t, size_t>> test_pairs = {
        {10, 10}, {32, 32}, {64, 64}, {128, 128}, {256, 256}, {512, 512}, {1024, 1024},
        {911, 33}, {33, 911}, {500, 50}, {1000, 10}, {800, 200}
    };
    for (auto& p : test_pairs) {
        size_t na = p.first, nb = p.second;
        std::vector<uint64_t> a(na), b(nb);
        for (size_t i = 0; i < na; i++) a[i] = (i * 123456789ULL + 987654321ULL) % BASE_10_9;
        for (size_t i = 0; i < nb; i++) b[i] = (i * 987654321ULL + 123456789ULL) % BASE_10_9;

        std::vector<uint64_t> ws((na + nb) * 20 + 2048, 0);
        std::vector<uint64_t> res_k(na + nb + 8, 0);

        auto ref = schoolbook_mul(a, b);

        karatsuba_mul_rec(res_k.data(), a.data(), na, b.data(), nb, ws.data());
        while (!res_k.empty() && res_k.back() == 0) res_k.pop_back();

        bool match = (ref == res_k);
        std::cout << "na=" << na << ", nb=" << nb << ": Match = " << (match ? "YES" : "NO") << "\n";
        if (!match) return 1;
    }
    std::cout << "All balanced & unbalanced tests PASSED!\n";
    return 0;
}
