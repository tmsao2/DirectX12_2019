#include "Input.h"

#include <Windows.h>

Input::Input()
{
}


Input::~Input()
{
}

void Input::Update()
{
	_keyOld = _key;
	GetKeyboardState(_key.data());
}

std::array<unsigned char, 256> Input::GetKey()
{
	return _key;
}
