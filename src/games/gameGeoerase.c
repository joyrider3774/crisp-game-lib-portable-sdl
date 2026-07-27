#include "cglp.h"
#include "string.h"

static char *title = "GEOERASE";
static char *description = "[Hold] Mark\n[Release] Erase";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = false};

typedef struct {
  float x;
  float y;
} Player;
static Player player;

typedef struct {
  float x;
  float y;
  float vx;
  float vy;
  bool isSquare;
  bool marked;
  float scaleY;
  float rotation;
  bool isAlive;
} Enemy;
// Sized well above the ~15-17 concurrent enemies estimated at peak (difficulty 3-4).
#define GEOERASE_MAX_ENEMY_COUNT 64
static Enemy enemies[GEOERASE_MAX_ENEMY_COUNT];
static int enemyIndex;

typedef struct {
  float x;
  float y;
  int life;
  int maxLife;
  float size;
  bool isSquare;
  bool isAlive;
} Trail;
// Sized generously above ~3-4 trails per enemy times up to ~17 concurrent enemies.
#define GEOERASE_MAX_TRAIL_COUNT 128
static Trail trails[GEOERASE_MAX_TRAIL_COUNT];
static int trailIndex;

static float laserAngle;
static float laserSpeed;
static float spawnTimer;
static int combo;

