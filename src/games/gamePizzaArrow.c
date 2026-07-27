#include "cglp.h"
#include "string.h"

static char *title = "PIZZA ARROW";
static char *description = "[Hold]\n Pull\n[Release]\n Release";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    " r   r",
    "rrllll",
    " r   r",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 6, .isDarkColor = false};

typedef struct {
  float from;
  float to;
  float angle;
  float angleVel;
  float y;
} Pizza;
static Pizza pizza;

typedef struct {
  float from;
  float to;
  float angle;
  Vector pos;
} Part;
static Part part;
static bool partActive;

typedef struct {
  float x;
  float vx;
} Arrow;
static Arrow arrow;
static bool arrowActive;

static int arrowCount;
static int nextArrowCount;
static float nextPizzaTicks;
static float gameSpeed;
static int multiplier;

static void update() {
  if (!ticks) {
    arrowActive = false;
    arrowCount = 1;
    nextArrowCount = 1;
    nextPizzaTicks = 16;
    gameSpeed = 1;
    multiplier = 1;
    partActive = false;
  }
  nextPizzaTicks--;
  if (nextPizzaTicks >= 0) {
    if (nextPizzaTicks <= 15) {
      if (nextPizzaTicks == 15) {
        pizza.from = M_PI / 4;
        pizza.to = (M_PI / 4) * 7;
        pizza.angle = rnd(0, M_PI / 2);
        pizza.angleVel = 0.2;
        pizza.y = 0;
      }
      pizza.y = 50 - nextPizzaTicks * 5;
      multiplier = 1;
    } else {
      pizza.y = (30 - nextPizzaTicks) * 5 + 50;
    }
  }
  color = YELLOW;
  Vector c;
  vectorSet(&c, 30, pizza.y);
  float f = pizza.from + pizza.angle;
  float t = pizza.to + pizza.angle;
  thickness = 4;
  arc(VEC_XY(c), 20, f, t);
  Vector lf, lt;
  vectorSet(&lf, c.x, c.y);
  addWithAngle(&lf, f, 5);
  vectorSet(&lt, c.x, c.y);
  addWithAngle(&lt, f, 20);
  thickness = 4;
  line(VEC_XY(lf), VEC_XY(lt));
  vectorSet(&lf, c.x, c.y);
  addWithAngle(&lf, t, 5);
  vectorSet(&lt, c.x, c.y);
  addWithAngle(&lt, t, 20);
  thickness = 4;
  line(VEC_XY(lf), VEC_XY(lt));
  color = RED;
  thickness = 4;
  arc(30, pizza.y, 5, f, t - M_PI * 2);
  pizza.angle += pizza.angleVel * gameSpeed;
  if (nextPizzaTicks < 0 && !arrowActive && input.isPressed) {
    play(SELECT);
    arrow.x = 80;
    arrow.vx = 1;
    arrowActive = true;
    arrowCount--;
  }
  if (nextPizzaTicks < 0) {
    color = BLACK;
    TIMES(arrowCount, i) { character("a", 95, 40 - i * 3); }
    char multText[16];
    strcpy(multText, "x");
    strcat(multText, intToChar(multiplier));
    text(multText, 3, 10);
  }
  if (arrowActive) {
    if (input.isPressed) {
      gameSpeed += (0.05 - gameSpeed) * 0.1;
    }
    if (input.isJustReleased || arrow.x > 90) {
      play(LASER);
      arrow.vx = -5;
    }
    if (arrow.vx < 0) {
      gameSpeed += (1 - gameSpeed) * 0.2;
    }
    if (arrow.x > 70) {
      color = LIGHT_BLACK;
      thickness = 2;
      line(80, 30, arrow.x + 4, 51);
      thickness = 2;
      line(80, 70, arrow.x + 4, 51);
    }
    arrow.x += arrow.vx * gameSpeed;
    color = BLACK;
    Collision ac = character("a", arrow.x, 50);
    if (ac.isColliding.rect[YELLOW]) {
      play(HIT);
      float a = wrap(-pizza.angle, 0, M_PI * 2);
      if (a > pizza.from && a < pizza.to) {
        float sa;
        if (a - pizza.from > pizza.to - a) {
          sa = pizza.to - a;
          part.from = a;
          part.to = pizza.to;
          part.angle = pizza.angle;
          vectorSet(&part.pos, 30, 50);
          pizza.to = a;
        } else {
          sa = a - pizza.from;
          part.from = pizza.from;
          part.to = a;
          part.angle = pizza.angle;
          vectorSet(&part.pos, 30, 50);
          pizza.from = a;
        }
        partActive = true;
        play(COIN);
        addScore(ceil(sa * 100 * multiplier), 40, 50);
        multiplier++;
      }
      arrowActive = false;
      if (arrowCount == 0) {
        play(POWER_UP);
        nextArrowCount++;
        arrowCount = nextArrowCount;
        nextPizzaTicks = 30;
      }
    } else if (ac.isColliding.rect[RED]) {
      play(EXPLOSION);
      arrowActive = false;
      gameOver();
    }
  }
  if (partActive) {
    vectorAdd(&part.pos, -5, 3);
    color = LIGHT_YELLOW;
    Vector pc;
    vectorSet(&pc, part.pos.x, part.pos.y);
    float pf = part.from + part.angle;
    float pt = part.to + part.angle;
    thickness = 4;
    arc(VEC_XY(pc), 20, pf, pt);
    Vector plf, plt;
    vectorSet(&plf, pc.x, pc.y);
    addWithAngle(&plf, pf, 5);
    vectorSet(&plt, pc.x, pc.y);
    addWithAngle(&plt, pf, 20);
    thickness = 4;
    line(VEC_XY(plf), VEC_XY(plt));
    vectorSet(&plf, pc.x, pc.y);
    addWithAngle(&plf, pt, 5);
    vectorSet(&plt, pc.x, pc.y);
    addWithAngle(&plt, pt, 20);
    thickness = 4;
    line(VEC_XY(plf), VEC_XY(plt));
    if (part.pos.x < -20) {
      partActive = false;
    }
  }
}

void addGamePizzaArrow() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
