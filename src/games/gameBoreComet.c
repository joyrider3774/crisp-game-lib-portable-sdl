#include "cglp.h"

static char *title = "BORE COMET";
static char *description = "[Hold] Drill thrust\n[Release] Fall";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 39, .isDarkColor = false};

typedef struct {
  Vector pos;
  float vy;
} Player;
static Player player;

typedef struct {
  Vector pos;
  float w;
  float h;
  float boreT;
  bool isAlive;
} Obstacle;
// Lifetime and spawn interval both scale with (0.5 + difficulty*0.1), so the
// concurrent count stays near a constant ~2.4-4.4 regardless of difficulty -
// sized with generous headroom.
#define BORECOMET_MAX_OBSTACLE_COUNT 24
static Obstacle obstacles[BORECOMET_MAX_OBSTACLE_COUNT];
static int obstacleIndex;

typedef struct {
  Vector pos;
  float vx;
  bool isAlive;
} Enemy;
// Concurrent enemy count stays near ~3 across the difficulty range (see
// port analysis) - sized generously.
#define BORECOMET_MAX_ENEMY_COUNT 32
static Enemy enemies[BORECOMET_MAX_ENEMY_COUNT];
static int enemyIndex;

static float nextRockX;
static float nextEnemyT;
static bool wasInRock;
static float hitstop;
static float shakeMag;
static float shakeT;

