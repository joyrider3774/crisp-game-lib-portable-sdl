#include "cglp.h"

static char *title = "RAID";
static char *description = "[Hold]\n Speed up\n[Release]\n Bomb";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "ll    ",
        "lllll ",
        "llllll",
            "      ",
        "      ",
        "      ",
},
    {
        "lll   ",
        "lll   ",
        "lll   ",
        "lll   ",
        " l    ",
            "      ",
},
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 400, .isDarkColor = true};

static int buildingColors[4] = {GREEN, YELLOW, PURPLE, CYAN};

typedef struct {
  Vector pos;
  Vector vel;
  float speed;
  float downDist;
  float bombVy;
} Ship;
static Ship ship;

static bool hasBomb;
static Vector bombPos;
static Vector bombVel;

typedef struct {
  float height;
} Building;
static Building buildings[8];

typedef struct {
  Vector pos;
  float vy;
  bool isAlive;
} Falling;
// Upstream JS uses forEach (not remove()) on this list, so fallen debris is
// never actually removed - it just keeps drawing forever, permanently off
// the bottom of the view. A ring buffer recycles the oldest (always
// long-offscreen) slot instead of growing without bound; visually identical.
#define MAX_FALLING_COUNT 64
static Falling fallings[MAX_FALLING_COUNT];
static int fallingIndex;

typedef struct {
  Vector pos;
  float size;
  bool isAlive;
} Cloud;
#define MAX_CLOUD_COUNT 64
static Cloud clouds[MAX_CLOUD_COUNT];
static int cloudIndex;
static float nextCloudDist;

static void addClouds(float y) {
  float x = rnd(-10, 90);
  int c = rndi(2, 5);
  TIMES(c, ci) {
    float size = rnd(7, 12);
    ASSIGN_ARRAY_ITEM(clouds, cloudIndex, Cloud, nc);
    vectorSet(&nc->pos, x, y + rnd(0, 3) * RNDPM());
    nc->size = size;
    nc->isAlive = true;
    cloudIndex = wrap(cloudIndex + 1, 0, MAX_CLOUD_COUNT);
    x += size * rnd(0.4, 0.6);
  }
}

