#pragma once

#include <SDL.h>
#include "pixeldisplay.h"
#include "systems.h"

using namespace Qk;
using namespace NES;

namespace Qk
{
	class SDLNES
	{
	public:
		SDLNES();

		void Run();
		void LoadROM(const std::string& path);

	private:
		void OnKeyBoard(SDL_KeyboardEvent& event, bool pressed);

	private:
		NESConsole m_nes;
		std::string m_windowTitle;
	};
}