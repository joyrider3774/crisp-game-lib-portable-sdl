#include "cglp.h"
#include "string.h"

static char *title = "TONGUE HUNTER";
static char *description = "[Hold] Extend tongue";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    " llll ",
    "llllll",
    "llllll",
    "llllll",
    "llllll",
    " l  l ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};

typedef struct {
  float x;
  float y;
} Frog;
static Frog frog;

static float tongueLength;
static float tongueAngle;
static int combo;
static float frogSquash;

typedef struct {
  float x;
  float y;
  float vx;
  float squash;
  bool isAlive;
} Prey;
// Worked out worst-case concurrency around difficulty 3-4 (spawn interval
// clamps to 20 ticks while lifetime is still ~220 ticks) at roughly 11-12
// concurrent prey - sized well above that with headroom.
#define TONGUEHUNTER_MAX_PREY_COUNT 48
static Prey preys[TONGUEHUNTER_MAX_PREY_COUNT];
static int preyIndex;

typedef struct {
  float x;
  float y;
  float vy;
  float rot;
  bool isAlive;
} Hazard;
// Concurrency peaks around difficulty 1 at roughly 2.5 - generous headroom.
#define TONGUEHUNTER_MAX_HAZARD_COUNT 16
static Hazard hazards[TONGUEHUNTER_MAX_HAZARD_COUNT];
static int hazardIndex;

typedef struct {
  float x;
  float y;
  float life;
  bool isAlive;
} Trail;
// One trail spawned per tick while the tongue is extended (life 1.0,
// decays 0.15/tick -> ~7 tick lifetime) - up to ~7 concurrent, doubled here.
#define TONGUEHUNTER_MAX_TRAIL_COUNT 16
static Trail trails[TONGUEHUNTER_MAX_TRAIL_COUNT];
static int trailIndex;

