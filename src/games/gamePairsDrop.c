#include "cglp.h"

static char *title = "PAIRS DROP";
static char *description = "[Tap] Open";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "l  l  ",
        "l l l ",
        "l l l ",
        "l l l ",
        "l  l  ",
            "      ",
},
    {
        "l l l ",
        " l l  ",
        "l l l ",
        " l l  ",
        "l l l ",
            "      ",
},
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 101, .isDarkColor = false};

typedef struct {
  Vector pos;
  int n;
  bool isOpen;
  float vy;
  bool isAlive;
} Card;
#define MAX_CARD_COUNT 128
static Card cards[MAX_CARD_COUNT];
static int cardIndex;

static Vector nextCardPos;
static float scr;
#define MAX_OPENED_COUNT 2
static int openedCardIndices[MAX_OPENED_COUNT];
static int openedCardCount;
static float openedTicks;
static int multiplier;

static void fallCards(float px, float py) {
  FOR_EACH(cards, i) {
    ASSIGN_ARRAY_ITEM(cards, i, Card, c);
    SKIP_IS_NOT_ALIVE(c);
    if (c->pos.x == px && c->pos.y >= py) {
      c->vy = 1;
    }
  }
}

static void addCards() {
  int ns[26];
  TIMES(26, i) { ns[i] = i % 13; }
  TIMES(99, k) {
    int i1 = rndi(0, 26);
    int i2 = rndi(0, 26);
    int tn = ns[i1];
    ns[i1] = ns[i2];
    ns[i2] = tn;
  }
  TIMES(26, i) {
    ASSIGN_ARRAY_ITEM(cards, cardIndex, Card, nc);
    nc->pos = nextCardPos;
    nc->n = ns[i];
    nc->isOpen = false;
    nc->vy = 0;
    nc->isAlive = true;
    cardIndex = wrap(cardIndex + 1, 0, MAX_CARD_COUNT);
    nextCardPos.x += 10;
    if (nextCardPos.x > 80) {
      nextCardPos.x = 20;
      nextCardPos.y -= 12;
    }
  }
}

static void update() {
  // Never reads a Collision result anywhere in this file - a card tap is
  // a direct fabs() distance check against input.pos, so the engine's own
  // O(n^2) hitbox scan is pure waste here.
  hasCollision = false;
  if (!ticks) {
    INIT_UNALIVED_ARRAY(cards);
    cardIndex = 0;
    vectorSet(&nextCardPos, 20, -6);
    scr = 0;
    openedCardCount = 0;
    openedTicks = 0;
    multiplier = 1;
  }
  float minY = 99;
  float maxY = 0;
  openedTicks--;
  if (openedTicks == 0) {
    TIMES(openedCardCount, i) { cards[openedCardIndices[i]].isOpen = false; }
    openedCardCount = 0;
  }
  bool isOpening = false;
  FOR_EACH(cards, i) {
    ASSIGN_ARRAY_ITEM(cards, i, Card, c);
    SKIP_IS_NOT_ALIVE(c);
    int cl;
    if (c->isOpen) {
      if (openedTicks > 0) {
        cl = GREEN;
      } else {
        cl = CYAN;
      }
    } else {
      cl = BLUE;
    }
    color = cl;
    rect(c->pos.x - 4, c->pos.y - 5, 9, 11);
    color = WHITE;
    rect(c->pos.x - 3, c->pos.y - 4, 7, 9);
    color = cl;
    c->pos.y += scr + c->vy;
    if (c->vy > 0) {
      c->vy += 0.1;
    }
    if (c->isOpen) {
      if (c->n == 0) {
        text("A", c->pos.x, c->pos.y);
      } else if (c->n < 9) {
        text(intToChar(c->n + 1), c->pos.x, c->pos.y);
      } else if (c->n == 9) {
        character("a", c->pos.x, c->pos.y);
      } else {
        char fc[2];
        if (c->n == 10) {
          fc[0] = 'J';
        } else if (c->n == 11) {
          fc[0] = 'Q';
        } else {
          fc[0] = 'K';
        }
        fc[1] = 0;
        text(fc, c->pos.x, c->pos.y);
      }
    } else {
      character("b", c->pos.x, c->pos.y);
      if (c->vy == 0 && input.isJustPressed && fabs(c->pos.x - input.pos.x) < 5 &&
          fabs(c->pos.y - input.pos.y) < 6) {
        play(HIT);
        if (openedTicks >= 0) {
          TIMES(openedCardCount, k) { cards[openedCardIndices[k]].isOpen = false; }
          openedCardCount = 0;
          openedTicks = 0;
        }
        isOpening = true;
        c->isOpen = true;
        if (openedCardCount < MAX_OPENED_COUNT) {
          openedCardIndices[openedCardCount] = i;
          openedCardCount++;
        }
      }
    }
    if (c->pos.y < minY) {
      minY = c->pos.y;
    }
    if (c->pos.y > maxY) {
      maxY = c->pos.y;
    }
    if (c->vy == 0 && c->pos.y > 95) {
      play(EXPLOSION);
      color = RED;
      text("X", c->pos.x, c->pos.y);
      gameOver();
    }
    if (c->pos.y > 99) {
      play(COIN);
      addScore(multiplier, c->pos.x, c->pos.y);
      multiplier++;
      c->isAlive = false;
      continue;
    }
  }
  if (openedTicks < 0 && openedCardCount == 2) {
    if (cards[openedCardIndices[0]].n == cards[openedCardIndices[1]].n) {
      play(POWER_UP);
      fallCards(cards[openedCardIndices[0]].pos.x, cards[openedCardIndices[0]].pos.y);
      fallCards(cards[openedCardIndices[1]].pos.x, cards[openedCardIndices[1]].pos.y);
      openedCardCount = 0;
      multiplier = 1;
    } else {
      play(LASER);
    }
    openedTicks = 60;
  }
  nextCardPos.y += scr;
  float openBonus;
  if (isOpening) {
    openBonus = 0.15;
  } else {
    openBonus = 0;
  }
  scr += (difficulty * 0.0015 - scr) * 0.2 + openBonus;
  if (maxY < 50) {
    scr += (50 - maxY) * 0.01;
  }
  if (minY > -6) {
    addCards();
  }
}

void addGamePairsDrop() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
