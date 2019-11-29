#pragma once
#include <array>

class Input
{
private:
	std::array<unsigned char, 256> _key;
	std::array<unsigned char, 256> _keyOld;
public:
	Input();
	~Input();
	void Update();
	std::array<unsigned char, 256> GetKey();
};

