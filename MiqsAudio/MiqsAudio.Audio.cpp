#include "pch.h"
#include "MiqsAudio.Audio.h"

using namespace MiqsAudio;


using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Devices::Enumeration;
using namespace winrt::Windows::Media::Devices;

using namespace Microsoft::WRL;

namespace MiqsAudio
{
struct AudioHandle: AudioHandleinterface
{
	AudioHandle() { format[0] = nullptr; format[1] = nullptr; }
	~AudioHandle() { freeFormat(); }
	Microsoft::WRL::ComPtr<IAudioCaptureClient> captureAudioClient;
	Microsoft::WRL::ComPtr<IAudioRenderClient> renderAudioClient;
	Microsoft::WRL::ComPtr<IAudioClient3> audioClient[2];
	WAVEFORMATEX* format[2];
private:
	void freeFormat()
	{
		if (format[0])CoTaskMemFree(format[0]);
		if (format[1])CoTaskMemFree(format[1]);
		format[0] = nullptr;
		format[1] = nullptr;
	}
};



}



template <typename T>
inline void CopySamples(byte_t* dst, size_t idxDst, byte_t* src, size_t idxSrc, size_t size)
{
	if (size == 0) return;
	memcpy((void*)(&((T*)dst)[idxDst]), (void*)(&((T*)src)[idxSrc]), size);
}

void ConvertDifferentSamplerate(byte_t * out, byte_t * in,
								const unsigned int nChannel,
								const unsigned int inSamplerate,
								const unsigned int outSamplerate,
								const unsigned int /*iSampleCount*/,
								unsigned int &oSampleCount,
								//double& sampleFraction,
								AudioFormat format)
{
	double sampleRatio = (double)outSamplerate / inSamplerate;
	double sampleStep = 1.0 / sampleRatio;
	size_t i{};
	double sampleFraction{ 0.0 };


	for (size_t o{}; o < oSampleCount; ++o)
	{
		//sampleFraction = std::fmod(sampleFraction, (double)oSampleCount);
		i = (size_t)sampleFraction;

		switch (format)
		{
		case AudioFormat::Int8:
			CopySamples<int8_t>(out, o*nChannel, in, i*nChannel, nChannel * sizeof(int8_t));
			break;
		case AudioFormat::Int16:
			CopySamples<int16_t>(out, o*nChannel, in, i*nChannel, nChannel * sizeof(int16_t));
			break;
		case AudioFormat::Int32:
			CopySamples<int32_t>(out, o*nChannel, in, i*nChannel, nChannel * sizeof(int32_t));
			break;
		case AudioFormat::Float32:
			CopySamples<float>(out, o*nChannel, in, i*nChannel, nChannel * sizeof(float));
			break;
		case AudioFormat::Float64:
			CopySamples<double>(out, o*nChannel, in, i*nChannel, nChannel * sizeof(double));
			break;
		}


		sampleFraction += sampleStep;
	}
}








//=======================================================================

MiqsAudio::AudioInterface::AudioInterface()
{ _ClearStreamInfo(); }

MiqsAudio::AudioInterface::~AudioInterface()
{}

size_t MiqsAudio::AudioInterface::GetRenderDeviceCount()
{
	if (!m_device_enumerator.IsInitialized())m_device_enumerator.Update();
	return m_device_enumerator.RenderDeviceInfo().size();
}

size_t MiqsAudio::AudioInterface::GetCaptureDeviceCount()
{
	if (!m_device_enumerator.IsInitialized())m_device_enumerator.Update();
	return m_device_enumerator.CaptureDeviceInfo().size();
}

DeviceInfo MiqsAudio::AudioInterface::GetRenderDeviceInfo(uint32_t idx)
{
	return m_device_enumerator.GetDeviceInfo(idx, winrt::Windows::Devices::Enumeration::DeviceClass::AudioRender);
}

DeviceInfo MiqsAudio::AudioInterface::GetCaptureDeviceInfo(uint32_t idx)
{
	return m_device_enumerator.GetDeviceInfo(idx, winrt::Windows::Devices::Enumeration::DeviceClass::AudioCapture);
}


