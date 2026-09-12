public class BenchObject {
    static class Point {
        double x, y;
        Point(double x, double y) { this.x = x; this.y = y; }
        double distanceSq() { return x * x + y * y; }
        double move(double dx, double dy) { x += dx; y += dy; return distanceSq(); }
    }

    static class Line {
        Point p1, p2;
        Line(double x1, double y1, double x2, double y2) { p1 = new Point(x1, y1); p2 = new Point(x2, y2); }
        double length() { double dx = p1.x - p2.x; double dy = p1.y - p2.y; return dx*dx + dy*dy; }
    }

    public static void main(String[] args) {
        // Warmup
        Point wp = new Point(1,2); wp.distanceSq(); new Line(0,0,1,1).length();

        System.out.println("=== Java Object Benchmark ===");

        // 1. Object creation: 100k
        long t1 = System.nanoTime();
        double s1 = 0;
        for (int i = 0; i < 100000; i++) { Point p = new Point(i, i*2.0); s1 += p.x; }
        long t2 = System.nanoTime();
        System.out.printf("create(100k)=%.1f  time=%.6fs%n", s1, (t2-t1)/1e9);

        // 2. Field access: 100k
        Point p = new Point(3, 4);
        long t3 = System.nanoTime();
        double s2 = 0;
        for (int i = 0; i < 100000; i++) s2 += p.x + p.y;
        long t4 = System.nanoTime();
        System.out.printf("field(100k)=%.1f  time=%.6fs%n", s2, (t4-t3)/1e9);

        // 3. Method call: 100k
        long t5 = System.nanoTime();
        double s3 = 0;
        for (int i = 0; i < 100000; i++) s3 += p.distanceSq();
        long t6 = System.nanoTime();
        System.out.printf("method(100k)=%.1f  time=%.6fs%n", s3, (t6-t5)/1e9);

        // 4. Object array: 10k
        long t7 = System.nanoTime();
        Point[] arr = new Point[10000];
        for (int i = 0; i < 10000; i++) arr[i] = new Point(i, i*0.5);
        double s4 = 0;
        for (int i = 0; i < 10000; i++) s4 += arr[i].distanceSq();
        long t8 = System.nanoTime();
        System.out.printf("arrObj(10k)=%.1f  time=%.6fs%n", s4, (t8-t7)/1e9);

        // 5. Nested object: 50k
        long t9 = System.nanoTime();
        double s5 = 0;
        for (int i = 0; i < 50000; i++) { Line line = new Line(0,0,i,i*2.0); s5 += line.length(); }
        long t10 = System.nanoTime();
        System.out.printf("nested(50k)=%.1f  time=%.6fs%n", s5, (t10-t9)/1e9);

        // 6. Object mutation: 100k
        long t11 = System.nanoTime();
        Point pm = new Point(0, 0);
        for (int i = 0; i < 100000; i++) pm.move(1, 1);
        long t12 = System.nanoTime();
        System.out.printf("mutate(100k)=(%.1f,%.1f)  time=%.6fs%n", pm.x, pm.y, (t12-t11)/1e9);
    }
}
