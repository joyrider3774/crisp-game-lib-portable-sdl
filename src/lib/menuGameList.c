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
void addGameFracave();

//working
void addGameBCannon();
void addGameBaroll();
void addGameBamboo();
void addGameAerialBar();
void addGameBallsBombs();
void addGameAntLion();
void addGameTwofaced();
void addGameBombup();
void addGameBsfish();
void addGameBwalls();
void addGameCatapult();
void addGameCateP();
void addGameChargeBeam();
void addGameCirclew();
void addGameCounterB();
void addGameCTower();
void addGameDescents();
void addGameDivarr();
void addGameDLaser();

//working requires mouse 
void addGameBBlast();
void addGameBalance();
void addGameBreedc();
void addGameCardq();
void addGameCNodes();
void addGameCrossLine();
void addGameDarkCave();
void addGameDFight();

//working inccorectly
void addGameAccelB();

//working inccorectly requires mouse 

void addGameSection(char* sectionName)
{
    Options o = { 0 };
    addGame(sectionName, "", NULL, 0, o, false, NULL);
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
  addGameBsfish();
  addGameBwalls();
  addGameCatapult();
  addGameCateP();
  addGameChargeBeam();
  addGameCirclew();
  addGameCounterB();
  addGameCTower();
  addGameFracave();
  addGameDescents();
  addGameDivarr();
  addGameDLaser();

  addGameSection("======MOUSE======");
  addGameBBlast();
  addGameBalance();
  addGameBreedc();
  addGameCardq();
  addGameCNodes();
  addGameCrossLine();
  addGameDarkCave();
  addGameDFight();

  //inccorect
  addGameSection("======BUGGED=====");
  addGameAccelB();

  addGameSection("===BUGGED MOUSE==");

}