void MiqsAudio::AudioInterface::UpdateDeviceList() { m_device_enumerator.Update(); }

void MiqsAudio::AudioInterface::Initialize(DeviceParameter const * input,
										   DeviceParameter const * output,
										   AudioFormat format,
										   uint32_t samplerate,
										   uint32_t bufferSize)
{
	if (m_streaminfo.audioState != AudioState::Closed)
	{
		throw ErrorException(L"AudioInterface::Initialize()  - Device is not closed.", AudioInterfaceError::InvalidDeviceState);
	}

	this->_ClearStreamInfo();

	if (!input && !output)
	{
		throw ErrorException(L"AudioInterface::Initialize() - Both device parameters is nullptr.", AudioInterfaceError::InvalidArguments);
	}

	size_t oCount = this->GetRenderDeviceCount(),
		iCount = this->GetCaptureDeviceCount();
	if (input && input->deviceId >= iCount)
	{
		throw ErrorException(L"AudioInterface::Initialize() - Input DeviceIdx is Invalid.", AudioInterfaceError::InvalidArguments);
	}

	if (output&& output->deviceId >= oCount)
	{
		throw ErrorException(L"AudioInterface::Initialize() - Output DeviceIdx is Invalid.", AudioInterfaceError::InvalidArguments);
	}

	m_streaminfo.interfaceHandle = std::make_unique<AudioHandle>();


	if (output && !_InitializeDevice(RENDER, output->deviceId, samplerate, bufferSize, format))
	{
		throw ErrorException(L"AudioInterface::Initialize() - Device intialize Error", AudioInterfaceError::DeviceError);
	}

	if (input && !_InitializeDevice(CAPTURE, input->deviceId, samplerate, bufferSize, format))
	{
		throw ErrorException(L"AudioInterface::Initialize() - Device intialize Error", AudioInterfaceError::DeviceError);
	}

	if (output)
	{
		m_streaminfo.deviceId[RENDER] = output->deviceId;
		m_streaminfo.deviceInitialState = DeviceInitialState::RENDER_INITIALIZED;
	}
	if (input)
	{
		m_streaminfo.deviceId[CAPTURE] = input->deviceId;
		m_streaminfo.deviceInitialState
			= static_cast<DeviceInitialState>(static_cast<int>(m_streaminfo.deviceInitialState) | static_cast<int>(DeviceInitialState::CAPTURE_INITIALIZED));
	}

	m_streaminfo.audioInfo.bufferSize = bufferSize;
	m_streaminfo.audioInfo.format = format;
	m_streaminfo.audioInfo.sampleRate = samplerate;

	m_streaminfo.audioState = AudioState::Stopped;

	//// notify
	DeviceInitialized.emit(m_streaminfo.audioInfo);
	AudioStateChanged.emit(AudioState::Initialized);
}

void MiqsAudio::AudioInterface::Start()
{
	_CheckStream();
	if (this->IsAudioDeviceRunning())
	{
		throw ErrorException(L"AudioInterface::Start() - Already Running", AudioInterfaceError::Warning);
	}
	this->m_streaminfo.audioState = AudioState::Running;
	::concurrency::create_task(concurrency::task<void>{std::bind(&AudioInterface::_Threading, this)});

	// audio state event
	this->_EmitCurrentAudioStateEvent();
}

