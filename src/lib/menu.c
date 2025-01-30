#include "menu.h"

#include "cglp.h"

#define MAX_GAME_COUNT 200
#define MAX_GAMES_PER_PAGE 15
#define KEY_REPEAT_DURATION 30
int gameCount = 0;
static Game games[MAX_GAME_COUNT];
static int gameIndex = 2;
static int keyRepeatTicks = 0;

static void update() {
  color = BLUE;
  text("[A]      [B]", 3, 3);
  color = BLACK;
  text("   Select   Down", 3, 3);

  if (input.b.isPressed || input.down.isPressed || input.up.isPressed) {
    keyRepeatTicks++;
  } else {
    keyRepeatTicks = 0;
  }
  if (input.b.isJustPressed || input.down.isJustPressed ||
      (keyRepeatTicks > KEY_REPEAT_DURATION &&
       (input.b.isPressed || input.down.isPressed))) {
    gameIndex++;
    while(games[gameIndex].update == NULL)
    {
      gameIndex++;
      gameIndex = wrap(gameIndex, 1, gameCount);
    }
    if (keyRepeatTicks > KEY_REPEAT_DURATION) {
      keyRepeatTicks = KEY_REPEAT_DURATION / 3 * 2;
    }
  }
  if (input.up.isJustPressed ||
      (keyRepeatTicks > KEY_REPEAT_DURATION && input.up.isPressed)) {
    gameIndex--;
    while(games[gameIndex].update == NULL)
    {
      gameIndex--;
      gameIndex = wrap(gameIndex, 1, gameCount);
    }
    if (keyRepeatTicks > KEY_REPEAT_DURATION) {
      keyRepeatTicks = KEY_REPEAT_DURATION / 3 * 2;
    }
  }

  
  // Calculate current page and starting index
  int currentGames = 0;
  int page = 0;
  int startIndex = 1; // Start from 1 to skip first game

  // Find which page we're on and the starting index
  int tempIndex = 1; // Start from 1
  int gamesPerPage = page == 0 ? MAX_GAMES_PER_PAGE : MAX_GAMES_PER_PAGE - 1;
  while (tempIndex + gamesPerPage <= gameIndex) {
      tempIndex += gamesPerPage;
      startIndex = tempIndex;
      page++;
  }

  // Draw the games for current page
  currentGames = gamesPerPage;
  for (int i = 0; i < currentGames; i++) {
      int gamePos = startIndex + i;
      if (gamePos >= gameCount) break;
      
      float y = (i + 1) * 6; // Always offset by one line
      
      // Draw selection cursor
      if (gamePos == gameIndex) {
          color = BLUE;
          text(">", 3, y + 3);
      }
    
      // Offset normal games vs category
      // i assume games without update function are category
      int offset = 0;
      color = BLACK;
      if(games[gamePos].update == NULL)
      {
        color = RED;
        offset = -9;
      }
      // Draw game title
      text(games[gamePos].title, 9 + offset, y + 3);
  }

  if (input.a.isJustPressed) {
    if(games[gameIndex].update != NULL) {
      restartGame(gameIndex);
    }
  }
}

void addGame(char *title, char *description,
             char (*characters)[CHARACTER_WIDTH][CHARACTER_HEIGHT + 1],
             int charactersCount, Options options, void (*update)(void)) {
  if (gameCount >= MAX_GAME_COUNT) {
    return;
  }
  Game *g = &games[gameCount];
  g->title = title;
  g->description = description;
  g->characters = characters;
  g->charactersCount = charactersCount;
  g->options = options;
  g->update = update;
  gameCount++;
}

Game getGame(int index) { return games[index]; }

void addMenu() {
  Options o = {
      .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};
  addGame("", "", NULL, 0, o, update);
}
