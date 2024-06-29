#pragma once
#include <string>
#include <SDL2/SDL_scancode.h>

enum InputKey
{
	A,
	D,
	LEFT,
	RIGHT,
	DELETE,
	LCTRL,
	RCTRL,
	N,
	Q,
	W,
	E,
	C,
	M,
	S,
	X,
	O,
	G,
	F2,
	LSHIFT,
	RSHIFT,

	COUNT
};

static int keys[] = {
	SDL_SCANCODE_A,
	SDL_SCANCODE_D,
	SDL_SCANCODE_LEFT,
	SDL_SCANCODE_RIGHT,
	SDL_SCANCODE_DELETE,
	SDL_SCANCODE_LCTRL,
	SDL_SCANCODE_RCTRL,
	SDL_SCANCODE_N,
	SDL_SCANCODE_Q,
	SDL_SCANCODE_W,
	SDL_SCANCODE_E,
	SDL_SCANCODE_C,
	SDL_SCANCODE_M,
	SDL_SCANCODE_S,
	SDL_SCANCODE_X,
	SDL_SCANCODE_O,
	SDL_SCANCODE_G,
	SDL_SCANCODE_F2,
	SDL_SCANCODE_LSHIFT,
	SDL_SCANCODE_RSHIFT,
};

static std::string keyNames[] = {
	"A",
	"D",
	"Left Arrow",
	"Right Arrow",
	"Delete",
	"Left Ctrl",
	"Right Ctrl",
	"N",
	"Q",
	"W",
	"E",
	"C",
	"M",
	"S",
	"X",
	"O",
	"G",
	"F2",
	"Left Shift",
	"Right Shift",
};

const int keyCount = InputKey::COUNT;

enum InputMouse
{
	LEFT_BUTTON,
	MIDDLE_BUTTON
};
static std::string mouseButtonNames[] = {
	"Left Button",
	"Right Button"
};
const int mouseButtonCount = 2;

inline const std::string& GetKeyName(InputKey key)
{
	return keyNames[key];
}
inline const std::string& GetMouseButtonName(InputMouse mouseButton)
{
	return mouseButtonNames[mouseButton];
}