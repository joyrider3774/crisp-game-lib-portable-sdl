#include <stdlib.h>
#include <SDL.h>
#include <SDL_joystick.h>
#include <math.h>
#include <stdbool.h>
#include "CInput.h"
#include "cglpSDL2.h"

CInput* CInput_Create()
{
	SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
	CInput* Result = (CInput*)malloc(sizeof(CInput));
	Result->JoystickDeadZone = 10000;
	Result->TriggerDeadZone = 10000;
	Result->GameController = NULL;
	CInput_ResetButtons(Result);
	for (int i=0; i < SDL_NumJoysticks(); i++)
	{
		if(SDL_IsGameController(i))
		{
			Result->GameController = SDL_GameControllerOpen(i);
			SDL_GameControllerEventState(SDL_ENABLE);
			SDL_Log("Joystick Detected!\n");
			break;
		}
	}
	return Result;
}

void CInput_Destroy(CInput *Input)
{
	if(Input->GameController)
	{
		SDL_GameControllerClose(Input->GameController);
		Input->GameController = NULL;
	}
	free(Input);
	Input = NULL;
	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}


void CInput_HandleJoystickButtonEvent(CInput *Input, int Button, bool Value)
{
	switch (Button)
	{
		case SDL_CONTROLLER_BUTTON_Y:
			Input->Buttons.ButY = Value;
			break;
		case SDL_CONTROLLER_BUTTON_X:
			Input->Buttons.ButX = Value;
			break;
		case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
			Input->Buttons.ButLB = Value;
			break;
		case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
			Input->Buttons.ButRB = Value;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			Input->Buttons.ButDpadUp = Value;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			Input->Buttons.ButDpadDown = Value;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			Input->Buttons.ButDpadLeft = Value;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			Input->Buttons.ButDpadRight = Value;
			break;
		case SDL_CONTROLLER_BUTTON_A:
			Input->Buttons.ButA = Value;
			break;		
		case SDL_CONTROLLER_BUTTON_B:
			Input->Buttons.ButB = Value;
			break;
		case SDL_CONTROLLER_BUTTON_START:
			Input->Buttons.ButStart = Value;
			break;
		case SDL_CONTROLLER_BUTTON_BACK:
			Input->Buttons.ButBack = Value;
			break;
		default:
			break;
	}
}

void CInput_HandleKeyboardEvent(CInput *Input, int Key, bool Value)
{
	switch (Key)
	{
		case SDLK_F4:
			Input->Buttons.ButQuit = Value;
			break;
		case SDLK_F3:
			Input->Buttons.ButFullscreen = Value;
			break;
		case BUTTON_VOLUP:
			Input->Buttons.ButRB = Value;
			break;
		case BUTTON_VOLDOWN:
			Input->Buttons.ButLB = Value;
			break;
		case BUTTON_UP:
			Input->Buttons.ButUp = Value;
			break;
		case BUTTON_DOWN:
			Input->Buttons.ButDown = Value;
			break;
		case BUTTON_LEFT:
			Input->Buttons.ButLeft = Value;
			break;
		case BUTTON_RIGHT:
			Input->Buttons.ButRight = Value;
			break;
		case BUTTON_MENU:
			Input->Buttons.ButBack = Value;
			break;
		case BUTTON_A:
			Input->Buttons.ButA = Value;
			break;
		case BUTTON_B:
			Input->Buttons.ButB = Value;
			break;
		case BUTTON_SOUNDSWITCH:
			Input->Buttons.ButY = Value;
			break;
		case BUTTON_GLOWSWITCH:
			Input->Buttons.ButX = Value;
			break;
		case BUTTON_DARKSWITCH:
			Input->Buttons.ButStart = Value;
			break;
		default:
			break;
	}
}

void CInput_HandleJoystickAxisEvent(CInput *Input, int Axis, int Value)
{
	switch(Axis)
	{
		case SDL_CONTROLLER_AXIS_LEFTX:
			if (abs(Value) < Input->JoystickDeadZone)
			{
				Input->Buttons.ButRight = false;
				Input->Buttons.ButLeft = false;
				return;
			}
			if(Value > 0)
				Input->Buttons.ButRight = true;
			else
				Input->Buttons.ButLeft = true;
			break;

		case SDL_CONTROLLER_AXIS_LEFTY:
			if (abs(Value) < Input->JoystickDeadZone)
			{
				Input->Buttons.ButUp = false;
				Input->Buttons.ButDown = false;
				return;
			}
			if(Value < 0)
				Input->Buttons.ButUp = true;
			else
				Input->Buttons.ButDown = true;
			break;

		case SDL_CONTROLLER_AXIS_RIGHTX:
			if (abs(Value) < Input->JoystickDeadZone)
			{
				Input->Buttons.ButRight2 = false;
				Input->Buttons.ButLeft2 = false;
				return;
			}
			if(Value > 0)
				Input->Buttons.ButRight2 = true;
			else
				Input->Buttons.ButLeft2 = true;
			break;

		case SDL_CONTROLLER_AXIS_RIGHTY:
			if (abs(Value) < Input->JoystickDeadZone)
			{
				Input->Buttons.ButUp2 = false;
				Input->Buttons.ButDown2 = false;
				return;
			}
			if(Value < 0)
				Input->Buttons.ButUp2 = true;
			else
				Input->Buttons.ButDown2 = true;
			break;

		case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
			if (abs(Value) < Input->TriggerDeadZone)
			{
				Input->Buttons.ButLT = false;
				return;
			}
			Input->Buttons.ButLT = true;
			break;

		case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
			if (abs(Value) < Input->TriggerDeadZone)
			{
				Input->Buttons.ButRT = false;
				return;
			}
			Input->Buttons.ButRT = true;
			break;
	}
}