void MiqsAudio::AudioInterface::Stop(bool fireEvent)
{
	_CheckStream();

	if (m_streaminfo.audioState == AudioState::Stopped)
		throw ErrorException(L"AudioInterface::Stop() - Device is Already Stopped", AudioInterfaceError::InvalidDeviceState);

	m_streaminfo.audioState = AudioState::Stopping;
	// audio state event
	if (fireEvent)this->_EmitCurrentAudioStateEvent();

	using namespace std::chrono_literals;
	while (m_streaminfo.audioState != AudioState::Stopping)
	{
		std::this_thread::sleep_for(1ms);
	}
	using namespace std::chrono_literals;
	auto time = 1ms;
	time *= (long long)((double)m_streaminfo.audioInfo.bufferSize / (double)m_streaminfo.audioInfo.sampleRate);
	std::this_thread::sleep_for(time);


	if (m_streaminfo.interfaceHandle && reinterpret_cast<AudioHandle*>(m_streaminfo.interfaceHandle.get())->audioClient[CAPTURE])
	{
		HRESULT hr = reinterpret_cast<AudioHandle*>(m_streaminfo.interfaceHandle.get())->audioClient[CAPTURE]->Stop();
		if (FAILED(hr))
		{
			throw ErrorException(L"AudioInterface::Stop() - Fail to stop capture stream", AudioInterfaceError::DeviceError);
		}
	}

	if (m_streaminfo.interfaceHandle && reinterpret_cast<AudioHandle*>(m_streaminfo.interfaceHandle.get())->audioClient[RENDER])
	{
		HRESULT hr = reinterpret_cast<AudioHandle*>(m_streaminfo.interfaceHandle.get())->audioClient[RENDER]->Stop();
		if (FAILED(hr))
		{
			throw ErrorException(L"AudioInterface::Stop() - Fail to stop render stream", AudioInterfaceError::DeviceError);
		}
	}
	// audio state event
	if (fireEvent)this->_EmitCurrentAudioStateEvent();
}

void MiqsAudio::AudioInterface::Close()
{
	if (m_streaminfo.audioState == AudioState::Closed)
	{
		throw ErrorException(L"AudioInterface::Close() - Already closed", AudioInterfaceError::Warning);
	}

	if (m_streaminfo.audioState != AudioState::Stopped)
		this->Stop(false);

	// clean up stream memory
	_ClearStreamInfo();

	// update stream state
	m_streaminfo.audioState = AudioState::Closed;

	// audio state event
	this->_EmitCurrentAudioStateEvent();
}




//========================================PRIVATE
void MiqsAudio::AudioInterface::_ClearStreamInfo()
{

	this->m_streaminfo.audioInfo.bufferSize = 0;
	this->m_streaminfo.audioInfo.format = AudioFormat::Float64;
	this->m_streaminfo.audioInfo.nChannels[0] = 0;
	this->m_streaminfo.audioInfo.nChannels[1] = 0;
	this->m_streaminfo.audioInfo.sampleRate = 0;
	this->m_streaminfo.audioInfo.interleaved = true;
	this->m_streaminfo.needToConvert[0] = false;
	this->m_streaminfo.needToConvert[1] = false;


	this->m_streaminfo.audioState = AudioState::Closed;

	this->m_streaminfo.interfaceHandle.reset(nullptr);

	this->m_streaminfo.buffers[0].reset();
	this->m_streaminfo.buffers[1].reset();



	this->m_streaminfo.convertInfo[0].dstChannelOffset.clear();
	this->m_streaminfo.convertInfo[0].dstFormat = AudioFormat::Float64;
	this->m_streaminfo.convertInfo[0].dstSampleStep = 0;
	this->m_streaminfo.convertInfo[0].srcChannelOffset.clear();
	this->m_streaminfo.convertInfo[0].srcFormat = AudioFormat::Float64;
	this->m_streaminfo.convertInfo[0].srcSampleStep = 0;
	this->m_streaminfo.convertInfo[0].nChannels = 0;

	this->m_streaminfo.convertInfo[1].dstChannelOffset.clear();
	this->m_streaminfo.convertInfo[1].dstFormat = AudioFormat::Float64;
	this->m_streaminfo.convertInfo[1].dstSampleStep = 0;
	this->m_streaminfo.convertInfo[1].srcChannelOffset.clear();
	this->m_streaminfo.convertInfo[1].srcFormat = AudioFormat::Float64;
	this->m_streaminfo.convertInfo[1].srcSampleStep = 0;
	this->m_streaminfo.convertInfo[1].nChannels = 0;

	this->m_streaminfo.deviceFormat[0] = AudioFormat::Float64;
	this->m_streaminfo.deviceFormat[1] = AudioFormat::Float64;
	this->m_streaminfo.deviceId[0] = 0;
	this->m_streaminfo.deviceId[1] = 0;


	this->m_streaminfo.deviceInitialState = DeviceInitialState::UNINITIALIZED;
	this->m_streaminfo.time = 0;

}

