#include "cglp.h"
#include "string.h"

static char *title = "TAPE J";
static char *description = "[Hold]    Pull\n[Release] Release";

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
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = false};

#define TAPEJ_GROUND_Y 90

typedef struct {
  float x;
  Vector size;
  bool isAlive;
} Rect;
#define TAPEJ_MAX_RECT_COUNT 64
static Rect rects[TAPEJ_MAX_RECT_COUNT];
static int rectIndex;
static float nextRectDist;

typedef struct {
  Vector from;
  Vector to;
} Tape;
// Real order-sensitive queue (front = oldest tape, animated/dismissed
// first) - generous fixed capacity with shift-from-front on completion.
#define TAPEJ_MAX_TAPE_COUNT 64
static Tape tapes[TAPEJ_MAX_TAPE_COUNT];
static int tapeCount;

#define TAPEJ_HEAD_TYPE_GROUND 0
#define TAPEJ_HEAD_TYPE_UP 1
#define TAPEJ_HEAD_TYPE_TOP 2
#define TAPEJ_HEAD_TYPE_DOWN 3
typedef struct {
  Vector from;
  Vector to;
  int type;
  int rectIndex;
} Head;
static Head head;

typedef struct {
  Vector pos;
  Vector vel;
  float size;
  bool isAlive;
} Fire;
#define TAPEJ_MAX_FIRE_COUNT 64
static Fire fires[TAPEJ_MAX_FIRE_COUNT];
static int fireIndex;
static float nextFireTicks;

