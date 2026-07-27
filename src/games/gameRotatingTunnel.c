#include "cglp.h"

static char *title = "ROTATING TUNNEL";
static char *description = "[Hold] Move outward\n[Release] Move inward";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 1;

// "shape" theme has no equivalent in this port's Options (only "dark"/
// "shapeDark" map to isDarkColor) - dropped per the porting convention.
static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 70, .isDarkColor = false};

#define ROTATINGTUNNEL_CENTER_X 50
#define ROTATINGTUNNEL_CENTER_Y 50
#define ROTATINGTUNNEL_INNER_RADIUS 10
#define ROTATINGTUNNEL_OUTER_RADIUS 50
#define ROTATINGTUNNEL_SHIP_MOVE_SPEED 0.65
#define ROTATINGTUNNEL_OBSTACLE_MOVE_SPEED 0.25
#define ROTATINGTUNNEL_OBSTACLE_SPAWN_INTERVAL 80

typedef struct {
  Vector pos;
  float angle;
  float radius;
} Ship;
static Ship ship;

typedef struct {
  float radius;
  float gapStart;
  float gapSize;
  bool isGold;
  bool isAlive;
} Obstacle;
#define ROTATINGTUNNEL_MAX_OBSTACLE_COUNT 32
static Obstacle obstacles[ROTATINGTUNNEL_MAX_OBSTACLE_COUNT];
static int obstacleIndex;

static float scoreAddingTicks;
static float multiplier;
static float nextObstacleTicks;

static void update() {
  if (!ticks) {
    vectorSet(&ship.pos, ROTATINGTUNNEL_CENTER_X, ROTATINGTUNNEL_CENTER_Y);
    ship.angle = 0;
    ship.radius = 20;
    scoreAddingTicks = 0;
    multiplier = 0;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    nextObstacleTicks = 0;
  }
  float sd = sqrt(difficulty);
  ship.angle += 0.025 * sd;
  if (input.isJustPressed) {
    play(CLICK);
  }
  if (input.isPressed) {
    ship.radius = clamp(ship.radius + ROTATINGTUNNEL_SHIP_MOVE_SPEED,
                         ROTATINGTUNNEL_INNER_RADIUS, ROTATINGTUNNEL_OUTER_RADIUS);
  } else {
    ship.radius = clamp(ship.radius - ROTATINGTUNNEL_SHIP_MOVE_SPEED,
                         ROTATINGTUNNEL_INNER_RADIUS, ROTATINGTUNNEL_OUTER_RADIUS);
  }
  vectorSet(&ship.pos, ROTATINGTUNNEL_CENTER_X + ship.radius * cos(ship.angle),
            ROTATINGTUNNEL_CENTER_Y + ship.radius * sin(ship.angle));

  nextObstacleTicks -= sd;
  if (nextObstacleTicks < 0) {
    bool isGold = rnd(0, 1) < 0.3;
    if (isGold) {
      play(COIN);
    } else {
      play(LASER);
    }
    ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, o);
    o->radius = ROTATINGTUNNEL_OUTER_RADIUS;
    o->gapStart = rnd(0, 2 * M_PI);
    o->gapSize = rnd(M_PI / 4, M_PI / 3);
    o->isGold = isGold;
    o->isAlive = true;
    obstacleIndex = wrap(obstacleIndex + 1, 0, ROTATINGTUNNEL_MAX_OBSTACLE_COUNT);
    nextObstacleTicks += ROTATINGTUNNEL_OBSTACLE_SPAWN_INTERVAL;
  }

  FOR_EACH(obstacles, oi) {
    ASSIGN_ARRAY_ITEM(obstacles, oi, Obstacle, o2);
    SKIP_IS_NOT_ALIVE(o2);
    o2->radius -= ROTATINGTUNNEL_OBSTACLE_MOVE_SPEED * sd;
    if (o2->radius <= ROTATINGTUNNEL_INNER_RADIUS) {
      o2->isAlive = false;
      continue;
    }
  }

  color = BLUE;
  arc(ROTATINGTUNNEL_CENTER_X, ROTATINGTUNNEL_CENTER_Y, ROTATINGTUNNEL_OUTER_RADIUS, 0, M_PI * 2);
  color = LIGHT_BLUE;
  arc(ROTATINGTUNNEL_CENTER_X, ROTATINGTUNNEL_CENTER_Y, ROTATINGTUNNEL_INNER_RADIUS, 0, M_PI * 2);
  color = CYAN;
  float shipSize = 1;
  Vector sp0;
  Vector sp1;
  Vector sp2;
  vectorSet(&sp0, ship.pos.x - shipSize * cos(ship.angle),
            ship.pos.y - shipSize * sin(ship.angle));
  vectorSet(&sp1, ship.pos.x + shipSize * cos(ship.angle),
            ship.pos.y + shipSize * sin(ship.angle));
  vectorSet(&sp2, ship.pos.x - shipSize * sin(ship.angle),
            ship.pos.y + shipSize * cos(ship.angle));
  line(VEC_XY(sp0), VEC_XY(sp1));
  line(VEC_XY(sp1), VEC_XY(sp2));
  line(VEC_XY(sp2), VEC_XY(sp0));

  FOR_EACH(obstacles, oi2) {
    ASSIGN_ARRAY_ITEM(obstacles, oi2, Obstacle, o3);
    SKIP_IS_NOT_ALIVE(o3);
    float startAngle = o3->gapStart;
    float endAngle = o3->gapStart + o3->gapSize;
    if (o3->isGold) {
      color = YELLOW;
    } else {
      color = PURPLE;
    }
    thickness = 3;
    Collision c = arc(ROTATINGTUNNEL_CENTER_X, ROTATINGTUNNEL_CENTER_Y, o3->radius, startAngle, endAngle);
    if (c.isColliding.rect[CYAN]) {
      if (o3->isGold) {
        scoreAddingTicks -= sd;
        if (scoreAddingTicks < 0) {
          play(POWER_UP);
          multiplier++;
          addScore(ceil(multiplier), VEC_XY(ship.pos));
          scoreAddingTicks += 5;
        }
      } else {
        play(EXPLOSION);
        gameOver();
      }
    }
  }
  multiplier *= 0.99;
}

void addGameRotatingTunnel() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