void MiqsAudio::AudioInterface::_UpdateConvertInfo(DeviceMode type)
{

	if (type == CAPTURE)
	{
		m_streaminfo.convertInfo[type].dstFormat = m_streaminfo.audioInfo.format;
		m_streaminfo.convertInfo[type].srcFormat = m_streaminfo.deviceFormat[type];
		m_streaminfo.convertInfo[type].dstSampleStep = m_streaminfo.audioInfo.nChannels[type];
		m_streaminfo.convertInfo[type].srcSampleStep = m_streaminfo.audioInfo.nChannels[type];

	} else
	{
		m_streaminfo.convertInfo[type].dstFormat = m_streaminfo.deviceFormat[type];
		m_streaminfo.convertInfo[type].srcFormat = m_streaminfo.audioInfo.format;
		m_streaminfo.convertInfo[type].dstSampleStep = m_streaminfo.audioInfo.nChannels[type];
		m_streaminfo.convertInfo[type].srcSampleStep = m_streaminfo.audioInfo.nChannels[type];
	}

	m_streaminfo.convertInfo[type].nChannels = m_streaminfo.audioInfo.nChannels[type];

	for (size_t i = {}; i < m_streaminfo.convertInfo[type].nChannels; ++i)
	{
		m_streaminfo.convertInfo[type].srcChannelOffset.push_back(i);
		m_streaminfo.convertInfo[type].dstChannelOffset.push_back(i);
	}

}

void MiqsAudio::AudioInterface::_EmitCurrentAudioStateEvent()
{
	auto state = m_streaminfo.audioState;
	concurrency::create_task([this, state]() {
		this->AudioStateChanged.emit(state);
	});
}





void MiqsAudio::AudioInterface::_CheckStream()
{
	if (m_streaminfo.audioState == AudioState::Closed)
	{
		throw ErrorException(L"AudioInterface::_CheckStream() - Current Device State is Closed", AudioInterfaceError::InvalidDeviceState);
	}
}