static void update() {
  if (!ticks) {
    vectorSet(&ship.pos, 50, 15);
    vectorSet(&ship.vel, 1, 0);
    ship.speed = 1;
    ship.downDist = 0;
    ship.bombVy = 0;
    hasBomb = false;
    TIMES(8, bi) { buildings[bi].height = rnd(10, 60); }
    INIT_UNALIVED_ARRAY(fallings);
    fallingIndex = 0;
    INIT_UNALIVED_ARRAY(clouds);
    cloudIndex = 0;
    TIMES(3, cci) { addClouds(rnd(10, 90)); }
    nextCloudDist = 0;
  }
  float scr = 0;
  if (ship.pos.y > 15) {
    scr += (ship.pos.y - 15) * 0.05;
  }
  nextCloudDist -= scr * 0.3;
  if (nextCloudDist < 0) {
    addClouds(110);
    nextCloudDist = rnd(20, 40);
  }
  color = LIGHT_BLUE;
  FOR_EACH(clouds, ci2) {
    ASSIGN_ARRAY_ITEM(clouds, ci2, Cloud, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.y -= scr * 0.3;
    box(VEC_XY(c->pos), c->size, c->size);
    if (c->pos.y < -9) {
      c->isAlive = false;
      continue;
    }
  }
  if (input.isPressed) {
    ship.speed += 0.01;
  } else {
    ship.speed += (1 - ship.speed) * 0.2;
    if (!hasBomb && ticks > 30 && input.isJustReleased) {
      play(POWER_UP);
      hasBomb = true;
      bombPos = ship.pos;
      bombVel = ship.vel;
      vectorMul(&bombVel, ship.speed);
      ship.bombVy = 0.33;
    }
  }
  vectorAdd(&ship.pos, ship.vel.x * ship.speed, ship.vel.y * ship.speed);
  if (ship.vel.y == 0) {
    if ((ship.pos.x < 10 && ship.vel.x < 0) || (ship.pos.x > 90 && ship.vel.x > 0)) {
      play(SELECT);
      ship.vel.y = 0.1;
      ship.downDist = difficulty;
    }
  } else {
    float target;
    if (ship.pos.x < 50) {
      target = 1;
    } else {
      target = -1;
    }
    ship.vel.x += (target - ship.vel.x) * 0.05;
    ship.downDist -= ship.vel.y * ship.speed;
    if (ship.downDist < 0) {
      float vx;
      if (ship.pos.x < 50) {
        vx = 1;
      } else {
        vx = -1;
      }
      vectorSet(&ship.vel, vx, 0);
    }
  }
  ship.pos.y += ship.bombVy - scr;
  ship.bombVy *= 0.8;
  color = BLACK;
  characterOptions.isMirrorX = ship.vel.x < 0;
  if (ship.vel.y == 0) {
    character("a", VEC_XY(ship.pos));
  } else {
    character("b", VEC_XY(ship.pos));
  }
  characterOptions.isMirrorX = false;
  if (hasBomb) {
    bombVel.y += 0.1;
    vectorMul(&bombVel, 0.99);
    vectorAdd(&bombPos, VEC_XY(bombVel));
    bombPos.y -= scr;
    color = RED;
    thickness = 3;
    bar(VEC_XY(bombPos), 3, vectorAngle(&bombVel));
    if ((bombPos.x < 0 && bombVel.x < 0) || (bombPos.x > 99 && bombVel.x > 0)) {
      bombVel.x *= -1;
    }
    if (bombPos.y > 99) {
      hasBomb = false;
    }
  }
  color = LIGHT_BLACK;
  FOR_EACH(fallings, fi) {
    ASSIGN_ARRAY_ITEM(fallings, fi, Falling, f);
    SKIP_IS_NOT_ALIVE(f);
    f->vy += 0.1;
    f->pos.y += f->vy - scr;
    rect(VEC_XY(f->pos), 9, 10);
  }
  TIMES(8, bi2) {
    Building *b = &buildings[bi2];
    b->height += scr;
    float x = bi2 * 10 + 10;
    int c = (int)ceil(b->height / 10);
    float y = 100 - b->height + c * 10;
    bool isDestroyed = false;
    int multiplier = 1;
    TIMES(c, ci3) {
      if (isDestroyed) {
        play(HIT);
        ASSIGN_ARRAY_ITEM(fallings, fallingIndex, Falling, nf);
        vectorSet(&nf->pos, x, y);
        nf->vy = -multiplier * 0.5;
        nf->isAlive = true;
        fallingIndex = wrap(fallingIndex + 1, 0, MAX_FALLING_COUNT);
        addScore(multiplier, x + 5, y);
        multiplier++;
        y -= 10;
        continue;
      }
      color = buildingColors[(bi2 * 17) % 4];
      Collision cc = rect(x, y, 9, 10);
      color = WHITE;
      rect(x + 1, y + 1, 7, 3);
      rect(x + 1, y + 6, 7, 3);
      if (cc.isColliding.rect[RED]) {
        play(EXPLOSION);
        addScore(multiplier, x + 5, y);
        multiplier++;
        isDestroyed = true;
        b->height = 100 - y;
        color = RED;
        particle(x + 5, y + 5, 19, 2, 0, M_PI * 2);
        hasBomb = false;
      }
      if (cc.isColliding.rect[LIGHT_BLACK]) {
        play(HIT);
        b->height = 100 - y;
        color = RED;
        particle(x + 5, y + 5, 16, 1, 0, M_PI * 2);
      }
      if (cc.isColliding.character['a'] || cc.isColliding.character['b']) {
        play(EXPLOSION);
        gameOver();
      }
      y -= 10;
    }
  }
}

void addGameRaid() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
