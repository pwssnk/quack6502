#include "bus.h"

using namespace Qk;

/**************************************************
	Qk::Bus
***************************************************/

/*
	Constructors, destructor
*/

Bus::Bus()
{

}

Bus::~Bus()
{

}

/*
	Internals
*/

bool Bus::CheckAddressBelongsTo(const Device& device, word address) const
{
	const AddressRange& devRange = device.GetAddressableRange();
	return address >= devRange.Min && address <= devRange.Max;
}

bool Bus::CheckAddressRangeConflict(const Device& device, const AddressRange& range) const
{
	const AddressRange& devRange = device.GetAddressableRange();
	return (range.Min >= devRange.Min && range.Min <= devRange.Max) || (range.Max >= devRange.Min && range.Max <= devRange.Max);
}

bool Bus::CheckRangeAvailable(const AddressRange& range) const
{
	for (auto device : m_addressableDevices)
	{
		if (CheckAddressRangeConflict(*device, range))
			return false;
	}

	return true;
}

Bus::Device* Bus::GetDeviceAt(word address)
{
	// First check if another call to previous device
	if (CheckAddressBelongsTo(*m_cachedDevicePtr, address))
		return m_cachedDevicePtr;

	for (auto device : m_addressableDevices)
	{
		if (CheckAddressBelongsTo(*device, address))
		{
			m_cachedDevicePtr = device;
			return device;
		}
	}

	return nullptr;
}

/*
	Interfacing with device subclasses
*/

// Connecting devices

void Bus::ConnectDevice(Device& device)
{
	if (!device.IsAddressable())
	{
		m_nonAddressableDevices.push_back(&device);
		return;
	}

	if (CheckRangeAvailable(device.GetAddressableRange()))
	{
		m_addressableDevices.push_back(&device);
		m_cachedDevicePtr = &device;
	}
	else
	{
		throw QkError("Address mapping conflict: two devices want to occupy overlapping address ranges on bus", 301);
	}
}


/*
	Bus I/O
*/

byte Bus::ReadFromBus(word address)
{
	Device* dev = GetDeviceAt(address);

	if (dev != nullptr)
		return dev->ReadFromDevice(address);
	else
		return 0;
}

void Bus::WriteToBus(word address, byte data)
{
	Device* dev = GetDeviceAt(address);
	
	if (dev != nullptr)
		dev->WriteToDevice(address, data);
}

void Bus::EmitSignal(int signalId)
{
	// Loop over all devices, call on signal
	// function. May want to optimize this at 
	// some point, as not all devices are
	// actually listening for particular signals
	// or any signal at all

	for (auto ptr : m_addressableDevices)
	{
		ptr->OnBusSignal(signalId);
	}

	for (auto ptr : m_nonAddressableDevices)
	{
		ptr->OnBusSignal(signalId);
	}
}

byte Bus::Peek(word address)
{
	// Read from device, but tell device that we're only
	// 'peeking', i.e. the device should leave internal state
	// unaffected by the read. This may be necessary, as some
	// emulated hardware changes state when read.

	Device* dev = GetDeviceAt(address);

	if (dev != nullptr)
		return dev->ReadFromDevice(address, true);
	else
		return 0;
}


/**************************************************
	Qk::Bus::Device
***************************************************/

/*
	Constructors
*/

Bus::Device::Device(Bus& bus, bool isAddressable, const AddressRange& addressRange)
	: BUS(bus), m_isAddressable(isAddressable), m_addressableRange(addressRange)
{
	if (m_addressableRange.Min > m_addressableRange.Max)
		throw QkError("Invalid address range: min exceeds max", 310);

	BUS.ConnectDevice(*this);
}

Bus::Device::Device(Bus& bus)
	: BUS(bus), m_isAddressable(false), m_addressableRange(AddressRange(0, 0))
{
	if (m_addressableRange.Min > m_addressableRange.Max)
		throw QkError("Invalid address range: min exceeds max", 310);

	BUS.ConnectDevice(*this);
}


/*
	Public interface methods
*/

const AddressRange& Bus::Device::GetAddressableRange() const
{
	return m_addressableRange;
}

bool Bus::Device::IsAddressable() const
{
	return m_isAddressable;
}

byte Qk::Bus::Device::ReadFromDevice(word address, bool peek)
{
#ifdef _DEBUG
	throw QkError("ReadFromDevice method not implemented by child class. This method should not be called.", 311);
#endif
	return 0;
}

void Qk::Bus::Device::WriteToDevice(word address, byte data)
{
#ifdef _DEBUG
	throw QkError("WriteToDevice method not implemented by child class. This method should not be called.", 312);
#endif
}

void Qk::Bus::Device::OnBusSignal(int signalId)
{
	// Default signal behaviour: ignore
	return;
}
