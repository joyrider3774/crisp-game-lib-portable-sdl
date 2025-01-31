#include "cglp.h"

//original games
void addGamePakuPaku();
void addGameThunder();
void addGameBallTour();
void addGameHexmin();
void addGameSurvivor();
void addGamePinCLimb();
void addGameColorRoll();
void addGameCastN();
void addGameReflector();
void addGameFroooog();
void addGameRWheel();
void addGameLadderDrop();
void addGameGrapplingH();
void addGameInTow();
void addGameTimberTest();

//working
void addGameBCannon();
void addGameBaroll();
void addGameBamboo();
void addGameAerialBar();
void addGameBallsBombs();
void addGameAntLion();
void addGameTwofaced();
void addGameBombup();

//working requires mouse 
void addGameBBlast();
void addGameBalance();
void addGameBreedc();

//working inccorectly
void addGameAccelB();

//working inccorectly requires mouse 

void addGameSection(char* sectionName)
{
    Options o = { 0 };
    addGame(sectionName, "", NULL, 0, o, NULL);
}


void addGames() {
  addGameSection("==ORIGNAL GAMES==");
  addGamePakuPaku();
  addGameThunder();
  addGameBallTour();
  addGameHexmin();
  addGameSurvivor();
  addGamePinCLimb();
  addGameColorRoll();
  addGameCastN();
  addGameReflector();
  addGameFroooog();
  addGameRWheel();
  addGameLadderDrop();
  addGameGrapplingH();
  addGameInTow();
  addGameTimberTest();
  
  //new games
  addGameSection("=======NEW=======");
  addGameBCannon();
  addGameBaroll();
  addGameBamboo();
  addGameAerialBar();
  addGameBallsBombs();
  addGameAntLion();
  addGameTwofaced();
  addGameBombup();

  addGameSection("======MOUSE======");
  addGameBBlast();
  addGameBalance();
  addGameBreedc();

  //inccorect
  addGameSection("======BUGGED=====");
  addGameAccelB();

  addGameSection("===BUGGED MOUSE==");

}
