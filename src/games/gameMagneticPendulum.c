#include "cglp.h"
#include "string.h"

static char *title = "MAGNETIC\nPENDULUM";
static char *description = "[Hold] Shorten rope\nCollect falling magnets";

// Sprites trimmed from upstream's 8/7 rows to this engine's fixed 6-row grid.
#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " rrrr ",
        "rRlllr",
        "rlRRRr",
        "rlllRr",
        "rRRRlr",
        " rrrr ",
    },
    {
        " bbbb ",
        "blBBlb",
        "bllBlb",
        "blBllb",
        "blBBlb",
        " bbbb ",
    },
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

#define MAGNETICPENDULUM_ROPE_MAX_LENGTH 66
#define MAGNETICPENDULUM_ROPE_MIN_LENGTH 10
#define MAGNETICPENDULUM_ROPE_CHANGE_SPEED 1.0
#define MAGNETICPENDULUM_PENDULUM_RADIUS 3
#define MAGNETICPENDULUM_ANCHOR_X 50
#define MAGNETICPENDULUM_ANCHOR_Y 5
#define MAGNETICPENDULUM_MAGNET_RADIUS 4
#define MAGNETICPENDULUM_MAGNET_FALL_SPEED_BASE 0.3
#define MAGNETICPENDULUM_MAGNET_SPAWN_INTERVAL_BASE 90
#define MAGNETICPENDULUM_MAGNETIC_FORCE 0.15
#define MAGNETICPENDULUM_MAGNETIC_RANGE 30
#define MAGNETICPENDULUM_GRAVITY_BASE 0.1
#define MAGNETICPENDULUM_DAMPING 0.99
#define MAGNETICPENDULUM_ANGLE_VELOCITY_INIT_BASE 0.02
#define MAGNETICPENDULUM_DIFFICULTY_SPEED_MULT 0.5
#define MAGNETICPENDULUM_DIFFICULTY_INTERVAL_MULT 0.3
#define MAGNETICPENDULUM_DIFFICULTY_GRAVITY_MULT 0.3
#define MAGNETICPENDULUM_DIFFICULTY_SWING_MULT 0.2
#define MAGNETICPENDULUM_FLOOR_RISE_AMOUNT 10
#define MAGNETICPENDULUM_FLOOR_LOWER_SPEED 0.01
#define MAGNETICPENDULUM_FLOOR_TRANSITION_SPEED 0.5
#define MAGNETICPENDULUM_FLOOR_COLLECT_BONUS 2
#define MAGNETICPENDULUM_MAX_MULTIPLIER 9
#define MAGNETICPENDULUM_BASE_SCORE 1

typedef struct {
  Vector pos;
  Vector velocity;
} MagneticpendulumPendulum;
static MagneticpendulumPendulum pendulum;

typedef struct {
  Vector pos;
  Vector velocity;
  bool isAlive;
} MagneticpendulumMagnet;
// Spawn interval and fall speed both scale with difficulty, keeping
// concurrent count low (~3-4 in practice) - generous headroom applied.
#define MAGNETICPENDULUM_MAX_MAGNET_COUNT 32
static MagneticpendulumMagnet magnets[MAGNETICPENDULUM_MAX_MAGNET_COUNT];
static int magnetIndex;

static float ropeLength;
static float angle;
static float angularVelocity;
static float magnetSpawnTimer;
static float floorHeight;
static float targetFloorHeight;
static int multiplier;

