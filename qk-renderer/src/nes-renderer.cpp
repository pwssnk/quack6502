#include <iostream>
#include <memory>
#include <SDL.h>
#include "system-renderers.h"
#include "pixeldisplay.h"
#include "frameratecontroller.h"

using namespace Qk;
using namespace Qk::NES;


SDLNES::SDLNES()
{
	m_windowTitle = "Quack6502 | Nintendo Entertainment System";
}

void SDLNES::LoadROM(const std::string& path)
{
	m_nes.InsertCartridge(std::make_shared<NES::Cartridge>(path));
	m_nes.Reset();
}

void SDLNES::Run()
{
	// Set up video
	FramebufferDescriptor* fi = m_nes.GetVideoOutput();
	PixelDisplay display(m_windowTitle, fi->Width * 3, fi->Height * 3);
	display.SetFramebufferInterface(fi);

#ifdef NES_AUDIO_ENABLED

	// Set up audio
	SDL_AudioSpec as;
	as.freq = (int)m_nes.GetAudioSampleRate();
	as.samples = m_nes.GetAudioBufferSize();
	as.channels = 1;
	as.format = AUDIO_U8; // Unsigned 8 bit int
	as.userdata = &m_nes;
	as.callback = [](void* userdata, Uint8* stream, int len) -> void {
		audiosample* out = (audiosample*)stream;
		NESConsole* nes = (NESConsole*)userdata;
		size_t numSamples = len / sizeof(audiosample);
		nes->FillAudioBuffer(out, numSamples);
	};

	SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(NULL, 0, &as, NULL, 0);

	if (audioDevice == 0)
		throw QkError("Failed to open audio device", 7301);

	// Unpause audio
	SDL_PauseAudioDevice(audioDevice, 0);

#endif

	// MAIN LOOP
	FramerateController timer;
	SDL_Event event;
	bool exit = false;
	unsigned long frames = 0;
	const int inputPollingInterval = 300;

	while (!exit)
	{
		timer.StartFrameTimer();

		while(frames == m_nes.GetPPUFrameCount())
		{
			for (int i = 0; i < inputPollingInterval; i++)
			{
				m_nes.Clock();
			}

			if (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
				case SDL_QUIT:
					exit = true;
					break;
				case SDL_KEYDOWN:
					OnKeyBoard(event.key, true);
					break;
				case SDL_KEYUP:
					OnKeyBoard(event.key, false);
					break;
				}
			}
		}

		frames = m_nes.GetPPUFrameCount();
		display.RenderFrame();

		timer.StopFrameTimer();
		timer.SleepRemaining();
	}
}

void SDLNES::OnKeyBoard(SDL_KeyboardEvent& event, bool pressed)
{
	switch (event.keysym.sym)
	{
			// Just P1 for now
		case SDLK_LEFT:
			m_nes.ControllerInput(Controller::Player::One, Controller::Button::Left, pressed);
			break;
		case SDLK_RIGHT:
			m_nes.ControllerInput(Controller::Player::One, Controller::Button::Right, pressed);
			break;
		case SDLK_UP:
			m_nes.ControllerInput(Controller::Player::One, Controller::Button::Up, pressed);
			break;
		case SDLK_DOWN:
			m_nes.ControllerInput(Controller::Player::One, Controller::Button::Down, pressed);
			break;
		case SDLK_z:
			m_nes.ControllerInput(Controller::Player::One, Controller::Button::A, pressed);
			break;
		case SDLK_x:
			m_nes.ControllerInput(Controller::Player::One, Controller::Button::B, pressed);
			break;
		case SDLK_RETURN:
			m_nes.ControllerInput(Controller::Player::One, Controller::Button::Start, pressed);
			break;
		case SDLK_RSHIFT:
			m_nes.ControllerInput(Controller::Player::One, Controller::Button::Select, pressed);
			break;

			// P1 alternate
		case SDLK_a:
			m_nes.ControllerInput(Controller::Player::One, Controller::Button::Left, pressed);
			break;
		case SDLK_d:
			m_nes.ControllerInput(Controller::Player::One, Controller::Button::Right, pressed);
			break;
		case SDLK_w:
			m_nes.ControllerInput(Controller::Player::One, Controller::Button::Up, pressed);
			break;
		case SDLK_s:
			m_nes.ControllerInput(Controller::Player::One, Controller::Button::Down, pressed);
			break;
		case SDLK_COMMA:
			m_nes.ControllerInput(Controller::Player::One, Controller::Button::A, pressed);
			break;
		case SDLK_PERIOD:
			m_nes.ControllerInput(Controller::Player::One, Controller::Button::B, pressed);
			break;
	}
}
