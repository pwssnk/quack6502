#include <iostream>
#include <SDL.h>
#include "system-renderers.h"


using namespace Qk;


int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cout << "usage: qk [path to nes romfile]" << std::endl;
		return 0;
	}

	std::string romPath(argv[1]);
	int returnCode = 0;

	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);

	try
	{
		SDLNES nesrender;
		nesrender.LoadROM(romPath);
		nesrender.Run();
	}
	catch (const QkError& ex)
	{
		std::cout << "[ERROR] " << ex.what() << std::endl;
		returnCode = ex.code();
	}
	
	SDL_Quit();

	return returnCode;
}