import time

class Point:
    def __init__(self, x, y):
        self.x = x
        self.y = y
    
    def distance_sq(self, p):
        dx = self.x - p.x
        dy = self.y - p.y
        return dx * dx + dy * dy

start = time.time()
sum = 0
for i in range(100000):
    p1 = Point(i, i)
    p2 = Point(i+1, i+1)
    sum = sum + p1.distance_sq(p2)

end = time.time()
print("Python result:", sum)
print("Python Time:", end - start, " s")