void CInput_HandleMouseEvent(CInput *Input, int Button, bool Value)
{
	switch (Button)
	{
		case SDL_BUTTON_LEFT:
			Input->Buttons.ButA = Value;
			break;
		case SDL_BUTTON_RIGHT:
			Input->Buttons.ButB = Value;
			break;
		default:
			break;
	}
}

void CInput_Update(CInput *Input)
{
	SDL_Event Event;
	Input->PrevButtons = Input->Buttons;
	Input->Buttons.ButQuit = false;
	Input->Buttons.RenderReset = false;
	while (SDL_PollEvent(&Event))
	{
		if (Event.type == SDL_RENDER_TARGETS_RESET)
			Input->Buttons.RenderReset = true;

		if (Event.type == SDL_QUIT)
			Input->Buttons.ButQuit = true;

		if (Event.type == SDL_JOYDEVICEADDED)
			if(Input->GameController == NULL)
				if(SDL_IsGameController(Event.jdevice.which))
				Input->GameController = SDL_GameControllerOpen(Event.jdevice.which);

		if (Event.type == SDL_JOYDEVICEREMOVED)
		{
			SDL_Joystick* Joystick = SDL_GameControllerGetJoystick(Input->GameController);
			if (Joystick)
				if (Event.jdevice.which == SDL_JoystickInstanceID(Joystick))
				{
					SDL_GameControllerClose(Input->GameController);
					Input->GameController = NULL;
				}
		}

		if (Event.type == SDL_CONTROLLERAXISMOTION)
			CInput_HandleJoystickAxisEvent(Input, Event.jaxis.axis, Event.jaxis.value);

		if (Event.type == SDL_CONTROLLERBUTTONUP)
			CInput_HandleJoystickButtonEvent(Input, Event.cbutton.button, false);

		if (Event.type == SDL_CONTROLLERBUTTONDOWN)
			CInput_HandleJoystickButtonEvent(Input, Event.cbutton.button, true);

		if (Event.type == SDL_KEYUP)
			CInput_HandleKeyboardEvent(Input, Event.key.keysym.sym, false);

		if (Event.type == SDL_KEYDOWN)
			CInput_HandleKeyboardEvent(Input, Event.key.keysym.sym, true);
		
		if (Event.type == SDL_MOUSEBUTTONDOWN)
			CInput_HandleMouseEvent(Input, Event.button.button, true);

		if (Event.type == SDL_MOUSEBUTTONUP)
			CInput_HandleMouseEvent(Input, Event.button.button, false);
	}
	SDL_GetMouseState(&Input->Buttons.MouseX, &Input->Buttons.MouseY);
}

void CInput_ResetButtons(CInput *Input)
{
	Input->Buttons.MouseX = 0;
	Input->Buttons.MouseY = 0;
	Input->Buttons.ButLeft = false;
	Input->Buttons.ButRight = false;
	Input->Buttons.ButUp = false;
	Input->Buttons.ButDown = false;
	Input->Buttons.ButLB = false;
	Input->Buttons.ButRB = false;
	Input->Buttons.ButLT = false;
	Input->Buttons.ButRT = false;
	Input->Buttons.ButBack = false;
	Input->Buttons.ButA = false;
	Input->Buttons.ButB = false;
	Input->Buttons.ButX = false;
	Input->Buttons.ButY = false;
	Input->Buttons.ButStart = false;
	Input->Buttons.ButQuit = false;
	Input->Buttons.ButFullscreen = false;
	Input->Buttons.RenderReset = false;
	Input->Buttons.ButDpadLeft = false;
	Input->Buttons.ButDpadRight = false;
	Input->Buttons.ButDpadUp = false;
	Input->Buttons.ButDpadDown = false;
	Input->Buttons.ButLeft2 = false;
	Input->Buttons.ButRight2 = false;
	Input->Buttons.ButUp2 = false;
	Input->Buttons.ButDown2 = false;
	Input->PrevButtons = Input->Buttons;
}
