#include "cglp.h"
#include "string.h"

static char *title = "ATTACK CHAIN";
static char *description = "[Tap]\n Select card";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 0;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = false};

#define ATTACKCHAIN_CARD_COUNT 6

typedef struct {
  float v;
  int type;
  bool isValid;
} AttackchainCard;
static AttackchainCard cards[ATTACKCHAIN_CARD_COUNT];

static float barY;
static float totalDamage;
static float totalDamageTicks;
static float damage;
static float damageTicks;
static int lastAttackType;
static char message[20];
static int messageColor;
static float messageTicks;
static int turn;
static int totalTurn;
static int multiplier;

static void initCards() {
  play(POWER_UP);
  int greenCount = 2;
  TIMES(ATTACKCHAIN_CARD_COUNT, i) {
    int type;
    if (rnd(0, 1) < 0.2 && greenCount > 0) {
      type = GREEN;
      greenCount--;
    } else {
      if (rnd(0, 1) < 0.5) {
        type = RED;
      } else {
        type = BLUE;
      }
    }
    cards[i].v =
        floor(rndi(3, 7) * (4 + sqrt(clamp(multiplier, 1, 20))));
    cards[i].type = type;
    cards[i].isValid = true;
  }
}

static int cardCount() {
  int v = 0;
  TIMES(ATTACKCHAIN_CARD_COUNT, i) {
    if (cards[i].isValid) {
      v++;
    }
  }
  return v;
}

static void setMessage(char *m, int cl) {
  if (m[0] == 0) {
    return;
  }
  strcpy(message, m);
  messageColor = cl;
  messageTicks = 60;
}

static void returnCards(int count, bool isBestReturner) {
  if (count == 0) {
    return;
  }
  if (isBestReturner) {
    int bi = 0;
    float bv = 0;
    TIMES(ATTACKCHAIN_CARD_COUNT, i) {
      if (!cards[i].isValid && cards[i].v > bv) {
        bv = cards[i].v;
        bi = i;
      }
    }
    cards[bi].isValid = true;
    count--;
  }
  TIMES(count, k) {
    int ivi[ATTACKCHAIN_CARD_COUNT];
    int iviCount = 0;
    TIMES(ATTACKCHAIN_CARD_COUNT, i) {
      if (!cards[i].isValid) {
        ivi[iviCount] = i;
        iviCount++;
      }
    }
    if (iviCount > 0) {
      cards[ivi[rndi(0, iviCount)]].isValid = true;
    }
  }
}

static void update() {
  // Never reads a Collision result anywhere in this file - the tapped card
  // column is computed directly from input.pos via grid math (see "i =
  // floor(...)" below), so the engine's own O(n^2) hitbox scan is pure
  // waste here.
  hasCollision = false;
  if (!ticks) {
    barY = 0;
    totalDamage = 0;
    totalDamageTicks = 0;
    damage = 0;
    damageTicks = 0;
    lastAttackType = -1;
    message[0] = 0;
    messageColor = BLACK;
    messageTicks = 0;
    turn = 0;
    totalTurn = 0;
    multiplier = 1;
    initCards();
  }
  TIMES(ATTACKCHAIN_CARD_COUNT, i) {
    AttackchainCard *c = &cards[i];
    if (!c->isValid) {
      continue;
    }
    float x = (i * 80.0) / 5 + 10;
    color = c->type;
    box(x, 90, 6, 6);
    color = BLACK;
    float offX;
    if (c->v > 99) {
      offX = x - 6;
    } else {
      offX = x - 3;
    }
    text(intToChar((int)c->v), offX, 82);
  }
  color = BLACK;
  rect(0, 75, 100, 1);
  color = RED;
  barY += (difficulty + sqrt(multiplier)) * 0.002;
  if (damageTicks <= 0 && totalDamageTicks <= 0 && barY >= 75) {
    play(EXPLOSION);
    barY = 75;
    gameOver();
  }
  rect(0, barY, 100, 1);
  color = BLACK;
  if (damageTicks <= 0 && totalDamageTicks <= 0 && input.isJustPressed) {
    int i = (int)floor(input.pos.x / (100.0 / ATTACKCHAIN_CARD_COUNT));
    if (i >= 0 && i < ATTACKCHAIN_CARD_COUNT && cards[i].isValid) {
      play(SELECT);
      barY += difficulty + sqrt(multiplier) * 0.05;
      AttackchainCard *c = &cards[i];
      c->isValid = false;
      if (turn == 0 && c->type == RED) {
        setMessage("First x1.25", RED);
        damage = c->v * 1.25;
      } else if (c->type == GREEN) {
        if (totalDamage > 99) {
          damage = 0;
          setMessage("Limit <=99", GREEN);
        } else if (totalDamage + c->v > 99) {
          damage = 99 - totalDamage;
          setMessage("Limit <=99", GREEN);
        } else {
          damage = c->v;
        }
      } else {
        damage = c->v;
      }
      damage = floor(damage);
      damageTicks = 30;
      turn++;
      lastAttackType = c->type;
      c->v = floor(c->v * 1.1 + 10);
    }
  }
  if (damageTicks > 0) {
    damageTicks -= difficulty;
    float dOfs;
    if (damage > 99) {
      dOfs = 6;
    } else {
      dOfs = 3;
    }
    text(intToChar((int)damage), 50 - dOfs, 45 + damageTicks / 2);
    if (damageTicks <= 0) {
      play(HIT);
      totalDamage += damage;
      if (totalDamage >= 100 || cardCount() == 0) {
        totalDamageTicks = 30;
      }
    }
  }
  float pctOfs;
  if (totalDamage > 99) {
    pctOfs = 12;
  } else if (totalDamage > 9) {
    pctOfs = 6;
  } else {
    pctOfs = 0;
  }
  float totalY;
  if (totalDamageTicks > 0) {
    totalY = totalDamageTicks;
  } else {
    totalY = 30;
  }
  text(intToChar((int)totalDamage), 50 - pctOfs, totalY);
  text("%", 59 + pctOfs, totalY + 3);
  if (totalDamageTicks > 0) {
    totalDamageTicks -= difficulty;
    if (totalDamageTicks <= 0) {
      play(CLICK);
      addScore(totalDamage * multiplier, 50, 30);
      if (totalDamage >= 115) {
        barY -= (totalDamage - 115) * sqrt(totalDamage - 115) /
                sqrt(multiplier);
      }
      if (barY < 0) {
        barY = 0;
      }
      int rc = 0;
      char m[20];
      m[0] = 0;
      int cl = BLACK;
      if (totalDamage >= 200) {
        strcpy(m, "AMAZING");
        rc = 3;
      } else if (totalDamage >= 150) {
        strcpy(m, "BRAVO");
        rc = 2;
      } else if (totalDamage >= 100) {
        strcpy(m, "COOL");
        rc = 1;
      }
      if (rc > 0 && lastAttackType == BLUE) {
        strcat(m, " B_Return");
        cl = BLUE;
      }
      setMessage(m, cl);
      turn = 0;
      totalTurn++;
      totalDamage = 0;
      returnCards(rc, lastAttackType == BLUE);
      if (cardCount() == 0 || totalTurn > 6) {
        totalTurn = 0;
        multiplier++;
        initCards();
      }
    }
  }
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
  if (messageTicks > 0) {
    messageTicks -= difficulty;
    color = messageColor;
    text(message, 50 - strlen(message) * 3, 70);
  }
}

void addGameAttackChain() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
