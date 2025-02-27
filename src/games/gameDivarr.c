#include "cglp.h"

static char *title = "DIVARR";
static char *description = "[Tap]\n Fire / Split";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
          "yy    ",
          " yy   ",
          "llllll",
          "llllll",
          " yy   ",
          "yy    ",
      },
      {
          "  rr  ",
          "  ll  ",
          " llll ",
          "  ll  ",
          "y ll y",
          "yyyyyy",
      },
      {
          "   rr ",
          "  p   ",
          " llll ",
          "ll lll",
          "l llll",
          " llll ",
      },
      {
          "      ",
          "  yy  ",
          " yggy ",
          "  bb  ",
          " y  y ",
          "      ",
      }};
static int charactersCount = 4;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = false};

typedef struct {
  Vector pos;
  int angle;
  int cc;  // Collision counter
  bool isAlive;
} Shot;

#define DIVARR_MAX_SHOT_COUNT 32
static Shot shots[DIVARR_MAX_SHOT_COUNT];
static int shotIndex;

typedef struct {
  Vector pos;
  Vector vel;
  bool isHuman;
  bool isAlive;
} Falling;

#define DIVARR_MAX_FALLING_COUNT 64
static Falling fallings[DIVARR_MAX_FALLING_COUNT];
static int fallingIndex;