static void update() {
  if (!ticks) {
    player.x = 50;
    player.y = 50;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    laserAngle = 0;
    laserSpeed = 0.04;
    spawnTimer = 0;
    combo = 1;
    INIT_UNALIVED_ARRAY(trails);
    trailIndex = 0;
  }

  float spawnInterval = fmax(20, 60 / difficulty);
  spawnTimer -= 1;
  if (spawnTimer <= 0) {
    spawnTimer = spawnInterval;
    int side = (int)floor(rnd(0, 4));
    float ex;
    float ey;
    if (side == 0) {
      ex = rnd(0, 100);
      ey = -5;
    } else if (side == 1) {
      ex = rnd(0, 100);
      ey = 105;
    } else if (side == 2) {
      ex = -5;
      ey = rnd(0, 100);
    } else {
      ex = 105;
      ey = rnd(0, 100);
    }
    bool isSquare = rnd(0, 1) < 0.3;
    float spawnSpeed;
    if (isSquare) {
      spawnSpeed = 0.4 * sqrt(difficulty);
    } else {
      spawnSpeed = 0.25 * sqrt(difficulty);
    }
    float angle = atan2(player.y - ey, player.x - ex);
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    ne->x = ex;
    ne->y = ey;
    ne->vx = cos(angle) * spawnSpeed;
    ne->vy = sin(angle) * spawnSpeed;
    ne->isSquare = isSquare;
    ne->marked = false;
    ne->scaleY = 1.3;
    ne->rotation = rnd(0, M_PI * 2);
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, GEOERASE_MAX_ENEMY_COUNT);
    color = LIGHT_CYAN;
    particle(ex, ey, 3, 1, angle + M_PI, M_PI / 2);
  }

  if (input.isPressed) {
    laserAngle += laserSpeed * sqrt(difficulty);
  } else {
    laserAngle += laserSpeed * 2 * sqrt(difficulty);
  }

  if (input.isJustReleased) {
    int eraseCount = 0;
    FOR_EACH(enemies, ei) {
      ASSIGN_ARRAY_ITEM(enemies, ei, Enemy, e);
      SKIP_IS_NOT_ALIVE(e);
      if (e->marked) {
        int pts;
        if (e->isSquare) {
          pts = 2;
        } else {
          pts = 1;
        }
        addScore(pts * combo, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
        color = YELLOW;
        particle(e->x, e->y, 15, 2.5, 0, M_PI);
        play(LASER);
        eraseCount++;
        e->isAlive = false;
        continue;
      }
    }
    if (eraseCount > 1) {
      combo = (int)fmin(combo + eraseCount - 1, 16);
    } else {
      combo = (int)fmax(1, combo - 1);
    }
    FOR_EACH(enemies, uei) {
      ASSIGN_ARRAY_ITEM(enemies, uei, Enemy, e2);
      SKIP_IS_NOT_ALIVE(e2);
      e2->marked = false;
    }
  }

  float breathScale = 1 + sin(ticks * 0.1) * 0.05;
  color = CYAN;
  box(player.x, player.y, 5 * breathScale, 5 * breathScale);
  color = BLACK;
  float eyeOffsetX = cos(laserAngle) * 1.2;
  float eyeOffsetY = sin(laserAngle) * 1.2;
  box(player.x - 1 + eyeOffsetX * 0.5, player.y + eyeOffsetY * 0.5, 1, 1);
  box(player.x + 1 + eyeOffsetX * 0.5, player.y + eyeOffsetY * 0.5, 1, 1);

  color = LIGHT_YELLOW;
  float laserLen = 60;
  float lx = player.x + cos(laserAngle) * laserLen;
  float ly = player.y + sin(laserAngle) * laserLen;
  if (input.isPressed) {
    float prevAngle = laserAngle - laserSpeed * sqrt(difficulty) * 0.5;
    float plx = player.x + cos(prevAngle) * laserLen * 0.9;
    float ply = player.y + sin(prevAngle) * laserLen * 0.9;
    thickness = 1;
    line(player.x, player.y, plx, ply);
  }
  color = YELLOW;
  if (input.isPressed) {
    thickness = 2;
  } else {
    thickness = 1;
  }
  line(player.x, player.y, lx, ly);

  if (input.isPressed) {
    FOR_EACH(enemies, mi) {
      ASSIGN_ARRAY_ITEM(enemies, mi, Enemy, e);
      SKIP_IS_NOT_ALIVE(e);
      float dx = e->x - player.x;
      float dy = e->y - player.y;
      float dist = sqrt(dx * dx + dy * dy);
      if (dist < laserLen && dist > 3) {
        float enemyAngle = atan2(dy, dx);
        float angleDiff = fabs(enemyAngle - laserAngle);
        while (angleDiff > M_PI) {
          angleDiff -= M_PI * 2;
        }
        angleDiff = fabs(angleDiff);
        if (angleDiff < 0.2) {
          if (!e->marked) {
            play(CLICK);
            color = YELLOW;
            particle(e->x, e->y, 3, 0.5, 0, M_PI);
          }
          e->marked = true;
          e->scaleY = 0.7;
        }
      }
    }
  }

  FOR_EACH(trails, ti) {
    ASSIGN_ARRAY_ITEM(trails, ti, Trail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->life -= 1;
    if (t->life <= 0) {
      t->isAlive = false;
      continue;
    }
    float alpha = (float)t->life / t->maxLife;
    float size = t->size * alpha;
    if (t->isSquare) {
      color = LIGHT_PURPLE;
      box(t->x, t->y, size, size);
    } else {
      color = LIGHT_RED;
      float s = size * 0.5;
      thickness = 1;
      line(t->x, t->y - s, t->x - s, t->y + s);
      line(t->x - s, t->y + s, t->x + s, t->y + s);
      line(t->x + s, t->y + s, t->x, t->y - s);
    }
  }

  FOR_EACH(enemies, upi) {
    ASSIGN_ARRAY_ITEM(enemies, upi, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    float espeed = sqrt(e->vx * e->vx + e->vy * e->vy);
    if (espeed > 0.3 && ticks % 3 == 0) {
      ASSIGN_ARRAY_ITEM(trails, trailIndex, Trail, nt);
      nt->x = e->x;
      nt->y = e->y;
      nt->life = 10;
      nt->maxLife = 10;
      nt->size = 6;
      nt->isSquare = e->isSquare;
      nt->isAlive = true;
      trailIndex = wrap(trailIndex + 1, 0, GEOERASE_MAX_TRAIL_COUNT);
    }
    e->x += e->vx;
    e->y += e->vy;
    e->rotation += espeed * 0.15;
    e->scaleY += (1 - e->scaleY) * 0.1;
  }

  FOR_EACH(enemies, edi) {
    ASSIGN_ARRAY_ITEM(enemies, edi, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (e->marked) {
      color = YELLOW;
    } else if (e->isSquare) {
      color = PURPLE;
    } else {
      color = RED;
    }
    bool hitPlayer = false;
    float scaleX = 2 - e->scaleY;
    if (e->isSquare) {
      float halfW = 3 * scaleX;
      float halfH = 3 * e->scaleY;
      float cosR = cos(e->rotation);
      float sinR = sin(e->rotation);
      float c0x = e->x + (-halfW) * cosR - (-halfH) * sinR;
      float c0y = e->y + (-halfW) * sinR + (-halfH) * cosR;
      float c1x = e->x + (halfW)*cosR - (-halfH) * sinR;
      float c1y = e->y + (halfW)*sinR + (-halfH) * cosR;
      float c2x = e->x + (halfW)*cosR - (halfH)*sinR;
      float c2y = e->y + (halfW)*sinR + (halfH)*cosR;
      float c3x = e->x + (-halfW) * cosR - (halfH)*sinR;
      float c3y = e->y + (-halfW) * sinR + (halfH)*cosR;
      thickness = 2;
      line(c0x, c0y, c1x, c1y);
      line(c1x, c1y, c2x, c2y);
      line(c2x, c2y, c3x, c3y);
      line(c3x, c3y, c0x, c0y);
      Collision hitCol = box(e->x, e->y, 6 * scaleX, 6 * e->scaleY);
      hitPlayer = hitCol.isColliding.rect[CYAN];

      if (e->marked) {
        color = RED;
      } else {
        color = BLACK;
      }
      float eyeAngle = atan2(player.y - e->y, player.x - e->x);
      float pupilX = cos(eyeAngle);
      float pupilY = sin(eyeAngle);
      box(e->x - 1.5 + pupilX * 0.5, e->y + pupilY * 0.5, 1, 1);
      box(e->x + 1.5 + pupilX * 0.5, e->y + pupilY * 0.5, 1, 1);
    } else {
      float hgt = 3 * e->scaleY;
      float wid = 3 * scaleX;
      thickness = 2;
      line(e->x, e->y - hgt, e->x - wid, e->y + hgt);
      line(e->x - wid, e->y + hgt, e->x + wid, e->y + hgt);
      line(e->x + wid, e->y + hgt, e->x, e->y - hgt);
      Collision hitCol = box(e->x, e->y, 6 * scaleX, 6 * e->scaleY);
      hitPlayer = hitCol.isColliding.rect[CYAN];

      if (e->marked) {
        color = RED;
      } else {
        color = BLACK;
      }
      float eyeAngle = atan2(player.y - e->y, player.x - e->x);
      float pupilX = cos(eyeAngle) * 0.8;
      float pupilY = sin(eyeAngle) * 0.8;
      box(e->x + pupilX, e->y + pupilY, 1.5, 1.5);
    }
    if (hitPlayer) {
      play(EXPLOSION);
      color = RED;
      particle(player.x, player.y, 20, 3, 0, M_PI);
      gameOver();
    }
  }

  FOR_EACH(enemies, ofi) {
    ASSIGN_ARRAY_ITEM(enemies, ofi, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (!(e->x > -10 && e->x < 110 && e->y > -10 && e->y < 110)) {
      e->isAlive = false;
      continue;
    }
  }

  color = BLACK;
  char comboText[16];
  strcpy(comboText, "x");
  strcat(comboText, intToChar(combo));
  text(comboText, 3, 9);
}

void addGameGeoerase() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
