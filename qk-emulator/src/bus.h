#pragma once

#include <vector>
#include "definitions.h"


namespace Qk
{
	// Common bus signals
	static constexpr int SIGNAL_CPU_IRQ = 510; // CPU interrupt request
	static constexpr int SIGNAL_CPU_NMI = 520; // CPU non-maskable interrupt request
	static constexpr int SIGNAL_CPU_HLT = 577; // CPU halt
	static constexpr int SIGNAL_CPU_RSM = 578; // CPU resume

	class Bus 
	{
	public:
		class Device
		{
		protected:
			const AddressRange m_addressableRange;
			const bool m_isAddressable;

		protected:
			Bus& BUS;

		public:
			Device(Bus& bus, bool isAddressable, const AddressRange& addressRange);
			Device(Bus&);

			const AddressRange& GetAddressableRange() const;
			bool IsAddressable() const;

			virtual byte ReadFromDevice(word address, bool peek = false);
			virtual void WriteToDevice(word address, byte data);
			virtual void OnBusSignal(int signalId);
		};

		friend class Device;

	public:
		Bus();
		~Bus();

		byte ReadFromBus(word address);
		void WriteToBus(word address, byte data);
		void EmitSignal(int signalId);
		byte Peek(word address);

	protected:
		std::vector<Device*> m_addressableDevices;
		std::vector<Device*> m_nonAddressableDevices;
		Device* m_cachedDevicePtr = nullptr;
		
		bool CheckAddressBelongsTo(const Device& device, word address) const;
		bool CheckAddressRangeConflict(const Device& device, const AddressRange& range) const;
		bool CheckRangeAvailable(const AddressRange& range) const;
		Device* GetDeviceAt(word address);
		void ConnectDevice(Device& device);
	};
}
