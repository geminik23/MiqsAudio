#pragma once
#include "MiqsAudio.Base.h"

namespace MiqsAudio
{


struct IAudioSignalHandledListener
{
	virtual void OnAudioSignalHandled() = 0;
};

struct IAudioSampleHandler
{
	concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }

	virtual void PushInputSignal(MiqsAudio::byte_t* inputBuffer, MiqsAudio::AudioInfo const& audioInfo) = 0;
	virtual void Process() = 0;
	virtual void PullOutputSignal(MiqsAudio::byte_t* outputBuffer, MiqsAudio::AudioInfo const& audioInfo) = 0;
	virtual bool Check() = 0;

private:
	concurrency::critical_section m_criticalSection;
};



struct MiqsAudioHandler
{
	void operator()(MiqsAudio::byte_t* inputBuffer,
					MiqsAudio::byte_t * outputBuffer,
					MiqsAudio::AudioInfo audioInfo,
					MiqsAudio::AudioState audioState,
					uint64_t time, void*data);

	std::shared_ptr<IAudioSampleHandler> AudioSampleHandler() const noexcept { return m_sampleHandler; }
	void AudioSampleHandler(std::shared_ptr<IAudioSampleHandler> const& handler) noexcept { m_sampleHandler = handler; }

	void AudioSignalHandledListener(IAudioSignalHandledListener * listener) noexcept { this->m_handledListener = listener; }

protected:
	void NotifyAudioSignalHandled() noexcept { if (this->m_handledListener) m_handledListener->OnAudioSignalHandled(); }

	IAudioSignalHandledListener * m_handledListener{ nullptr };
	std::shared_ptr<IAudioSampleHandler> m_sampleHandler;
};

inline void MiqsAudioHandler::operator()(MiqsAudio::byte_t * inputBuffer,
										 MiqsAudio::byte_t * outputBuffer,
										 MiqsAudio::AudioInfo audioInfo,
										 MiqsAudio::AudioState /*audioState*/,
										 uint64_t /*time*/, void * /*data*/)
{
	if (!this->m_sampleHandler)return;

	this->m_sampleHandler->PushInputSignal(inputBuffer, audioInfo);
	this->m_sampleHandler->Process();
	this->m_sampleHandler->PullOutputSignal(outputBuffer, audioInfo);

	if (this->m_sampleHandler->Check()) this->NotifyAudioSignalHandled();
}



}