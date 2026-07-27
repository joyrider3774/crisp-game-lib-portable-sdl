#include "cglp.h"

static char *title = "DOSHIN";
static char *description = "[Tap] Press";

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
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = true};

typedef struct {
  Vector pos;
  Vector vel;
  float t;
  int type;
  float ft;
  bool isAlive;
} DoshinHm;
#define DOSHIN_MAX_HM_COUNT 32
static DoshinHm doshinHms[DOSHIN_MAX_HM_COUNT];
static int doshinHmIndex;

typedef struct {
  float x;
  float t;
} DoshinDs;
#define DOSHIN_DS_COUNT 4
static DoshinDs doshinDss[DOSHIN_DS_COUNT];

static float doshinBt;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(doshinHms);
    doshinHmIndex = 0;
    TIMES(DOSHIN_DS_COUNT, i) {
      doshinDss[i].x = 50 * (0.5 + i);
      doshinDss[i].t = -1;
    }
    doshinBt = 0;
  }
  if (rnd(0, 1) < 0.03 * difficulty) {
    doshinBt--;
    int type;
    if (doshinBt < 0) {
      type = 0;
    } else {
      type = 1;
    }
    if (doshinBt < 0) {
      doshinBt = rndi(10, 20);
    }
    ASSIGN_ARRAY_ITEM(doshinHms, doshinHmIndex, DoshinHm, nh);
    vectorSet(&nh->pos, 0, 87);
    float vx;
    if (type == 0) {
      vx = (rndi(0, 2) * 0.2 + 0.6) * difficulty;
    } else {
      vx = (rndi(0, 4) * 0.2 + 0.1) * difficulty;
    }
    vectorSet(&nh->vel, vx, 0);
    nh->t = 0;
    nh->type = type;
    nh->ft = -1;
    nh->isAlive = true;
    doshinHmIndex = wrap(doshinHmIndex + 1, 0, DOSHIN_MAX_HM_COUNT);
  }
  color = LIGHT_BLACK;
  rect(0, 90, 199, 1);
  TIMES(DOSHIN_DS_COUNT, i) {
    DoshinDs *ds = &doshinDss[i];
    if (ds->t < 0 && input.isJustPressed && input.pos.x > i * 50 &&
        input.pos.x < (i + 1) * 50) {
      play(HIT);
      ds->t = 0;
    }
    float y;
    if (ds->t < 0) {
      y = 30;
    } else {
      y = 90 - ds->t;
    }
    if (ds->t == 0) {
      color = RED;
    } else if (ds->t == -1) {
      color = PURPLE;
    } else {
      color = LIGHT_PURPLE;
    }
    rect(ds->x - 15, y - 90, 30, 90);
    if (ds->t >= 0) {
      ds->t += difficulty;
    }
    if (ds->t > 60) {
      ds->t = -1;
    }
    color = LIGHT_RED;
    rect(ds->x - 20, 0, 40, 20);
  }
  int sc = 1;
  FOR_EACH(doshinHms, i) {
    ASSIGN_ARRAY_ITEM(doshinHms, i, DoshinHm, hm);
    SKIP_IS_NOT_ALIVE(hm);
    vectorAdd(&hm->pos, hm->vel.x, hm->vel.y);
    if (hm->type == 0) {
      color = BLUE;
    } else {
      color = RED;
    }
    if (hm->ft < 0) {
      hm->t++;
      characterOptions.isMirrorX = false;
      characterOptions.isMirrorY = false;
      characterOptions.rotation = 0;
      char hc[2];
      hc[0] = 'a' + hm->type * 2 + ((int)(hm->t / 30) % 2);
      hc[1] = 0;
      Collision c = character(hc, VEC_XY(hm->pos));
      if (c.isColliding.rect[RED]) {
        hm->ft = 0;
        hm->vel.y = -1;
      }
      if (hm->pos.x > 199) {
        if (hm->type == 0) {
          play(COIN);
          addScore(10, 190, hm->pos.y);
        } else {
          play(RANDOM);
          color = RED;
          text("X", hm->pos.x - 6, hm->pos.y);
          gameOver();
        }
        hm->isAlive = false;
        continue;
      }
    } else {
      hm->vel.y += 0.1;
      characterOptions.isMirrorX = false;
      characterOptions.isMirrorY = true;
      characterOptions.rotation = 0;
      char hc2[2];
      hc2[0] = 'a' + hm->type * 2;
      hc2[1] = 0;
      character(hc2, VEC_XY(hm->pos));
      characterOptions.isMirrorY = false;
      if (hm->pos.y > 99) {
        if (hm->type == 0) {
          play(RANDOM);
          color = RED;
          text("X", hm->pos.x, hm->pos.y - 6);
          gameOver();
        } else {
          play(SELECT);
          addScore(sc, hm->pos.x, hm->pos.y);
          sc++;
        }
        hm->isAlive = false;
        continue;
      }
    }
  }
}

void addGameDoshin() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
