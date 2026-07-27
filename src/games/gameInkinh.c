#include "cglp.h"
#include "string.h"

static char *title = "INKINH";
static char *description = "[Hold] Thrust & Inhale";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};

typedef struct {
  Vector pos;
  Vector vel;
  float angle;
  float stretch;
} Player;
static Player player;

typedef struct {
  Vector pos;
  int timer;
  float phase;
  bool isAlive;
} Orb;
// The JS itself caps concurrent orbs at 8 (spawn is gated on
// "orbs.length < 8"), so this is an exact cap, not an estimate.
#define INKINH_MAX_ORB_COUNT 8
static Orb orbs[INKINH_MAX_ORB_COUNT];
static int orbIndex;

typedef struct {
  Vector pos;
  Vector vel;
  float rot;
  float rotSpeed;
  bool isAlive;
} Obstacle;
// Lifetime (~236/sqrt(difficulty) frames) and spawn interval
// (~72/sqrt(difficulty) frames) both scale the same way with difficulty,
// so concurrent count stays near a constant ~3.3 - sized with headroom.
#define INKINH_MAX_OBSTACLE_COUNT 24
static Obstacle obstacles[INKINH_MAX_OBSTACLE_COUNT];
static int obstacleIndex;

typedef struct {
  Vector pos;
  float alpha;
  bool isAlive;
} Trail;
#define INKINH_MAX_TRAIL_COUNT 16
static Trail trails[INKINH_MAX_TRAIL_COUNT];
static int trailIndex;

