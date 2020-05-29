#pragma once

#include <cstdint>
#include <exception>

namespace Qk 
{
	typedef uint8_t byte;
	typedef uint16_t word;
	typedef uint32_t dword;

	struct AddressRange
	{
		AddressRange(word min, word max) : Min(min), Max(max) { };
		const word Min;
		const word Max;
	};

	struct Pixel
	{
		Pixel() : Red(0), Green(0), Blue(0) { };
		Pixel(byte r, byte g, byte b) : Red(r), Green(g), Blue(b) { };
		byte Red;
		byte Green;
		byte Blue;
	};

	struct FramebufferDescriptor
	{
		FramebufferDescriptor(Pixel* pixels, int width, int height) : PixelArray(pixels), Width(width), Height(height) {};
		const Pixel* PixelArray;
		const int Width;
		const int Height;
	};

	typedef uint8_t audiosample;

	class QkError : public std::exception
	{
	protected:
		int m_errorCode = 0;

	public:
		QkError(const char* message, const int errorCode) : exception(message), m_errorCode(errorCode) {};
		int code() const { return m_errorCode; };
	};
}