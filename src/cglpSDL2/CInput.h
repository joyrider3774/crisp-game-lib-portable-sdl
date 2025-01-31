#ifndef CINPUT_H
#define CINPUT_H

#include <SDL.h>
#include <stdbool.h>

struct SButtons 
{
	bool ButLeft, ButRight, ButUp, ButDown,
		 ButDpadLeft, ButDpadRight, ButDpadUp, ButDpadDown,
		 ButLeft2, ButRight2, ButUp2, ButDown2,
		 ButBack, ButStart, ButA, ButB,
		 ButX, ButY, ButLB, ButRB, ButFullscreen, ButQuit, ButRT, ButLT,
		 RenderReset;
	int MouseX, MouseY;
};
typedef struct SButtons SButtons;

struct CInput 
{
	SDL_GameController* GameController;
	SButtons Buttons, PrevButtons;
	int JoystickDeadZone, TriggerDeadZone;
};
typedef struct CInput CInput;

CInput* CInput_Create();
void CInput_Destroy(CInput *Input);
void CInput_Update(CInput *Input);
void CInput_ResetButtons(CInput *Input);

#endif