static void update() {
  if (!ticks) {
    ropeLength = MAGNETICPENDULUM_ROPE_MAX_LENGTH;
    angle = 0;
    angularVelocity =
        MAGNETICPENDULUM_ANGLE_VELOCITY_INIT_BASE * (1 + difficulty * MAGNETICPENDULUM_DIFFICULTY_SWING_MULT);
    vectorSet(&pendulum.pos, MAGNETICPENDULUM_ANCHOR_X,
              MAGNETICPENDULUM_ANCHOR_Y + ropeLength);
    vectorSet(&pendulum.velocity, 0, 0);
    INIT_UNALIVED_ARRAY(magnets);
    magnetIndex = 0;
    magnetSpawnTimer =
        MAGNETICPENDULUM_MAGNET_SPAWN_INTERVAL_BASE / (1 + difficulty * MAGNETICPENDULUM_DIFFICULTY_INTERVAL_MULT);
    floorHeight = 100;
    targetFloorHeight = 100;
    multiplier = 1;
  }

  targetFloorHeight = fmin(100, targetFloorHeight + MAGNETICPENDULUM_FLOOR_LOWER_SPEED);

  float heightDiff = targetFloorHeight - floorHeight;
  floorHeight += heightDiff * MAGNETICPENDULUM_FLOOR_TRANSITION_SPEED;

  if (input.isPressed) {
    ropeLength = fmax(MAGNETICPENDULUM_ROPE_MIN_LENGTH,
                       ropeLength - MAGNETICPENDULUM_ROPE_CHANGE_SPEED);
  } else {
    ropeLength = fmin(MAGNETICPENDULUM_ROPE_MAX_LENGTH,
                       ropeLength + MAGNETICPENDULUM_ROPE_CHANGE_SPEED);
  }

  magnetSpawnTimer--;
  if (magnetSpawnTimer <= 0) {
    float spawnX = rnd(15, 85);
    float magnetSpeed =
        MAGNETICPENDULUM_MAGNET_FALL_SPEED_BASE * (1 + difficulty * MAGNETICPENDULUM_DIFFICULTY_SPEED_MULT);
    ASSIGN_ARRAY_ITEM(magnets, magnetIndex, MagneticpendulumMagnet, nm);
    vectorSet(&nm->pos, spawnX, 0);
    vectorSet(&nm->velocity, 0, magnetSpeed);
    nm->isAlive = true;
    magnetIndex = wrap(magnetIndex + 1, 0, MAGNETICPENDULUM_MAX_MAGNET_COUNT);
    magnetSpawnTimer =
        MAGNETICPENDULUM_MAGNET_SPAWN_INTERVAL_BASE / (1 + difficulty * MAGNETICPENDULUM_DIFFICULTY_INTERVAL_MULT);
  }

  color = LIGHT_BLACK;
  box(MAGNETICPENDULUM_ANCHOR_X, MAGNETICPENDULUM_ANCHOR_Y, 4, 2);

  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  // Vircon32 port note: JS drew this with an unsupported isSmallText option;
  // drawn at normal text size instead (see gameSlimestretcher.c precedent).
  text(multText, 3, 9);

  if (floorHeight < 100) {
    color = BLACK;
    rect(0, floorHeight, 100, 100 - floorHeight);
  }

  float totalMagneticForceX = 0;
  float totalMagneticForceY = 0;

  FOR_EACH(magnets, mfi) {
    ASSIGN_ARRAY_ITEM(magnets, mfi, MagneticpendulumMagnet, mf);
    SKIP_IS_NOT_ALIVE(mf);
    float dx = mf->pos.x - pendulum.pos.x;
    float dy = mf->pos.y - pendulum.pos.y;
    float dist = sqrt(dx * dx + dy * dy);

    if (dist < MAGNETICPENDULUM_MAGNETIC_RANGE && dist > 0) {
      float force = MAGNETICPENDULUM_MAGNETIC_FORCE * (1 - dist / MAGNETICPENDULUM_MAGNETIC_RANGE);
      totalMagneticForceX += (dx / dist) * force;
      totalMagneticForceY += (dy / dist) * force;
    }
  }

  float tangentialForceX = cos(angle);
  float tangentialForceY = -sin(angle);
  float tangentialMagneticForce =
      totalMagneticForceX * tangentialForceX + totalMagneticForceY * tangentialForceY;

  float currentGravity = MAGNETICPENDULUM_GRAVITY_BASE * (1 + difficulty * MAGNETICPENDULUM_DIFFICULTY_GRAVITY_MULT);
  float angularAcceleration =
      -(currentGravity * sin(angle)) / ropeLength +
      tangentialMagneticForce / ropeLength;

  angularVelocity += angularAcceleration;
  angularVelocity *= MAGNETICPENDULUM_DAMPING;
  angle += angularVelocity;

  pendulum.pos.x = MAGNETICPENDULUM_ANCHOR_X + sin(angle) * ropeLength;
  pendulum.pos.y = MAGNETICPENDULUM_ANCHOR_Y + cos(angle) * ropeLength;

  color = LIGHT_RED;
  FOR_EACH(magnets, mli) {
    ASSIGN_ARRAY_ITEM(magnets, mli, MagneticpendulumMagnet, ml);
    SKIP_IS_NOT_ALIVE(ml);
    float dx2 = ml->pos.x - pendulum.pos.x;
    float dy2 = ml->pos.y - pendulum.pos.y;
    float dist2 = sqrt(dx2 * dx2 + dy2 * dy2);

    if (dist2 < MAGNETICPENDULUM_MAGNETIC_RANGE && dist2 > 0) {
      thickness = 1;
      line(VEC_XY(pendulum.pos), VEC_XY(ml->pos));
    }
  }

  color = LIGHT_BLUE;
  thickness = 2;
  line(MAGNETICPENDULUM_ANCHOR_X, MAGNETICPENDULUM_ANCHOR_Y, VEC_XY(pendulum.pos));

  color = BLACK;
  character("b", VEC_XY(pendulum.pos));

  if (pendulum.pos.y + MAGNETICPENDULUM_PENDULUM_RADIUS > floorHeight) {
    play(EXPLOSION);
    gameOver();
  }

  color = BLACK;
  FOR_EACH(magnets, mi) {
    ASSIGN_ARRAY_ITEM(magnets, mi, MagneticpendulumMagnet, m);
    SKIP_IS_NOT_ALIVE(m);
    vectorAdd(&m->pos, m->velocity.x, m->velocity.y);

    Collision magnetCol = character("a", VEC_XY(m->pos));

    if (magnetCol.isColliding.character['b']) {
      play(COIN);
      particle(m->pos.x, m->pos.y, 10, 2, 0, M_PI * 2);
      addScore(MAGNETICPENDULUM_BASE_SCORE * multiplier, VEC_XY(m->pos));
      multiplier = (int)fmin(MAGNETICPENDULUM_MAX_MULTIPLIER, multiplier + 1);
      targetFloorHeight = fmin(100, targetFloorHeight + MAGNETICPENDULUM_FLOOR_COLLECT_BONUS);
      m->isAlive = false;
      continue;
    }

    if (m->pos.y + MAGNETICPENDULUM_MAGNET_RADIUS > floorHeight) {
      play(HIT);
      targetFloorHeight = fmax(0, targetFloorHeight - MAGNETICPENDULUM_FLOOR_RISE_AMOUNT);
      multiplier = 1;
      m->isAlive = false;
      continue;
    }
  }
}

void addGameMagneticPendulum() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
