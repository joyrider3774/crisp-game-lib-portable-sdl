#include "cglp.h"
#include "string.h"

static char *title = "L RAIN";
static char *description = "[Slide] Move";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "  ll  ",
    "  ll  ",
    "b ll b",
    "b ll b",
    "bbllbb",
    "b ll b",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = true};

typedef struct {
  Vector pos;
  float vy;
  float width;
  float ticks;
  bool isAlive;
} LrainLaser;
// 32->1024: at high difficulty l->ticks (used as sin(l->ticks*0.1) phase)
// steps by a large amount per tick, and once the step nears the ~62.8-tick
// sine period the phase can stall near a positive value for thousands of
// ticks before finally crossing zero - simulation shows laser count spiking
// past 30 (normally ~4-21) after roughly an hour of continuous play.
#define LRAIN_MAX_LASER_COUNT 1024
static LrainLaser lasers[LRAIN_MAX_LASER_COUNT];
static int laserIndex;
static float nextLaserTicks;
static float laserDurationAngle;

typedef struct {
  Vector pos;
  float vy;
  Vector size;
  int color;
  bool isAlive;
} LrainLaserBox;
// 128->4096: each traveling laser that reaches the top/bottom edge spawns
// floor(w) boxes (w up to width*2, so up to ~18) EVERY tick for its whole
// box-emitting phase; simulation of the real spawn/lifetime formulas shows
// the box count blows past 128 within the first 1-2 seconds of play in
// every trial (peaking 600-1000+ within 25 minutes), so 128 was never enough.
#define LRAIN_MAX_LASERBOX_COUNT 4096
static LrainLaserBox laserBoxes[LRAIN_MAX_LASERBOX_COUNT];
static int laserBoxIndex;

