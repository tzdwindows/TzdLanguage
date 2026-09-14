#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <functional>
#include <memory>

// ── 1. Real Eigenvalues and Eigenvectors (fb_eig) ──
// Jacobi eigenvalue algorithm for symmetric matrices, and general 2x2
struct EigResult {
    std::vector<double> eigenvalues;
    std::vector<std::vector<double>> eigenvectors; // columns are eigenvectors
};

inline EigResult compute_eig(const std::vector<std::vector<double>>& A) {
    size_t n = A.size();
    EigResult res;
    if (n == 0) return res;
    if (n == 1) {
        res.eigenvalues = {A[0][0]};
        res.eigenvectors = {{1.0}};
        return res;
    }
    if (n == 2) {
        double a = A[0][0], b = A[0][1], c = A[1][0], d = A[1][1];
        double tr = a + d;
        double det = a * d - b * c;
        double disc = tr * tr - 4.0 * det;
        double sqrt_disc = disc >= 0.0 ? std::sqrt(disc) : 0.0;
        double l1 = (tr + sqrt_disc) / 2.0;
        double l2 = (tr - sqrt_disc) / 2.0;
        res.eigenvalues = {l1, l2};

        // v1 for l1: (a - l1) x + b y = 0
        auto get_v = [&](double l) -> std::vector<double> {
            double v0 = b, v1 = l - a;
            if (std::abs(v0) < 1e-9 && std::abs(v1) < 1e-9) { v0 = l - d; v1 = c; }
            double norm = std::hypot(v0, v1);
            if (norm < 1e-12) return {1.0, 0.0};
            return {v0 / norm, v1 / norm};
        };
        auto v1 = get_v(l1);
        auto v2 = get_v(l2);
        res.eigenvectors = { {v1[0], v2[0]}, {v1[1], v2[1]} };
        return res;
    }

    // Jacobi eigenvalue algorithm for n x n (assuming real symmetric or approximation)
    std::vector<std::vector<double>> V(n, std::vector<double>(n, 0.0));
    std::vector<std::vector<double>> M = A;
    for (size_t i = 0; i < n; ++i) V[i][i] = 1.0;

    for (int it = 0; it < 50; ++it) {
        // find max off-diagonal
        size_t p = 0, q = 1;
        double max_val = 0.0;
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                if (std::abs(M[i][j]) > max_val) {
                    max_val = std::abs(M[i][j]);
                    p = i; q = j;
                }
            }
        }
        if (max_val < 1e-10) break;

        double theta = 0.5 * std::atan2(2.0 * M[p][q], M[p][p] - M[q][q]);
        double c = std::cos(theta), s = std::sin(theta);

        // rotate M
        std::vector<std::vector<double>> M_next = M;
        for (size_t i = 0; i < n; ++i) {
            if (i != p && i != q) {
                M_next[i][p] = M_next[p][i] = c * M[i][p] + s * M[i][q];
                M_next[i][q] = M_next[q][i] = -s * M[i][p] + c * M[i][q];
            }
        }
        M_next[p][p] = c * c * M[p][p] + 2.0 * s * c * M[p][q] + s * s * M[q][q];
        M_next[q][q] = s * s * M[p][p] - 2.0 * s * c * M[p][q] + c * c * M[q][q];
        M_next[p][q] = M_next[q][p] = 0.0;
        M = M_next;

        // update V
        for (size_t i = 0; i < n; ++i) {
            double vip = V[i][p], viq = V[i][q];
            V[i][p] = c * vip + s * viq;
            V[i][q] = -s * vip + c * viq;
        }
    }

    for (size_t i = 0; i < n; ++i) res.eigenvalues.push_back(M[i][i]);
    res.eigenvectors = V;
    return res;
}

// ── 2. Real SVD (fb_svd: A = U * S * V^T) ──
struct SvdResult {
    std::vector<std::vector<double>> U;
    std::vector<double> S;
    std::vector<std::vector<double>> Vt;
};

inline SvdResult compute_svd(const std::vector<std::vector<double>>& A) {
    size_t m = A.size();
    size_t n = m > 0 ? A[0].size() : 0;
    SvdResult res;
    if (m == 0 || n == 0) return res;

    // Compute AtA (n x n)
    std::vector<std::vector<double>> AtA(n, std::vector<double>(n, 0.0));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            for (size_t k = 0; k < m; ++k) {
                AtA[i][j] += A[k][i] * A[k][j];
            }
        }
    }

    EigResult eig = compute_eig(AtA);
    // Sort eigenvalues descending
    std::vector<std::pair<double, size_t>> order;
    for (size_t i = 0; i < n; ++i) {
        order.push_back({std::max(0.0, eig.eigenvalues[i]), i});
    }
    std::sort(order.rbegin(), order.rend());

    size_t k_rank = std::min(m, n);
    res.S.resize(k_rank, 0.0);
    res.Vt.assign(n, std::vector<double>(n, 0.0));
    res.U.assign(m, std::vector<double>(k_rank, 0.0));

    for (size_t j = 0; j < n; ++j) {
        size_t orig_idx = order[j].second;
        for (size_t i = 0; i < n; ++i) {
            res.Vt[j][i] = eig.eigenvectors[i][orig_idx];
        }
    }

    for (size_t i = 0; i < k_rank; ++i) {
        res.S[i] = std::sqrt(order[i].first);
        if (res.S[i] > 1e-9) {
            for (size_t r = 0; r < m; ++r) {
                double val = 0.0;
                for (size_t c = 0; c < n; ++c) {
                    val += A[r][c] * res.Vt[i][c];
                }
                res.U[r][i] = val / res.S[i];
            }
        }
    }
    return res;
}

