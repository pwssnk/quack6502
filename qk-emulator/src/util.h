#pragma once

#include <vector>
#include "definitions.h"

namespace Qk { namespace Util
{
	bool ValidateBCD(byte bcd);
	byte BCDtoBIN(byte bcd);
	byte BINtoBCD(byte bin);

	byte ReverseBits(byte data);

	void PrintBits(byte data);

	template<typename T>
	class CircularBuffer
	{
	protected:
		std::vector<T> m_vec;
		int m_maxSize;

	public:
		CircularBuffer(int size) : m_maxSize(size) 
		{ 
			m_vec.resize(m_maxSize);
		};

		void SetSize(int size)
		{
			m_maxSize = size;
			m_vec.resize(size);
		}

		int GetSize() const
		{
			return m_vec.size();
		}

		void Push(T value)
		{
			if (m_vec.size() == m_maxSize)
				m_vec.erase(m_vec.begin());

			m_vec.push_back(value);
		}

		void Copy(T* target, size_t size)
		{
			if (size >= m_vec.size())
			{
				std::copy(m_vec.begin(), m_vec.end(), target);
			}
			else
			{
				std::copy(m_vec.begin(), m_vec.begin() + (size - 1), target);
			}
		}

		T* Data()
		{
			return m_vec.data();
		}
	};
}}