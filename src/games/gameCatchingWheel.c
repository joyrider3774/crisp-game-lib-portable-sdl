#include "cglp.h"

static char *title = "CATCHING WHEEL";
static char *description = "[Hold] Rotate wheel";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "lllll ",
        " lll  ",
        " lll  ",
            "      ",
        "      ",
        "      ",
},
    {
        "  ll  ",
        "l l  l",
        " llll ",
        "  l   ",
        " l l  ",
        "l   l ",
    },
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 8, .isDarkColor = true};

#define CATCHINGWHEEL_SPOKE_COUNT 6
#define CATCHINGWHEEL_BASKET_CATCH_RADIUS 9

static Vector wheelCenter;
static float wheelRadius;
static float wheelAngle;
static float wheelRotationSpeed;
static bool wheelIsAlive[CATCHINGWHEEL_SPOKE_COUNT];

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Obstacle;
#define CATCHINGWHEEL_MAX_OBSTACLE_COUNT 32
static Obstacle obstacles[CATCHINGWHEEL_MAX_OBSTACLE_COUNT];
static int obstacleIndex;
static float nextObstacleTicks;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Human;
// Spawn interval ~34/difficulty ticks but fall lifetime is a fixed ~100
// ticks (gravity isn't difficulty-scaled for humans) -> concurrent count
// ~2.94*difficulty grows unbounded; 512 covers difficulty up to ~170.
#define CATCHINGWHEEL_MAX_HUMAN_COUNT 512
static Human humans[CATCHINGWHEEL_MAX_HUMAN_COUNT];
static int humanIndex;
static float nextHumanTicks;

static int multiplier;

#define CATCHINGWHEEL_BASE_HUMAN_SPAWN_INTERVAL 40
#define CATCHINGWHEEL_BASE_OBSTACLE_SPAWN_INTERVAL 99
#define CATCHINGWHEEL_GRAVITY 0.02

static void destroyBasket(int i) {
  play(EXPLOSION);
  wheelIsAlive[i] = false;
  color = RED;
  float spokeAngle = wheelAngle + i * 2 * M_PI / CATCHINGWHEEL_SPOKE_COUNT;
  Vector spokeEnd;
  spokeEnd = wheelCenter;
  addWithAngle(&spokeEnd, spokeAngle, wheelRadius);
  particle(spokeEnd.x, clamp(spokeEnd.y, 0, 95), 20, 2, 0, M_PI * 2);
  multiplier = 1;
}

