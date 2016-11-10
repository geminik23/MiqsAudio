#include "pch.h"
#include "MiqsAudio.Base.h"

using namespace MiqsAudio;

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Devices::Enumeration;

using namespace Microsoft::WRL;

using namespace concurrency;

STDMETHODIMP AudioInterfaceActivator::ActivateCompleted(IActivateAudioInterfaceAsyncOperation * operation)
{
	//m_ActivateCompleted.set();
	wait = true;

	return S_OK;
}



ComPtr<IAudioClient3> AudioInterfaceActivator::ActivateAudioClientAsync(LPCWCHAR deviceId)
{
	ComPtr<AudioInterfaceActivator> pActivator = Make<AudioInterfaceActivator>();

	ComPtr<IActivateAudioInterfaceAsyncOperation> pAsyncOp;
	//ComPtr<IActivateAudioInterfaceCompletionHandler> pHandler = pActivator;


	HRESULT hr = ActivateAudioInterfaceAsync(
		deviceId,
		__uuidof(IAudioClient3),
		nullptr,
		pActivator.Get(),
		&pAsyncOp);

	if (FAILED(hr))
		throw ErrorException(L"Fail to activate 'AudioInterface'", AudioInterfaceError::DeviceError);

	while (!pActivator->wait) {}
	//task<void> waiting(pActivator->m_ActivateCompleted);
	//waiting.wait();
	//create_task(pActivator->m_ActivateCompleted);

	//HRESULT hr = S_OK;
	HRESULT hrActivateResult = S_OK;
	ComPtr<IUnknown> pUnk;

	hr = pAsyncOp->GetActivateResult(&hrActivateResult, &pUnk);

	ComPtr<IAudioClient3> pAudioClient3;

	if (SUCCEEDED(hr) && SUCCEEDED(hrActivateResult))
	{
		//Get the pointer for the AudioClient
		hr = pUnk.As(&pAudioClient3);

	}
	else  throw ErrorException(L"Fail to 'GetActivateResult'", AudioInterfaceError::DeviceError);

	if (FAILED(hr)) throw ErrorException(L"Fail to 'Query for the activated IAudioClient3 interface'", AudioInterfaceError::DeviceError);

	// Return retrieved interface
	return pAudioClient3;
}