static float scr;
static float dist;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(rects);
    rectIndex = 0;
    nextRectDist = 0;
    tapeCount = 0;
    vectorSet(&head.from, 100, TAPEJ_GROUND_Y - 1);
    vectorSet(&head.to, 100, TAPEJ_GROUND_Y - 1);
    head.type = TAPEJ_HEAD_TYPE_GROUND;
    head.rectIndex = -1;
    INIT_UNALIVED_ARRAY(fires);
    fireIndex = 0;
    nextFireTicks = 100;
    scr = 0;
    dist = 0;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isPressed) {
    float spd = difficulty;
    dist += spd;
    if (head.type == TAPEJ_HEAD_TYPE_GROUND) {
      head.to.x += spd;
    } else if (head.type == TAPEJ_HEAD_TYPE_UP) {
      head.to.y -= spd;
      if (head.to.y < TAPEJ_GROUND_Y - rects[head.rectIndex].size.y) {
        head.to.y = TAPEJ_GROUND_Y - rects[head.rectIndex].size.y;
        if (tapeCount < TAPEJ_MAX_TAPE_COUNT) {
          tapes[tapeCount].from = head.from;
          tapes[tapeCount].to = head.to;
          tapeCount++;
        }
        head.type = TAPEJ_HEAD_TYPE_TOP;
        play(COIN);
        head.from = head.to;
      }
    } else if (head.type == TAPEJ_HEAD_TYPE_TOP) {
      head.to.x += spd;
      float edgeX = rects[head.rectIndex].x + rects[head.rectIndex].size.x + 1;
      if (head.to.x > edgeX) {
        head.to.x = edgeX;
        if (tapeCount < TAPEJ_MAX_TAPE_COUNT) {
          tapes[tapeCount].from = head.from;
          tapes[tapeCount].to = head.to;
          tapeCount++;
        }
        head.type = TAPEJ_HEAD_TYPE_DOWN;
        play(COIN);
        head.from = head.to;
      }
    } else if (head.type == TAPEJ_HEAD_TYPE_DOWN) {
      head.to.y += spd;
      if (head.to.y > TAPEJ_GROUND_Y - 1) {
        head.to.y = TAPEJ_GROUND_Y - 1;
        if (tapeCount < TAPEJ_MAX_TAPE_COUNT) {
          tapes[tapeCount].from = head.from;
          tapes[tapeCount].to = head.to;
          tapeCount++;
        }
        head.type = TAPEJ_HEAD_TYPE_GROUND;
        play(COIN);
        head.from = head.to;
        head.to.x += 3;
        head.rectIndex = -1;
      }
    }
  } else {
    if (dist > 0) {
      play(POWER_UP);
      addScore(floor(sqrt(dist) * dist * 0.1 + 1), VEC_XY(head.to));
      dist = 0;
    }
    if (tapeCount > 0) {
      Tape *t = &tapes[0];
      t->from.x += (t->to.x - t->from.x) * 0.2 * sqrt(difficulty);
      t->from.y += (t->to.y - t->from.y) * 0.2 * sqrt(difficulty);
      if (distanceTo(&t->from, t->to.x, t->to.y) < 3) {
        memcpy(&tapes[0], &tapes[1], (tapeCount - 1) * sizeof(tapes[0]));
        tapeCount--;
      }
    } else {
      head.from.x += (head.to.x - head.from.x) * 0.2 * sqrt(difficulty);
      head.from.y += (head.to.y - head.from.y) * 0.2 * sqrt(difficulty);
    }
  }
  head.from.x -= scr;
  if (head.from.x < 0) {
    head.from.x = 0;
  }
  if (head.to.x < 0) {
    play(EXPLOSION);
    gameOver();
  }
  head.to.x -= scr;
  color = BLACK;
  int ti = 0;
  while (ti < tapeCount) {
    Tape *t = &tapes[ti];
    t->from.x -= scr;
    if (t->from.x < 0) {
      t->from.x = 0;
    }
    t->to.x -= scr;
    thickness = 3;
    line(VEC_XY(t->from), VEC_XY(t->to));
    if (t->to.x < 0) {
      memcpy(&tapes[ti], &tapes[ti + 1], (tapeCount - 1 - ti) * sizeof(tapes[0]));
      tapeCount--;
    } else {
      ti++;
    }
  }
  thickness = 3;
  line(VEC_XY(head.from), VEC_XY(head.to));
  color = LIGHT_YELLOW;
  Vector j;
  if (tapeCount > 0) {
    j = tapes[0].from;
  } else {
    j = head.from;
  }
  box(j.x - 1, j.y - 1, 5, 5);
  color = PURPLE;
  box(VEC_XY(head.to), 3, 3);
  scr = difficulty * 0.1;
  if (head.to.x > 50) {
    scr += (head.to.x - 50) * 0.1;
  }
  nextRectDist -= scr;
  if (nextRectDist < 0) {
    float w = rnd(20, 40);
    ASSIGN_ARRAY_ITEM(rects, rectIndex, Rect, nr);
    nr->x = 200;
    vectorSet(&nr->size, w, rnd(10, 50));
    nr->isAlive = true;
    rectIndex = wrap(rectIndex + 1, 0, TAPEJ_MAX_RECT_COUNT);
    nextRectDist = w + rnd(20, 70);
  }
  color = LIGHT_BLACK;
  rect(0, TAPEJ_GROUND_Y, 200, 100 - TAPEJ_GROUND_Y);
  FOR_EACH(rects, ri) {
    ASSIGN_ARRAY_ITEM(rects, ri, Rect, r);
    SKIP_IS_NOT_ALIVE(r);
    Collision rc = rect(r->x, 90, r->size.x, -r->size.y);
    if (rc.isColliding.rect[PURPLE] && head.type == TAPEJ_HEAD_TYPE_GROUND) {
      Vector to;
      vectorSet(&to, r->x - 1, TAPEJ_GROUND_Y - 1);
      if (tapeCount < TAPEJ_MAX_TAPE_COUNT) {
        tapes[tapeCount].from = head.from;
        tapes[tapeCount].to = to;
        tapeCount++;
      }
      head.from = to;
      head.to = to;
      head.type = TAPEJ_HEAD_TYPE_UP;
      play(COIN);
      head.rectIndex = ri;
    }
    r->x -= scr;
    if (r->x < -r->size.x) {
      r->isAlive = false;
      continue;
    }
  }
  nextFireTicks--;
  if (nextFireTicks < 0) {
    play(LASER);
    float size = rnd(5, 15);
    ASSIGN_ARRAY_ITEM(fires, fireIndex, Fire, nf);
    vectorSet(&nf->pos, rnd(150, 220), -size);
    vectorSet(&nf->vel, -rnd(0.5, 1), rnd(0.8, 1.2));
    vectorMul(&nf->vel, difficulty);
    nf->size = size;
    nf->isAlive = true;
    fireIndex = wrap(fireIndex + 1, 0, TAPEJ_MAX_FIRE_COUNT);
    nextFireTicks = rnd(40, 60) / difficulty;
  }
  color = RED;
  FOR_EACH(fires, fi) {
    ASSIGN_ARRAY_ITEM(fires, fi, Fire, f);
    SKIP_IS_NOT_ALIVE(f);
    vectorAdd(&f->pos, f->vel.x, f->vel.y);
    f->pos.x -= scr;
    Collision cl = box(f->pos.x, f->pos.y, f->size, f->size);
    particle(f->pos.x + rnd(0, f->size / 2) * RNDPM(), f->pos.y + rnd(0, f->size / 2) * RNDPM(),
             rnd(0, 1) * f->size / 2, -vectorLength(&f->vel), vectorAngle(&f->vel), 0.3);
    if (cl.isColliding.rect[BLACK] || cl.isColliding.rect[PURPLE] ||
        cl.isColliding.rect[LIGHT_YELLOW]) {
      play(EXPLOSION);
      gameOver();
    }
    if (cl.isColliding.rect[LIGHT_BLACK] || f->pos.y > TAPEJ_GROUND_Y) {
      play(HIT);
      particle(f->pos.x, f->pos.y, f->size * 5, 1, 0, M_PI * 2);
      f->isAlive = false;
      continue;
    }
  }
}

void addGameTapeJ() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
