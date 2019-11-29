#pragma once
#include <Windows.h>
#include <memory>
#include "Geometry.h"

class Dx12Wrapper;

class Application
{
private:
	HWND _hwnd;	//�E�B���h�E�n���h��
	WNDCLASSEX _wndClass = {};
	//�����֎~
	Application();
	//����֎~
	Application(const Application&) = delete;
	//�R�s�[�֎~
	void operator=(Application&) = delete;

	bool InitWindow();
	std::shared_ptr<Dx12Wrapper> _dx12;
public:
	~Application();
	bool Init();
	void Run();
	void Terminate();
	Size GetWindowSize();

	static Application& Instance();
	
};

