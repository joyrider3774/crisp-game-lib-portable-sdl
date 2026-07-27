#include "cglp.h"

static char *title = "LIFT UP";
static char *description = "[Hold]\n Go up fast";

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
        "l   l ",
        "llll  ",
        "lll   ",
        "llll  ",
        "l   l ",
            "      ",
},
    {
        "  ll  ",
        "   ll ",
        "llllll",
        "   ll ",
        "  ll  ",
            "      ",
},
    {
        " llll ",
        "l llll",
        "llll l",
        "llll l",
        "ll   l",
        " llll ",
    },
};
static int charactersCount = 5;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 90, .isDarkColor = false};

typedef struct {
  Vector pos;
  float width;
} LiftupWall;
#define LIFTUP_WALL_COUNT 19
static LiftupWall walls[LIFTUP_WALL_COUNT];
static float wallVx;
static float wallVw;

typedef struct {
  Vector pos;
  float vx;
  float ty;
} LiftupPlayer;
static LiftupPlayer player;
static float pressRatio;

#define LIFTUP_ITEM_TYPE_TURN 0
#define LIFTUP_ITEM_TYPE_BONUS 1
typedef struct {
  Vector pos;
  int type;
  bool isAlive;
} LiftupItem;
#define LIFTUP_MAX_ITEM_COUNT 64
static LiftupItem items[LIFTUP_MAX_ITEM_COUNT];
static int itemIndex;
static float nextItemDist;
static int bonusItemCount;
static float bonusItemX;
static int nextItemSide;
static float topWallX;
static float topWallW;
static int multiplier;