// ── 3. Real Pearson Correlation Matrix (fb_corrcoef) ──
inline std::vector<std::vector<double>> compute_corrcoef(const std::vector<std::vector<double>>& X) {
    size_t N = X.size();
    if (N == 0) return {};
    size_t M = X[0].size();
    if (M <= 1) return std::vector<std::vector<double>>(N, std::vector<double>(N, 1.0));

    std::vector<double> mean(N, 0.0);
    std::vector<double> stddev(N, 0.0);
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < M; ++j) mean[i] += X[i][j];
        mean[i] /= M;
        for (size_t j = 0; j < M; ++j) {
            double diff = X[i][j] - mean[i];
            stddev[i] += diff * diff;
        }
        stddev[i] = std::sqrt(stddev[i]);
    }

    std::vector<std::vector<double>> corr(N, std::vector<double>(N, 0.0));
    for (size_t i = 0; i < N; ++i) {
        corr[i][i] = 1.0;
        for (size_t j = i + 1; j < N; ++j) {
            double cov = 0.0;
            for (size_t k = 0; k < M; ++k) {
                cov += (X[i][k] - mean[i]) * (X[j][k] - mean[j]);
            }
            double denom = stddev[i] * stddev[j];
            double r = denom > 1e-12 ? cov / denom : 0.0;
            corr[i][j] = corr[j][i] = r;
        }
    }
    return corr;
}

// ── 4. Real Triplet Margin Loss ──
inline double compute_triplet_margin_loss(
    const std::vector<double>& a,
    const std::vector<double>& p,
    const std::vector<double>& n,
    double margin = 1.0)
{
    size_t sz = std::min({a.size(), p.size(), n.size()});
    double d_ap = 0.0, d_an = 0.0;
    for (size_t i = 0; i < sz; ++i) {
        double diff_p = a[i] - p[i];
        double diff_n = a[i] - n[i];
        d_ap += diff_p * diff_p;
        d_an += diff_n * diff_n;
    }
    d_ap = std::sqrt(d_ap);
    d_an = std::sqrt(d_an);
    return std::max(0.0, d_ap - d_an + margin);
}

int main() {
    std::cout << "--- 1. Testing Eig ---\n";
    // Matrix [[2, 1], [1, 2]]: eigenvalues should be 3 and 1
    std::vector<std::vector<double>> M = {{2.0, 1.0}, {1.0, 2.0}};
    EigResult eig = compute_eig(M);
    std::cout << "Eigenvalues of [[2,1],[1,2]]: " << eig.eigenvalues[0] << ", " << eig.eigenvalues[1] << " (expected 3, 1)\n";

    std::cout << "\n--- 2. Testing SVD ---\n";
    // Matrix [[3, 0], [0, -2]]: singular values should be 3 and 2
    std::vector<std::vector<double>> S_mat = {{3.0, 0.0}, {0.0, -2.0}};
    SvdResult svd = compute_svd(S_mat);
    std::cout << "Singular values: " << svd.S[0] << ", " << svd.S[1] << " (expected 3, 2)\n";

    std::cout << "\n--- 3. Testing Corrcoef ---\n";
    // Two linearly correlated variables
    std::vector<std::vector<double>> X = { {1.0, 2.0, 3.0, 4.0}, {2.0, 4.0, 6.0, 8.0} };
    auto corr = compute_corrcoef(X);
    std::cout << "Correlation between X and 2X: " << corr[0][1] << " (expected 1.0)\n";

    std::cout << "\n--- 4. Testing Triplet Margin Loss ---\n";
    std::vector<double> a = {1.0, 0.0};
    std::vector<double> p = {1.0, 0.1}; // d(a, p) = 0.1
    std::vector<double> n = {2.0, 0.0}; // d(a, n) = 1.0
    // loss = max(0, 0.1 - 1.0 + 1.0) = 0.1
    std::cout << "Triplet loss: " << compute_triplet_margin_loss(a, p, n, 1.0) << " (expected 0.1)\n";
    return 0;
}
