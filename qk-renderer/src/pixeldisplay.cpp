#include "pixeldisplay.h"


using namespace Qk;


PixelDisplay::PixelDisplay(const std::string& windowTitle, int windowWidth, int windowHeight, bool windowResizeable)
{
	if (!SDL_WasInit(SDL_INIT_VIDEO))
		throw QkError("SDL video subsystem not initialized", 7300);

	// Create window and renderer
	m_window = SDL_CreateWindow(
		windowTitle.c_str(),
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		windowWidth,
		windowHeight,
		0
	);

	SDL_SetWindowResizable(m_window, windowResizeable ? SDL_TRUE : SDL_FALSE);

	m_renderer = SDL_CreateRenderer(m_window, -1, 0);

	SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
	SDL_RenderClear(m_renderer);
}

PixelDisplay::~PixelDisplay()
{
	if (m_texture != nullptr)
		SDL_DestroyTexture(m_texture);

	SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);
}

void PixelDisplay::SetWindowTitle(const std::string& title)
{
	SDL_SetWindowTitle(m_window, title.c_str());
}

void PixelDisplay::SetFramebufferInterface(FramebufferDescriptor* ps)
{
	// Store pixel source
	m_fi = ps;

	// Reset the texture if needed
	if (m_texture != nullptr)
	{
		SDL_DestroyTexture(m_texture);
		m_texture = nullptr;
	}
	
	// Create new texture
	m_texture = SDL_CreateTexture(
		m_renderer,
		SDL_PIXELFORMAT_RGB24, // Each pixel represented by 3 bytes of RGB data. Must match size and configuration of Qk::Pixel
		SDL_TEXTUREACCESS_STATIC, 
		ps->Width, 
		ps->Height
	);

	// Configure render scaling to fit window size
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0); // 0 means nearest pixel sampling
	SDL_RenderSetLogicalSize(m_renderer, ps->Width, ps->Height);
}

void PixelDisplay::RenderFrame()
{
	if (m_fi == nullptr)
		return;

	SDL_UpdateTexture(m_texture, NULL, m_fi->PixelArray, m_fi->Width * sizeof(Qk::Pixel));
	
	// Clearing not needed as long as texture overwrites entire screen
	//SDL_RenderClear(m_renderer);
	
	SDL_RenderCopy(m_renderer, m_texture, NULL, NULL);
	SDL_RenderPresent(m_renderer);
}