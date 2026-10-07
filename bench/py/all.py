import time

def fib(n):
    if n <= 2:
        return 1
    return fib(n - 1) + fib(n - 2)

def count_primes(limit):
    sieve = [True] * limit
    sieve[0] = sieve[1] = False
    p = 2
    while p * p < limit:
        if sieve[p]:
            m = p * p
            while m < limit:
                sieve[m] = False
                m += p
        p += 1
    count = 0
    for k in range(limit):
        if sieve[k]:
            count += 1
    return count

def mandelbrot(width, height, max_iter):
    total = 0
    for py in range(height):
        for px in range(width):
            x0 = px / width * 3.5 - 2.5
            y0 = py / height * 2.0 - 1.0
            x = 0.0
            y = 0.0
            it = 0
            while it < max_iter and x * x + y * y <= 4.0:
                xt = x * x - y * y + x0
                y = 2.0 * x * y + y0
                x = xt
                it += 1
            total += it
    return total

def loop_sum(n):
    s = 0
    for i in range(n):
        s += i
    return s

def bench(name, fn, *args):
    t0 = time.perf_counter()
    r = fn(*args)
    t1 = time.perf_counter()
    print(f"{name}={r} | {(t1 - t0) * 1000:.1f} ms")

bench("fib(34)", fib, 34)
bench("primes<1e6", count_primes, 1000000)
bench("mandelbrot", mandelbrot, 400, 400, 100)
bench("loopSum_1e8", loop_sum, 100000000)
