#pragma once

#include <string>
#include "SDL.h"
#include "definitions.h"

namespace Qk
{
	class PixelDisplay
	{
	public:
		PixelDisplay(const std::string& windowTitle, int windowWidth, int windowHeight, bool windowResizable = true);
		~PixelDisplay();

		void SetWindowTitle(const std::string& title);
		void SetFramebufferInterface(FramebufferDescriptor* fb);
		void RenderFrame();
	protected:
		SDL_Window* m_window = nullptr;
		SDL_Renderer* m_renderer = nullptr;
		SDL_Texture* m_texture = nullptr;

		FramebufferDescriptor* m_fi = nullptr;
	};
}