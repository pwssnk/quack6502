#ifdef DEBUG
#include <stdexcept>
#endif

#include <fstream>
#include "memory.h"

using namespace Qk;


/**************************************************
	RAM (Generic RAM emulation)
***************************************************/

/*
	Constructors, destructor
*/

RAM::RAM(Bus& bus, const AddressRange& addressableRange)
	: Device(bus, true, addressableRange)
{
	m_size = addressableRange.Max - addressableRange.Min + 1;
	m_data = new byte[m_size](); // Allocate to heap and initialize "RAM"
}

RAM::~RAM()
{
	delete[] m_data;
}

/*
	Internal
*/

word RAM::LocalizeAddress(word addressOnBus)
{
	return addressOnBus - m_addressableRange.Min;
}

/*
	Public
*/

word RAM::GetSize() const
{
	return m_size;
}

byte RAM::ReadFromDevice(word address, bool peek)
{
	word localAddress = LocalizeAddress(address);

	return m_data[localAddress];
}

void RAM::WriteToDevice(word address, byte data)
{
	word localAddress = LocalizeAddress(address);

	m_data[localAddress] = data;
}


/**************************************************
	ROM (Generic ROM emulation)
***************************************************/


ROM::ROM(Bus& bus, const AddressRange& addressableRange) : RAM(bus, addressableRange)
{

}

void ROM::WriteToDevice(word address, byte data)
{
#ifdef _DEBUG
	throw QkError("Illegal write operation to ROM. ROM is read-only", 400);
#endif
	return;
}

bool ROM::LoadROM(const std::string& filepath)
{
	std::ifstream input(filepath, std::ios::binary);
	unsigned int length = 0;

	// Check if file available
	if (!input)
		return false;

	// Get length of file
	input.seekg(std::ios::end);
	length = (unsigned int)input.tellg();
	input.seekg(std::ios::beg);

	// If file exceeds available memory,
	// just read as much as will fit
	// and discard the remaining bytes
	if (length > m_size)
		length = m_size;

	// Read file into byte array
	input.read((char*)m_data, length);

	input.close();

	return true;
}