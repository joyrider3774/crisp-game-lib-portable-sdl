#include "cglp.h"
#include "string.h"

static char *title = "RICOCHET PINS";
static char *description = "[Tap] Shoot pins\nand recoil";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  cc  ",
        " c cc ",
        "cc  cc",
        " c cc ",
        "  cc  ",
            "      ",
},
    {
        "r rr r",
        " rrrr ",
        "rrrrrr",
        " rrrr ",
        "r rr r",
            "      ",
},
    {
        "  yy  ",
        " yggy ",
        "ygggyy",
        " yggy ",
        "  yy  ",
            "      ",
},
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = true};

#define PLAYER_INITIAL_SPEED 0.5
#define PLAYER_FRICTION 0.98
#define PLAYER_BASE_SCALE 1.0
#define RECOIL_FORCE 2.0
#define PIN_SCALE 1.0
#define INITIAL_PIN_COUNT 5
#define PIN_COUNT_INCREMENT 1
#define MAX_HEAT 100
#define HEAT_PER_SHOT 25
#define COOLDOWN_RATE 1.0
#define OVERHEAT_RECOVERY 30
#define HEAT_SCALE_FACTOR 0.008
#define MOVING_PIN_RATIO 0.4
#define PIN_BASE_MOVE_SPEED 0.3
#define PIN_SPEED_INCREASE_PER_DIFFICULTY 0.05

typedef struct {
  Vector pos;
  Vector vel;
  float heat;
  bool isOverheated;
} Player;
static Player player;

typedef struct {
  Vector pos;
  bool isMoving;
  bool isHorizontal;
  Vector vel;
  bool isAlive;
} Pin;
// Pin count grows by 1 every time a full wave is cleared, with no upstream
// cap - clamped here to this array's capacity (see the clamp in the "wave
// cleared" branch below) so a very long, skilled session can't overflow it.
#define MAX_PIN_COUNT 64
static Pin pins[MAX_PIN_COUNT];

typedef struct {
  Vector pos;
  bool exists;
} Item;
static Item item;

static int currentPinCount;
static int multiplier;
static Vector shotTarget;
static bool shotTargetSet;
static int shotTimer;

// Vircon32 port note: vector.h has no normalize() - divide by length here.
static void normalize(Vector *v) {
  float len = vectorLength(v);
  if (len > 0) {
    vectorMul(v, 1.0 / len);
  }
}

static void spawnPins() {
  INIT_UNALIVED_ARRAY(pins);
  for (int i = 0; i < currentPinCount; i++) {
    int attempts = 0;
    bool validPos = false;
    float px = 0;
    float py = 0;
    while (!validPos && attempts < 50) {
      px = rnd(15, 85);
      py = rnd(15, 85);
      float distToPlayer = distanceTo(&player.pos, px, py);
      if (distToPlayer > 30) {
        validPos = true;
      }
      attempts++;
    }

    bool isMoving = rnd(0, 1) < MOVING_PIN_RATIO;
    bool isHorizontal = rnd(0, 1) < 0.5;
    float currentSpeed = PIN_BASE_MOVE_SPEED + difficulty * PIN_SPEED_INCREASE_PER_DIFFICULTY;
    Vector velocity;
    if (isMoving) {
      if (isHorizontal) {
        float sx;
        if (rnd(0, 1) < 0.5) {
          sx = -currentSpeed;
        } else {
          sx = currentSpeed;
        }
        vectorSet(&velocity, sx, 0);
      } else {
        float sy;
        if (rnd(0, 1) < 0.5) {
          sy = -currentSpeed;
        } else {
          sy = currentSpeed;
        }
        vectorSet(&velocity, 0, sy);
      }
    } else {
      vectorSet(&velocity, 0, 0);
    }

    Pin *newPin = &pins[i];
    vectorSet(&newPin->pos, px, py);
    newPin->isMoving = isMoving;
    newPin->isHorizontal = isHorizontal;
    newPin->vel = velocity;
    newPin->isAlive = true;
  }

  int itemAttempts = 0;
  bool validItemPos = false;
  float ix = 0;
  float iy = 0;
  while (!validItemPos && itemAttempts < 50) {
    ix = rnd(15, 85);
    iy = rnd(15, 85);
    float distToPlayer = distanceTo(&player.pos, ix, iy);
    bool farFromPins = true;
    for (int i = 0; i < currentPinCount; i++) {
      if (distanceTo(&pins[i].pos, ix, iy) < 15) {
        farFromPins = false;
      }
    }
    if (distToPlayer > 20 && farFromPins) {
      validItemPos = true;
    }
    itemAttempts++;
  }
  vectorSet(&item.pos, ix, iy);
  item.exists = true;
}

