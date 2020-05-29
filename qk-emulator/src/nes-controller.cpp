#include "nes-controller.h"

using namespace Qk;
using namespace Qk::NES;

/*
	Constructors
*/

ControllerInterface::ControllerInterface(Bus& bus) 
	: Bus::Device(bus, true, AddressRange(0x4016, 0x4017))
{

}

ControllerInterface::ControllerInterface(Bus& bus, const AddressRange& addressableRange) 
	: Bus::Device(bus, true, addressableRange)
{

}


/*
	Controller interface
*/

void ControllerInterface::PressButton(Controller::Player pad, Controller::Button button)
{
	byte& ctrlr = pad == Controller::Player::One ? m_ctlr1Parallel : m_ctlr2Parallel;
	ctrlr |= static_cast<int>(button);
}

void ControllerInterface::ReleaseButton(Controller::Player pad, Controller::Button button)
{
	byte& ctrlr = pad == Controller::Player::One ? m_ctlr1Parallel : m_ctlr2Parallel;
	ctrlr &= ~static_cast<int>(button);
}


/*
	Bus I/O
*/

void ControllerInterface::WriteToDevice(word address, byte data)
{
	if (address == m_addressableRange.Min)
	{
		// CPU is polling controllers
		m_ctlr1Shift = m_ctlr1Parallel;
		m_ctlr2Shift = m_ctlr2Parallel;
	}
	else
	{
		// CPU is updating APU frame counter
		byte flag = (data & 0xC0) >> 6;
		BUS.EmitSignal(SIGNAL_APU_FRC_NONE | flag);
	}
}

byte ControllerInterface::ReadFromDevice(word address, bool peek)
{
	byte data = 0;

	if (address == m_addressableRange.Min)
	{
		// Gamepad 1
		data = (m_ctlr1Shift & 0x80) != 0 ? 0x01 : 0x00;

		if (!peek)
			m_ctlr1Shift <<= 1;
	}
	else
	{
		// Gamepad 2
		data = (m_ctlr2Shift & 0x80) != 0 ? 0x01: 0x00;

		if (!peek)
			m_ctlr2Shift <<= 1;
	}

	return data;
}
