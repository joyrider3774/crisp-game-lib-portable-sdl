#include "cglp.h"

static char *title = "STOMPING\nBUBBLES";
static char *description = "[Hold] Stomp";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 5, .isDarkColor = false};

#define STOMPINGBUBBLES_PLAYER_SIZE 5
#define STOMPINGBUBBLES_MIN_BUBBLE_SIZE 5
#define STOMPINGBUBBLES_MAX_BUBBLE_SIZE 9
#define STOMPINGBUBBLES_MIN_BUBBLE_SPEED 0.1
#define STOMPINGBUBBLES_MAX_BUBBLE_SPEED 0.3

typedef struct {
  Vector pos;
  float vy;
} Player;
static Player player;
// Intentionally not reset in the !ticks block - matches upstream, which never resets it either.
static float playerSpeed = 1.5;

typedef struct {
  Vector pos;
  float size;
  float speed;
  bool isAlive;
} Bubble;
#define STOMPINGBUBBLES_MAX_BUBBLE_COUNT 192
static Bubble bubbles[STOMPINGBUBBLES_MAX_BUBBLE_COUNT];
static int bubbleIndex;
static float nextBubbleTicks;

typedef struct {
  Vector pos;
  float size;
  float duration;
  bool isAlive;
} ChainReaction;
#define STOMPINGBUBBLES_MAX_CHAIN_REACTION_COUNT 64
static ChainReaction chainReactions[STOMPINGBUBBLES_MAX_CHAIN_REACTION_COUNT];
static int chainReactionIndex;

static float multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 50, 0);
    player.vy = 0;
    INIT_UNALIVED_ARRAY(bubbles);
    bubbleIndex = 0;
    nextBubbleTicks = 0;
    INIT_UNALIVED_ARRAY(chainReactions);
    chainReactionIndex = 0;
    multiplier = 1;
  }
  float sd = sqrt(difficulty);
  player.pos.x += playerSpeed * sd;
  if ((player.pos.x > 95 && playerSpeed > 0) || (player.pos.x < 5 && playerSpeed < 0)) {
    playerSpeed *= -1;
  }
  player.pos.y += player.vy * sd;
  if (input.isPressed) {
    player.vy += 0.01 * 9;
  } else {
    player.vy += 0.01 * 1;
  }
  player.vy *= 0.99;
  if (player.pos.y < 0 && player.vy < 0) {
    player.vy *= -0.5;
  }
  if (player.pos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  nextBubbleTicks -= sd;
  if (nextBubbleTicks < 0) {
    float size = rnd(STOMPINGBUBBLES_MIN_BUBBLE_SIZE, STOMPINGBUBBLES_MAX_BUBBLE_SIZE);
    ASSIGN_ARRAY_ITEM(bubbles, bubbleIndex, Bubble, nb);
    vectorSet(&nb->pos, rnd(0, 100), 102 + size);
    nb->size = size;
    nb->speed = rnd(STOMPINGBUBBLES_MIN_BUBBLE_SPEED, STOMPINGBUBBLES_MAX_BUBBLE_SPEED);
    nb->isAlive = true;
    bubbleIndex = wrap(bubbleIndex + 1, 0, STOMPINGBUBBLES_MAX_BUBBLE_COUNT);
    nextBubbleTicks += 9;
  }
  FOR_EACH(bubbles, bi) {
    ASSIGN_ARRAY_ITEM(bubbles, bi, Bubble, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.y -= b->speed * sd;
  }
  FOR_EACH(chainReactions, ci) {
    ASSIGN_ARRAY_ITEM(chainReactions, ci, ChainReaction, cr);
    SKIP_IS_NOT_ALIVE(cr);
    cr->size += sd;
    cr->duration -= sd;
    if (cr->duration <= 0) {
      cr->isAlive = false;
      continue;
    }
  }

  color = RED;
  box(VEC_XY(player.pos), STOMPINGBUBBLES_PLAYER_SIZE, STOMPINGBUBBLES_PLAYER_SIZE);
  color = YELLOW;
  FOR_EACH(chainReactions, ci2) {
    ASSIGN_ARRAY_ITEM(chainReactions, ci2, ChainReaction, cr2);
    SKIP_IS_NOT_ALIVE(cr2);
    arc(cr2->pos.x, cr2->pos.y, cr2->size, 0, M_PI * 2);
  }

  color = CYAN;
  bool isHit = false;
  FOR_EACH(bubbles, bi2) {
    ASSIGN_ARRAY_ITEM(bubbles, bi2, Bubble, b2);
    SKIP_IS_NOT_ALIVE(b2);
    Collision c = arc(b2->pos.x, b2->pos.y, b2->size, 0, M_PI * 2);
    if (c.isColliding.rect[RED] || c.isColliding.rect[YELLOW]) {
      play(POWER_UP);
      particle(b2->pos.x, b2->pos.y, b2->size * 2, 3, 0, M_PI * 2);
      addScore(multiplier, VEC_XY(b2->pos));
      multiplier++;
      ASSIGN_ARRAY_ITEM(chainReactions, chainReactionIndex, ChainReaction, ncr);
      vectorSet(&ncr->pos, b2->pos.x, b2->pos.y);
      ncr->size = b2->size;
      ncr->duration = 5;
      ncr->isAlive = true;
      chainReactionIndex = wrap(chainReactionIndex + 1, 0, STOMPINGBUBBLES_MAX_CHAIN_REACTION_COUNT);
      if (c.isColliding.rect[RED] && !isHit) {
        player.vy *= -1;
        player.pos.y += player.vy * 2;
        isHit = true;
      }
      b2->isAlive = false;
      continue;
    }
    if (b2->pos.y < -b2->size) {
      b2->isAlive = false;
      continue;
    }
  }
  COUNT_IS_ALIVE(chainReactions, chainReactionCount);
  if (chainReactionCount == 0) {
    multiplier = 1;
  }
}

void addGameStompingBubbles() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