static void update() {
  // Never reads a Collision result - pin hits are decided by distance math.
  hasCollision = false;
  if (!ticks) {
    vectorSet(&player.pos, 50, 50);
    vectorSet(&player.vel, PLAYER_INITIAL_SPEED, 0);
    player.heat = 0;
    player.isOverheated = false;
    currentPinCount = INITIAL_PIN_COUNT;
    multiplier = 1;
    item.exists = false;
    spawnPins();
    shotTargetSet = false;
    shotTimer = 0;
  }

  vectorAdd(&player.pos, VEC_XY(player.vel));
  vectorMul(&player.vel, PLAYER_FRICTION);

  if (player.heat > 0) {
    player.heat -= COOLDOWN_RATE;
    if (player.heat < 0) {
      player.heat = 0;
    }
  }

  if (player.isOverheated && player.heat <= OVERHEAT_RECOVERY) {
    player.isOverheated = false;
  }

  float currentPlayerScale = PLAYER_BASE_SCALE + player.heat * HEAT_SCALE_FACTOR;
  float currentPlayerRadius = 3 * currentPlayerScale;

  if (player.pos.x < currentPlayerRadius || player.pos.x > 100 - currentPlayerRadius) {
    player.vel.x *= -1;
    player.pos.x = clamp(player.pos.x, currentPlayerRadius, 100 - currentPlayerRadius);
    play(SELECT);
  }
  if (player.pos.y < currentPlayerRadius || player.pos.y > 100 - currentPlayerRadius) {
    player.vel.y *= -1;
    player.pos.y = clamp(player.pos.y, currentPlayerRadius, 100 - currentPlayerRadius);
    play(SELECT);
  }

  FOR_EACH(pins, pinIdx) {
    ASSIGN_ARRAY_ITEM(pins, pinIdx, Pin, pin);
    SKIP_IS_NOT_ALIVE(pin);
    if (pin->isMoving) {
      vectorAdd(&pin->pos, VEC_XY(pin->vel));
      float pinRadius = 3 * PIN_SCALE;
      if (pin->isHorizontal) {
        if (pin->pos.x < pinRadius || pin->pos.x > 100 - pinRadius) {
          pin->vel.x *= -1;
          pin->pos.x = clamp(pin->pos.x, pinRadius, 100 - pinRadius);
        }
      } else {
        if (pin->pos.y < pinRadius || pin->pos.y > 100 - pinRadius) {
          pin->vel.y *= -1;
          pin->pos.y = clamp(pin->pos.y, pinRadius, 100 - pinRadius);
        }
      }
    }

    float pinRadius2 = 3 * PIN_SCALE;
    float dist = distanceTo(&player.pos, VEC_XY(pin->pos));
    float collisionDist = currentPlayerRadius + pinRadius2;
    if (dist < collisionDist) {
      play(EXPLOSION);
      gameOver();
    }

    if (dist < collisionDist + 10 && dist >= collisionDist) {
      color = YELLOW;
      if (ticks % 10 < 5) {
        thickness = 2;
        arc(VEC_XY(pin->pos), 12, 0, M_PI * 2);
      }
    }

    if (pin->isMoving) {
      color = PURPLE;
    } else {
      color = RED;
    }
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    // Vircon32 port note: character() has no scale option (PIN_SCALE is
    // always 1.0 here anyway, so this is a no-op visually).
    character("b", VEC_XY(pin->pos));

    if (pin->isMoving) {
      color = LIGHT_PURPLE;
      thickness = 1;
      if (pin->isHorizontal) {
        line(pin->pos.x - 8, pin->pos.y, pin->pos.x + 8, pin->pos.y);
      } else {
        line(pin->pos.x, pin->pos.y - 8, pin->pos.x, pin->pos.y + 8);
      }
    }
  }

  if (item.exists) {
    float itemDist = distanceTo(&player.pos, VEC_XY(item.pos));
    if (itemDist < 7) {
      play(COIN);
      particle(VEC_XY(item.pos), 15, 3, 0, M_PI * 2);
      addScore(10 * multiplier, VEC_XY(player.pos));
      item.exists = false;
      multiplier++;
    } else {
      color = BLACK;
      characterOptions.isMirrorX = false;
      characterOptions.isMirrorY = false;
      characterOptions.rotation = 0;
      // Vircon32 port note: the JS pulses this item's scale over time -
      // character() has no scale option, drawn at normal size (visual
      // only, doesn't affect the fixed-radius collection check below).
      character("c", VEC_XY(item.pos));
    }
  }

  Pin *nearestPin = NULL;
  float nearestDist = 999999;
  FOR_EACH(pins, npi) {
    ASSIGN_ARRAY_ITEM(pins, npi, Pin, np);
    SKIP_IS_NOT_ALIVE(np);
    float d = distanceTo(&player.pos, VEC_XY(np->pos));
    if (d < nearestDist) {
      nearestDist = d;
      nearestPin = np;
    }
  }

  if (nearestPin != NULL) {
    color = LIGHT_BLACK;
    thickness = 1;
    line(VEC_XY(player.pos), VEC_XY(nearestPin->pos));
  }

  if (player.isOverheated) {
    color = RED;
    if (ticks % 5 == 0) {
      particle(VEC_XY(player.pos), 3, 1, 0, M_PI * 2);
    }
  } else if (player.heat > 75) {
    color = YELLOW;
  } else if (player.heat > 50) {
    color = LIGHT_CYAN;
  } else {
    color = CYAN;
  }
  characterOptions.isMirrorX = false;
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  // No scale option on character(); drawn at normal size (collision radius still scales via currentPlayerRadius).
  character("a", VEC_XY(player.pos));

  if (shotTimer > 0) {
    if (shotTargetSet) {
      float intensity = shotTimer / 5.0;
      color = WHITE;
      thickness = 5 * intensity;
      line(VEC_XY(player.pos), VEC_XY(shotTarget));
      color = YELLOW;
      thickness = 3 * intensity;
      line(VEC_XY(player.pos), VEC_XY(shotTarget));
      if (shotTimer >= 5) {
        color = BLACK;
        box(VEC_XY(player.pos), 5 * intensity, 5 * intensity);
      }
    }
    shotTimer--;
  }

  if (input.isJustPressed && nearestPin != NULL && !player.isOverheated) {
    Pin *targetPin = nearestPin;
    play(HIT);
    vectorSet(&shotTarget, VEC_XY(targetPin->pos));
    shotTargetSet = true;
    shotTimer = 9;

    particle(VEC_XY(player.pos), 5, 2, 0, M_PI * 2);

    Vector beamDir;
    vectorSet(&beamDir, targetPin->pos.x - player.pos.x, targetPin->pos.y - player.pos.y);
    TIMES(5, bi) {
      float t = (bi + 1) / 6.0;
      float particlePosX = player.pos.x + beamDir.x * t;
      float particlePosY = player.pos.y + beamDir.y * t;
      particle(particlePosX, particlePosY, 3, 1, 0, M_PI * 2);
    }

    particle(VEC_XY(targetPin->pos), 20, 5, 0, M_PI * 2);
    targetPin->isAlive = false;

    Vector recoilDir;
    vectorSet(&recoilDir, player.pos.x - targetPin->pos.x, player.pos.y - targetPin->pos.y);
    normalize(&recoilDir);
    vectorAdd(&player.vel, recoilDir.x * RECOIL_FORCE, recoilDir.y * RECOIL_FORCE);

    addScore(multiplier, VEC_XY(targetPin->pos));

    player.heat += HEAT_PER_SHOT;
    if (player.heat >= MAX_HEAT) {
      player.heat = MAX_HEAT;
      player.isOverheated = true;
      play(CLICK);
    }

    bool anyPinAlive = false;
    FOR_EACH(pins, api) {
      ASSIGN_ARRAY_ITEM(pins, api, Pin, ap);
      SKIP_IS_NOT_ALIVE(ap);
      anyPinAlive = true;
    }
    if (!anyPinAlive) {
      play(POWER_UP);
      currentPinCount += PIN_COUNT_INCREMENT;
      if (currentPinCount > MAX_PIN_COUNT) {
        currentPinCount = MAX_PIN_COUNT;
      }
      spawnPins();
      addScore(currentPinCount * multiplier * 2, 50, 50);
    }
  }

  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGameRicochetPins() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