typedef struct {
  Vector pos;
  float px;
  float energy;
  float invincible;
} LrainShip;
static LrainShip ship;
static float safeX;
static float safeVx;
static int multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(lasers);
    laserIndex = 0;
    nextLaserTicks = 0;
    laserDurationAngle = 0;
    INIT_UNALIVED_ARRAY(laserBoxes);
    laserBoxIndex = 0;
    vectorSet(&ship.pos, 50, 50);
    ship.px = 50;
    ship.energy = 0;
    ship.invincible = 0;
    safeX = 50;
    safeVx = 0;
    multiplier = 1;
  }
  ship.px = ship.pos.x;
  ship.pos.x = clamp(input.pos.x, 0, 99);
  color = WHITE;
  rect(ship.pos.x + 2, ship.pos.y - 3, ship.px + 6 - (ship.pos.x + 2), 6);
  rect(ship.pos.x - 2, ship.pos.y - 3, ship.px - 6 - (ship.pos.x - 2), 6);
  color = BLACK;
  rect(ship.pos.x, ship.pos.y - 2, 1, 4);
  COUNT_IS_ALIVE(lasers, aliveLaserCount);
  safeX += safeVx / (aliveLaserCount + 1);
  safeVx += wrap(safeVx + rnd(0, 0.01) * RNDPM(), -0.1, 0.1);
  safeVx *= 0.99;
  if ((safeX < 9 && safeVx < 0) || (safeX > 90 && safeVx > 0)) {
    safeVx *= -1;
  }
  laserDurationAngle += 0.001 * difficulty;
  nextLaserTicks--;
  if (nextLaserTicks < 0) {
    play(LASER);
    float x = rnd(0, 99);
    float vy = sqrt(difficulty) * 3 * (rndi(0, 2) * 2 - 1);
    float width = rnd(3, 9);
    if (fabs(x - safeX) > width + 2) {
      ASSIGN_ARRAY_ITEM(lasers, laserIndex, LrainLaser, nl);
      float ly;
      if (vy > 0) {
        ly = 0;
      } else {
        ly = 99;
      }
      vectorSet(&nl->pos, x, ly);
      nl->vy = vy;
      nl->width = width;
      nl->ticks = 0;
      nl->isAlive = true;
      laserIndex = wrap(laserIndex + 1, 0, LRAIN_MAX_LASER_COUNT);
    }
    float tr = fabs(cos(laserDurationAngle));
    if (tr < 0.15) {
      tr = 5;
    }
    nextLaserTicks = rnd(0, 30 / tr) / difficulty;
  }
  color = LIGHT_RED;
  FOR_EACH(lasers, i) {
    ASSIGN_ARRAY_ITEM(lasers, i, LrainLaser, l);
    SKIP_IS_NOT_ALIVE(l);
    if (l->pos.y < 0 || l->pos.y > 99) {
      l->ticks += difficulty;
      float w = sin(l->ticks * 0.1) * l->width * 2;
      if (w <= 0) {
        l->isAlive = false;
        continue;
      }
      w = clamp(w, 0, l->width - 1);
      float sy = fabs(l->vy) * rnd(3, 5);
      int boxCount = (int)floor(w);
      TIMES(boxCount, k) {
        ASSIGN_ARRAY_ITEM(laserBoxes, laserBoxIndex, LrainLaserBox, nb);
        float ySign;
        if (l->vy > 0) {
          ySign = -1;
        } else {
          ySign = 1;
        }
        float by = l->pos.y + (50 - l->pos.y) * 2 + (rnd(0, 9) + sy / 2) * ySign;
        vectorSet(&nb->pos, l->pos.x + rnd(0, w) * RNDPM(), by);
        nb->vy = l->vy * 3;
        vectorSet(&nb->size, 3, sy);
        if (rnd(0, 1) < 0.5) {
          nb->color = RED;
        } else {
          nb->color = PURPLE;
        }
        nb->isAlive = true;
        laserBoxIndex = wrap(laserBoxIndex + 1, 0, LRAIN_MAX_LASERBOX_COUNT);
      }
    } else {
      l->pos.y += l->vy;
      float y;
      if (l->vy > 0) {
        y = 0;
      } else {
        y = 99;
      }
      rect(l->pos.x - l->width, y, l->width * 2, l->pos.y - y);
      if (l->pos.y < 0 || l->pos.y > 99) {
        play(COIN);
      }
    }
  }
  bool isHit = false;
  FOR_EACH(laserBoxes, i) {
    ASSIGN_ARRAY_ITEM(laserBoxes, i, LrainLaserBox, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.y += b->vy;
    color = b->color;
    Collision bc = box(b->pos.x, b->pos.y, b->size.x, b->size.y);
    if (bc.isColliding.rect[WHITE]) {
      play(HIT);
      addScore(multiplier, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      color = BLACK;
      particle(ship.pos.x, ship.pos.y, 1, 1, 0, M_PI * 2);
      ship.energy += 0.1;
      if (ship.energy > 15) {
        play(EXPLOSION);
        play(POWER_UP);
        multiplier++;
        particle(ship.pos.x, ship.pos.y, 30, 3, 0, M_PI * 2);
        ship.energy = 0;
        ship.invincible = 30;
      }
    }
    if (bc.isColliding.rect[BLACK]) {
      isHit = true;
    }
    b->isAlive = !(b->pos.y < -b->size.y / 2 || b->pos.y > 99 + b->size.y / 2);
  }
  if (ship.invincible < 3 && isHit) {
    play(RANDOM);
    gameOver();
  }
  color = LIGHT_CYAN;
  thickness = 2;
  arc(VEC_XY(ship.pos), 15, 0, M_PI * 2);
  color = CYAN;
  thickness = 2;
  arc(ship.pos.x, ship.pos.y, ship.energy, 0, M_PI * 2);
  color = BLACK;
  if (ship.invincible >= 3) {
    ship.invincible *= 1 - 0.02 * sqrt(difficulty);
    thickness = 3;
    arc(ship.pos.x, ship.pos.y, ship.invincible, 0, M_PI * 2);
    if (ticks % 4 < 2) {
      color = CYAN;
    }
  }
  character("a", VEC_XY(ship.pos));
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGameLRain() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
