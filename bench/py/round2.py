import time

# ===== Round-2 kernels (optimizer-resistant) =====

def string_build(n):
    s = ""
    for i in range(n):
        s = s + str(i) + "-" + str(i % 7)
    return len(s)

def monte_carlo(n):
    x = 1
    count = 0
    for _ in range(n):
        x = (x * 16807) % 2147483647
        y = (x * 16807) % 2147483647
        fx = x / 2147483647.0
        fy = y / 2147483647.0
        if fx * fx + fy * fy <= 1.0:
            count += 1
    return 4.0 * count / n

def map_ops(n):
    m = {}
    for i in range(n):
        m[str(i)] = i
    total = 0
    for i in range(n):
        total = total + m[str(i)]
    return total

class Pt:
    __slots__ = ("x", "y")
    def __init__(self, x, y):
        self.x = x
        self.y = y
    def d2(self, p):
        dx = self.x - p.x
        dy = self.y - p.y
        return dx * dx + dy * dy

def oop_dispatch(n):
    x = 1
    total = 0.0
    for _ in range(n):
        x = (x * 16807) % 2147483647
        a = x % 1000
        x = (x * 16807) % 2147483647
        b = x % 1000
        x = (x * 16807) % 2147483647
        c = x % 1000
        x = (x * 16807) % 2147483647
        d = x % 1000
        p1 = Pt(a * 1.0, b * 1.0)
        p2 = Pt(c * 1.0, d * 1.0)
        total = total + p1.d2(p2)
    return total

N = 300000
G = [0] * N

def qsortG(lo, hi):
    if lo >= hi:
        return
    p = G[hi]
    i = lo - 1
    j = lo
    while j < hi:
        if G[j] <= p:
            i += 1
            G[i], G[j] = G[j], G[i]
        j += 1
    i += 1
    G[i], G[hi] = G[hi], G[i]
    qsortG(lo, i - 1)
    qsortG(i + 1, hi)

def quick_sort():
    global G
    x = 1
    fill_sum = 0
    for i in range(N):
        x = (x * 16807) % 2147483647
        v = x % 1000000007
        G[i] = v
        fill_sum += v
    qsortG(0, N - 1)
    print(f"qsort min={G[0]} max={G[N-1]} fill={fill_sum}")

def bench(name, fn, *args):
    t0 = time.perf_counter()
    r = fn(*args)
    t1 = time.perf_counter()
    print(f"{name} | {(t1 - t0) * 1000:.1f} ms")
    return r

r1 = bench("stringBuild", string_build, 20000)
print(f"stringBuild len={r1}")
r2 = bench("monteCarlo", monte_carlo, 1000000)
print(f"monteCarlo pi={r2}")
r3 = bench("mapOps", map_ops, 200000)
print(f"mapOps sum={r3}")
r4 = bench("oop", oop_dispatch, 200000)
print(f"oop sum={r4}")
bench("qsort", quick_sort)
