#include "cglp.h"

static char *title = "SMOKE G";
static char *description = "[Tap] Smoke";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
            "      ",
},
    {
        "  lll ",
        "ll l l",
        " llll ",
        " l  l ",
        "ll  ll",
            "      ",
},
    {
        "  lll ",
        "ll l l",
        " llll ",
        "  ll  ",
        " l  l ",
        " l  l ",
    },
};
static int charactersCount = 4;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 9, .isDarkColor = false};

#define SHOT_RANGE 50

typedef struct {
  Vector pos;
  Vector target;
  float ticks;
  bool isAlive;
} Grenade;
#define MAX_GRENADE_COUNT 32
static Grenade grenades[MAX_GRENADE_COUNT];
static int grenadeIndex;

typedef struct {
  Vector pos;
  float radius;
  bool isExtending;
  bool isAlive;
} Smoke;
#define MAX_SMOKE_COUNT 16
static Smoke smokes[MAX_SMOKE_COUNT];
static int smokeIndex;

typedef struct {
  Vector pos;
  float angle;
  bool isAlive;
} Enemy;
#define MAX_ENEMY_COUNT 64
static Enemy enemies[MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyDist;
static int currentSide;
static float nextSideChangeCount;

typedef struct {
  Vector pos;
  float angle;
} Player;
static Player player;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(grenades);
    grenadeIndex = 0;
    INIT_UNALIVED_ARRAY(smokes);
    smokeIndex = 0;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyDist = 0;
    currentSide = 1;
    nextSideChangeCount = 0;
    vectorSet(&player.pos, 50, 90);
    player.angle = -M_PI / 2;
  }
  float scr = sqrt(difficulty) * 0.1;
  COUNT_IS_ALIVE(smokes, aliveSmokeCount);
  if (input.isJustPressed && aliveSmokeCount < 9) {
    play(SELECT);
    ASSIGN_ARRAY_ITEM(grenades, grenadeIndex, Grenade, ng);
    ng->pos = player.pos;
    vectorSet(&ng->target, clamp(input.pos.x, 0, 99), clamp(input.pos.y, 0, 99));
    ng->ticks = 0;
    ng->isAlive = true;
    grenadeIndex = wrap(grenadeIndex + 1, 0, MAX_GRENADE_COUNT);
  }
  color = LIGHT_BLACK;
  FOR_EACH(grenades, gi) {
    ASSIGN_ARRAY_ITEM(grenades, gi, Grenade, g);
    SKIP_IS_NOT_ALIVE(g);
    g->ticks += sqrt(difficulty);
    g->pos.x += (g->target.x - g->pos.x) * 0.1;
    g->pos.y += (g->target.y - g->pos.y) * 0.1;
    g->pos.y += cos((g->ticks / 30) * M_PI * 4) + scr;
    box(g->pos.x, g->pos.y, 4, 4);
    if (g->ticks > 30) {
      play(HIT);
      ASSIGN_ARRAY_ITEM(smokes, smokeIndex, Smoke, ns);
      ns->pos = g->target;
      ns->radius = 0;
      ns->isExtending = true;
      ns->isAlive = true;
      smokeIndex = wrap(smokeIndex + 1, 0, MAX_SMOKE_COUNT);
      g->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  FOR_EACH(smokes, si) {
    ASSIGN_ARRAY_ITEM(smokes, si, Smoke, s);
    SKIP_IS_NOT_ALIVE(s);
    if (s->isExtending) {
      s->radius += (10 - s->radius) * 0.2 * sqrt(difficulty);
      if (s->radius > 9) {
        s->isExtending = false;
        s->radius = 10;
      }
    } else {
      s->radius *= 1 - 0.005 * sqrt(difficulty);
    }
    s->pos.y += scr;
    thickness = 3;
    arc(s->pos.x, s->pos.y, s->radius, 0, M_PI * 2);
    if (!s->isExtending && s->radius < 2) {
      s->isAlive = false;
      continue;
    }
  }
  COUNT_IS_ALIVE(enemies, aliveEnemyCount0);
  if (aliveEnemyCount0 == 0) {
    nextEnemyDist = 0;
  }
  nextEnemyDist -= scr;
  if (nextEnemyDist < 0) {
    nextSideChangeCount--;
    if (nextSideChangeCount < 0) {
      currentSide *= -1;
      nextSideChangeCount = rnd(1, 5);
      nextEnemyDist += 7;
    }
    float ex = 50 + rnd(0, 40) * currentSide;
    float ey = -3;
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    vectorSet(&ne->pos, ex, ey);
    ne->angle = angleTo(&player.pos, ex, ey) + rnd(0, 0.2) * RNDPM();
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, MAX_ENEMY_COUNT);
    nextEnemyDist += rnd(5, 9);
  }
  bool hasTe = false;
  int teIndex = -1;
  float minDist = 99;
  color = TRANSPARENT;
  FOR_EACH(enemies, ei) {
    ASSIGN_ARRAY_ITEM(enemies, ei, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    float ta = angleTo(&player.pos, e->pos.x, e->pos.y);
    float d = distanceTo(&player.pos, e->pos.x, e->pos.y);
    thickness = 2;
    barCenterPosRatio = 0;
    Collision ec = bar(player.pos.x, player.pos.y, d, ta);
    if (ec.isColliding.rect[BLACK]) {
      continue;
    }
    if (d < minDist) {
      minDist = d;
      hasTe = true;
      teIndex = ei;
    }
  }
  if (hasTe) {
    float ta2 = angleTo(&player.pos, enemies[teIndex].pos.x, enemies[teIndex].pos.y);
    float oa = wrap(ta2 - player.angle, -M_PI, M_PI);
    float av = 0.012 * sqrt(difficulty);
    if (fabs(oa) < av) {
      player.angle = ta2;
    } else {
      if (oa < 0) {
        player.angle -= av;
      } else {
        player.angle += av;
      }
    }
  }
  color = LIGHT_CYAN;
  thickness = 2;
  barCenterPosRatio = 0;
  bar(player.pos.x, player.pos.y, SHOT_RANGE * 1.1, player.angle);
  color = BLUE;
  char pc[2];
  pc[0] = 'a' + (ticks / 20) % 2;
  pc[1] = 0;
  float wrappedPlayerAngle = wrap(player.angle, -M_PI, M_PI);
  characterOptions.isMirrorX = fabs(wrappedPlayerAngle) >= M_PI / 2;
  character(pc, player.pos.x, player.pos.y);
  characterOptions.isMirrorX = false;
  FOR_EACH(enemies, ei2) {
    ASSIGN_ARRAY_ITEM(enemies, ei2, Enemy, e2);
    SKIP_IS_NOT_ALIVE(e2);
    e2->pos.y += scr;
    color = TRANSPARENT;
    float d1 = distanceTo(&e2->pos, player.pos.x, player.pos.y);
    float ta1 = angleTo(&e2->pos, player.pos.x, player.pos.y);
    thickness = 2;
    barCenterPosRatio = 0;
    Collision c1 = bar(e2->pos.x, e2->pos.y, d1, ta1);
    float av2 = 0.008 * sqrt(difficulty);
    bool blocked = c1.isColliding.rect[BLACK];
    if (!blocked) {
      float ta2b = angleTo(&e2->pos, player.pos.x, player.pos.y);
      float oa2 = wrap(ta2b - e2->angle, -M_PI, M_PI);
      if (fabs(oa2) < av2) {
        e2->angle = ta2b;
      } else {
        if (oa2 < 0) {
          e2->angle -= av2;
        } else {
          e2->angle += av2;
        }
      }
      color = LIGHT_PURPLE;
    } else {
      color = LIGHT_BLACK;
    }
    thickness = 2;
    barCenterPosRatio = 0;
    Collision c2 = bar(e2->pos.x, e2->pos.y, SHOT_RANGE, e2->angle);
    if (!blocked && (c2.isColliding.character['a'] || c2.isColliding.character['b'])) {
      play(EXPLOSION);
      color = PURPLE;
      thickness = 4;
      barCenterPosRatio = 0;
      bar(e2->pos.x, e2->pos.y, SHOT_RANGE, e2->angle);
      gameOver();
    }
    if (blocked) {
      color = LIGHT_RED;
    } else {
      color = RED;
    }
    char ec2[2];
    ec2[0] = 'c' + (ticks / 30) % 2;
    ec2[1] = 0;
    float wrappedEnemyAngle = wrap(e2->angle, -M_PI, M_PI);
    characterOptions.isMirrorX = fabs(wrappedEnemyAngle) >= M_PI / 2;
    Collision c3 = character(ec2, e2->pos.x, e2->pos.y);
    characterOptions.isMirrorX = false;
    if (c3.isColliding.rect[LIGHT_CYAN]) {
      play(LASER);
      color = CYAN;
      thickness = 4;
      barCenterPosRatio = 0;
      bar(player.pos.x, player.pos.y, SHOT_RANGE, player.angle);
      particle(player.pos.x, player.pos.y, 20, 3, player.angle, 0);
      color = RED;
      particle(e2->pos.x, e2->pos.y, 16, 1, 0, M_PI * 2);
      addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      e2->isAlive = false;
      continue;
    }
    if (e2->pos.y > 99) {
      play(EXPLOSION);
      color = RED;
      text("X", e2->pos.x, 96);
      gameOver();
    }
  }
}

void addGameSmokeG() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
