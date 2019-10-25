#pragma once
#include <Windows.h>
#include <memory>
#include "Geometry.h"

class Dx12Wrapper;

class Application
{
private:
	HWND _hwnd;	//ウィンドウハンドル
	WNDCLASSEX _wndClass = {};
	//生成禁止
	Application();
	//代入禁止
	Application(const Application&) = delete;
	//コピー禁止
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

