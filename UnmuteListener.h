#pragma once

#undef  WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers

#include <atlsync.h>	// contains windows.h and other basic headers
#include <mmdeviceapi.h>
#include <endpointvolume.h>

extern HWND g_hwndOSD;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

class CListener : IMMNotificationClient, IAudioEndpointVolumeCallback {
private:
    BOOL                            m_bRegisteredForEndpointNotifications;
    BOOL                            m_bRegisteredForVolumeNotifications;
    CComPtr<IMMDeviceEnumerator>    m_spEnumerator;
    CComPtr<IMMDevice>              m_spAudioEndpoint;
    CComPtr<IAudioEndpointVolume>   m_spVolumeControl;
    CCriticalSection                m_csEndpoint;

    long                            m_cRef;

    ~CListener();       // refcounted object... make the destructor private

    // IMMNotificationClient (only need to really implement OnDefaultDeviceChanged)
    IFACEMETHODIMP OnDeviceStateChanged(LPCWSTR /*pwstrDeviceId*/, DWORD /*dwNewState*/) { return S_OK; }
    IFACEMETHODIMP OnDeviceAdded(LPCWSTR /*pwstrDeviceId*/) { return S_OK; }
    IFACEMETHODIMP OnDeviceRemoved(LPCWSTR /*pwstrDeviceId*/) { return S_OK; }
    IFACEMETHODIMP OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId);   // ****
    IFACEMETHODIMP OnPropertyValueChanged(LPCWSTR /*pwstrDeviceId*/, const PROPERTYKEY /*key*/) { return S_OK; }
    IFACEMETHODIMP OnDeviceQueryRemove() { return S_OK; }
    IFACEMETHODIMP OnDeviceQueryRemoveFailed() { return S_OK; }
    IFACEMETHODIMP OnDeviceRemovePending() { return S_OK; }

    // IAudioEndpointVolumeCallback
    IFACEMETHODIMP OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify);   // we don't need this but it's required for class instantiation

    // IUnknown (COM)
    IFACEMETHODIMP QueryInterface(const IID& iid, void** ppUnk);

public:
    CListener();

    HRESULT Initialize();
    void    Dispose();
    HRESULT UnmuteAndSetMaxVol();
    void    DetachFromEndpoint();
    HRESULT AttachToDefaultEndpoint();

    // IUnknown (COM)
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();
};
