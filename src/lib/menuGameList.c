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
void addGameBwalls();
void addGameCatapult();
void addGameCateP();
void addGameChargeBeam();

//working requires mouse 
void addGameBBlast();
void addGameBalance();
void addGameBreedc();
void addGameCardq();

//working inccorectly
void addGameAccelB();
void addGameBsfish();

//working inccorectly requires mouse 

void addGameSection(char* sectionName)
{
    Options o = { 0 };
    addGame(sectionName, "", NULL, 0, o, NULL);
}


void addGames() {
  addGameSection("==DEFAULT GAMES==");
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
  addGameBwalls();
  addGameCatapult();
  addGameCateP();
  addGameChargeBeam();

  addGameSection("======MOUSE======");
  addGameBBlast();
  addGameBalance();
  addGameBreedc();
  addGameCardq();

  //inccorect
  addGameSection("======BUGGED=====");
  addGameAccelB();
  addGameBsfish();

  addGameSection("===BUGGED MOUSE==");

}
