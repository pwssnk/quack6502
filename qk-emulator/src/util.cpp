#include <iostream>
#include "util.h"
#include "definitions.h"


using namespace Qk;


byte Util::BCDtoBIN(byte bcd)
{
	byte tens = bcd >> 4;
	byte ones = (bcd & 0x0F);
	return tens * 10 + ones;
}

byte Util::BINtoBCD(byte bin)
{
	byte tens = (bin > 9) ? bin / 10 : 0;
	byte ones = bin - tens * 10;
	return ((tens << 4) & 0xF0) | (ones & 0x0F);
}

bool Util::ValidateBCD(byte bcd)
{
	return ((bcd & 0xF0) < 0xA0) && ((bcd & 0x0F) < 0x0A);
}

byte Util::ReverseBits(byte data)
{
	byte b = 0;

	for (int i = 7; i >= 0; i--)
	{
		b |= (((data >> i) & 0x01)) << (7 - i);
	}

	return b;
}

void Util::PrintBits(byte data)
{
	for (int i = 7; i >= 0; i--)
	{
		std::cout << (bool)((data >> i) & 0x01);
	}
	std::cout << std::endl;
}