bool AudioInterface::_InitializeDevice(DeviceMode type, uint32_t deviceId, uint32_t samplerate, uint32_t bufferSize, AudioFormat format)
{
	bool result = false;
	int cur_mode = static_cast<int>(type);
	auto& streaminfo = m_streaminfo;


	// set stream information
	streaminfo.deviceId[cur_mode] = deviceId;
	streaminfo.audioInfo.sampleRate = samplerate;
	streaminfo.audioInfo.format = format;
	streaminfo.audioInfo.bufferSize = bufferSize;

	if (!streaminfo.interfaceHandle)
	{
		throw ErrorException(L"AudioInterface::InitializeDevice() - handle is not initialized", AudioInterfaceError::Warning);
	}


	auto operation = DeviceInformation::FindAllAsync((cur_mode == RENDER) ? DeviceClass::AudioRender : DeviceClass::AudioCapture);
	DeviceInformationCollection devices = operation.get();
	if (deviceId >= devices.Size()) return false;

	auto deviceInfo = devices.GetAt(deviceId);

	auto client = AudioInterfaceActivator::ActivateAudioClientAsync(deviceInfo.Id().c_str());

	// 	// check WAVEFORMATEX
	if (!client) return result;

	WAVEFORMATEX * mixFormat{ nullptr };
	HRESULT hr = S_OK;
	hr = client->GetMixFormat(&mixFormat);

	// check WAVEFORMATEX
	//TODO
	if (!mixFormat) return result;

	//------------------initialize
	auto audiohandle = reinterpret_cast<AudioHandle*>(streaminfo.interfaceHandle.get());
	streaminfo.deviceFormat[cur_mode] = AudioFormat::None;
	streaminfo.audioInfo.nChannels[cur_mode] = mixFormat->nChannels;


	if (mixFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
		(mixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
		((WAVEFORMATEXTENSIBLE*)mixFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
	{
		if (mixFormat->wBitsPerSample == 32)
		{
			streaminfo.deviceFormat[cur_mode] = AudioFormat::Float32;
		} else if (mixFormat->wBitsPerSample == 64)
		{
			streaminfo.deviceFormat[cur_mode] = AudioFormat::Float64;
		}
	} else if (mixFormat->wFormatTag == WAVE_FORMAT_PCM ||
		(mixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
			   ((WAVEFORMATEXTENSIBLE*)mixFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_PCM))
	{
		if (mixFormat->wBitsPerSample == 8)
		{
			streaminfo.deviceFormat[cur_mode] = AudioFormat::Int8;
		} else if (mixFormat->wBitsPerSample == 16)
		{
			streaminfo.deviceFormat[cur_mode] = AudioFormat::Int16;
		}
		//else if (mixFormat->wBitsPerSample == 24)
		//{
		//	streaminfo.deviceFormat[cur_mode] = AudioFormat::Int24;
		//}
		else if (mixFormat->wBitsPerSample == 32)
		{
			streaminfo.deviceFormat[cur_mode] = AudioFormat::Int32;
		}
	}


	audiohandle->audioClient[cur_mode] = client;
	audiohandle->format[cur_mode] = mixFormat;



	double ratioOfCaptureSR = ((double)audiohandle->format[cur_mode]->nSamplesPerSec / streaminfo.audioInfo.sampleRate);
	double desireBuffSize = streaminfo.audioInfo.bufferSize * ratioOfCaptureSR;
	REFERENCE_TIME desireBufferPeriod = (REFERENCE_TIME)(desireBuffSize * 10000000 / audiohandle->format[cur_mode]->nSamplesPerSec);


	hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED,
							0,//AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
							desireBufferPeriod,
							desireBufferPeriod,
							audiohandle->format[cur_mode],
							nullptr);
	if (FAILED(hr))
	{
		throw ErrorException(L"AudioInterface::InitializeDevice() - Failed to initialize audio client", AudioInterfaceError::DeviceError);
	}


	if (cur_mode == CAPTURE)
	{
		hr = client->GetService(__uuidof(IAudioCaptureClient),
			(void**)&(audiohandle->captureAudioClient));

		if (FAILED(hr))
		{
			throw ErrorException(L"AudioInterface::InitializeDevice() - Failed to retrieve the capture client", AudioInterfaceError::DeviceError);
		}

	} else
	{
		hr = client->GetService(__uuidof(IAudioRenderClient),
			(void**)&(audiohandle->renderAudioClient));

		if (FAILED(hr))
		{
			throw ErrorException(L"AudioInterface::InitializeDevice() - Failed to retrieve the render client", AudioInterfaceError::DeviceError);
		}

	}


	streaminfo.deviceInitialState = static_cast<DeviceInitialState>(
		streaminfo.deviceInitialState | ((cur_mode == CAPTURE) ? CAPTURE_INITIALIZED : RENDER_INITIALIZED));





	streaminfo.audioInfo.interleaved = true;

	// Set flags for buffer conversion.
	streaminfo.needToConvert[cur_mode] = false;
	if (streaminfo.audioInfo.format != streaminfo.deviceFormat[cur_mode])
		streaminfo.needToConvert[cur_mode] = true;

	if (streaminfo.needToConvert[cur_mode])
		_UpdateConvertInfo(static_cast<DeviceMode>(cur_mode));


	uint32_t nBytes = streaminfo.audioInfo.bufferSize *
		streaminfo.audioInfo.nChannels[cur_mode] *
		GetBytes(streaminfo.audioInfo.format);

	if (nBytes == 0)
	{
		this->Close();
		throw ErrorException(L"AudioInterface::InitializeDevice() - nBytes is zero", AudioInterfaceError::Warning);
	}

	streaminfo.buffers[cur_mode].reset(nBytes);

	result = true;
	return result;
}


void AudioInterface::_TickTime() noexcept
{
	this->m_streaminfo.time += static_cast<uint64_t>((m_streaminfo.audioInfo.bufferSize * 1.0 / m_streaminfo.audioInfo.sampleRate) * 1000000000);
}






































/*
	1. circular structure
	2.
	3.
*/



//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================


//
// thread function
//

void MiqsAudio::AudioInterface::_Threading()
{
	HRESULT hr{ S_OK };
	std::wstring ErrorMsg{ L"" };
	auto& streaminfo = m_streaminfo;
	auto* handle = reinterpret_cast<AudioHandle *>(streaminfo.interfaceHandle.get());


	auto & callback = [this](byte_t* inputBuffer,
							 byte_t * outputBuffer,
							 AudioInfo audioInfo,
							 AudioState audioState,
							 uint64_t time, void*data) {
		this->AudioCallbackSignal.emit(inputBuffer, outputBuffer, audioInfo, audioState, time, data);
	};


	double ratioOfCaptureSR{};
	double ratioOfRenderSR{};

	size_t uFrameBuffSize{};
	size_t uNumPadding{};

	BYTE * uBuffer = nullptr;

	bool u_cbEmitted = { false };

	unsigned int deviceSr[2];

	miqs::array<byte_t> convert_buffer;
	miqs::array<byte_t> device_buffer;

	size_t convertBuffSize;
	size_t deviceBuffSize;

	DWORD captureFlag;

	miqs::sample_queue<byte_t> renderBuffer;
	miqs::sample_queue<byte_t> captureBuffer;



	//capture
	if (handle->audioClient[CAPTURE])
	{
		deviceSr[CAPTURE] = handle->format[CAPTURE]->nSamplesPerSec;
		auto& client = handle->audioClient[CAPTURE];

		ratioOfCaptureSR = ((double)deviceSr[CAPTURE] / streaminfo.audioInfo.sampleRate);


		UINT32 inBufferSize = 0;
		hr = client->GetBufferSize(&inBufferSize);
		if (FAILED(hr))
		{
			ErrorMsg = L"Failed to get buffer size from capture client";
			goto tExit;
		}


		//:::::::::::::::::::::::::::::::::::::::
		unsigned int outBufferSize = (unsigned int)(streaminfo.audioInfo.bufferSize * ratioOfCaptureSR);

		captureBuffer.resize((miqs::max_value((size_t)(inBufferSize*1.5), inBufferSize + outBufferSize))*handle->format[CAPTURE]->nBlockAlign);


		// reset the capture stream
		hr = client->Reset();
		if (FAILED(hr))
		{
			ErrorMsg = L"Failed to reset capture client";
			goto tExit;
		}

		// start the capture stream
		hr = client->Start();
		if (FAILED(hr))
		{
			ErrorMsg = L"Failed to start capture client";
			goto tExit;
		}
	}

	//render
	if (handle->audioClient[RENDER])
	{
		deviceSr[RENDER] = handle->format[RENDER]->nSamplesPerSec;
		auto client = handle->audioClient[RENDER];

		ratioOfRenderSR = ((double)deviceSr[RENDER] / streaminfo.audioInfo.sampleRate);


		UINT32 outBufferSize = 0;
		hr = client->GetBufferSize(&outBufferSize);
		if (FAILED(hr))
		{
			ErrorMsg = L"Failed to get buffer size from render client";
			goto tExit;
		}

		// set Buffer size

		unsigned int inBufferSize = (unsigned int)(streaminfo.audioInfo.bufferSize * ratioOfRenderSR);
		renderBuffer.resize(miqs::max_value((size_t)(outBufferSize *1.5), outBufferSize + inBufferSize)*handle->format[RENDER]->nBlockAlign);

		// reset the render stream
		hr = client->Reset();
		if (FAILED(hr))
		{
			ErrorMsg = L"Failed to reset render client";
			goto tExit;
		}

		// start the render stream
		hr = client->Start();
		if (FAILED(hr))
		{
			ErrorMsg = L"Failed to start render client";
			goto tExit;
		}
	}

	switch (streaminfo.deviceInitialState)
	{
	case DeviceInitialState::CAPTURE_INITIALIZED:
		deviceBuffSize = (size_t)(streaminfo.audioInfo.bufferSize * ratioOfCaptureSR) * streaminfo.audioInfo.nChannels[CAPTURE] * GetBytes(streaminfo.deviceFormat[CAPTURE]);
		convertBuffSize = streaminfo.audioInfo.bufferSize * streaminfo.audioInfo.nChannels[CAPTURE] * GetBytes(streaminfo.deviceFormat[CAPTURE]);
		break;
	case DeviceInitialState::RENDER_INITIALIZED:
		deviceBuffSize = (size_t)(streaminfo.audioInfo.bufferSize * ratioOfRenderSR) * streaminfo.audioInfo.nChannels[RENDER] * GetBytes(streaminfo.deviceFormat[RENDER]);
		convertBuffSize = streaminfo.audioInfo.bufferSize * streaminfo.audioInfo.nChannels[RENDER] * GetBytes(streaminfo.deviceFormat[RENDER]);

		break;
	case DeviceInitialState::BOTH_INITIALIZED:
		deviceBuffSize = miqs::max_value((size_t)(streaminfo.audioInfo.bufferSize * ratioOfCaptureSR) * streaminfo.audioInfo.nChannels[CAPTURE] * GetBytes(streaminfo.deviceFormat[CAPTURE]),
			(size_t)(streaminfo.audioInfo.bufferSize * ratioOfRenderSR) * streaminfo.audioInfo.nChannels[RENDER] * GetBytes(streaminfo.deviceFormat[RENDER]));
		convertBuffSize = miqs::max_value(streaminfo.audioInfo.bufferSize * streaminfo.audioInfo.nChannels[CAPTURE] * GetBytes(streaminfo.deviceFormat[CAPTURE]),
										  streaminfo.audioInfo.bufferSize * streaminfo.audioInfo.nChannels[RENDER] * GetBytes(streaminfo.deviceFormat[RENDER]));
		break;
	}

	convert_buffer.reset(convertBuffSize);
	device_buffer.reset(deviceBuffSize);

	size_t cdevice_size = (size_t)(streaminfo.audioInfo.bufferSize * ratioOfCaptureSR);
	size_t rdevice_size = (size_t)(streaminfo.audioInfo.bufferSize * ratioOfRenderSR);


	bool deviceInitialized[2] = { handle->audioClient[CAPTURE] , handle->audioClient[RENDER] };

	//---------------------------------------------LOOPING
	while (m_streaminfo.audioState != AudioState::Stopping)
	{

		// check buffered size
		// 
		/*
		1. read in_original
		2. convert in_original to in_client
		3. callback
		4. convert out_client to out_orignal
		5. write out_original
		*/

		if (!u_cbEmitted && m_streaminfo.audioState == AudioState::Running
			&& renderBuffer.push_available(rdevice_size*(handle->format[RENDER] ? handle->format[RENDER]->nBlockAlign : 0))
			&& captureBuffer.pull_available(cdevice_size*(handle->format[CAPTURE] ? handle->format[CAPTURE]->nBlockAlign : 0)))
		{


			if (deviceInitialized[CAPTURE] && handle->audioClient[CAPTURE])
			{
				// pull from capture buffer. //check size
				// device pull from buffer
				captureBuffer.pull(miqs::ptr_begin(device_buffer), miqs::ptr_at(device_buffer, cdevice_size*handle->format[CAPTURE]->nBlockAlign));
				// samplerate - convert<-device
				ConvertDifferentSamplerate(convert_buffer, device_buffer,
										   streaminfo.audioInfo.nChannels[CAPTURE],
										   deviceSr[CAPTURE],
										   streaminfo.audioInfo.sampleRate,
										   cdevice_size,
										   streaminfo.audioInfo.bufferSize,
										   streaminfo.deviceFormat[CAPTURE]);

				// convert - streaminfo <- convert
				ConvertIOFormat(streaminfo.buffers[CAPTURE].begin(), convert_buffer,
								streaminfo.convertInfo[CAPTURE],
								streaminfo.audioInfo.bufferSize);
			}


			callback((deviceInitialized[CAPTURE] ? streaminfo.buffers[CAPTURE].begin() : nullptr),
				(deviceInitialized[RENDER] ? streaminfo.buffers[RENDER].begin() : nullptr),
					 streaminfo.audioInfo, streaminfo.audioState, streaminfo.time, nullptr);

			if (deviceInitialized[RENDER] && handle->audioClient[RENDER])
			{
				// convert type
				// convert_buffer<-streaminfo.buffer
				ConvertIOFormat(convert_buffer, streaminfo.buffers[RENDER].begin(),
								streaminfo.convertInfo[RENDER],
								streaminfo.audioInfo.bufferSize);

				// samplerate    
				// device <- convert_buffer
				ConvertDifferentSamplerate(device_buffer, convert_buffer,
										   streaminfo.audioInfo.nChannels[RENDER],
										   streaminfo.audioInfo.sampleRate,
										   deviceSr[RENDER],
										   streaminfo.audioInfo.bufferSize,
										   rdevice_size,
										   streaminfo.deviceFormat[RENDER]);

				// push to render buffer
				renderBuffer.push(miqs::ptr_begin(device_buffer), miqs::ptr_at(device_buffer, rdevice_size*handle->format[RENDER]->nBlockAlign));
			}

			u_cbEmitted = true;
		}


		if (deviceInitialized[CAPTURE] && handle->audioClient[CAPTURE] && m_streaminfo.audioState == AudioState::Running)
		{
			hr = handle->captureAudioClient->GetNextPacketSize(&uFrameBuffSize);
			if (FAILED(hr))
			{
				ErrorMsg = L"Failed to retrieve capture next packet size";
				goto tExit;
			}

			if (uFrameBuffSize > 0/*cdevice_size*/ && captureBuffer.push_available(uFrameBuffSize*handle->format[CAPTURE]->nBlockAlign))
			{
				hr = handle->captureAudioClient->GetBuffer(&uBuffer, &uFrameBuffSize, &captureFlag, nullptr, nullptr);
				if (FAILED(hr))
				{
					ErrorMsg = L"Failed to retrieve capture buffer";
					goto tExit;
				}

				//Push
				captureBuffer.push(uBuffer, uBuffer + (uFrameBuffSize*handle->format[CAPTURE]->nBlockAlign));


				hr = handle->captureAudioClient->ReleaseBuffer(uFrameBuffSize);
				if (FAILED(hr))
				{
					ErrorMsg = L"Failed to release capture buffer";
					goto tExit;
				}


			} else
			{
				hr = handle->captureAudioClient->ReleaseBuffer(0);
				if (FAILED(hr))
				{
					ErrorMsg = L"Failed to release capture buffer";
					goto tExit;
				}
			}
		}


		if (deviceInitialized[RENDER] && handle->audioClient[RENDER] && m_streaminfo.audioState == AudioState::Running)
		{
			hr = handle->audioClient[RENDER]->GetBufferSize(&uFrameBuffSize);
			if (FAILED(hr))
			{
				ErrorMsg = L"Failed to retrieve render buffer size";
				goto tExit;
			}

			hr = handle->audioClient[RENDER]->GetCurrentPadding(&uNumPadding);
			if (FAILED(hr))
			{
				ErrorMsg = L"Failed to retrieve render buffer padding";
				goto tExit;
			}

			uFrameBuffSize -= uNumPadding;
			if (uFrameBuffSize >= rdevice_size && renderBuffer.pull_available(rdevice_size*handle->format[RENDER]->nBlockAlign))
			{
				hr = handle->renderAudioClient->GetBuffer(rdevice_size, &uBuffer);

				if ((SUCCEEDED(hr)) && renderBuffer.pull(uBuffer, uBuffer + (rdevice_size*handle->format[RENDER]->nBlockAlign)))
				{
					hr = handle->renderAudioClient->ReleaseBuffer(rdevice_size, 0);
				} else
				{
					hr = handle->renderAudioClient->ReleaseBuffer(0, 0);
				}
				if (FAILED(hr)) {}

			}
		}
		if (u_cbEmitted)
		{
			u_cbEmitted = false;
			this->_TickTime();
		}

	}



tExit:
	// change 	
	streaminfo.audioState = AudioState::Stopped;
	if (ErrorMsg != L"")
	{
		throw ErrorException(ErrorMsg, AudioInterfaceError::DeviceError);
	}
}