#include "cglp.h"
#include "string.h"

static char *title = "CLEAN ROBO";
static char *description = "[Hold] Speed up";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 0;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 9, .isDarkColor = false};

#define CLEANROBO_ROBOT_RADIUS 10

typedef struct {
  Vector poss[3];
  float dist;
  float angle;
} Wall;
static Wall walls[2];

typedef struct {
  Vector pos;
  float angle;
  float vx;
  float speed;
} Robot;
static Robot robot;

static Vector garbage;
static bool hasGarbage;
static Vector leftGarbage;
static bool hasLeftGarbage;
static float multiplier;

static void setWall(int index, bool isChangingAngle) {
  Wall *w = &walls[index];
  float a;
  if (isChangingAngle) {
    a = rnd(0, M_PI / 7) * RNDPM();
  } else {
    a = w->angle;
  }
  float rr = CLEANROBO_ROBOT_RADIUS * 1.7;
  TIMES(3, i) {
    Vector *p = &w->poss[i];
    vectorSet(p, rr, 0);
    if (index == 0) {
      rotate(p, a + M_PI - M_PI * 2 / 3 * (i - 1));
      vectorAdd(p, 100 - w->dist, 50);
    } else {
      rotate(p, a - M_PI * 2 / 3 * (-i + 1));
      vectorAdd(p, 100 + w->dist, 50);
    }
  }
  w->angle = a;
  Vector *p1 = &w->poss[1];
  garbage.x = p1->x - 7 * (index * 2 - 1);
  garbage.y = (p1->y - 50) * 0.5 + 50;
  hasGarbage = true;
}

static void drawRoom() {
  thickness = 3;
  TIMES(2, wi) {
    Wall *w = &walls[wi];
    color = PURPLE;
    Vector *ps = w->poss;
    if (ps[1].x < 0 || ps[1].x > 199) {
      color = RED;
      play(RANDOM);
      gameOver();
    }
    line(ps[0].x, 0, ps[0].x, ps[0].y);
    line(ps[0].x, ps[0].y, ps[1].x, ps[1].y);
    line(ps[1].x, ps[1].y, ps[2].x, ps[2].y);
    line(ps[2].x, ps[2].y, ps[2].x, 99);
  }
}

static void update() {
  if (!ticks) {
    vectorSet(&robot.pos, 90, 50);
    robot.angle = 0;
    robot.vx = 1;
    robot.speed = 1;
    // Vircon32 port note: JS recreates `walls` from scratch each game
    // (`walls = times(2, () => ({..., dist: 50, ...}))`), but this port's
    // walls is a persistent global that setWall() only ever reads .dist
    // from, never resets - without this, a replay would start from
    // whatever .dist gameplay had nudged it to by the end of the previous
    // game (clamped to [30,100], not always 50), which could place a wall
    // off-screen and trip the x<0||x>199 gameOver() check in drawRoom()
    // on the very first frame of the new game.
    walls[0].dist = 50;
    walls[1].dist = 50;
    setWall(0, true);
    setWall(1, true);
    multiplier = 1;
    hasLeftGarbage = false;
  }
  drawRoom();
  float gs = 4;
  if (hasLeftGarbage) {
    color = RED;
    box(VEC_XY(leftGarbage), gs, gs);
  }
  if (hasGarbage) {
    color = YELLOW;
    box(VEC_XY(garbage), gs, gs);
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isPressed) {
    robot.speed += 0.3;
  } else {
    robot.angle += 0.05 * difficulty;
    robot.speed += (1 - robot.speed) * 0.2;
    if (multiplier > 1) {
      multiplier -= 0.1;
    }
  }
  robot.pos.x += robot.vx * robot.speed * difficulty * 0.2;
  bool icw = false;
  bool icg = false;
  color = LIGHT_BLACK;
  Vector rps[3];
  TIMES(3, i) {
    vectorSet(&rps[i], CLEANROBO_ROBOT_RADIUS, 0);
    rotate(&rps[i], robot.angle + M_PI * 2 / 3 * i);
    vectorAdd(&rps[i], robot.pos.x, robot.pos.y);
    thickness = 3;
    bar(VEC_XY(rps[i]), 5, ticks * 0.7 + i);
  }
  color = BLACK;
  TIMES(3, i) {
    int nextI = (i + 1) % 3;
    thickness = 3;
    Collision c = line(VEC_XY(rps[i]), VEC_XY(rps[nextI]));
    if (c.isColliding.rect[PURPLE]) {
      icw = true;
    }
    if (c.isColliding.rect[YELLOW]) {
      icg = true;
    }
  }
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar((int)ceil(multiplier)));
  text(multText, 3, 9);
  if (icg) {
    play(POWER_UP);
    addScore(ceil(multiplier), VEC_XY(garbage));
    multiplier += 10;
    hasGarbage = false;
  }
  if ((robot.pos.x - 100) * robot.vx > 0 && icw) {
    robot.vx *= -1;
    hasLeftGarbage = false;
    if (hasGarbage) {
      play(EXPLOSION);
      leftGarbage = garbage;
      hasLeftGarbage = true;
      robot.speed = 1;
      color = RED;
      particle(garbage.x, garbage.y, 9, 1, 0, M_PI * 2);
    } else {
      play(CLICK);
    }
    int wli = (int)((-robot.vx + 1) / 2);
    float distDelta;
    if (!hasGarbage) {
      distDelta = -10;
    } else {
      distDelta = 20;
    }
    walls[wli].dist = clamp(walls[wli].dist + distDelta, 30, 100);
    setWall(wli, true);
    int wai = (int)((robot.vx + 1) / 2);
    setWall(wai, false);
  }
}

void addGameCleanRobo() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
