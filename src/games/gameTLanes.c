#include "cglp.h"

static char *title = "T LANES";
static char *description = "[Tap]\n Change direction";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "yyyyy ",
        "yllly ",
        "yllly ",
        "yyyyy ",
            "      ",
        "      ",
},
    {
        " yyy  ",
        "yllly ",
        " yllly",
        "  ylll",
        "   yyy",
            "      ",
},
    {
        "rrrrr ",
        " rrrr ",
        "  rrr ",
        "   rr ",
        "    r ",
            "      ",
},
    {
        "l     ",
        "lll   ",
        "lllll ",
        "lll   ",
        "l     ",
            "      ",
},
    {
        "    b ",
        "   bb ",
        "  bbb ",
        " bbbb ",
        "bbbbb ",
            "      ",
},
    {
        "pLwwLp",
        "LpwwpL",
        "wwwwww",
        "LpwwpL",
        "pLwwLp",
            "      ",
},
};
static int charactersCount = 6;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 9, .isDarkColor = false};

#define TLANES_LANE_COUNT 5
#define TLANES_LANE_INTERVAL ((100.0 - 20) / (TLANES_LANE_COUNT - 1))

typedef struct {
  Vector pos;
  int angle;
  int currentAngle;
  bool isAlive;
} Cross;
#define TLANES_MAX_CROSS_COUNT 32
static Cross crosses[TLANES_MAX_CROSS_COUNT];
static int crossIndex;
static float nextCrossDist;
static bool nextCrossLanes[TLANES_LANE_COUNT * 2];
static int crossCount;

typedef struct {
  Vector pos;
  bool isAlive;
} Stop;
#define TLANES_MAX_STOP_COUNT 32
static Stop stops[TLANES_MAX_STOP_COUNT];
static int stopIndex;
static float nextStopDist;

typedef struct {
  Vector pos;
  int angle;
  float speed;
  float ty;
  bool onArrow;
  float invDist;
  bool isAlive;
} Car;
#define TLANES_MAX_CAR_COUNT 32
static Car cars[TLANES_MAX_CAR_COUNT];
static int carIndex;
static float nextCarDist;

