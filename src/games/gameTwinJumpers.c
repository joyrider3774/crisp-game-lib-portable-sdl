#include "cglp.h"

static char *title = "TWIN JUMPERS";
static char *description = "[Tap] Jump";

// character 'b' upstream is only 4 rows tall (" gggg \ngggggg\ngggggg\ngg  gg\n") -
// the remaining 2 rows of the fixed CHARACTER_HEIGHT-tall array are simply
// omitted from the initializer and come out zero-filled (a blank row).
#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " bb   ",
        " bb   ",
        "bbbb  ",
        "bbbb  ",
        "b  b  ",
        "b  b  ",
    },
    {
        " gggg ",
        "gggggg",
        "gggggg",
        "gg  gg",
            "      ",
        "      ",
},
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 9, .isDarkColor = false};

typedef struct {
  Vector pos;
  float vx;
  float vy;
  float onPlatformTicks;
} Jumper;
// Upstream always has exactly two jumpers (never added to or removed), so
// this is a plain fixed 2-element array rather than a ring-buffered/
// isAlive-tracked one.
static Jumper jumpers[2];
static float jumpPower;

typedef struct {
  Vector pos;
  float width;
  bool isAlive;
} Platform;
#define TWINJUMPERS_MAX_PLATFORM_COUNT 32
static Platform platforms[TWINJUMPERS_MAX_PLATFORM_COUNT];
static int platformIndex;
static float scrollSpeed;
static float nextPlatformDistance;

static void addPlatform(float x, float y, float width) {
  ASSIGN_ARRAY_ITEM(platforms, platformIndex, Platform, p);
  vectorSet(&p->pos, x, y);
  p->width = width;
  p->isAlive = true;
  platformIndex = wrap(platformIndex + 1, 0, TWINJUMPERS_MAX_PLATFORM_COUNT);
}

static void update() {
  if (!ticks) {
    vectorSet(&jumpers[0].pos, 20, 70);
    jumpers[0].vx = 1;
    jumpers[0].vy = 0;
    jumpers[0].onPlatformTicks = 0;
    vectorSet(&jumpers[1].pos, 80, 70);
    jumpers[1].vx = 1;
    jumpers[1].vy = 0;
    jumpers[1].onPlatformTicks = 0;
    jumpPower = 1;
    INIT_UNALIVED_ARRAY(platforms);
    platformIndex = 0;
    addPlatform(50, 80, 100);
    scrollSpeed = 0.1;
    nextPlatformDistance = 0;
    TIMES(3, i) {
      addPlatform(rnd(0, 40), 70 - i * 30, rnd(20, 50));
      addPlatform(rnd(60, 100), 70 - i * 30, rnd(20, 50));
    }
  }

  scrollSpeed = 0.1 * sqrt(difficulty);
  float my = fmax(jumpers[0].pos.y, jumpers[1].pos.y);
  if (my < 60) {
    scrollSpeed += (60 - my) * 0.05;
  }
  addScore(scrollSpeed, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);

  if (input.isJustPressed) {
    TIMES(2, i) {
      if (jumpers[i].onPlatformTicks > 0) {
        play(JUMP);
        if (i == 0) {
          jumpers[i].vy = -3 * jumpPower;
        } else {
          jumpers[i].vy = -2.5 * jumpPower;
        }
      }
    }
    jumpPower *= 0.8;
  } else {
    jumpPower = clamp(jumpPower + 0.01, 0, 1);
  }

  TIMES(2, i) {
    jumpers[i].pos.y += jumpers[i].vy;
    if (input.isPressed) {
      jumpers[i].vy += 0.1;
    } else {
      jumpers[i].vy += 0.2;
    }
    jumpers[i].pos.x += jumpers[i].vx * 0.6;
    jumpers[i].onPlatformTicks--;
    if ((jumpers[i].pos.x < 3 && jumpers[i].vx < 0) ||
        (jumpers[i].pos.x > 97 && jumpers[i].vx > 0)) {
      jumpers[i].vx = -jumpers[i].vx;
    }
    if (jumpers[i].pos.y < 0 && jumpers[i].vy < 0) {
      jumpers[i].pos.y = 0;
      jumpers[i].vy *= -0.5;
    }
  }

  FOR_EACH(platforms, pIdx) {
    ASSIGN_ARRAY_ITEM(platforms, pIdx, Platform, p);
    SKIP_IS_NOT_ALIVE(p);
    p->pos.y += scrollSpeed;
    p->isAlive = p->pos.y <= 109;
  }

  nextPlatformDistance -= scrollSpeed;
  if (nextPlatformDistance <= 0) {
    addPlatform(rnd(0, 50), -rnd(0, 20) - 5, rnd(30, 50));
    addPlatform(rnd(50, 100), -rnd(0, 20) - 5, rnd(30, 50));
    nextPlatformDistance = rnd(20, 30);
  }

  color = BLACK;
  character("a", jumpers[0].pos.x, jumpers[0].pos.y);
  character("b", jumpers[1].pos.x, jumpers[1].pos.y);

  color = YELLOW;
  FOR_EACH(platforms, pi2) {
    ASSIGN_ARRAY_ITEM(platforms, pi2, Platform, p2);
    SKIP_IS_NOT_ALIVE(p2);
    Collision c = box(p2->pos.x, p2->pos.y, p2->width, 4);
    if (c.isColliding.character['a'] && jumpers[0].vy > 0) {
      if (jumpers[0].vy > 2) {
        play(CLICK);
      }
      jumpers[0].pos.y = p2->pos.y - 4;
      jumpers[0].vy = 0;
      jumpers[0].onPlatformTicks = 9;
    }
    // Upstream's own check here literally reads "jumpers[0].vy > 2" inside
    // the *second* jumper's branch too (not jumpers[1].vy) - kept verbatim
    // to match the original game's behavior exactly, not fixed as a
    // "typo", since this only gates which sound effect plays and isn't
    // something safe to silently change.
    if (c.isColliding.character['b'] && jumpers[1].vy > 0) {
      if (jumpers[0].vy > 2) {
        play(CLICK);
      }
      jumpers[1].pos.y = p2->pos.y - 3;
      jumpers[1].vy = 0;
      jumpers[1].onPlatformTicks = 9;
    }
  }

  bool anyFallen = false;
  TIMES(2, i) {
    if (jumpers[i].pos.y > 102) {
      anyFallen = true;
    }
  }
  if (anyFallen) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameTwinJumpers() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
