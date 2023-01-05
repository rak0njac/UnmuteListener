#include "UnmuteListener.h"
#include <new>

// ..::SECTION::.. -- GLOBAL VARS

CListener*     g_pListener = NULL;
HWND           g_hwndOSD = NULL;

// ..::SECTION::.. -- WINAPI

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    // Init COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (SUCCEEDED(hr)) {
        // Init listener
        g_pListener = new (std::nothrow) CListener();
        if (g_pListener) {
            hr = g_pListener->Initialize();
            if (SUCCEEDED(hr)) {

                // let me know if even simpler class decl exists
                WNDCLASSEX wcex = { sizeof(wcex) };
                wcex.lpfnWndProc    = WndProc;
                wcex.hInstance      = hInstance;
                wcex.lpszClassName  = L"UNMTLSTN";

                RegisterClassEx(&wcex);

                // create window handle and start listening for events
                g_hwndOSD = CreateWindowEx(NULL, wcex.lpszClassName, NULL, NULL, 0, 0, 0, 0, NULL, NULL, wcex.hInstance, NULL);
                if (g_hwndOSD) {
                    MSG msg;
                    while (GetMessage(&msg, NULL, 0, 0)) {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }

                // properly destroy COM
                g_pListener->Dispose();
                g_pListener->Release();
            }
        }
        CoUninitialize();
    }
    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        //we have only 1 user event so no need to define custom event names
        case WM_USER:
            g_pListener->DetachFromEndpoint();
            g_pListener->AttachToDefaultEndpoint();
            g_pListener->UnmuteAndSetMaxVol();

            return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

// ..::SECTION::.. -- COM CLASS --  Everything below should be self-explanatory, for any needed reference read IMMNotificationClient
//                                  and IAudioEndpointVolumeCallback on WINAPI reference website

CListener::CListener()
    : m_bRegisteredForEndpointNotifications(FALSE), m_bRegisteredForVolumeNotifications(FALSE), m_cRef(1) {}

CListener::~CListener() {}

void CListener::Dispose() {
    DetachFromEndpoint();

    if (m_bRegisteredForEndpointNotifications) {
        m_spEnumerator->UnregisterEndpointNotificationCallback(this);
        m_bRegisteredForEndpointNotifications = FALSE;
    }
}

HRESULT CListener::Initialize() {
    HRESULT hr;

    hr = m_spEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));
    if (SUCCEEDED(hr)) {
        hr = m_spEnumerator->RegisterEndpointNotificationCallback(this);
        if (SUCCEEDED(hr))
            hr = AttachToDefaultEndpoint();
    }

    return hr;
}

HRESULT CListener::UnmuteAndSetMaxVol() {
    HRESULT hr = E_FAIL;

    m_csEndpoint.Enter();

    if (m_spVolumeControl != NULL) {
        hr = m_spVolumeControl->SetMute(FALSE, &GUID_NULL);
        if (SUCCEEDED(hr))
            hr = m_spVolumeControl->SetMasterVolumeLevelScalar(1.0, &GUID_NULL);
    }

    m_csEndpoint.Leave();

    return hr;
}

HRESULT CListener::AttachToDefaultEndpoint() {
    m_csEndpoint.Enter();

    HRESULT hr = m_spEnumerator->GetDefaultAudioEndpoint(eCapture, eMultimedia, &m_spAudioEndpoint);
    if (SUCCEEDED(hr)) {
        hr = m_spAudioEndpoint->Activate(__uuidof(m_spVolumeControl), CLSCTX_INPROC_SERVER, NULL, (void**)&m_spVolumeControl);
        if (SUCCEEDED(hr)) {
            hr = m_spVolumeControl->RegisterControlChangeNotify(this);
            m_bRegisteredForVolumeNotifications = SUCCEEDED(hr);
        }
    }

    m_csEndpoint.Leave();

    return TRUE;
}

void CListener::DetachFromEndpoint() {
    m_csEndpoint.Enter();

    if (m_spVolumeControl != NULL) {
        if (m_bRegisteredForVolumeNotifications)
        {
            m_spVolumeControl->UnregisterControlChangeNotify(this);
            m_bRegisteredForVolumeNotifications = FALSE;
        }
        m_spVolumeControl.Release();
    }

    if (m_spAudioEndpoint != NULL) {
        m_spAudioEndpoint.Release();
    }

    m_csEndpoint.Leave();
}

HRESULT CListener::OnDefaultDeviceChanged(EDataFlow flow, ERole, LPCWSTR) {
    if (flow == eCapture) {
        if (g_hwndOSD != NULL)
            PostMessage(g_hwndOSD, WM_USER, 0, 0);
    }
    return S_OK;
}

HRESULT CListener::OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA)
{
    return S_OK;
}

//  IUnknown methods -- COM -- don't touch

HRESULT CListener::QueryInterface(REFIID iid, void** ppUnk)
{
    if (iid == __uuidof(IUnknown) || iid == __uuidof(IMMNotificationClient))
        *ppUnk = static_cast<IMMNotificationClient*>(this);
    else if (iid == __uuidof(IAudioEndpointVolumeCallback))
        *ppUnk = static_cast<IAudioEndpointVolumeCallback*>(this);
    else {
        *ppUnk = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG CListener::AddRef() {
    return InterlockedIncrement(&m_cRef);
}

ULONG CListener::Release() {
    long lRef = InterlockedDecrement(&m_cRef);
    if (lRef == 0)
        delete this;
    return lRef;
}