static int combo;
static int comboTimer;

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 50, 80);
    vectorSet(&player.vel, 0, 0);
    player.angle = -M_PI_2;
    player.stretch = 1;
    INIT_UNALIVED_ARRAY(orbs);
    orbIndex = 0;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    INIT_UNALIVED_ARRAY(trails);
    trailIndex = 0;
    combo = 0;
    comboTimer = 0;
  }

  comboTimer--;
  if (comboTimer <= 0) {
    combo = 0;
  }

  int spawnRate = (int)floor(72 / sqrt(difficulty));
  if (spawnRate < 1) {
    spawnRate = 1;
  }
  if (ticks % spawnRate == 0) {
    float ox = rnd(10, 90);
    float speed = 0.5 * sqrt(difficulty);
    ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, no);
    vectorSet(&no->pos, ox, -8);
    vectorSet(&no->vel, rnd(-0.2, 0.2), speed);
    no->rot = 0;
    no->rotSpeed = rnd(-0.1, 0.1);
    no->isAlive = true;
    obstacleIndex = wrap(obstacleIndex + 1, 0, INKINH_MAX_OBSTACLE_COUNT);
  }

  int orbRate = 30;
  COUNT_IS_ALIVE(orbs, orbAliveCount);
  if (ticks % orbRate == 0 && orbAliveCount < 8) {
    Vector spawnPos;
    int attempt;
    for (attempt = 0; attempt < 9; attempt++) {
      vectorSet(&spawnPos, rnd(15, 85), rnd(20, 90));
      if (distanceTo(&spawnPos, VEC_XY(player.pos)) > 20) {
        break;
      }
    }
    ASSIGN_ARRAY_ITEM(orbs, orbIndex, Orb, orb);
    orb->pos = spawnPos;
    orb->timer = 300;
    orb->phase = rnd(0, M_PI * 2);
    orb->isAlive = true;
    orbIndex = wrap(orbIndex + 1, 0, INKINH_MAX_ORB_COUNT);
  }

  float pressTurn;
  if (input.isPressed) {
    pressTurn = 0.02;
  } else {
    pressTurn = 0.05;
  }
  player.angle += pressTurn * sqrt(difficulty);

  if (input.isPressed) {
    float thrust = 0.15;
    player.vel.x += cos(player.angle) * thrust;
    player.vel.y += sin(player.angle) * thrust;

    if (ticks % 3 == 0) {
      color = LIGHT_BLACK;
      float px = player.pos.x - cos(player.angle) * 5;
      float py = player.pos.y - sin(player.angle) * 5;
      particle(px, py, 3, 0.5, -player.angle, M_PI_4);
    }

    FOR_EACH(orbs, oi) {
      ASSIGN_ARRAY_ITEM(orbs, oi, Orb, o);
      SKIP_IS_NOT_ALIVE(o);
      float dx = player.pos.x - o->pos.x;
      float dy = player.pos.y - o->pos.y;
      float dist = sqrt(dx * dx + dy * dy);
      if (dist < 30 && dist > 0) {
        float pull = 0.6 / dist;
        o->pos.x += dx * pull;
        o->pos.y += dy * pull;
      }
    }
  }

  player.vel.x *= 0.96;
  player.vel.y *= 0.96;
  Vector scaledVel;
  scaledVel = player.vel;
  vectorMul(&scaledVel, sqrt(difficulty));
  vectorAdd(&player.pos, scaledVel.x, scaledVel.y);

  player.pos.x = clamp(player.pos.x, 5, 95);
  player.pos.y = clamp(player.pos.y, 5, 95);

  float speed = sqrt(player.vel.x * player.vel.x + player.vel.y * player.vel.y);

  float targetStretch = 1 + speed * 0.3;
  player.stretch += (targetStretch - player.stretch) * 0.2;

  if (speed > 0.8 && ticks % 2 == 0) {
    ASSIGN_ARRAY_ITEM(trails, trailIndex, Trail, nt);
    vectorSet(&nt->pos, player.pos.x, player.pos.y);
    nt->alpha = 1;
    nt->isAlive = true;
    trailIndex = wrap(trailIndex + 1, 0, INKINH_MAX_TRAIL_COUNT);
  }

  color = LIGHT_CYAN;
  FOR_EACH(trails, ti) {
    ASSIGN_ARRAY_ITEM(trails, ti, Trail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->alpha -= 0.15;
    if (t->alpha > 0) {
      box(t->pos.x, t->pos.y, 4 * t->alpha, 4 * t->alpha);
    } else {
      t->isAlive = false;
      continue;
    }
  }

  if (input.isPressed) {
    color = LIGHT_BLACK;
    thickness = 2;
    arc(VEC_XY(player.pos), 15, player.angle - 0.6, player.angle + 0.6);
  }

  color = CYAN;
  float stretchX = 6 / player.stretch;
  float stretchY = 6 * player.stretch;
  box(VEC_XY(player.pos), stretchX, stretchY);

  color = WHITE;
  float eyeOffsetX;
  float eyeOffsetY;
  if (speed > 0.3) {
    eyeOffsetX = player.vel.x * 0.5;
    eyeOffsetY = player.vel.y * 0.5;
  } else {
    eyeOffsetX = 0;
    eyeOffsetY = 0;
  }
  box(player.pos.x - 1.5 + eyeOffsetX, player.pos.y - 1 + eyeOffsetY, 2, 2);
  box(player.pos.x + 1.5 + eyeOffsetX, player.pos.y - 1 + eyeOffsetY, 2, 2);
  color = BLACK;
  box(player.pos.x - 1.5 + eyeOffsetX * 1.5, player.pos.y - 1 + eyeOffsetY * 1.5, 1, 1);
  box(player.pos.x + 1.5 + eyeOffsetX * 1.5, player.pos.y - 1 + eyeOffsetY * 1.5, 1, 1);

  float dirLen = 10;
  thickness = 2;
  line(player.pos.x, player.pos.y, player.pos.x + cos(player.angle) * dirLen,
       player.pos.y + sin(player.angle) * dirLen);

  color = YELLOW;
  FOR_EACH(orbs, oi2) {
    ASSIGN_ARRAY_ITEM(orbs, oi2, Orb, o2);
    SKIP_IS_NOT_ALIVE(o2);
    o2->timer--;
    o2->phase += 0.1;
    float alpha;
    if (o2->timer > 60) {
      alpha = 1;
    } else {
      alpha = (float)o2->timer / 60;
    }
    float breathe = 1 + sin(o2->phase) * 0.15;
    float sz = (4 + alpha * 2) * breathe;
    color = YELLOW;
    Collision orbCollision = box(VEC_XY(o2->pos), sz, sz);
    bool coll = orbCollision.isColliding.rect[CYAN];

    if (alpha > 0.3) {
      float toPlayerX = player.pos.x - o2->pos.x;
      float toPlayerY = player.pos.y - o2->pos.y;
      float toPlayerLen = sqrt(toPlayerX * toPlayerX + toPlayerY * toPlayerY);
      float eyeDirX = 0;
      float eyeDirY = 0;
      if (toPlayerLen > 0) {
        eyeDirX = toPlayerX / toPlayerLen;
        eyeDirY = toPlayerY / toPlayerLen;
      }
      color = WHITE;
      box(o2->pos.x + eyeDirX * 0.5, o2->pos.y + eyeDirY * 0.5, 2, 2);
      color = BLACK;
      box(o2->pos.x + eyeDirX * 1, o2->pos.y + eyeDirY * 1, 1, 1);
      color = YELLOW;
    }

    if (coll) {
      play(COIN);
      color = YELLOW;
      particle(o2->pos.x, o2->pos.y, 8, 1.5, 0, M_PI * 2);
      combo++;
      comboTimer = 60;
      addScore(combo, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      o2->isAlive = false;
      continue;
    }
    if (o2->timer <= 0) {
      o2->isAlive = false;
      continue;
    }
  }

  color = RED;
  FOR_EACH(obstacles, oi3) {
    ASSIGN_ARRAY_ITEM(obstacles, oi3, Obstacle, ob);
    SKIP_IS_NOT_ALIVE(ob);
    vectorAdd(&ob->pos, ob->vel.x, ob->vel.y);
    ob->rot += ob->rotSpeed;
    if (ob->pos.y >= 110) {
      ob->isAlive = false;
      continue;
    }

    float sz = 3.5;
    float c = cos(ob->rot);
    float s = sin(ob->rot);
    Vector p1;
    Vector p2;
    Vector p3;
    Vector p4;
    vectorSet(&p1, ob->pos.x + sz * c, ob->pos.y + sz * s);
    vectorSet(&p2, ob->pos.x - sz * s, ob->pos.y + sz * c);
    vectorSet(&p3, ob->pos.x - sz * c, ob->pos.y - sz * s);
    vectorSet(&p4, ob->pos.x + sz * s, ob->pos.y - sz * c);
    color = RED;
    thickness = 2;
    line(p1.x, p1.y, p2.x, p2.y);
    line(p2.x, p2.y, p3.x, p3.y);
    line(p3.x, p3.y, p4.x, p4.y);
    line(p4.x, p4.y, p1.x, p1.y);

    color = RED;
    Collision obCollision = box(VEC_XY(ob->pos), 7, 7);
    if (obCollision.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      color = RED;
      particle(ob->pos.x, ob->pos.y, 15, 2, 0, M_PI * 2);
      gameOver();
    }
  }

  if (combo > 1) {
    color = BLACK;
    char comboText[16];
    strcpy(comboText, "x");
    strcat(comboText, intToChar(combo));
    text(comboText, 3, 9);
  }
}

void addGameInkinh() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
