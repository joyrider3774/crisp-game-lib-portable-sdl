#include "cglp.h"
#include "string.h"

static char *title = "PRESSBOUND";
static char *description = "[Hold] Narrow\n[Release] Release";

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
  float scaleX;
  float scaleY;
  float rot;
} Ball;
#define MAX_BALL_COUNT 5
static Ball balls[MAX_BALL_COUNT];
static int ballCount;

typedef struct {
  float x;
  float y;
  int life;
  float size;
  bool isAlive;
} Trail;
#define MAX_TRAIL_COUNT 64
static Trail trails[MAX_TRAIL_COUNT];
static int trailIndex;

static float wallPress;
static float charge;
static bool frozen;

static void update() {
  if (!ticks) {
    ballCount = 1;
    vectorSet(&balls[0].pos, rnd(10, 90), rnd(10, 90));
    vectorSet(&balls[0].vel, rnd(0.5, 1) * RNDPM(), rnd(0.5, 1) * RNDPM());
    balls[0].scaleX = 1;
    balls[0].scaleY = 1;
    balls[0].rot = 0;
    INIT_UNALIVED_ARRAY(trails);
    trailIndex = 0;
    wallPress = 5;
    charge = 0;
    frozen = false;
  }

  float pressRate;
  if (input.isPressed) {
    pressRate = 0.2;
  } else {
    pressRate = 0.05;
  }
  wallPress += pressRate * sqrt(difficulty);

  if (input.isJustPressed) {
    play(SELECT);
    frozen = true;
    charge = 0;
    wallPress += 2;
  }

  float pb = charge * 0.1;
  int sc = (int)floor(pb * pb * ballCount * ballCount);
  if (input.isPressed && frozen) {
    charge += 1;
    char scoreText[16];
    strcpy(scoreText, intToChar(sc));
    color = BLACK;
    text(scoreText, 50 - strlen(scoreText) * 6 + 6, 30);
  }
  if (input.isJustReleased && frozen) {
    if (sc > 0) {
      addScore(sc, 50, 50);
      play(JUMP);
    }
    TIMES(12, pi2) {
      float angle = ((float)pi2 / 12) * M_PI * 2;
      float px = 50 + cos(angle) * 30;
      float py = 50 + sin(angle) * 30;
      color = YELLOW;
      particle(px, py, 8, 2, angle, 0.3);
    }
    wallPress = 5;
    charge = 0;
    frozen = false;
    INIT_UNALIVED_ARRAY(trails);
    trailIndex = 0;
    float speed = sqrt(difficulty);
    ballCount = rndi(2, 5);
    TIMES(ballCount, bi) {
      vectorSet(&balls[bi].pos, rnd(5, 95), rnd(5, 95));
      vectorSet(&balls[bi].vel, rnd(0.5, 1) * RNDPM() * speed, rnd(0.5, 1) * RNDPM() * speed);
      balls[bi].scaleX = 1;
      balls[bi].scaleY = 1;
      balls[bi].rot = 0;
    }
  }

  float maxWall = 43;
  if (wallPress >= maxWall && !frozen) {
    play(EXPLOSION);
    gameOver();
  }

  if (!frozen) {
    TIMES(ballCount, bi2) {
      Ball *b = &balls[bi2];
      float sspd = sqrt(b->vel.x * b->vel.x + b->vel.y * b->vel.y);
      if (sspd > 0.8) {
        ASSIGN_ARRAY_ITEM(trails, trailIndex, Trail, nt);
        nt->x = b->pos.x;
        nt->y = b->pos.y;
        nt->life = 8;
        nt->size = 4;
        nt->isAlive = true;
        trailIndex = wrap(trailIndex + 1, 0, MAX_TRAIL_COUNT);
      }

      b->pos.x += b->vel.x;
      b->pos.y += b->vel.y;

      float rotDir;
      if (b->vel.x > 0) {
        rotDir = 1;
      } else {
        rotDir = -1;
      }
      b->rot += sspd * 0.1 * rotDir;

      b->scaleX += (1 - b->scaleX) * 0.15;
      b->scaleY += (1 - b->scaleY) * 0.15;

      float margin = 1;
      float minB = wallPress + margin;
      float maxB = 100 - wallPress - margin;

      if (b->pos.x < minB) {
        b->pos.x = minB;
        b->vel.x = fabs(b->vel.x);
        b->scaleX = 0.6;
        b->scaleY = 1.4;
        color = WHITE;
        particle(b->pos.x, b->pos.y, 3, 1, 0, 0.5);
        play(HIT);
      }
      if (b->pos.x > maxB) {
        b->pos.x = maxB;
        b->vel.x = -fabs(b->vel.x);
        b->scaleX = 0.6;
        b->scaleY = 1.4;
        color = WHITE;
        particle(b->pos.x, b->pos.y, 3, 1, M_PI, 0.5);
        play(HIT);
      }
      if (b->pos.y < minB) {
        b->pos.y = minB;
        b->vel.y = fabs(b->vel.y);
        b->scaleX = 1.4;
        b->scaleY = 0.6;
        color = WHITE;
        particle(b->pos.x, b->pos.y, 3, 1, -M_PI / 2, 0.5);
        play(HIT);
      }
      if (b->pos.y > maxB) {
        b->pos.y = maxB;
        b->vel.y = -fabs(b->vel.y);
        b->scaleX = 1.4;
        b->scaleY = 0.6;
        color = WHITE;
        particle(b->pos.x, b->pos.y, 3, 1, M_PI / 2, 0.5);
        play(HIT);
      }
    }
  } else {
    TIMES(ballCount, bi3) {
      float breathe = sin(ticks * 0.2) * 0.1;
      balls[bi3].scaleX = 1 + breathe;
      balls[bi3].scaleY = 1 - breathe;
    }
  }

  color = LIGHT_CYAN;
  FOR_EACH(trails, tri) {
    ASSIGN_ARRAY_ITEM(trails, tri, Trail, tr);
    SKIP_IS_NOT_ALIVE(tr);
    tr->life -= 1;
    if (tr->life > 0) {
      float alpha = (float)tr->life / 8;
      box(tr->x, tr->y, tr->size * alpha, tr->size * alpha);
    } else {
      tr->isAlive = false;
    }
  }

  int wallColor;
  if (wallPress > 35 && !frozen) {
    if ((int)floor(ticks / 15) % 2 == 0) {
      wallColor = LIGHT_RED;
    } else {
      wallColor = LIGHT_PURPLE;
    }
  } else {
    wallColor = LIGHT_PURPLE;
  }
  color = wallColor;
  float wp = wallPress;
  rect(0, 0, wp, 100);
  rect(100, 0, -wp, 100);
  rect(0, 0, 100, wp);
  rect(0, 100, 100, -wp);

  TIMES(ballCount, bi4) {
    Ball *b = &balls[bi4];
    float crushMargin;
    if (frozen) {
      crushMargin = 6;
    } else {
      crushMargin = 4;
    }
    if (b->pos.x < wallPress + crushMargin || b->pos.x > 100 - wallPress - crushMargin ||
        b->pos.y < wallPress + crushMargin || b->pos.y > 100 - wallPress - crushMargin) {
      if (ticks % 3 == 0) {
        color = CYAN;
        particle(b->pos.x, b->pos.y, 3, 1, 0, M_PI * 2);
      }
    }

    if (frozen) {
      if ((int)floor(ticks / 3) % 2 == 0) {
        color = BLUE;
      } else {
        color = LIGHT_BLUE;
      }
    } else {
      color = CYAN;
    }

    float w = 5 * b->scaleX;
    float h = 5 * b->scaleY;
    box(b->pos.x, b->pos.y, w, h);

    float lineLen = 3;
    thickness = 1;
    barCenterPosRatio = 0.5;
    bar(b->pos.x, b->pos.y, lineLen, b->rot);
    thickness = 1;
    barCenterPosRatio = 0.5;
    bar(b->pos.x, b->pos.y, lineLen, b->rot + M_PI / 2);

    float eyeOffsetX;
    if (b->vel.x > 0) {
      eyeOffsetX = 1;
    } else {
      eyeOffsetX = -1;
    }
    float eyeOffsetY;
    if (b->vel.y > 0) {
      eyeOffsetY = 0.5;
    } else {
      eyeOffsetY = -0.5;
    }

    color = WHITE;
    box(b->pos.x - 1.5, b->pos.y - 1, 2, 2);
    box(b->pos.x + 1.5, b->pos.y - 1, 2, 2);

    color = BLACK;
    box(b->pos.x - 1.5 + eyeOffsetX * 0.5, b->pos.y - 1 + eyeOffsetY, 1, 1);
    box(b->pos.x + 1.5 + eyeOffsetX * 0.5, b->pos.y - 1 + eyeOffsetY, 1, 1);

    color = TRANSPARENT;
    Collision c = box(b->pos.x, b->pos.y, 1, 1);
    if (frozen && c.isColliding.rect[LIGHT_PURPLE]) {
      play(EXPLOSION);
      gameOver();
    }
  }
}

void addGamePressbound() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
