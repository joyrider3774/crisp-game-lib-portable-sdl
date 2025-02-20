#include <SDL3/SDL.h>
#include <SDL3/SDL_joystick.h>
#include <math.h>
#include <stdbool.h>
#include "CInput.h"
#include "cglpSDL3.h"


CInput* CInput_Create()
{
	SDL_InitSubSystem(SDL_INIT_GAMEPAD);
	CInput* Result = (CInput*)SDL_malloc(sizeof(CInput));
	Result->JoystickDeadZone = 10000;
	Result->TriggerDeadZone = 10000;
	Result->GameController = NULL;
	CInput_ResetButtons(Result);
	int jcount;
	SDL_GetJoysticks(&jcount);
	for (int i=0; i < jcount; i++)
	{
		if(SDL_IsGamepad(i))
		{
			Result->GameController = SDL_OpenGamepad(i);
			SDL_SetGamepadEventsEnabled(true);
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
		SDL_CloseGamepad(Input->GameController);
		Input->GameController = NULL;
	}
	SDL_free(Input);
	Input = NULL;
	SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}


void CInput_HandleJoystickButtonEvent(CInput *Input, int Button, bool Value)
{
	switch (Button)
	{
		case SDL_GAMEPAD_BUTTON_NORTH:
			Input->Buttons.ButY = Value;
			break;
		case SDL_GAMEPAD_BUTTON_WEST:
			Input->Buttons.ButX = Value;
			break;
		case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
			Input->Buttons.ButLB = Value;
			break;
		case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
			Input->Buttons.ButRB = Value;
			break;
		case SDL_GAMEPAD_BUTTON_DPAD_UP:
			Input->Buttons.ButDpadUp = Value;
			break;
		case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
			Input->Buttons.ButDpadDown = Value;
			break;
		case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
			Input->Buttons.ButDpadLeft = Value;
			break;
		case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
			Input->Buttons.ButDpadRight = Value;
			break;
		case SDL_GAMEPAD_BUTTON_SOUTH:
			Input->Buttons.ButA = Value;
			break;		
		case SDL_GAMEPAD_BUTTON_EAST:
			Input->Buttons.ButB = Value;
			break;
		case SDL_GAMEPAD_BUTTON_START:
			Input->Buttons.ButStart = Value;
			break;
		case SDL_GAMEPAD_BUTTON_BACK:
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
		case SDL_GAMEPAD_AXIS_LEFTX:
			if (SDL_abs(Value) < Input->JoystickDeadZone)
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

		case SDL_GAMEPAD_AXIS_LEFTY:
			if (SDL_abs(Value) < Input->JoystickDeadZone)
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

		case SDL_GAMEPAD_AXIS_RIGHTX:
			if (SDL_abs(Value) < Input->JoystickDeadZone)
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

		case SDL_GAMEPAD_AXIS_RIGHTY:
			if (SDL_abs(Value) < Input->JoystickDeadZone)
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

		case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
			if (SDL_abs(Value) < Input->TriggerDeadZone)
			{
				Input->Buttons.ButLT = false;
				return;
			}
			Input->Buttons.ButLT = true;
			break;

		case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
			if (SDL_abs(Value) < Input->TriggerDeadZone)
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
		if (Event.type == SDL_EVENT_RENDER_TARGETS_RESET)
			Input->Buttons.RenderReset = true;

		if (Event.type == SDL_EVENT_QUIT)
			Input->Buttons.ButQuit = true;

		if (Event.type == SDL_EVENT_JOYSTICK_ADDED)
			if(Input->GameController == NULL)
				if(SDL_IsGamepad(Event.jdevice.which))
				Input->GameController = SDL_OpenGamepad(Event.jdevice.which);

		if (Event.type == SDL_EVENT_JOYSTICK_REMOVED)
		{
			SDL_Joystick* Joystick = SDL_GetGamepadJoystick(Input->GameController);
			if (Joystick)
				if (Event.jdevice.which == SDL_GetJoystickID(Joystick))
				{
					SDL_CloseGamepad(Input->GameController);
					Input->GameController = NULL;
				}
		}

		if (Event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION)
			CInput_HandleJoystickAxisEvent(Input, Event.gaxis.axis, Event.gaxis.value);

		if (Event.type == SDL_EVENT_GAMEPAD_BUTTON_UP)
			CInput_HandleJoystickButtonEvent(Input, Event.gbutton.button, false);

		if (Event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
			CInput_HandleJoystickButtonEvent(Input, Event.gbutton.button, true);

		if (Event.type == SDL_EVENT_KEY_UP)
			CInput_HandleKeyboardEvent(Input, Event.key.key, false);

		if (Event.type == SDL_EVENT_KEY_DOWN)
			CInput_HandleKeyboardEvent(Input, Event.key.key, true);
		
		if (Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
			CInput_HandleMouseEvent(Input, Event.button.button, true);

		if (Event.type == SDL_EVENT_MOUSE_BUTTON_UP)
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