static int fallingAddingTicks;
static int humanAddingCount;
static int firstHumanCount;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(shots);
    shotIndex = 0;
    INIT_UNALIVED_ARRAY(fallings);
    fallingIndex = 0;
    fallingAddingTicks = 0;
    humanAddingCount = 3;
    firstHumanCount = 120;
  }
  
  const float df = sqrtf(difficulty);
  rect(0, 90, 99, 9);
  // Reset rotation before drawing static characters
  characterOptions.rotation = 0;
  character("b", 50, 87);
  
  if (input.isJustPressed) {
    COUNT_IS_ALIVE(shots, sc);
    if (sc == 0) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, s);
      s->pos.x = 50;
      s->pos.y = 85;
      s->angle = 0;  // Start with upward direction (0 = up in this version)
      s->cc = 0;
      s->isAlive = true;
      shotIndex = wrap(shotIndex + 1, 0, DIVARR_MAX_SHOT_COUNT);
    } else {
      play(SELECT);
      COUNT_IS_ALIVE(shots, sc);
      
      // Create a temporary array to track which shots need to be split
      bool splitShot[DIVARR_MAX_SHOT_COUNT] = {false};
      int splitCount = 0;
      
      // Mark active shots for splitting
      FOR_EACH(shots, i) {
        ASSIGN_ARRAY_ITEM(shots, i, Shot, s);
        if (s->isAlive) {
          splitShot[i] = true;
          splitCount++;
        }
      }
      
      // Now split the marked shots
      FOR_EACH(shots, i) {
        ASSIGN_ARRAY_ITEM(shots, i, Shot, s);
        if (!splitShot[i]) continue;
        
        // Keep track of the original position and angle
        Vector pos = s->pos;
        int angle = s->angle;
        
        // Deactivate the original
        s->isAlive = false;
        
        if (sc < 16) {
          // Add two new shots for each existing shot
          ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, ns1);
          vectorSet(&ns1->pos, pos.x, pos.y);
          ns1->angle = wrap(angle - 1, 0, 4);
          ns1->cc = 0;
          ns1->isAlive = true;
          shotIndex = wrap(shotIndex + 1, 0, DIVARR_MAX_SHOT_COUNT);
          
          ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, ns2);
          vectorSet(&ns2->pos, pos.x, pos.y);
          ns2->angle = wrap(angle + 1, 0, 4);
          ns2->cc = 0;
          ns2->isAlive = true;
          shotIndex = wrap(shotIndex + 1, 0, DIVARR_MAX_SHOT_COUNT);
        } else {
          // If max shots reached, just rotate
          ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, ns);
          vectorSet(&ns->pos, pos.x, pos.y);
          ns->angle = wrap(angle + 1, 0, 4);
          ns->cc = 0;
          ns->isAlive = true;
          shotIndex = wrap(shotIndex + 1, 0, DIVARR_MAX_SHOT_COUNT);
        }
      }
    }
  }
  
  // Update shots
  FOR_EACH(shots, i) {
    ASSIGN_ARRAY_ITEM(shots, i, Shot, s);
    SKIP_IS_NOT_ALIVE(s);
    
    // Define direction vectors: 0=up, 1=right, 2=down, 3=left
    const Vector w[4] = {
      {0, -1}, {1, 0}, {0, 1}, {-1, 0}
    };
    
    vectorAdd(&s->pos, w[s->angle].x * df, w[s->angle].y * df);
    
    // Check if shot is out of bounds
    if (s->pos.x < -2 || s->pos.y < -2 || 
        s->pos.x > 104 || s->pos.y > 90) {
      s->isAlive = false;
      continue;
    }
    
    // Set rotation for the rocket character
    // The rocket character (first character) initially faces right
    // So we need to rotate it -90 degrees (3 in the 4-direction system) to make it face up
    characterOptions.rotation = (s->angle + 3) % 4;
    Collision coll = character("a", s->pos.x, s->pos.y);
    // Reset rotation for other characters
    characterOptions.rotation = 0;
    if (coll.isColliding.character['a']) {
      s->cc++;
      if (s->cc > 8) {
        s->isAlive = false;
        continue;
      }
    } else {
      s->cc = 0;
    }
  }
  
  // Update fallings
  fallingAddingTicks--;
  if (fallingAddingTicks < 0) {
    humanAddingCount--;
    ASSIGN_ARRAY_ITEM(fallings, fallingIndex, Falling, f);
    f->pos.x = rnd(9, 90);
    f->pos.y = -3;
    vectorSet(&f->vel, 0, 0);
    f->isHuman = humanAddingCount < 0;
    f->isAlive = true;
    
    if (f->isHuman && firstHumanCount > 0) {
      f->pos.x = 25;
    }
    
    Vector target;
    target.x = clamp(f->pos.x + rnd(-clamp(df * 10, 0, 50), clamp(df * 10, 0, 50)), 9, 90);
    target.y = 90;
    
    float angle = angleTo(&f->pos, target.x, target.y);
    float speed = (1 + rnd(0, df)) * 0.1;
    if (f->isHuman) speed *= 1.6;
    
    addWithAngle(&f->vel, angle, speed);
    
    fallingIndex = wrap(fallingIndex + 1, 0, DIVARR_MAX_FALLING_COUNT);
    
    if (humanAddingCount < 0) {
      humanAddingCount = rndi(3, 15);
    }
    
    fallingAddingTicks += rnd(60, 80) / difficulty;
  }
  
  FOR_EACH(fallings, i) {
    ASSIGN_ARRAY_ITEM(fallings, i, Falling, f);
    SKIP_IS_NOT_ALIVE(f);
    
    vectorAdd(&f->pos, f->vel.x, f->vel.y);
    
    if (f->isHuman && (firstHumanCount > 0 || f->pos.y < 8)) {
      firstHumanCount--;
      text("Don't", f->pos.x - 10, f->pos.y + 6);
      text("hit me !", f->pos.x - 15, f->pos.y + 12);
    }
    
    if (f->isHuman && f->pos.y < 20) {
      color = LIGHT_BLACK;
      character("d", f->pos.x, f->pos.y);
      color = BLACK;
    } else {
      Collision coll = character(f->isHuman ? "d" : "c", f->pos.x, f->pos.y);
      if (coll.isColliding.character['a']) {
        if (f->isHuman) {
          play(RANDOM); // closest to "lucky" in JS
          color = RED;
          text("X", f->pos.x, f->pos.y);
          color = BLACK;
          gameOver();
        } else {
          play(HIT);
          color = RED;
          particle(f->pos.x, f->pos.y, 20, 2, 0, M_PI * 2);
          color = BLACK;
          COUNT_IS_ALIVE(shots, sc);
          addScore(sc, f->pos.x, f->pos.y);
        }
        f->isAlive = false;
        continue;
      }
    }
    
    if (f->pos.y > 88) {
      if (f->isHuman) {
        play(POWER_UP);
        addScore(100, f->pos.x, f->pos.y);
      } else {
        play(EXPLOSION);
        color = RED;
        text("X", f->pos.x, f->pos.y);
        color = BLACK;
        gameOver();
      }
      f->isAlive = false;
      continue;
    }
  }
}

void addGameDivarr() {
  addGame(title, description, characters, charactersCount, options, false, update);
}