static void update() {
  if (!ticks) {
    TIMES(LIFTUP_WALL_COUNT, i) {
      vectorSet(&walls[i].pos, 50, i * 6 - 3);
      walls[i].width = 60;
    }
    wallVx = 0;
    wallVw = 0;
    vectorSet(&player.pos, 40, 90);
    player.vx = 1;
    player.ty = 90;
    pressRatio = 0;
    INIT_UNALIVED_ARRAY(items);
    itemIndex = 0;
    nextItemDist = 0;
    bonusItemCount = 0;
    bonusItemX = 0;
    nextItemSide = 1;
    topWallX = 50;
    topWallW = 60;
    multiplier = 1;
  }
  float pr;
  if (input.isPressed) {
    pr = 1;
  } else {
    pr = 0;
  }
  pressRatio += (pr - pressRatio) * 0.1;
  float scr = difficulty * (1 + pressRatio * 3) * 0.2;
  TIMES(LIFTUP_WALL_COUNT, i) {
    LiftupWall *w = &walls[i];
    w->pos.y += scr;
    if (w->pos.y > 110) {
      w->pos.y -= LIFTUP_WALL_COUNT * 6;
      LiftupWall *pw = &walls[(int)wrap(i - 1, 0, LIFTUP_WALL_COUNT)];
      topWallX = pw->pos.x + wallVx;
      w->pos.x = topWallX;
      topWallW = pw->width + wallVw;
      w->width = topWallW;
      wallVx = clamp(wallVx + rnd(0, 1) * RNDPM() * sqrt(difficulty), -5, 5);
      wallVw = clamp(wallVw + rnd(0, 1) * RNDPM() * sqrt(difficulty), -3, 3);
      wallVx *= 0.95;
      wallVw *= 0.95;
      if ((w->pos.x + w->width / 2 > 95 && wallVx > 0) ||
          (w->pos.x - w->width / 2 < 5 && wallVx < 0)) {
        wallVx *= -0.5;
      }
      if ((w->width > 80 && wallVw > 0) || (w->width < 40 && wallVw < 0)) {
        wallVw *= -0.5;
      }
    }
    color = RED;
    character("c", w->pos.x - w->width / 2, w->pos.y);
    characterOptions.isMirrorX = true;
    character("c", w->pos.x + w->width / 2, w->pos.y);
    characterOptions.isMirrorX = false;
    color = LIGHT_RED;
    rect(w->pos.x - w->width / 2 - 2, w->pos.y - 2, -70, 5);
    rect(w->pos.x + w->width / 2 + 2, w->pos.y - 2, 70, 5);
  }
  nextItemDist -= scr;
  if (nextItemDist < 0) {
    if (bonusItemCount > 0) {
      ASSIGN_ARRAY_ITEM(items, itemIndex, LiftupItem, ni);
      vectorSet(&ni->pos, bonusItemX, -3);
      ni->type = LIFTUP_ITEM_TYPE_BONUS;
      ni->isAlive = true;
      itemIndex = wrap(itemIndex + 1, 0, LIFTUP_MAX_ITEM_COUNT);
      nextItemDist = 6;
    } else {
      if (bonusItemCount < 0 && rnd(0, 1) < 0.5) {
        bonusItemCount = rndi(2, 6);
        bonusItemX = topWallX + rnd(0, topWallW * 0.25) * RNDPM();
        nextItemDist = 0;
      } else {
        float x = topWallX + rnd(0.1, 0.4) * nextItemSide * topWallW;
        if (rnd(0, 1) < 0.8) {
          nextItemSide *= -1;
        }
        ASSIGN_ARRAY_ITEM(items, itemIndex, LiftupItem, nt);
        vectorSet(&nt->pos, x, -3);
        nt->type = LIFTUP_ITEM_TYPE_TURN;
        nt->isAlive = true;
        itemIndex = wrap(itemIndex + 1, 0, LIFTUP_MAX_ITEM_COUNT);
        nextItemDist = rnd(10, 20);
      }
    }
    bonusItemCount--;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  player.pos.x += player.vx * 0.05 * difficulty * (4 - pressRatio * 3);
  player.ty -= sqrt(difficulty) * 0.04;
  player.pos.y += (player.ty - player.pos.y) * 0.2;
  color = BLACK;
  char pc[2];
  pc[0] = 'a' + (int)floor(ticks / 15) % 2;
  pc[1] = 0;
  if (player.vx < 0) {
    characterOptions.isMirrorX = true;
  } else {
    characterOptions.isMirrorX = false;
  }
  Collision c = character(pc, VEC_XY(player.pos));
  characterOptions.isMirrorX = false;
  if (c.isColliding.rect[LIGHT_RED] || c.isColliding.character['c']) {
    play(EXPLOSION);
    gameOver();
  }
  FOR_EACH(items, i) {
    ASSIGN_ARRAY_ITEM(items, i, LiftupItem, it);
    SKIP_IS_NOT_ALIVE(it);
    it->pos.y += scr;
    if (it->type == LIFTUP_ITEM_TYPE_TURN) {
      color = CYAN;
      if (player.vx > 0) {
        characterOptions.isMirrorX = true;
      } else {
        characterOptions.isMirrorX = false;
      }
      Collision ic = character("d", VEC_XY(it->pos));
      characterOptions.isMirrorX = false;
      if (ic.isColliding.rect[LIGHT_RED] || ic.isColliding.character['c']) {
        if (it->pos.x > topWallX) {
          it->pos.x -= 1;
        } else {
          it->pos.x += 1;
        }
      }
      if (ic.isColliding.character['a'] || ic.isColliding.character['b']) {
        play(LASER);
        player.vx *= -1;
        it->isAlive = false;
        continue;
      }
    } else {
      color = YELLOW;
      Collision ic2 = character("e", VEC_XY(it->pos));
      if (ic2.isColliding.rect[LIGHT_RED] || ic2.isColliding.character['c']) {
        if (it->pos.x > topWallX) {
          it->pos.x -= 1;
        } else {
          it->pos.x += 1;
        }
      }
      if (ic2.isColliding.character['a'] || ic2.isColliding.character['b']) {
        play(COIN);
        addScore(multiplier, VEC_XY(it->pos));
        multiplier = (int)clamp(multiplier + 1, 1, 99);
        player.ty += (99 - player.ty) * 0.1;
        it->isAlive = false;
        continue;
      }
    }
    if (it->pos.y > 103) {
      if (it->type == LIFTUP_ITEM_TYPE_BONUS) {
        multiplier = (int)clamp(multiplier - 1, 1, 99);
      }
      it->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  rect(0, player.pos.y + 3, 100, 6);
  if (player.pos.y < 0) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameLiftUp() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
