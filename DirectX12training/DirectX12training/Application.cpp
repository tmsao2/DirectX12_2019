#include "Application.h"
#include "Dx12Wrapper.h"
#include <iostream>

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam,LPARAM lparam)
{
	if (msg == WM_DESTROY)//�E�B���h�E���j�����ꂽ��
	{
		PostQuitMessage(0);//OS�ɑ΂��ăA�v�����I�����邱�Ƃ�`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);//�K��̏������s��
}

Application::Application()
{
}


bool Application::InitWindow()
{
	_wndClass.hInstance = GetModuleHandle(nullptr);
	_wndClass.cbSize = sizeof(WNDCLASSEX);
	_wndClass.lpfnWndProc = (WNDPROC)WindowProcedure;
	_wndClass.lpszClassName = "DirectX12���K";
	RegisterClassEx(&_wndClass);

	RECT wrc = { 0,0,WINDOW_WIDTH,WINDOW_HEIGHT };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	_hwnd = CreateWindow(_wndClass.lpszClassName, "DirectX12���K",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		wrc.right - wrc.left, wrc.bottom - wrc.top,
		nullptr, nullptr, _wndClass.hInstance, nullptr);

	if (_hwnd == nullptr)
	{
		LPVOID messageBuffer = nullptr;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer,
			0, nullptr);

		OutputDebugString((TCHAR*)messageBuffer);
		std::cout << (TCHAR*)messageBuffer << std::endl;
		LocalFree(messageBuffer);
		return false;
	}

	return true;
}

Application::~Application()
{
}

bool Application::Init()
{
	InitWindow();
	_dx12 = std::make_shared<Dx12Wrapper>(_hwnd);
	_dx12->Init();
	return true;
}

void Application::Run()
{
	ShowWindow(_hwnd, SW_SHOW);
	MSG msg;
	while (true)
	{
		_dx12->Update();
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);//���z�L�[�֘A�̕ϊ�
			DispatchMessage(&msg);//��������Ȃ��������b�Z�[�W��OS�ɕԂ�
		}
		if (msg.message == WM_QUIT)
		{
			break;
		}
	}
}

void Application::Terminate()
{
	UnregisterClass(_wndClass.lpszClassName, _wndClass.hInstance);
}

Size Application::GetWindowSize()
{
	return Size(WINDOW_WIDTH, WINDOW_HEIGHT);
}

Application & Application::Instance()
{
	static Application s_Instance;
	return s_Instance;
}