static void update() {
  if (!ticks) {
    frog.x = 50;
    frog.y = 90;
    tongueLength = 0;
    tongueAngle = -M_PI_2;
    INIT_UNALIVED_ARRAY(preys);
    preyIndex = 0;
    INIT_UNALIVED_ARRAY(hazards);
    hazardIndex = 0;
    combo = 1;
    INIT_UNALIVED_ARRAY(trails);
    trailIndex = 0;
    frogSquash = 1;
  }

  float spawnRate = 60 / difficulty;
  if (spawnRate < 20) {
    spawnRate = 20;
  }
  if (ticks % (int)floor(spawnRate) == 0) {
    bool side = rnd(0, 1) < 0.5;
    ASSIGN_ARRAY_ITEM(preys, preyIndex, Prey, p);
    if (side) {
      p->x = -5;
      p->vx = rnd(0.3, 0.6);
    } else {
      p->x = 105;
      p->vx = rnd(-0.6, -0.3);
    }
    p->y = rnd(20, 60);
    p->squash = 1;
    p->isAlive = true;
    preyIndex = wrap(preyIndex + 1, 0, TONGUEHUNTER_MAX_PREY_COUNT);
  }

  int hazardMod = (int)floor(90 / sqrt(difficulty));
  if (hazardMod < 1) {
    hazardMod = 1;
  }
  if (ticks % hazardMod == 0) {
    ASSIGN_ARRAY_ITEM(hazards, hazardIndex, Hazard, h);
    h->x = rnd(10, 90);
    h->y = -5;
    h->vy = 0.5 * rnd(1, difficulty);
    h->rot = 0;
    h->isAlive = true;
    hazardIndex = wrap(hazardIndex + 1, 0, TONGUEHUNTER_MAX_HAZARD_COUNT);
  }

  if (input.isJustPressed) {
    frogSquash = 1.3;
  }
  frogSquash += (1 - frogSquash) * 0.15;

  float tongueEndX = frog.x + cos(tongueAngle) * tongueLength;
  float tongueEndY = frog.y + sin(tongueAngle) * tongueLength;

  Prey *nearestPrey = NULL;
  float nearestDist = 999;
  FOR_EACH(preys, npi) {
    ASSIGN_ARRAY_ITEM(preys, npi, Prey, p);
    SKIP_IS_NOT_ALIVE(p);
    float dx = p->x - tongueEndX;
    float dy = p->y - tongueEndY;
    float d = sqrt(dx * dx + dy * dy);
    if (d < nearestDist) {
      nearestDist = d;
      nearestPrey = p;
    }
  }

  if (input.isPressed && nearestPrey != NULL) {
    float ta = atan2(nearestPrey->y - frog.y, nearestPrey->x - frog.x);
    if (fabs(ta - tongueAngle) < 0.1) {
      tongueAngle = ta;
      tongueLength += 8;
    } else if (ta < tongueAngle) {
      tongueAngle -= 0.1;
    } else {
      tongueAngle += 0.1;
    }
  }
  tongueLength = clamp(tongueLength - 5, 0, 80);

  if (tongueLength > 10) {
    ASSIGN_ARRAY_ITEM(trails, trailIndex, Trail, nt);
    nt->x = tongueEndX;
    nt->y = tongueEndY;
    nt->life = 1;
    nt->isAlive = true;
    trailIndex = wrap(trailIndex + 1, 0, TONGUEHUNTER_MAX_TRAIL_COUNT);
  }

  FOR_EACH(trails, ti) {
    ASSIGN_ARRAY_ITEM(trails, ti, Trail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->life -= 0.15;
    if (t->life <= 0) {
      t->isAlive = false;
      continue;
    }
  }

  FOR_EACH(preys, pui) {
    ASSIGN_ARRAY_ITEM(preys, pui, Prey, p);
    SKIP_IS_NOT_ALIVE(p);
    p->x += p->vx * (1 + difficulty * 0.05);
    p->squash += (1 - p->squash) * 0.1;
    if (p->x <= -10 || p->x >= 110) {
      p->isAlive = false;
      continue;
    }
  }

  FOR_EACH(hazards, hui) {
    ASSIGN_ARRAY_ITEM(hazards, hui, Hazard, h);
    SKIP_IS_NOT_ALIVE(h);
    h->y += h->vy;
    h->rot += h->vy * 0.1;
    if (h->y >= 110) {
      h->isAlive = false;
      continue;
    }
  }

  // 1. Draw trails (decorative, no collision)
  color = LIGHT_YELLOW;
  FOR_EACH(trails, ti2) {
    ASSIGN_ARRAY_ITEM(trails, ti2, Trail, t);
    SKIP_IS_NOT_ALIVE(t);
    box(t->x, t->y, 4 * t->life, 4 * t->life);
  }

  // 2. Draw prey bodies FIRST (collision target for tongue)
  color = BLACK;
  FOR_EACH(preys, pdi) {
    ASSIGN_ARRAY_ITEM(preys, pdi, Prey, p);
    SKIP_IS_NOT_ALIVE(p);
    float w = 5 * p->squash;
    float h = 5 / p->squash;
    box(p->x, p->y, w, h);
  }

  // 3. Draw prey eyes (decorative, after body)
  FOR_EACH(preys, pei) {
    ASSIGN_ARRAY_ITEM(preys, pei, Prey, p);
    SKIP_IS_NOT_ALIVE(p);
    float eyeOffsetX;
    if (p->vx > 0) {
      eyeOffsetX = 1.5;
    } else {
      eyeOffsetX = -1.5;
    }
    color = WHITE;
    box(p->x + eyeOffsetX, p->y - 1, 2, 2);
    color = BLUE;
    float pupilOfs;
    if (p->vx > 0) {
      pupilOfs = 0.5;
    } else {
      pupilOfs = -0.5;
    }
    box(p->x + eyeOffsetX + pupilOfs, p->y - 1, 1, 1);
  }

  // 4. Draw frog
  color = GREEN;
  // Vircon32 port note: upstream draws this with a scaleX/scaleY squash
  // option this engine's character() has no equivalent for - drawn at
  // normal scale instead (cosmetic only, frog has no collision here).
  character("a", frog.x, frog.y);

  // 5. Draw frog eyes looking at prey
  if (nearestPrey != NULL) {
    float eyeDx;
    if (nearestPrey->x > frog.x) {
      eyeDx = 1;
    } else {
      eyeDx = -1;
    }
    float eyeDy;
    if (nearestPrey->y < frog.y) {
      eyeDy = -0.5;
    } else {
      eyeDy = 0.5;
    }
    color = WHITE;
    box(frog.x - 2, frog.y - 2, 2, 2);
    box(frog.x + 2, frog.y - 2, 2, 2);
    color = BLUE;
    box(frog.x - 2 + eyeDx * 0.5, frog.y - 2 + eyeDy, 1, 1);
    box(frog.x + 2 + eyeDx * 0.5, frog.y - 2 + eyeDy, 1, 1);
  }

  // 6. Draw tongue and check collision with prey (black)
  if (tongueLength > 5) {
    color = LIGHT_RED;
    thickness = 2;
    line(frog.x, frog.y, tongueEndX, tongueEndY);

    float tipPulse = 1 + sin(ticks * 0.3) * 0.2;
    color = CYAN;
    Collision tipCol = box(tongueEndX, tongueEndY, 6 * tipPulse, 6 * tipPulse);

    if (tipCol.isColliding.rect[BLACK]) {
      play(COIN);
      addScore(ceil(tongueLength) * combo, tongueEndX, tongueEndY);
      combo++;

      color = YELLOW;
      particle(tongueEndX, tongueEndY, 15, 2, 0, M_PI * 2);

      Prey *toRemove = NULL;
      float minD = 999;
      FOR_EACH(preys, ri) {
        ASSIGN_ARRAY_ITEM(preys, ri, Prey, p);
        SKIP_IS_NOT_ALIVE(p);
        float dx = p->x - tongueEndX;
        float dy = p->y - tongueEndY;
        float d = sqrt(dx * dx + dy * dy);
        if (d < minD) {
          minD = d;
          toRemove = p;
        }
      }
      if (toRemove != NULL) {
        toRemove->isAlive = false;
      }
    }
  } else {
    combo = 1;
  }

  bool isExtended = tongueLength > 5;

  // 7. Draw hazards and check collision with tongue
  FOR_EACH(hazards, hdi) {
    ASSIGN_ARRAY_ITEM(hazards, hdi, Hazard, h);
    SKIP_IS_NOT_ALIVE(h);
    float breathe = 1 + sin(ticks * 0.15 + h->x) * 0.1;
    float size = 8 * breathe;

    color = RED;
    thickness = size * 0.7;
    bar(h->x, h->y, size, h->rot);

    color = WHITE;
    box(h->x - 2, h->y, 2, 2);
    box(h->x + 2, h->y, 2, 2);
    color = PURPLE;
    box(h->x - 2, h->y + 0.5, 1, 1);
    box(h->x + 2, h->y + 0.5, 1, 1);

    color = RED;
    Collision hCol = box(h->x, h->y, size, size);
    if (isExtended && hCol.isColliding.rect[LIGHT_RED]) {
      play(EXPLOSION);
      color = RED;
      particle(h->x, h->y, 30, 3, 0, M_PI * 2);
      gameOver();
    }
  }

  color = BLACK;
  char comboText[16];
  strcpy(comboText, "x");
  strcat(comboText, intToChar(combo));
  text(comboText, 3, 9);
}

void addGameTongueHunter() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
