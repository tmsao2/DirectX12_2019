#include "Application.h"

#ifdef _DEBUG
int main()
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif // _DEBUG
{
	auto& app = Application::Instance();
	app.Init();
	app.Run();
	app.Terminate();
	return 0;
}