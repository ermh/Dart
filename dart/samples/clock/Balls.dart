class Balls {
  // Reference to Ball.RADIUS fails in JS at the moment... b/4460048
  static final double RADIUS2 = 14 * 14; // Ball.RADIUS * Ball.RADIUS;

  // TODO: "static const Array<String> PNGS" doesn't parse
  static final Array PNGS = ["images/ball-d9d9d9.png",
      "images/ball-009a49.png", "images/ball-13acfa.png",
      "images/ball-265897.png", "images/ball-b6b4b5.png",
      "images/ball-c0000b.png", "images/ball-c9c9c9.png"];

  HTMLDivElement root;
  int lastTime;
  var balls;

  Balls() :
      lastTime = Util.currentTimeMillis(),
      balls = new Array<Ball>() {
    root = CountDownClock.window.document.createElement('div');
    CountDownClock.window.document.body.appendChild(root);
    root.style.setProperty("zIndex", '100');
    Util.abs(root);
    Util.posSize(root, 0, 0, 0, 0);
  }

  void tick() {
    int now = Util.currentTimeMillis();
    double delta = (now - lastTime) / 1000.0;
    if (delta > 0.1) {
      delta = 0.1;
    }
    lastTime = now;

    for (int i = 0; i < balls.length; ++i) {
      Ball ball = balls[i];
      if (!ball.tick(delta)) {
        balls.removeAt(i);
        --i;
      }
    }

    collideBalls(delta);
  }

  void collideBalls(double delta) {
    // TODO: Make this nasty O(n^2) stuff better.
    for (int i = 0; i < balls.length; ++i) {
      for (int j = i + 1; j < balls.length; ++j) {
        Ball b0 = balls[i];
        Ball b1 = balls[j];

        // See if the two balls are intersecting.
        double dx = (b0.x - b1.x).abs();
        double dy = (b0.y - b1.y).abs();
        double d2 = dx * dx + dy * dy;
        if (d2 < RADIUS2) {
          // Make sure they're actually on a collision path
          // (not intersecting while moving apart).
          // This keeps balls that end up intersecting from getting stuck
          // without all the complexity of keeping them strictly separated.
          if (newDistanceSquared(delta, b0, b1) > d2) {
            continue;
          }

          // They've collided. Normalize the collision vector.
          double d = Math.sqrt(d2);
          if (d == 0) {
            // TODO: move balls apart.
            return;
          }
          dx /= d;
          dy /= d;

          // Calculate the impact velocity and speed along the collision vector.
          double impactx = b0.vx - b1.vx;
          double impacty = b0.vy - b1.vy;
          double impactSpeed = impactx * dx + impacty * dy;

          // Bump.
          b0.vx -= dx * impactSpeed;
          b0.vy -= dy * impactSpeed;
          b1.vx += dx * impactSpeed;
          b1.vy += dy * impactSpeed;
        }
      }
    }
  }

  double newDistanceSquared(double delta, Ball b0, Ball b1) {
    double nb0x = b0.x + b0.vx * delta;
    double nb0y = b0.y + b0.vy * delta;
    double nb1x = b1.x + b1.vx * delta;
    double nb1y = b1.y + b1.vy * delta;
    double ndx = (nb0x - nb1x).abs();
    double ndy = (nb0y - nb1y).abs();
    double nd2 = ndx * ndx + ndy * ndy;
    return nd2;
  }

  void add(int x, int y, int color) {
    balls.add(new Ball(root, x, y, color));
  }
}
