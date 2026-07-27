#include "cglp.h"

static char *title = "ZARTAN";
static char *description = "[Hold]\n Hold rope";

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

typedef struct {
  Vector pos;
  bool isAlive;
} Pile;
#define ZARTAN_MAX_PILE_COUNT 32
static Pile piles[ZARTAN_MAX_PILE_COUNT];
static int pileIndex;

static Vector p;
static Vector v;
// Port note: upstream's `anchor`/`nearest` are live references directly into
// Vector objects living inside the `piles` array. There are no shared
// references across fixed arrays that survive index recycling here, so the
// currently-grabbed pile is tracked by index instead.
// ZARTAN_MAX_PILE_COUNT is sized well above the handful of piles ever
// concurrently on screen (piles spawn no faster than every 9 ticks and take
// a while to cross the whole view before being removed), so the ring-buffer
// write index can never wrap back onto a still-anchored pile's slot before
// it's naturally released (by input release, by the rope breaking off-screen,
// or by grabbing a different pile).
static bool hasAnchor;
static int anchorIndex;
static float nextAnchorDist;

static void update() {
  // Port note: nothing in this game ever reads a Collision result (game-over
  // is plain y-position math, and the "nearest pile" search is plain distance
  // math over the piles array) - skip the engine's per-frame hitbox
  // bookkeeping entirely since every box()/line() call below would otherwise
  // register hitboxes purely for the checkHitBox() machinery to never be
  // asked about. Restored automatically when the next game starts, via
  // resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    INIT_UNALIVED_ARRAY(piles);
    pileIndex = 0;
    nextAnchorDist = 9;
    vectorSet(&p, 99, 9);
    vectorSet(&v, 0, 0);
    hasAnchor = false;
    anchorIndex = -1;
  }
  float scr;
  if (p.x > 30) {
    scr = (p.x - 30) * 0.1 + difficulty * 0.1;
  } else {
    scr = difficulty * 0.1;
  }
  score += scr;
  p.x -= scr;
  v.y += 0.02;
  if (v.y < 0 && p.y < 0) {
    v.y *= -1;
  }
  if (p.y > 99) {
    play(RANDOM);
    gameOver();
  }
  vectorMul(&v, 0.99);
  vectorAdd(&p, v.x, v.y);
  color = GREEN;
  box(VEC_XY(p), 7, 7);

  float minDist = 99;
  int nearestIndex = -1;
  FOR_EACH(piles, mi) {
    ASSIGN_ARRAY_ITEM(piles, mi, Pile, m);
    SKIP_IS_NOT_ALIVE(m);
    float dist = fabs(m->pos.y - p.y);
    if (m->pos.x > p.x && dist < minDist) {
      minDist = dist;
      nearestIndex = mi;
    }
  }
  color = CYAN;
  if (nearestIndex >= 0) {
    Pile *nearest = &piles[nearestIndex];
    box(VEC_XY(nearest->pos), 9, 9);
    if (input.isJustPressed) {
      play(SELECT);
      hasAnchor = true;
      anchorIndex = nearestIndex;
    }
  }
  if (input.isPressed && hasAnchor) {
    Pile *a = &piles[anchorIndex];
    Vector d;
    vectorSet(&d, a->pos.x - p.x, a->pos.y - p.y);
    vectorMul(&d, 1.0 / 199);
    vectorAdd(&v, d.x, d.y);
    line(VEC_XY(p), VEC_XY(a->pos));
    if (a->pos.x < 0) {
      hasAnchor = false;
    }
  }
  if (input.isJustReleased) {
    hasAnchor = false;
  }
  nextAnchorDist -= scr;
  if (nextAnchorDist < 0) {
    nextAnchorDist += rnd(9, 66);
    ASSIGN_ARRAY_ITEM(piles, pileIndex, Pile, np);
    vectorSet(&np->pos, 99, rnd(0, 66));
    np->isAlive = true;
    pileIndex = wrap(pileIndex + 1, 0, ZARTAN_MAX_PILE_COUNT);
  }
  color = BLACK;
  FOR_EACH(piles, pileIdx) {
    ASSIGN_ARRAY_ITEM(piles, pileIdx, Pile, pl);
    SKIP_IS_NOT_ALIVE(pl);
    pl->pos.x -= scr;
    box(VEC_XY(pl->pos), 5, 5);
    if (pl->pos.x <= 0) {
      pl->isAlive = false;
      continue;
    }
  }
}

void addGameZartan() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