typedef struct {
  Vector pos;
  float t;
  int colorIndex;
  float radius;
} Fx;
static Fx burstFx;
static Fx destFx;

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 35, 30);
    player.vy = 0;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextRockX = 20;
    nextEnemyT = 60;
    wasInRock = false;
    hitstop = 0;
    shakeMag = 0;
    shakeT = 0;
    vectorSet(&burstFx.pos, 0, 0);
    burstFx.t = 0;
    burstFx.colorIndex = LIGHT_YELLOW;
    burstFx.radius = 22;
    vectorSet(&destFx.pos, 0, 0);
    destFx.t = 0;
    destFx.colorIndex = LIGHT_BLACK;
    destFx.radius = 0;
  }

  float spd = 0.5 + difficulty * 0.1;
  bool drilling = input.isPressed;
  bool frozen = hitstop > 0;
  if (frozen) {
    hitstop--;
  }

  bool inRock = false;
  Obstacle *curRock = NULL;
  FOR_EACH(obstacles, ri) {
    ASSIGN_ARRAY_ITEM(obstacles, ri, Obstacle, r);
    SKIP_IS_NOT_ALIVE(r);
    if (fabs(player.pos.x - r->pos.x) < r->w / 2 + 2 &&
        fabs(player.pos.y - r->pos.y) < r->h / 2 + 2) {
      inRock = true;
      curRock = r;
    }
  }

  if (!frozen) {
    if (drilling) {
      if (inRock) {
        player.vy -= 0.06;
      } else {
        player.vy -= 0.11;
      }
      if (inRock && ticks % 2 == 0) {
        particle(player.pos.x, player.pos.y, 3, 1.5, 0, M_PI * 2);
      }
      if (inRock) {
        addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
        curRock->boreT++;
        float boreThreshold = round(sqrt(curRock->w * curRock->h) / (spd / 0.5));
        if (curRock->boreT >= boreThreshold) {
          float destRadius = 15 + (curRock->w + curRock->h) / 4;
          int m = 0;
          FOR_EACH(enemies, ei) {
            ASSIGN_ARRAY_ITEM(enemies, ei, Enemy, e);
            SKIP_IS_NOT_ALIVE(e);
            if (distanceTo(&e->pos, player.pos.x, player.pos.y) < destRadius) {
              m++;
            }
          }
          play(EXPLOSION);
          color = LIGHT_BLACK;
          particle(curRock->pos.x, curRock->pos.y, 30, 3, 0, M_PI * 2);
          addScore(boreThreshold, player.pos.x, player.pos.y - 6);
          if (m > 0) {
            FOR_EACH(enemies, ei2) {
              ASSIGN_ARRAY_ITEM(enemies, ei2, Enemy, e2);
              SKIP_IS_NOT_ALIVE(e2);
              if (distanceTo(&e2->pos, player.pos.x, player.pos.y) < destRadius) {
                color = PURPLE;
                particle(e2->pos.x, e2->pos.y, 10 + m * 2, 2, 0, M_PI * 2);
                e2->isAlive = false;
                continue;
              }
            }
            addScore(m * m * 3, player.pos.x, player.pos.y);
            play(POWER_UP);
          }
          hitstop = fmax(hitstop, clamp(3 + m, 3, 8));
          shakeMag = fmax(shakeMag, clamp(2 + m * 0.8, 2, 6));
          shakeT = 8;
          vectorSet(&destFx.pos, curRock->pos.x, curRock->pos.y);
          destFx.radius = destRadius;
          destFx.t = 14;
          curRock->isAlive = false;
          inRock = false;
        }
      }
    }
    player.vy += 0.05;
    player.vy = clamp(player.vy, -1.5, 1.8);
    if (inRock && drilling) {
      player.pos.y += player.vy * 0.45;
    } else {
      player.pos.y += player.vy;
    }
    if (player.pos.y < 10) {
      player.pos.y = 10;
      player.vy = 0;
    }
  }

  if (wasInRock && !inRock && drilling) {
    int n = 0;
    FOR_EACH(enemies, ei3) {
      ASSIGN_ARRAY_ITEM(enemies, ei3, Enemy, e3);
      SKIP_IS_NOT_ALIVE(e3);
      if (distanceTo(&e3->pos, player.pos.x, player.pos.y) < 22) {
        n++;
      }
    }
    int burstColor;
    if (n >= 4) {
      burstColor = LIGHT_RED;
    } else if (n >= 2) {
      burstColor = YELLOW;
    } else {
      burstColor = LIGHT_YELLOW;
    }
    play(EXPLOSION);
    color = burstColor;
    particle(player.pos.x, player.pos.y, 20 + n * 6, 3, 0, M_PI * 2);
    vectorSet(&burstFx.pos, player.pos.x, player.pos.y);
    burstFx.colorIndex = burstColor;
    burstFx.t = 12;
    if (n > 0) {
      FOR_EACH(enemies, ei4) {
        ASSIGN_ARRAY_ITEM(enemies, ei4, Enemy, e4);
        SKIP_IS_NOT_ALIVE(e4);
        if (distanceTo(&e4->pos, player.pos.x, player.pos.y) < 22) {
          color = burstColor;
          particle(e4->pos.x, e4->pos.y, 10 + n * 2, 2, 0, M_PI * 2);
          e4->isAlive = false;
          continue;
        }
      }
      addScore(n * n * 3, player.pos.x, player.pos.y);
      play(POWER_UP);
      hitstop = fmax(hitstop, clamp(2 + n, 2, 8));
      shakeMag = fmax(shakeMag, clamp(1.2 + n * 0.8, 1.2, 6));
      shakeT = 8;
    }
  }
  wasInRock = inRock;

  if (!frozen) {
    nextRockX -= spd;
    if (nextRockX < 0) {
      ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, nr);
      vectorSet(&nr->pos, 112, rnd(25, 75));
      nr->w = rnd(14, 30);
      nr->h = rnd(10, 22);
      nr->boreT = 0;
      nr->isAlive = true;
      obstacleIndex = wrap(obstacleIndex + 1, 0, BORECOMET_MAX_OBSTACLE_COUNT);
      nextRockX = rnd(30, 55);
    }
    nextEnemyT--;
    if (nextEnemyT < 0) {
      ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
      vectorSet(&ne->pos, 105, rnd(8, 84));
      ne->vx = -(spd + rnd(0.2, 0.5));
      ne->isAlive = true;
      enemyIndex = wrap(enemyIndex + 1, 0, BORECOMET_MAX_ENEMY_COUNT);
      nextEnemyT = rnd(50, 90) / sqrt(difficulty);
    }
  }

  float shakeY = 0;
  if (shakeT > 0) {
    shakeT--;
    float m2 = shakeMag * (shakeT / 8);
    shakeY = rnd(-m2, m2);
  }
  color = LIGHT_CYAN;
  rect(0, 6 + shakeY, 100, 1);
  color = LIGHT_RED;
  rect(0, 90 + shakeY, 100, 2);

  if (burstFx.t > 0) {
    burstFx.t--;
    color = burstFx.colorIndex;
    thickness = 2;
    arc(burstFx.pos.x, burstFx.pos.y + shakeY, burstFx.radius * (1 - burstFx.t / 12), 0,
        M_PI * 2);
  }
  if (destFx.t > 0) {
    destFx.t--;
    color = destFx.colorIndex;
    thickness = 2;
    arc(destFx.pos.x, destFx.pos.y + shakeY, destFx.radius * (1 - destFx.t / 14), 0,
        M_PI * 2);
  }

  FOR_EACH(obstacles, ri2) {
    ASSIGN_ARRAY_ITEM(obstacles, ri2, Obstacle, r2);
    SKIP_IS_NOT_ALIVE(r2);
    if (!frozen) {
      r2->pos.x -= spd;
    }
    color = LIGHT_BLACK;
    box(r2->pos.x, r2->pos.y, r2->w, r2->h);
    if (r2->pos.x < -20) {
      r2->isAlive = false;
      continue;
    }
  }

  if (inRock && !drilling) {
    play(EXPLOSION);
    particle(player.pos.x, player.pos.y, 25, 3, 0, M_PI * 2);
    gameOver();
  }

  FOR_EACH(enemies, ei5) {
    ASSIGN_ARRAY_ITEM(enemies, ei5, Enemy, e5);
    SKIP_IS_NOT_ALIVE(e5);
    if (!frozen) {
      e5->pos.x += e5->vx;
    }
    color = RED;
    box(e5->pos.x, e5->pos.y, 5, 4);
    if (!inRock && distanceTo(&e5->pos, player.pos.x, player.pos.y) < 5) {
      play(EXPLOSION);
      particle(player.pos.x, player.pos.y, 25, 3, 0, M_PI * 2);
      gameOver();
    }
    if (e5->pos.x < -8) {
      e5->isAlive = false;
      continue;
    }
  }

  color = RED;
  Collision seaCollision = rect(0, 93, 100, 7);
  if (seaCollision.isColliding.rect[CYAN]) {
    play(EXPLOSION);
    gameOver();
  }

  if (fabs(player.vy) > 1.0) {
    if (inRock && drilling) {
      color = YELLOW;
    } else {
      color = LIGHT_CYAN;
    }
    box(player.pos.x, clamp(player.pos.y - player.vy * 2.5, 10, 88), 4, 4);
    if (inRock && drilling) {
      color = LIGHT_YELLOW;
    } else {
      color = CYAN;
    }
    box(player.pos.x, clamp(player.pos.y - player.vy * 5, 10, 88), 3, 3);
  }

  if (inRock && drilling) {
    color = YELLOW;
  } else {
    color = CYAN;
  }
  box(player.pos.x, player.pos.y, 5, 5);
  if (drilling) {
    color = LIGHT_YELLOW;
    thickness = 2;
    bar(player.pos.x, player.pos.y - 4, 4, -M_PI / 2);
  }
  if (player.pos.y > 91) {
    play(EXPLOSION);
    particle(player.pos.x, player.pos.y, 25, 3, 0, M_PI * 2);
    gameOver();
  }
}

void addGameBoreComet() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
