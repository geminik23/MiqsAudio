#pragma once
#include "MiqsAudio.Base.h"
#include "MiqsAudio.Devices.h"

namespace MiqsAudio
{

	class AudioInterface
	{
	public:
		AudioStateChanged AudioStateChanged;
		DeviceInitializedSignal DeviceInitialized;
		CallbackSignal AudioCallbackSignal;


		AudioInterface();
		~AudioInterface();


		uint32_t GetCurrentRenderId() const noexcept { return m_streaminfo.deviceId[RENDER]; }
		uint32_t GetCurrentCaptureId() const noexcept { return m_streaminfo.deviceId[CAPTURE]; }

		size_t GetRenderDeviceCount();
		size_t GetCaptureDeviceCount();
		DeviceInfo GetRenderDeviceInfo(uint32_t idx);
		DeviceInfo GetCaptureDeviceInfo(uint32_t idx);
		void UpdateDeviceList();
		void Initialize(DeviceParameter const* in_idx,
						DeviceParameter const* out_idx,
						AudioFormat format,
						uint32_t samplerate,
						uint32_t bufferSize);

		void Start();
		void Stop(bool fireEvent = true);
		void Close();


		bool IsAudioDeviceOpen() const noexcept;
		bool IsAudioDeviceRunning() const noexcept;
		AudioState GetAudioState() const noexcept{ return m_streaminfo.audioState;}

	private:


		enum DeviceMode
		{
			CAPTURE = 0,
			RENDER,
			DEVICE_COUNT = 2
		};

		// for thread
		void _Threading();
		//

		void _TickTime() noexcept;
		void _CheckStream();
		bool _InitializeDevice(DeviceMode type, uint32_t deviceId, uint32_t samplerate, uint32_t bufferSize, AudioFormat format);
		void _ClearStreamInfo();
		void _UpdateConvertInfo(DeviceMode type);
		void _EmitCurrentAudioStateEvent();



		struct StreamInfo
		{
			uint32_t deviceId[DEVICE_COUNT];
			DeviceInitialState deviceInitialState; // initialized?
			miqs::array<byte_t> buffers[DEVICE_COUNT]; //

			AudioInfo audioInfo; //
			AudioFormat deviceFormat[DEVICE_COUNT]; //

			uint64_t time;
			std::unique_ptr<AudioHandleinterface> interfaceHandle;
			
			bool needToConvert[DEVICE_COUNT];
			ConvertInfo convertInfo[DEVICE_COUNT];
			AudioState audioState;
		} m_streaminfo;

		DeviceEnumerator m_device_enumerator;
	};



	inline bool AudioInterface::IsAudioDeviceOpen() const noexcept
	{
		return m_streaminfo.audioState != AudioState::Closed;
	}

	inline bool AudioInterface::IsAudioDeviceRunning() const noexcept
	{
		return m_streaminfo.audioState == AudioState::Running;
	}


}