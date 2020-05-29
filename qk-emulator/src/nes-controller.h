#pragma once

#include "definitions.h"
#include "nes-definitions.h"
#include "bus.h"

namespace Qk { namespace NES 
{
	class ControllerInterface : public Bus::Device
	{
	public:
		ControllerInterface(Bus& bus);
		ControllerInterface(Bus& bus, const AddressRange& addressableRange);

		void PressButton(Controller::Player pad, Controller::Button button);
		void ReleaseButton(Controller::Player pad, Controller::Button buton);

		void WriteToDevice(word address, byte data) override;
		byte ReadFromDevice(word address, bool peek = false) override;

	protected:
		byte m_ctlr1Parallel = 0;
		byte m_ctlr2Parallel = 0;
		byte m_ctlr1Shift = 0;
		byte m_ctlr2Shift = 0;
	};
}}