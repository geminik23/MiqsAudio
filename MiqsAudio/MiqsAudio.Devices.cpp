#include "pch.h"
#include "MiqsAudio.Devices.h"



using namespace MiqsAudio;

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Devices::Enumeration;
using namespace winrt::Windows::Media::Devices;

using namespace Microsoft::WRL;

using namespace concurrency;



void MiqsAudio::DeviceEnumerator::Update(DeviceType type)
{
	if ((int)type | (int)DeviceType::Capture)this->_UpdateCaptureDevice();
	if ((int)type | (int)DeviceType::Render)this->_UpdateRenderDevice();
	m_initialized = true;
}

void MiqsAudio::DeviceEnumerator::_UpdateCaptureDevice()
{
	auto operation = DeviceInformation::FindAllAsync(DeviceClass::AudioCapture);

	DeviceInformationCollection devices = operation.get();
	for (size_t i{}; i < devices.Size(); ++i)
	{
		auto info = devices.GetAt(i);
		m_capturer.push_back({ info.Name().c_str() });
	}


}

void MiqsAudio::DeviceEnumerator::_UpdateRenderDevice()
{
	auto operation = DeviceInformation::FindAllAsync(DeviceClass::AudioRender);

	DeviceInformationCollection devices = operation.get();
	for (size_t i{}; i < devices.Size(); ++i)
	{
		auto info = devices.GetAt(i);
		m_renderer.push_back({ info.Name().c_str() });
	}
}


MiqsAudio::DeviceInfo MiqsAudio::DeviceEnumerator::GetDeviceInfo(uint32_t idx, winrt::Windows::Devices::Enumeration::DeviceClass dclass)
{
	DeviceInfo info;

	std::wstring defaultid;
	if (dclass == DeviceClass::AudioCapture)
		defaultid = MediaDevice::GetDefaultAudioCaptureId(winrt::Windows::Media::Devices::AudioDeviceRole::Default).c_str();
	else
		defaultid = MediaDevice::GetDefaultAudioRenderId(winrt::Windows::Media::Devices::AudioDeviceRole::Default).c_str();

	DeviceInformationCollection devices = DeviceInformation::FindAllAsync(dclass).get();

	auto deviceInfo = devices.GetAt(idx);
	info.deviceId = deviceInfo.Id();
	info.isDefaultDevice = (info.deviceId == defaultid);
	info.isCaptureDevice = dclass == DeviceClass::AudioCapture;
	info.deviceName = deviceInfo.Name();

	auto pAudioClient = AudioInterfaceActivator::ActivateAudioClientAsync(info.deviceId.c_str());

	DeviceInfo result = info;
	if (!pAudioClient) return result;

	WAVEFORMATEX * mixformat{ nullptr };
	HRESULT hr = S_OK;
	hr = pAudioClient->GetMixFormat(&mixformat);

	// check WAVEFORMATEX
	if (!mixformat) return result;


	result.nChannel = mixformat->nChannels;
	result.preferSampleRate = mixformat->nSamplesPerSec;
	result.audioFormat = AudioFormat::None;

	if (mixformat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
		(mixformat->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
		((WAVEFORMATEXTENSIBLE*)mixformat)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
	{
		if (mixformat->wBitsPerSample == 32)
		{
			result.audioFormat = AudioFormat::Float32;
		}
		else if (mixformat->wBitsPerSample == 64)
		{
			result.audioFormat = AudioFormat::Float64;
		}
	}
	else if (mixformat->wFormatTag == WAVE_FORMAT_PCM ||
		(mixformat->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
			 ((WAVEFORMATEXTENSIBLE*)mixformat)->SubFormat == KSDATAFORMAT_SUBTYPE_PCM))
	{
		if (mixformat->wBitsPerSample == 8)
		{
			result.audioFormat = AudioFormat::Int8;
		}
		else if (mixformat->wBitsPerSample == 16)
		{
			result.audioFormat = AudioFormat::Int16;
		}
		//else if (mixFormat->wBitsPerSample == 24)
		//{
		//	info.audioFormat = AudioFormat::Int24;
		//}
		else if (mixformat->wBitsPerSample == 32)
		{
			result.audioFormat = AudioFormat::Int32;
		}
	}
	CoTaskMemFree(mixformat);
	return result;
}

