#pragma once


#pragma comment(lib,"windowsapp")
#pragma comment(lib, "runtimeobject")
#include <winrt/ppl.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Media.Devices.h>
#include <winrt/Windows.Media.Audio.h>
#include <concrt.h>


#include <wrl.h>
#include <mfapi.h>
#include <AudioClient.h>
#include <mmdeviceapi.h>

#include <miqs>
#include <vector>
#include <string>



namespace MiqsAudio
{

typedef std::uint8_t byte_t;



//
// Audio Format
//
enum class AudioFormat
{
	None,
	Int8,
	Int16,
	//Int24,
	Int32,
	Float32,
	Float64
};

//
// Current Audio State
//
enum class AudioState
{
	Stopped,
	Stopping,
	Running,
	Closed,
	Initialized
};

//
// Audio Information Data
//
struct AudioInfo
{
	uint32_t sampleRate;
	uint32_t bufferSize;
	uint32_t nChannels[2];	// capture and render
	bool interleaved;
	AudioFormat format;
};

//
// Device Initial State
//
enum DeviceInitialState
{
	RENDER_INITIALIZED = 2,
	CAPTURE_INITIALIZED = 4,
	BOTH_INITIALIZED = 6,
	UNINITIALIZED = 0,
};

//
// Device Parameter
//
struct DeviceParameter
{
	uint32_t deviceId;
};

// Error Types
//
enum class AudioInterfaceError
{
	Unspecified,
	InvalidDeviceState,
	InvalidArguments,
	DeviceError,
	Warning,
	MemoryError,
	SystemError
};

// Audio Handle Interface
struct AudioHandleinterface {};

// Device Information
struct DeviceInfo
{
	std::wstring deviceName;
	std::wstring deviceId;
	uint32_t nChannel;
	uint32_t preferSampleRate;
	bool isCaptureDevice;
	bool isDefaultDevice;
	AudioFormat audioFormat;

	DeviceInfo():
		deviceName{ L"" },
		deviceId{ L"" },
		nChannel{ 0 },
		preferSampleRate{ 0 },
		isCaptureDevice{ false },
		isDefaultDevice{ false },
		audioFormat{ AudioFormat::Float32 }
	{}
};

// ConvertInfo
struct ConvertInfo
{
	size_t nChannels;
	AudioFormat srcFormat;
	AudioFormat dstFormat;
	size_t srcSampleStep, dstSampleStep;
	std::vector<size_t> srcChannelOffset, dstChannelOffset;
};

template <typename Func>
struct signal
{
	using container_type = std::list<Func>;
	signal() = default;
	signal(signal&& other)
	{
		this->signals = std::move(other.signals);
	}

	signal& operator=(signal&& other) { this->signals = std::move(other.signals); return *this; }
	signal& operator=(signal const& other) { this->signals = other.signals; return *this; }


	void connect(Func const& f) noexcept
	{
		signals.push_back(f);
	}
	void disconnect(Func const& f) noexcept
	{
		auto iter = std::find(std::begin(signals), std::end(signals), f);
		if (iter != std::end(signals)) signals.erase(iter);
	}

	template <typename ...Args>
	void emit(Args... args)
	{
		for (auto& f : signals) f(args...);
	}

private:
	container_type signals;
};


// MiqAudio Callback Function and Signal
using CallbackFunction = std::function<void(byte_t* inputBuffer,
											byte_t * outputBuffer,
											AudioInfo audioInfo,
											AudioState audioState,
											uint64_t time, void*data)>;
using CallbackSignal = signal<CallbackFunction>;


// Device State Changed
using DeviceInitializedFunction = std::function<void(AudioInfo const& info)>;
using DeviceInitializedSignal = signal<DeviceInitializedFunction>;

// AudioInterface State Changed
using AudioStateCallFunction = std::function<void(AudioState state)>;
using AudioStateChanged = signal<AudioStateCallFunction>;



//
// Exception
//
class ErrorException
{
public:
	ErrorException(std::wstring const & message, AudioInterfaceError error = AudioInterfaceError::Unspecified) throw()
		:m_msg{ message }, m_type{ error } {}

	virtual ~ErrorException() {}

	virtual const AudioInterfaceError getType(void) const { return m_type; }

	virtual const std::wstring& getMessage(void) const { return m_msg; }

	virtual const wchar_t* what(void) const { return m_msg.c_str(); }

private:
	std::wstring m_msg;
	AudioInterfaceError m_type;
};


//==========================================
size_t GetBytes(AudioFormat format);
void ConvertIOFormat(byte_t *dstBuffer, byte_t *srcBuffer, ConvertInfo &info, size_t bufferSize);
//===========


// Audio Interface Activator
class AudioInterfaceActivator:
	public Microsoft::WRL::RuntimeClass < Microsoft::WRL::RuntimeClassFlags< Microsoft::WRL::ClassicCom >, Microsoft::WRL::FtmBase, IActivateAudioInterfaceCompletionHandler >
{
	//concurrency::task_completion_event<void> m_ActivateCompleted;
	bool wait{ false };

	STDMETHODIMP ActivateCompleted(IActivateAudioInterfaceAsyncOperation  *pAsyncOp);

public:
	static Microsoft::WRL::ComPtr<IAudioClient3> AudioInterfaceActivator::ActivateAudioClientAsync(LPCWCHAR deviceId);
};





}


inline size_t MiqsAudio::GetBytes(AudioFormat format)
{
	switch (format)
	{
	case AudioFormat::Int8: return 1;
	case AudioFormat::Int16: return 2;
	case AudioFormat::Int32: case AudioFormat::Float32: return 4;
		//case AudioFormat::Int24: return 3;
	case AudioFormat::Float64: return 8;
	}
	return 0;
}