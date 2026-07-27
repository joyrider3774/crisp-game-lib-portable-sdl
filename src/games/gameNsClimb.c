#include "cglp.h"

static char *title = "NS CLIMB";
static char *description = "[Tap] Reverse";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 4, .isDarkColor = false};

#define NSCLIMB_TYPE_NONE 0
#define NSCLIMB_TYPE_N 1
#define NSCLIMB_TYPE_S 2

typedef struct {
  Vector pos;
  int type;
  int wallIndex;
} NsClimbMagnet;
#define NSCLIMB_MAGNET_COUNT 22
static NsClimbMagnet magnets[NSCLIMB_MAGNET_COUNT];

typedef struct {
  int type;
  int prevType;
  int count;
  float x;
  float vx;
} NsClimbWall;
#define NSCLIMB_WALL_COUNT 2
static NsClimbWall walls[NSCLIMB_WALL_COUNT];

static Vector pos;
static Vector vel;
static int type;

static void update() {
  if (!ticks) {
    TIMES(NSCLIMB_MAGNET_COUNT, i) {
      float mx;
      if (i % 2 == 0) {
        mx = 20;
      } else {
        mx = 80;
      }
      vectorSet(&magnets[i].pos, mx, i * 5 - 5);
      if (i < 10) {
        magnets[i].type = NSCLIMB_TYPE_S;
      } else {
        magnets[i].type = NSCLIMB_TYPE_NONE;
      }
      magnets[i].wallIndex = i % 2;
    }
    walls[0].type = NSCLIMB_TYPE_NONE;
    walls[0].prevType = NSCLIMB_TYPE_S;
    walls[0].count = rndi(2, 4);
    walls[0].x = 20;
    walls[0].vx = 0;
    walls[1].type = NSCLIMB_TYPE_NONE;
    walls[1].prevType = NSCLIMB_TYPE_S;
    walls[1].count = rndi(2, 4);
    walls[1].x = 80;
    walls[1].vx = 0;
    vectorSet(&pos, 50, 90);
    vectorSet(&vel, 0, -1);
    type = NSCLIMB_TYPE_N;
  }
  if (input.isJustPressed) {
    if (type == NSCLIMB_TYPE_N) {
      type = NSCLIMB_TYPE_S;
      play(SELECT);
    } else {
      type = NSCLIMB_TYPE_N;
      play(LASER);
    }
  }
  vectorAdd(&pos, vel.x, vel.y);
  vel.x *= 0.98;
  vel.y += 0.001 * difficulty;
  float scr = 0;
  if (pos.y < 70) {
    scr = (70 - pos.y) * 0.1;
  }
  if (pos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
  pos.y += scr;
  addScore(scr, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  if (type == NSCLIMB_TYPE_N) {
    color = RED;
  } else {
    color = BLUE;
  }
  box(pos.x, pos.y, 7, 7);
  color = WHITE;
  char typeChar[2];
  if (type == NSCLIMB_TYPE_N) {
    typeChar[0] = 'N';
  } else {
    typeChar[0] = 'S';
  }
  typeChar[1] = 0;
  text(typeChar, pos.x - 1, pos.y - 1);
  TIMES(NSCLIMB_MAGNET_COUNT, i) {
    NsClimbMagnet *m = &magnets[i];
    m->pos.y += scr;
    if (m->type != NSCLIMB_TYPE_NONE) {
      float d = distanceTo(&pos, m->pos.x, m->pos.y);
      float a = angleTo(&pos, m->pos.x, m->pos.y);
      float mag;
      if (m->type == type) {
        mag = -1;
      } else {
        mag = 1;
      }
      addWithAngle(&vel, a, (difficulty / d / d) * mag);
    }
    if (m->type == NSCLIMB_TYPE_NONE) {
      color = LIGHT_BLACK;
    } else if (m->type == NSCLIMB_TYPE_N) {
      color = RED;
    } else {
      color = BLUE;
    }
    Collision mc = box(m->pos.x, m->pos.y, 9, 9);
    if (mc.isColliding.rect[BLUE] || mc.isColliding.rect[RED]) {
      play(HIT);
      float ofs;
      if (m->wallIndex == 0) {
        ofs = 10;
      } else {
        ofs = -10;
      }
      pos.x = m->pos.x + ofs;
      if ((m->wallIndex == 0 && vel.x < 0) || (m->wallIndex == 1 && vel.x > 0)) {
        vel.x *= -0.7;
      }
    }
    color = WHITE;
    char mTypeChar[2];
    if (m->type == NSCLIMB_TYPE_NONE) {
      mTypeChar[0] = 0;
    } else if (m->type == NSCLIMB_TYPE_N) {
      mTypeChar[0] = 'N';
    } else {
      mTypeChar[0] = 'S';
    }
    mTypeChar[1] = 0;
    text(mTypeChar, m->pos.x - 1, m->pos.y - 1);
    if (m->pos.y > 105) {
      m->pos.y -= 110;
      NsClimbWall *w = &walls[m->wallIndex];
      w->x += w->vx;
      m->pos.x = w->x;
      w->vx += rnd(0, 0.1) * RNDPM();
      if (m->wallIndex == 0) {
        if ((w->x < 10 && w->vx < 0) || (w->x > 40 && w->vx > 0)) {
          w->vx *= -0.5;
        }
      } else {
        if ((w->x < 60 && w->vx < 0) || (w->x > 90 && w->vx > 0)) {
          w->vx *= -0.5;
        }
      }
      w->count--;
      if (w->count < 0) {
        if (w->type == NSCLIMB_TYPE_NONE) {
          if (w->prevType == NSCLIMB_TYPE_N) {
            w->type = NSCLIMB_TYPE_S;
          } else {
            w->type = NSCLIMB_TYPE_N;
          }
          w->prevType = w->type;
          w->count = rndi(3, 9);
        } else {
          w->type = NSCLIMB_TYPE_NONE;
          w->count = rndi(2, 4);
        }
      }
      m->type = w->type;
    }
  }
}

void addGameNsClimb() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
