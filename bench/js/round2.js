// ===== Round-2 kernels (optimizer-resistant) =====

function stringBuild(n) {
    let s = "";
    for (let i = 0; i < n; i++) {
        s = s + String(i) + "-" + String(i % 7);
    }
    return s.length;
}

function monteCarlo(n) {
    let x = 1;
    let count = 0;
    for (let i = 0; i < n; i++) {
        x = (x * 16807) % 2147483647;
        const y = (x * 16807) % 2147483647;
        const fx = x / 2147483647.0;
        const fy = y / 2147483647.0;
        if (fx * fx + fy * fy <= 1.0) count++;
    }
    return 4.0 * count / n;
}

function mapOps(n) {
    const m = new Map();
    for (let i = 0; i < n; i++) m.set(String(i), i);
    let sum = 0;
    for (let i = 0; i < n; i++) sum = sum + m.get(String(i));
    return sum;
}

class Pt {
    constructor(x, y) { this.x = x; this.y = y; }
    d2(p) {
        const dx = this.x - p.x;
        const dy = this.y - p.y;
        return dx * dx + dy * dy;
    }
}

function oopDispatch(n) {
    let x = 1;
    let sum = 0.0;
    for (let i = 0; i < n; i++) {
        x = (x * 16807) % 2147483647;
        const a = x % 1000;
        x = (x * 16807) % 2147483647;
        const b = x % 1000;
        x = (x * 16807) % 2147483647;
        const c = x % 1000;
        x = (x * 16807) % 2147483647;
        const d = x % 1000;
        const p1 = new Pt(a * 1.0, b * 1.0);
        const p2 = new Pt(c * 1.0, d * 1.0);
        sum = sum + p1.d2(p2);
    }
    return sum;
}

const N = 300000;
const G = new Array(N).fill(0);

function qsortG(lo, hi) {
    if (lo >= hi) return;
    const p = G[hi];
    let i = lo - 1;
    let j = lo;
    while (j < hi) {
        if (G[j] <= p) {
            i++;
            const t = G[i]; G[i] = G[j]; G[j] = t;
        }
        j++;
    }
    i++;
    const t2 = G[i]; G[i] = G[hi]; G[hi] = t2;
    qsortG(lo, i - 1);
    qsortG(i + 1, hi);
}

function quickSort() {
    let x = 1;
    let fillSum = 0;
    for (let i = 0; i < N; i++) {
        x = (x * 16807) % 2147483647;
        const v = x % 1000000007;
        G[i] = v;
        fillSum += v;
    }
    qsortG(0, N - 1);
    console.log(`qsort min=${G[0]} max=${G[N - 1]} fill=${fillSum}`);
}

function bench(name, fn, ...args) {
    const t0 = performance.now();
    const r = fn(...args);
    const t1 = performance.now();
    console.log(`${name} | ${(t1 - t0).toFixed(1)} ms`);
    return r;
}

const r1 = bench("stringBuild", stringBuild, 20000);
console.log(`stringBuild len=${r1}`);
const r2 = bench("monteCarlo", monteCarlo, 1000000);
console.log(`monteCarlo pi=${r2}`);
const r3 = bench("mapOps", mapOps, 200000);
console.log(`mapOps sum=${r3}`);
const r4 = bench("oop", oopDispatch, 200000);
console.log(`oop sum=${r4}`);
bench("qsort", quickSort);
