#include "cglp.h"

static char *title = "DESCENT S";
static char *description = "[Hold] Thrust up";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "l cc  ",
        "lllll ",
        "llllll",
        "      ",
        "      ",
        "      ",
    },
    {
        "  ll  ",
        "  ll  ",
        "  ll  ",
        "llllll",
        " llll ",
        "  ll  ",
    },
    {
        "ll    ",
        "  l   ",
        " l    ",
        "lll   ",
        "      ",
        "      ",
    }
};

static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

typedef struct {
  Vector pos;
  Vector vel;
  int up;
  int down;
  int nextUp;
  int nextDown;
} Ship;

typedef struct {
  Vector pos;
  float width;
  bool isAlive;
} Deck;

#define DESCENT_MAX_DECK_COUNT 10
static Deck decks[DESCENT_MAX_DECK_COUNT];
static int deckCount; // Number of active decks
static Ship ship;

// Function to shift decks in the array
static void shiftDecksDown(int removedIndex) {
  // Move all decks after the removed one down one position
  for (int i = removedIndex; i < deckCount - 1; i++) {
    decks[i] = decks[i + 1];
  }
  // Clear the last position
  decks[deckCount - 1].isAlive = false;
  // Decrement deck count
  deckCount--;
}

static void update() {
  if (!ticks) {
    // Initialize ship
    vectorSet(&ship.pos, 9, 9);
    vectorSet(&ship.vel, 0.2, 0.2);
    ship.up = -2;
    ship.down = 2;
    ship.nextUp = -3;
    ship.nextDown = 3;
    
    // Initialize decks
    INIT_UNALIVED_ARRAY(decks);
    deckCount = 0;
    
    // First deck
    vectorSet(&decks[0].pos, 30, 50);
    decks[0].width = 40;
    decks[0].isAlive = true;
    deckCount++;
    
    // Second deck
    vectorSet(&decks[1].pos, 50, 75);
    decks[1].width = 35;
    decks[1].isAlive = true;
    deckCount++;
    
    // Third deck
    vectorSet(&decks[2].pos, 70, 99);
    decks[2].width = 30;
    decks[2].isAlive = true;
    deckCount++;
  }
  
  // Update ship velocity
  ship.vel.y += input.isPressed ? ship.up * 0.005 : ship.down * 0.005;
  
  if (input.isJustPressed) {
    play(COIN);
  }
  
  if (input.isJustReleased) {
    play(LASER);
  }
  
  // Update ship position
  ship.pos.y += ship.vel.y;
  
  // Draw velocity gauge
  if (ship.vel.y > 1) {
    color = RED;
  } else if (ship.vel.y > 0.6) {
    color = YELLOW;
  } else {
    color = BLUE;
  }
  rect(1, 20, 3, ship.vel.y * 20);
  
  color = RED;
  rect(0, 40, 5, 1);
  
  // Camera scrolling
  float scrY = 0;
  if (ship.pos.y > 19) {
    scrY = (19 - ship.pos.y) * 0.2;
  } else if (ship.pos.y < 9) {
    scrY = (9 - ship.pos.y) * 0.2;
  }
  ship.pos.y += scrY;
  
  // Process decks
  for (int i = 0; i < deckCount; i++) {
    // Update deck position
    decks[i].pos.x -= ship.vel.x;
    decks[i].pos.y += scrY;
    
    // Draw deck with appropriate color (first deck is black, others are light black)
    color = (i == 0) ? BLACK : LIGHT_BLACK;
    rect(decks[i].pos.x, decks[i].pos.y, decks[i].width, 3);
    
    // Draw finish line for first deck
    if (i == 0) {
      color = YELLOW;
      rect(0, decks[i].pos.y + 3, decks[i].pos.x + decks[i].width, 2);
      rect(decks[i].pos.x + decks[i].width, 0, 1, decks[i].pos.y + 5);
    }
  }
  
  // Check collisions
  color = BLACK;
  Collision collision = character("a", VEC_XY(ship.pos));
  
  if (collision.isColliding.rect[BLACK]) {
    if (ship.vel.y > 1) {
      // Crash due to high speed
      play(EXPLOSION);
      gameOver();
    } else {
      // Successfully landed
      if (deckCount > 0) {
        play(POWER_UP);
        addScore(floorf(decks[0].pos.x + decks[0].width - ship.pos.x + 1), VEC_XY(ship.pos));
        
        // Remove the first deck (shift all decks down)
        shiftDecksDown(0);
        
        // Add a new deck at the end
        if (deckCount > 0) {
          // Based on the last deck
          Deck *lastDeck = &decks[deckCount - 1];
          
          // Add new deck at the end
          vectorSet(&decks[deckCount].pos, 
                  lastDeck->pos.x + lastDeck->width * 0.7 + rnd(0, 30), 
                  lastDeck->pos.y + rnd(20, 40));
          decks[deckCount].width = rnd(25, 50);
          decks[deckCount].isAlive = true;
          deckCount++;
        } else {
          // No decks left, create one based on ship position
          vectorSet(&decks[0].pos, ship.pos.x + 50, ship.pos.y + 30);
          decks[0].width = rnd(25, 50);
          decks[0].isAlive = true;
          deckCount = 1;
        }
        
        // Update ship parameters
        ship.up = ship.nextUp;
        ship.down = ship.nextDown;
        ship.nextUp = floorf(-difficulty * 4 + rnd(-difficulty * 3, difficulty * 3));
        ship.nextDown = floorf(difficulty * 4 + rnd(-difficulty * 2, difficulty * 2));
        vectorSet(&ship.vel, difficulty * 0.2, 0);
      }
    }
  }
  
  // Check failure conditions
  if (collision.isColliding.rect[YELLOW] || ship.pos.y < 0) {
    play(EXPLOSION);
    gameOver();
  }
  
  // Draw UI elements
  // Create a buffer with sufficient space
  char buffer[16];
  
  // Thrust up indicator
  color = input.isPressed ? YELLOW : CYAN;
  characterOptions.isMirrorY = true;
  character("b", 60, 10);
  characterOptions.isMirrorY = false;
  
  color = BLACK;
  snprintf(buffer, sizeof(buffer), "%dm/s", -ship.up);
  text(buffer, 76 - (-ship.up > 9 ? 6 : 0), 10);
  character("c", 97, 9);
  
  // Thrust down indicator
  color = !input.isPressed ? YELLOW : CYAN;
  character("b", 60, 17);
  
  color = BLACK;
  snprintf(buffer, sizeof(buffer), "%dm/s", ship.down);
  text(buffer, 76 - (ship.down > 9 ? 6 : 0), 17);
  character("c", 97, 16);
  
  // Next indicators
  text("NEXT", 70, 24);
  
  color = CYAN;
  characterOptions.isMirrorY = true;
  character("b", 60, 31);
  characterOptions.isMirrorY = false;
  
  color = BLACK;
  snprintf(buffer, sizeof(buffer), "%dm/s", abs(ship.nextUp));
  text(buffer, 76 - (-ship.nextUp > 9 ? 6 : 0), 31);
  character("c", 97, 30);
  
  color = CYAN;
  character("b", 60, 38);
  
  color = BLACK;
  snprintf(buffer, sizeof(buffer), "%dm/s", ship.nextDown);
  text(buffer, 76 - (ship.nextDown > 9 ? 6 : 0), 38);
  character("c", 97, 37);
}

void addGameDescents() {
  addGame(title, description, characters, charactersCount, options, false, update);
}