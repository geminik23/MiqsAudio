#pragma once

#include "MiqsAudio.Base.h"

namespace MiqsAudio
{
	using DeviceInfoArray = std::vector<std::wstring>;

	enum class DeviceType
	{
		Capture = 1,
		Render,
		Both,
	};
	struct DeviceEnumerator
	{
		// get list of renderer and capturer
		
		void Update(DeviceType type = DeviceType::Both);

		DeviceInfoArray& CaptureDeviceInfo() noexcept { return m_capturer; }
		DeviceInfoArray& RenderDeviceInfo() noexcept { return m_renderer; }

		bool IsInitialized() const noexcept { return m_initialized; }

		MiqsAudio::DeviceInfo GetDeviceInfo(uint32_t idx, winrt::Windows::Devices::Enumeration::DeviceClass dclass);
	private:
		void _UpdateRenderDevice();
		void _UpdateCaptureDevice();

		bool m_initialized{ false };
		DeviceInfoArray m_renderer;
		DeviceInfoArray m_capturer;

	};

}