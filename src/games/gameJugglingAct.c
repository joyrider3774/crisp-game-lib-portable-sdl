#include "cglp.h"

static char *title = "JUGGLING ACT";
static char *description = "[Hold] Throw ball back\n[Release] Move";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "  lll ",
    "  lll ",
    "  ll  ",
    "  ll  ",
    " llll ",
    "ll  ll",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

#define JUGGLINGACT_STATE_READY 0
#define JUGGLINGACT_STATE_CATCH 1
#define JUGGLINGACT_BALL_FALLING 0
#define JUGGLINGACT_BALL_RISING 1
#define JUGGLINGACT_GRAVITY 0.02
#define JUGGLINGACT_THROW_VELOCITY 2
#define JUGGLINGACT_JUGGLER_VELOCITY 0.36

typedef struct {
  Vector pos;
  float velX;
  int state;
} JugglingactJuggler;
static JugglingactJuggler juggler;

typedef struct {
  Vector pos;
  Vector vel;
  int state;
  bool isAlive;
} JugglingactBall;
// Balls only die by falling past the juggler (a missed catch); perfect play
// never removes any while the spawn timer keeps firing every ~200/sqrt(
// difficulty) ticks regardless of how many are already alive, so count grows
// unbounded over a long session (~16 reached within about a minute).
#define JUGGLINGACT_MAX_BALL_COUNT 512
static JugglingactBall balls[JUGGLINGACT_MAX_BALL_COUNT];
static int ballIndex;
static float nextBallSpawnTime;

static int findNearestBallIndex() {
  int best = -1;
  float bestDist = 0;
  FOR_EACH(balls, i) {
    ASSIGN_ARRAY_ITEM(balls, i, JugglingactBall, ball);
    SKIP_IS_NOT_ALIVE(ball);
    if (ball->state != JUGGLINGACT_BALL_FALLING) {
      continue;
    }
    float distance = fabs(ball->pos.y - juggler.pos.y);
    if (best < 0 || distance < bestDist) {
      best = i;
      bestDist = distance;
    }
  }
  return best;
}

static void update() {
  // Never reads a Collision result anywhere in this file - catching is a
  // direct y-band + fabs() distance check against the juggler's position
  // (see "ball->pos.y > juggler.pos.y - 8" below), so the engine's own
  // O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure waste here.
  // Restored automatically when the next real game starts, via
  // resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    vectorSet(&juggler.pos, 50, 90);
    juggler.velX = 0;
    juggler.state = JUGGLINGACT_STATE_READY;
    INIT_UNALIVED_ARRAY(balls);
    ballIndex = 0;
    nextBallSpawnTime = 0;
  }
  float sd = sqrt(difficulty);
  if (input.isJustPressed) {
    play(SELECT);
    juggler.state = JUGGLINGACT_STATE_CATCH;
  } else if (input.isJustReleased) {
    juggler.state = JUGGLINGACT_STATE_READY;
  }
  if (!input.isPressed) {
    int nbi = findNearestBallIndex();
    if (nbi >= 0) {
      JugglingactBall *nearestBall = &balls[nbi];
      float moveDirection;
      if (nearestBall->pos.x > juggler.pos.x) {
        moveDirection = 1;
      } else {
        moveDirection = -1;
      }
      juggler.velX += moveDirection * JUGGLINGACT_JUGGLER_VELOCITY;
    }
  }
  juggler.velX *= 0.85;
  juggler.pos.x += juggler.velX * sd;
  if ((juggler.pos.x < 5 && juggler.velX < 0) ||
      (juggler.pos.x > 95 && juggler.velX > 0)) {
    juggler.velX *= -0.5;
  }
  color = BLUE;
  float handOffset;
  if (juggler.state == JUGGLINGACT_STATE_CATCH) {
    handOffset = -5;
  } else {
    handOffset = 0;
  }
  box(juggler.pos.x - 5, juggler.pos.y + handOffset, 3, 3);
  box(juggler.pos.x + 5, juggler.pos.y + handOffset, 3, 3);
  if (juggler.velX > 0) {
    characterOptions.isMirrorX = false;
  } else {
    characterOptions.isMirrorX = true;
  }
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  character("a", VEC_XY(juggler.pos));
  color = RED;
  FOR_EACH(balls, i) {
    ASSIGN_ARRAY_ITEM(balls, i, JugglingactBall, ball);
    SKIP_IS_NOT_ALIVE(ball);
    vectorAdd(&ball->pos, ball->vel.x * sd, ball->vel.y * sd);
    vectorMul(&ball->vel, 0.99);
    if ((ball->pos.x < 0 && ball->vel.x < 0) || (ball->pos.x > 100 && ball->vel.x > 0)) {
      ball->vel.x *= -1;
    }
    if (ball->state == JUGGLINGACT_BALL_FALLING) {
      ball->vel.y += JUGGLINGACT_GRAVITY * sd;
      if (juggler.state == JUGGLINGACT_STATE_CATCH &&
          ball->pos.y > juggler.pos.y - 8 && ball->pos.y < juggler.pos.y) {
        if (fabs(ball->pos.x - juggler.pos.x) < 10) {
          play(JUMP);
          ball->state = JUGGLINGACT_BALL_RISING;
          ball->vel.y = -JUGGLINGACT_THROW_VELOCITY;
          ball->vel.x = (ball->pos.x - juggler.pos.x) / 10 + rnd(0, 0.1) * RNDPM();
          COUNT_IS_ALIVE(balls, aliveBallCount);
          addScore(aliveBallCount, VEC_XY(ball->pos));
        }
      }
    } else {
      ball->vel.y += JUGGLINGACT_GRAVITY * sd;
      if (ball->vel.y >= 0) {
        ball->state = JUGGLINGACT_BALL_FALLING;
      }
    }
    thickness = 3;
    arc(VEC_XY(ball->pos), 2, 0, M_PI * 2);
    if (ball->pos.y > 100) {
      play(CLICK);
      ball->isAlive = false;
      continue;
    }
  }
  nextBallSpawnTime -= sd;
  if (nextBallSpawnTime <= 0) {
    play(LASER);
    ASSIGN_ARRAY_ITEM(balls, ballIndex, JugglingactBall, nb);
    vectorSet(&nb->pos, rnd(10, 90), 0);
    vectorSet(&nb->vel, 0, 0);
    nb->state = JUGGLINGACT_BALL_FALLING;
    nb->isAlive = true;
    ballIndex = wrap(ballIndex + 1, 0, JUGGLINGACT_MAX_BALL_COUNT);
    nextBallSpawnTime = 200;
  }
  COUNT_IS_ALIVE(balls, aliveBallCount2);
  if (aliveBallCount2 == 0) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameJugglingAct() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