static void update() {
  if (!ticks) {
    vectorSet(&wheelCenter, 50, 90);
    wheelRadius = 40;
    wheelAngle = 0;
    wheelRotationSpeed = 0.05;
    TIMES(CATCHINGWHEEL_SPOKE_COUNT, i) { wheelIsAlive[i] = true; }
    INIT_UNALIVED_ARRAY(humans);
    humanIndex = 0;
    nextHumanTicks = CATCHINGWHEEL_BASE_HUMAN_SPAWN_INTERVAL;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    nextObstacleTicks = CATCHINGWHEEL_BASE_OBSTACLE_SPAWN_INTERVAL;
    multiplier = 1;
  }
  nextObstacleTicks -= difficulty;
  if (nextObstacleTicks <= 0) {
    ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, no);
    vectorSet(&no->pos, rnd(20, 80), 0);
    vectorSet(&no->vel, 0, 0);
    no->isAlive = true;
    obstacleIndex = wrap(obstacleIndex + 1, 0, CATCHINGWHEEL_MAX_OBSTACLE_COUNT);
    nextObstacleTicks = CATCHINGWHEEL_BASE_OBSTACLE_SPAWN_INTERVAL * rnd(0.8, 1.2);
  }
  color = RED;
  FOR_EACH(obstacles, i) {
    ASSIGN_ARRAY_ITEM(obstacles, i, Obstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    o->vel.y += CATCHINGWHEEL_GRAVITY;
    o->pos.y += o->vel.y * difficulty;
    text("*", VEC_XY(o->pos));
    if (o->pos.y > 95) {
      o->isAlive = false;
    }
  }
  color = BLUE;
  thickness = 3;
  arc(VEC_XY(wheelCenter), wheelRadius, 0, M_PI * 2);
  if (input.isPressed) {
    wheelAngle += wheelRotationSpeed * difficulty;
  }
  TIMES(CATCHINGWHEEL_SPOKE_COUNT, i) {
    float spokeAngle = wheelAngle + i * 2 * M_PI / CATCHINGWHEEL_SPOKE_COUNT;
    Vector spokeEnd;
    spokeEnd = wheelCenter;
    addWithAngle(&spokeEnd, spokeAngle, wheelRadius);
    color = BLACK;
    thickness = 3;
    line(VEC_XY(wheelCenter), VEC_XY(spokeEnd));
    if (wheelIsAlive[i]) {
      color = YELLOW;
      character("a", VEC_XY(spokeEnd));
      // Vircon32 port note: the JS version draws this basket at 3x scale
      // (a much bigger catch radius than the plain 6x6 character), which
      // this port's character() has no equivalent for. Catching a falling
      // obstacle is therefore reimplemented here as a direct distanceTo()
      // proximity check (matching the JS scaled hitbox radius) instead of
      // relying on drawn-collision, while the basket-catches-human check
      // below still uses real character collision (unaffected by scale,
      // since neither side is drawn scaled there).
      FOR_EACH(obstacles, k) {
        ASSIGN_ARRAY_ITEM(obstacles, k, Obstacle, o);
        SKIP_IS_NOT_ALIVE(o);
        if (distanceTo(&o->pos, VEC_XY(spokeEnd)) < CATCHINGWHEEL_BASKET_CATCH_RADIUS) {
          destroyBasket(i);
          break;
        }
      }
    }
  }
  nextHumanTicks -= difficulty;
  if (nextHumanTicks <= 0) {
    ASSIGN_ARRAY_ITEM(humans, humanIndex, Human, nh);
    vectorSet(&nh->pos, rnd(10, 90), 0);
    vectorSet(&nh->vel, 0, 0);
    nh->isAlive = true;
    humanIndex = wrap(humanIndex + 1, 0, CATCHINGWHEEL_MAX_HUMAN_COUNT);
    nextHumanTicks = CATCHINGWHEEL_BASE_HUMAN_SPAWN_INTERVAL * rnd(0.7, 1);
  }
  color = YELLOW;
  FOR_EACH(humans, i) {
    ASSIGN_ARRAY_ITEM(humans, i, Human, human);
    SKIP_IS_NOT_ALIVE(human);
    human->vel.y += CATCHINGWHEEL_GRAVITY;
    vectorAdd(&human->pos, human->vel.x, human->vel.y);
    Collision c = character("b", VEC_XY(human->pos));
    if (c.isColliding.text['*']) {
      human->isAlive = false;
      continue;
    }
    if (c.isColliding.character['a']) {
      play(COIN);
      addScore(multiplier, VEC_XY(human->pos));
      multiplier++;
      int bi = rndi(0, CATCHINGWHEEL_SPOKE_COUNT);
      TIMES(CATCHINGWHEEL_SPOKE_COUNT, k) {
        if (!wheelIsAlive[bi]) {
          wheelIsAlive[bi] = true;
          break;
        }
        bi = (bi + 1) % CATCHINGWHEEL_SPOKE_COUNT;
      }
      human->isAlive = false;
      continue;
    }
    if (human->pos.y > 100) {
      int bi = rndi(0, CATCHINGWHEEL_SPOKE_COUNT);
      TIMES(CATCHINGWHEEL_SPOKE_COUNT, k) {
        if (wheelIsAlive[bi]) {
          destroyBasket(bi);
          break;
        }
        bi = (bi + 1) % CATCHINGWHEEL_SPOKE_COUNT;
      }
      play(EXPLOSION);
      human->isAlive = false;
      continue;
    }
  }
  bool anyAlive = false;
  TIMES(CATCHINGWHEEL_SPOKE_COUNT, i) {
    if (wheelIsAlive[i]) {
      anyAlive = true;
      break;
    }
  }
  if (!anyAlive) {
    gameOver();
  }
}

void addGameCatchingWheel() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