static float scr;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(crosses);
    crossIndex = 0;
    nextCrossDist = 0;
    crossCount = 0;
    INIT_UNALIVED_ARRAY(stops);
    stopIndex = 0;
    nextStopDist = 199;
    float cy = 10 + TLANES_LANE_INTERVAL * floor(TLANES_LANE_COUNT / 2.0);
    INIT_UNALIVED_ARRAY(cars);
    carIndex = 0;
    ASSIGN_ARRAY_ITEM(cars, carIndex, Car, c1);
    vectorSet(&c1->pos, 20, cy);
    c1->angle = 0;
    c1->speed = 0.1;
    c1->ty = 0;
    c1->onArrow = false;
    c1->invDist = 0;
    c1->isAlive = true;
    carIndex = wrap(carIndex + 1, 0, TLANES_MAX_CAR_COUNT);
    ASSIGN_ARRAY_ITEM(cars, carIndex, Car, c2);
    vectorSet(&c2->pos, 99, cy);
    c2->angle = 0;
    c2->speed = 0;
    c2->ty = 0;
    c2->onArrow = false;
    c2->invDist = 0;
    c2->isAlive = true;
    carIndex = wrap(carIndex + 1, 0, TLANES_MAX_CAR_COUNT);
    nextCarDist = 0;
    scr = 0;
  }
  int carCount = 0;
  FOR_EACH(cars, cci) {
    ASSIGN_ARRAY_ITEM(cars, cci, Car, cc);
    SKIP_IS_NOT_ALIVE(cc);
    if (cc->speed > 0) {
      carCount++;
    }
  }
  if (carCount == 0) {
    gameOver();
  }
  nextCrossDist -= scr;
  if (nextCrossDist < 0) {
    addScore(carCount, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    crossCount--;
    if (crossCount < 0) {
      TIMES(TLANES_LANE_COUNT * 2, li) {
        nextCrossLanes[li] = !(li == TLANES_LANE_COUNT - 1 || li == TLANES_LANE_COUNT);
      }
      crossCount = TLANES_LANE_COUNT * 2 - 3;
    }
    int ci = rndi(0, TLANES_LANE_COUNT * 2);
    while (!nextCrossLanes[ci]) {
      ci = (int)wrap(ci + 1, 0, TLANES_LANE_COUNT * 2);
    }
    nextCrossLanes[ci] = false;
    int angle;
    if (ci < TLANES_LANE_COUNT) {
      angle = 1;
    } else {
      angle = -1;
    }
    int currentAngle;
    if (rnd(0, 1) < 0.5) {
      currentAngle = 0;
    } else {
      currentAngle = angle;
    }
    ASSIGN_ARRAY_ITEM(crosses, crossIndex, Cross, nc);
    vectorSet(&nc->pos, 200, 10 + TLANES_LANE_INTERVAL * (ci % TLANES_LANE_COUNT));
    nc->angle = angle;
    nc->currentAngle = currentAngle;
    nc->isAlive = true;
    crossIndex = wrap(crossIndex + 1, 0, TLANES_MAX_CROSS_COUNT);
    nextCrossDist = 24;
  }
  color = LIGHT_BLACK;
  TIMES(TLANES_LANE_COUNT, li2) {
    float y = 10 + TLANES_LANE_INTERVAL * li2;
    rect(0, y - 1, 200, 1);
    rect(0, y + 1, 200, 1);
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  FOR_EACH(crosses, cri) {
    ASSIGN_ARRAY_ITEM(crosses, cri, Cross, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.x -= scr;
    if (input.isJustPressed) {
      if (c->currentAngle == 0) {
        c->currentAngle = c->angle;
      } else {
        c->currentAngle = 0;
      }
    }
    if (c->currentAngle == 0) {
      color = LIGHT_BLACK;
    } else if (c->currentAngle == -1) {
      color = RED;
    } else {
      color = BLUE;
    }
    thickness = 1;
    line(c->pos.x, c->pos.y - 1, c->pos.x + TLANES_LANE_INTERVAL,
         c->pos.y - 1 + TLANES_LANE_INTERVAL * c->angle);
    line(c->pos.x, c->pos.y + 1, c->pos.x + TLANES_LANE_INTERVAL,
         c->pos.y + 1 + TLANES_LANE_INTERVAL * c->angle);
    color = BLACK;
    char cc2[2];
    cc2[0] = 'c' + c->currentAngle + 1;
    cc2[1] = 0;
    character(cc2, c->pos.x + 3, c->pos.y);
    if (c->pos.x < -TLANES_LANE_INTERVAL) {
      c->isAlive = false;
      continue;
    }
  }
  nextStopDist -= scr;
  if (nextStopDist < 0) {
    ASSIGN_ARRAY_ITEM(stops, stopIndex, Stop, ns);
    vectorSet(&ns->pos, 205, 10 + TLANES_LANE_INTERVAL * rndi(0, TLANES_LANE_COUNT));
    ns->isAlive = true;
    stopIndex = wrap(stopIndex + 1, 0, TLANES_MAX_STOP_COUNT);
    nextStopDist += rnd(99, 120);
  }
  color = BLACK;
  FOR_EACH(stops, sti) {
    ASSIGN_ARRAY_ITEM(stops, sti, Stop, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.x -= scr;
    Collision sc = character("f", VEC_XY(s->pos));
    if (sc.isColliding.character['c'] || sc.isColliding.character['d'] ||
        sc.isColliding.character['e']) {
      s->isAlive = false;
      continue;
    }
    if (s->pos.x < -3) {
      s->isAlive = false;
      continue;
    }
  }
  nextCarDist -= scr;
  if (nextCarDist < 0) {
    ASSIGN_ARRAY_ITEM(cars, carIndex, Car, nc2);
    vectorSet(&nc2->pos, 203, 10 + TLANES_LANE_INTERVAL * rndi(0, TLANES_LANE_COUNT));
    nc2->angle = 0;
    nc2->speed = 0;
    nc2->ty = 0;
    nc2->onArrow = false;
    nc2->invDist = 0;
    nc2->isAlive = true;
    carIndex = wrap(carIndex + 1, 0, TLANES_MAX_CAR_COUNT);
    nextCarDist += rnd(40, 60);
  }
  float maxX = 0;
  FOR_EACH(cars, cai) {
    ASSIGN_ARRAY_ITEM(cars, cai, Car, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.x -= scr;
    if (input.isJustPressed && c->pos.x < 50) {
      c->pos.x -= (50 - c->pos.x) * 0.1;
    }
    if (c->speed > 0) {
      c->speed += (1 - c->speed) * 0.05;
    }
    addWithAngle(&c->pos, (M_PI / 4) * c->angle, c->speed * sqrt(difficulty));
    if ((c->angle == -1 && c->pos.y < c->ty) || (c->angle == 1 && c->pos.y > c->ty)) {
      c->pos.y = c->ty;
      c->angle = 0;
      c->invDist = 5;
    }
    color = BLACK;
    Collision cl;
    if (c->angle == 0) {
      characterOptions.isMirrorY = false;
      cl = character("a", VEC_XY(c->pos));
    } else {
      characterOptions.isMirrorY = c->angle < 0;
      cl = character("b", VEC_XY(c->pos));
    }
    characterOptions.isMirrorY = false;
    c->invDist -= sqrt(difficulty);
    if (c->angle == 0 && c->invDist < 0 && cl.isColliding.character['f']) {
      if (c->speed > 0) {
        play(EXPLOSION);
        particle(c->pos.x, c->pos.y, 16, 1, 0, M_PI * 2);
      }
      c->isAlive = false;
      continue;
    }
    if (c->speed == 0 && cl.isColliding.character['a']) {
      play(POWER_UP);
      addScore(carCount * 10, VEC_XY(c->pos));
      c->speed = rnd(0.01, 0.3);
    }
    if (c->speed > 0 && c->angle == 0) {
      if (!c->onArrow) {
        int na;
        if (cl.isColliding.character['c']) {
          na = -1;
        } else if (cl.isColliding.character['e']) {
          na = 1;
        } else {
          na = 0;
        }
        if (na != 0) {
          play(LASER);
          color = TRANSPARENT;
          TIMES(9, bi) {
            c->pos.x--;
            Collision cl2 = character("a", VEC_XY(c->pos));
            if (!(cl2.isColliding.character['c'] || cl2.isColliding.character['e'])) {
              break;
            }
          }
          c->angle = na;
          c->ty = c->pos.y + TLANES_LANE_INTERVAL * na;
          c->onArrow = true;
        }
        if (cl.isColliding.character['d']) {
          c->onArrow = true;
        }
      } else {
        if (!(cl.isColliding.character['c'] || cl.isColliding.character['d'] ||
              cl.isColliding.character['e'])) {
          c->onArrow = false;
        }
      }
    }
    if (c->speed > 0 && c->pos.x > maxX) {
      maxX = c->pos.x;
    }
    if (c->pos.x < -3) {
      c->isAlive = false;
      continue;
    }
  }
  COUNT_IS_ALIVE(cars, aliveCarsNow);
  if (aliveCarsNow == 0) {
    gameOver();
  }
  if (maxX > 50) {
    scr = (maxX - 50) * 0.1;
  } else {
    scr = 0;
  }
}

void addGameTLanes() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
