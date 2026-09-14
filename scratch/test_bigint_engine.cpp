#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <iomanip>

struct NativeBigInt {
    bool negative = false;
    std::vector<int> digits; // least significant digit at index 0

    NativeBigInt() : negative(false) {}
    NativeBigInt(int64_t v) {
        if (v < 0) { negative = true; v = -v; }
        if (v == 0) digits.push_back(0);
        while (v > 0) {
            digits.push_back(v % 10);
            v /= 10;
        }
    }
    NativeBigInt(const std::string& str) {
        std::string s = str;
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        s.erase(s.find_last_not_of(" \t\r\n") + 1);
        if (s.empty()) { digits.push_back(0); return; }
        size_t start = 0;
        if (s[0] == '-') { negative = true; start = 1; }
        else if (s[0] == '+') { start = 1; }
        for (int i = (int)s.size() - 1; i >= (int)start; --i) {
            if (std::isdigit((unsigned char)s[i])) {
                digits.push_back(s[i] - '0');
            }
        }
        trim();
    }

    void trim() {
        while (digits.size() > 1 && digits.back() == 0) digits.pop_back();
        if (digits.empty()) digits.push_back(0);
        if (digits.size() == 1 && digits[0] == 0) negative = false;
    }

    bool is_zero() const {
        return digits.empty() || (digits.size() == 1 && digits[0] == 0);
    }

    std::string to_string() const {
        if (digits.empty()) return "0";
        std::string s;
        if (negative) s += '-';
        for (int i = (int)digits.size() - 1; i >= 0; --i) {
            s += (char)('0' + digits[i]);
        }
        return s;
    }

    int cmp_abs(const NativeBigInt& o) const {
        if (digits.size() != o.digits.size()) return digits.size() < o.digits.size() ? -1 : 1;
        for (int i = (int)digits.size() - 1; i >= 0; --i) {
            if (digits[i] != o.digits[i]) return digits[i] < o.digits[i] ? -1 : 1;
        }
        return 0;
    }

    NativeBigInt add_abs(const NativeBigInt& b) const {
        NativeBigInt res;
        int carry = 0, n = std::max(digits.size(), b.digits.size());
        for (int i = 0; i < n || carry; ++i) {
            int sum = carry;
            if (i < (int)digits.size()) sum += digits[i];
            if (i < (int)b.digits.size()) sum += b.digits[i];
            res.digits.push_back(sum % 10);
            carry = sum / 10;
        }
        res.trim();
        return res;
    }

    NativeBigInt sub_abs(const NativeBigInt& b) const {
        NativeBigInt res;
        int borrow = 0;
        for (size_t i = 0; i < digits.size(); ++i) {
            int diff = digits[i] - borrow - (i < b.digits.size() ? b.digits[i] : 0);
            if (diff < 0) { diff += 10; borrow = 1; }
            else borrow = 0;
            res.digits.push_back(diff);
        }
        res.trim();
        return res;
    }

    NativeBigInt operator+(const NativeBigInt& b) const {
        if (negative == b.negative) {
            NativeBigInt res = add_abs(b);
            res.negative = negative;
            return res;
        }
        if (cmp_abs(b) >= 0) {
            NativeBigInt res = sub_abs(b);
            res.negative = negative;
            return res;
        } else {
            NativeBigInt res = b.sub_abs(*this);
            res.negative = b.negative;
            return res;
        }
    }

    NativeBigInt operator-(const NativeBigInt& b) const {
        NativeBigInt neg_b = b;
        neg_b.negative = !b.negative;
        return *this + neg_b;
    }

    NativeBigInt mul_small(int64_t v) const {
        if (v == 0 || is_zero()) return NativeBigInt(0);
        NativeBigInt res;
        res.negative = (negative ^ (v < 0));
        if (v < 0) v = -v;
        int64_t carry = 0;
        for (size_t i = 0; i < digits.size() || carry; ++i) {
            int64_t prod = carry + (i < digits.size() ? digits[i] * v : 0);
            res.digits.push_back(prod % 10);
            carry = prod / 10;
        }
        res.trim();
        return res;
    }

    NativeBigInt operator*(const NativeBigInt& b) const {
        if (is_zero() || b.is_zero()) return NativeBigInt(0);
        NativeBigInt res;
        res.negative = negative ^ b.negative;
        res.digits.assign(digits.size() + b.digits.size(), 0);
        for (size_t i = 0; i < digits.size(); ++i) {
            int carry = 0;
            for (size_t j = 0; j < b.digits.size() || carry; ++j) {
                int64_t cur = res.digits[i + j] + carry + (int64_t)digits[i] * (j < b.digits.size() ? b.digits[j] : 0);
                res.digits[i + j] = cur % 10;
                carry = (int)(cur / 10);
            }
        }
        res.trim();
        return res;
    }

    std::pair<NativeBigInt, NativeBigInt> divmod(const NativeBigInt& b) const {
        if (b.is_zero()) return {NativeBigInt(0), NativeBigInt(0)};
        NativeBigInt q, rem;
        q.digits.assign(digits.size(), 0);
        for (int i = (int)digits.size() - 1; i >= 0; --i) {
            rem.digits.insert(rem.digits.begin(), digits[i]);
            rem.trim();
            int l = 0, r = 9, best = 0;
            while (l <= r) {
                int mid = (l + r) / 2;
                if (b.mul_small(mid).cmp_abs(rem) <= 0) {
                    best = mid;
                    l = mid + 1;
                } else {
                    r = mid - 1;
                }
            }
            q.digits[i] = best;
            rem = rem.sub_abs(b.mul_small(best));
        }
        q.negative = negative ^ b.negative;
        rem.negative = negative;
        q.trim();
        rem.trim();
        return {q, rem};
    }

    NativeBigInt operator/(const NativeBigInt& b) const { return divmod(b).first; }
    NativeBigInt operator%(const NativeBigInt& b) const { return divmod(b).second; }

    static NativeBigInt gcd(NativeBigInt a, NativeBigInt b) {
        a.negative = false;
        b.negative = false;
        while (!b.is_zero()) {
            NativeBigInt r = a % b;
            a = b;
            b = r;
        }
        return a;
    }

    static NativeBigInt factorial(int64_t n) {
        if (n <= 1) return NativeBigInt(1);
        NativeBigInt res(1);
        for (int64_t i = 2; i <= n; ++i) {
            res = res.mul_small(i);
        }
        return res;
    }
};

int main() {
    NativeBigInt f100 = NativeBigInt::factorial(100);
    std::cout << "100! = " << f100.to_string() << "\n";
    std::cout << "100! length = " << f100.to_string().size() << " (expected 158 digits)\n";

    NativeBigInt a("123456789012345678901234567890");
    NativeBigInt b("987654321098765432109876543210");
    std::cout << "a + b = " << (a + b).to_string() << "\n";
    std::cout << "gcd(a, b) = " << NativeBigInt::gcd(a, b).to_string() << "\n";
    return 0;
}
