#include "Win32Common.hpp"
#include <shellapi.h>
#include <initguid.h>
#include <Usbiodef.h>
#include <winver.h>
#include <Dbt.h>

#if _WIN32_WINNT_WINBLUE
PFN_GetScaleFactorForMonitor MyGetScaleFactorForMonitor = nullptr;
#endif

#if _DEBUG
#define D3D11_CREATE_DEVICE_FLAGS D3D11_CREATE_DEVICE_DEBUG
#else
#define D3D11_CREATE_DEVICE_FLAGS 0
#endif

#include "boo/System.hpp"
#include "boo/IApplication.hpp"
#include "boo/inputdev/DeviceFinder.hpp"
#include "boo/graphicsdev/D3D.hpp"
#include "logvisor/logvisor.hpp"

#if BOO_HAS_VULKAN
#include "boo/graphicsdev/Vulkan.hpp"
#endif

DWORD g_mainThreadId = 0;

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#if _WIN32_WINNT_WIN10
PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignaturePROC = nullptr;
#endif
pD3DCompile D3DCompilePROC = nullptr;
pD3DCreateBlob D3DCreateBlobPROC = nullptr;

static bool FindBestD3DCompile()
{
    HMODULE d3dCompilelib = LoadLibraryW(L"D3DCompiler_47.dll");
    if (!d3dCompilelib)
    {
        d3dCompilelib = LoadLibraryW(L"D3DCompiler_46.dll");
        if (!d3dCompilelib)
        {
            d3dCompilelib = LoadLibraryW(L"D3DCompiler_45.dll");
            if (!d3dCompilelib)
            {
                d3dCompilelib = LoadLibraryW(L"D3DCompiler_44.dll");
                if (!d3dCompilelib)
                {
                    d3dCompilelib = LoadLibraryW(L"D3DCompiler_43.dll");
                }
            }
        }
    }
    if (d3dCompilelib)
    {
        D3DCompilePROC = (pD3DCompile)GetProcAddress(d3dCompilelib, "D3DCompile");
        D3DCreateBlobPROC = (pD3DCreateBlob)GetProcAddress(d3dCompilelib, "D3DCreateBlob");
        return D3DCompilePROC != nullptr && D3DCreateBlobPROC != nullptr;
    }
    return false;
}

namespace boo
{
static logvisor::Module Log("boo::ApplicationWin32");
Win32Cursors WIN32_CURSORS;

std::shared_ptr<IWindow> _WindowWin32New(SystemStringView title, Boo3DAppContextWin32& d3dCtx);

class ApplicationWin32 final : public IApplication
{
    IApplicationCallback& m_callback;
    const SystemString m_uniqueName;
    const SystemString m_friendlyName;
    const SystemString m_pname;
    const std::vector<SystemString> m_args;
    std::unordered_map<HWND, std::weak_ptr<IWindow>> m_allWindows;
    bool m_singleInstance;

    Boo3DAppContextWin32 m_3dCtx;
#if BOO_HAS_VULKAN
    PFN_vkGetInstanceProcAddr m_getVkProc = nullptr;
#endif

    void _deletedWindow(IWindow* window)
    {
        m_allWindows.erase(HWND(window->getPlatformHandle()));
    }

public:

    ApplicationWin32(IApplicationCallback& callback,
                     SystemStringView uniqueName,
                     SystemStringView friendlyName,
                     SystemStringView pname,
                     const std::vector<SystemString>& args,
                     std::string_view gfxApi,
                     uint32_t samples,
                     uint32_t anisotropy,
                     bool singleInstance)
    : m_callback(callback),
      m_uniqueName(uniqueName),
      m_friendlyName(friendlyName),
      m_pname(pname),
      m_args(args),
      m_singleInstance(singleInstance)
    {
        m_3dCtx.m_ctx11.m_sampleCount = samples;
        m_3dCtx.m_ctx11.m_anisotropy = anisotropy;
        m_3dCtx.m_ctx12.m_sampleCount = samples;
        m_3dCtx.m_ctx12.m_anisotropy = anisotropy;
        m_3dCtx.m_ctxOgl.m_glCtx.m_sampleCount = samples;
        m_3dCtx.m_ctxOgl.m_glCtx.m_anisotropy = anisotropy;
#if BOO_HAS_VULKAN
        g_VulkanContext.m_sampleCountColor = samples;
        g_VulkanContext.m_sampleCountDepth = samples;
        g_VulkanContext.m_anisotropy = anisotropy;
#endif

        HMODULE dxgilib = LoadLibraryW(L"dxgi.dll");
        if (!dxgilib)
            Log.report(logvisor::Fatal, "unable to load dxgi.dll");

        typedef HRESULT(WINAPI*CreateDXGIFactory1PROC)(REFIID riid, _COM_Outptr_ void **ppFactory);
        CreateDXGIFactory1PROC MyCreateDXGIFactory1 = (CreateDXGIFactory1PROC)GetProcAddress(dxgilib, "CreateDXGIFactory1");
        if (!MyCreateDXGIFactory1)
            Log.report(logvisor::Fatal, "unable to find CreateDXGIFactory1 in DXGI.dll\n");

        bool yes12 = false;
        bool noD3d = false;
#if BOO_HAS_VULKAN
        bool useVulkan = false;
#endif
        if (!gfxApi.empty())
        {
#if BOO_HAS_VULKAN
            if (!gfxApi.compare("D3D12"))
            {
                useVulkan = false;
                yes12 = true;
            }
            if (!gfxApi.compare("Vulkan"))
            {
                noD3d = true;
                useVulkan = true;
            }
            if (!gfxApi.compare("OpenGL"))
            {
                noD3d = true;
                useVulkan = false;
            }
#else
            if (!gfxApi.compare("D3D12"))
                yes12 = true;
            if (!gfxApi.compare("OpenGL"))
                noD3d = true;
#endif
        }
        for (const SystemString& arg : args)
        {
#if BOO_HAS_VULKAN
            if (!arg.compare(L"--d3d12"))
            {
                useVulkan = false;
                yes12 = true;
                noD3d = false;
            }
            if (!arg.compare(L"--d3d11"))
            {
                useVulkan = false;
                yes12 = false;
                noD3d = false;
            }
            if (!arg.compare(L"--vulkan"))
            {
                noD3d = true;
            }
            if (!arg.compare(L"--gl"))
            {
                noD3d = true;
                useVulkan = false;
            }
#else
            if (!arg.compare(L"--d3d12"))
            {
                yes12 = true;
                noD3d = false;
            }
            if (!arg.compare(L"--d3d11"))
            {
                yes12 = false;
                noD3d = false;
            }
            if (!arg.compare(L"--gl"))
                noD3d = true;
#endif
        }

#if _WIN32_WINNT_WIN10
        HMODULE d3d12lib = nullptr;
        if (yes12 && !noD3d)
            d3d12lib = LoadLibraryW(L"D3D12.dll");
        if (d3d12lib)
        {   
#if _DEBUG
            {
                PFN_D3D12_GET_DEBUG_INTERFACE MyD3D12GetDebugInterface =
                (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12lib, "D3D12GetDebugInterface");
                ComPtr<ID3D12Debug> debugController;
                if (SUCCEEDED(MyD3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
                {
                    debugController->EnableDebugLayer();
                }
            }
#endif
            if (!FindBestD3DCompile())
                Log.report(logvisor::Fatal, "unable to find D3DCompile_[43-47].dll");

            D3D12SerializeRootSignaturePROC =
            (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(d3d12lib, "D3D12SerializeRootSignature");

            /* Create device */
            PFN_D3D12_CREATE_DEVICE MyD3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12lib, "D3D12CreateDevice");
            if (!MyD3D12CreateDevice)
                Log.report(logvisor::Fatal, "unable to find D3D12CreateDevice in D3D12.dll");

            /* Obtain DXGI Factory */
            HRESULT hr = MyCreateDXGIFactory1(__uuidof(IDXGIFactory2), &m_3dCtx.m_ctx12.m_dxFactory);
            if (FAILED(hr))
                Log.report(logvisor::Fatal, "unable to create DXGI factory");

            /* Adapter */
            ComPtr<IDXGIAdapter1> ppAdapter;
            for (UINT adapterIndex = 0; ; ++adapterIndex)
            {
                ComPtr<IDXGIAdapter1> pAdapter;
                if (DXGI_ERROR_NOT_FOUND == m_3dCtx.m_ctx12.m_dxFactory->EnumAdapters1(adapterIndex, &pAdapter))
                    break;

                // Check to see if the adapter supports Direct3D 12, but don't create the
                // actual device yet.
                if (SUCCEEDED(MyD3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
                {
                    ppAdapter = std::move(pAdapter);
                    break;
                }
            }

            /* Create device */
            hr = ppAdapter ? MyD3D12CreateDevice(ppAdapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), &m_3dCtx.m_ctx12.m_dev) : E_FAIL;
            if (!FAILED(hr))
            {
                /* Establish loader objects */
                if (FAILED(m_3dCtx.m_ctx12.m_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                    __uuidof(ID3D12CommandAllocator), &m_3dCtx.m_ctx12.m_loadqalloc)))
                    Log.report(logvisor::Fatal, "unable to create loader allocator");

                D3D12_COMMAND_QUEUE_DESC desc =
                {
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
                    D3D12_COMMAND_QUEUE_FLAG_NONE
                };
                if (FAILED(m_3dCtx.m_ctx12.m_dev->CreateCommandQueue(&desc, __uuidof(ID3D12CommandQueue), &m_3dCtx.m_ctx12.m_loadq)))
                    Log.report(logvisor::Fatal, "unable to create loader queue");

                if (FAILED(m_3dCtx.m_ctx12.m_dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), &m_3dCtx.m_ctx12.m_loadfence)))
                    Log.report(logvisor::Fatal, "unable to create loader fence");

                m_3dCtx.m_ctx12.m_loadfencehandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

                if (FAILED(m_3dCtx.m_ctx12.m_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_3dCtx.m_ctx12.m_loadqalloc.Get(),
                    nullptr, __uuidof(ID3D12GraphicsCommandList), &m_3dCtx.m_ctx12.m_loadlist)))
                    Log.report(logvisor::Fatal, "unable to create loader list");

                Log.report(logvisor::Info, "initialized D3D12 renderer");
                return;
            }
            else
            {
                /* Some Win10 client HW doesn't support D3D12 (despite being supposedly HW-agnostic) */
                m_3dCtx.m_ctx12.m_dev.Reset();
                m_3dCtx.m_ctx12.m_dxFactory.Reset();
            }
        }
#endif
        HMODULE d3d11lib = nullptr;
        if (!noD3d)
            d3d11lib = LoadLibraryW(L"D3D11.dll");
        if (d3d11lib)
        {
            if (!FindBestD3DCompile())
                Log.report(logvisor::Fatal, "unable to find D3DCompile_[43-47].dll");

            /* Create device proc */
            PFN_D3D11_CREATE_DEVICE MyD3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11lib, "D3D11CreateDevice");
            if (!MyD3D11CreateDevice)
                Log.report(logvisor::Fatal, "unable to find D3D11CreateDevice in D3D11.dll");

            /* Create device */
            D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;
            ComPtr<ID3D11Device> tempDev;
            ComPtr<ID3D11DeviceContext> tempCtx;
            if (FAILED(MyD3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_FLAGS, &level,
                                           1, D3D11_SDK_VERSION, &tempDev, nullptr, &tempCtx)))
                Log.report(logvisor::Fatal, "unable to create D3D11 device");

            ComPtr<IDXGIDevice2> device;
            if (FAILED(tempDev.As<ID3D11Device1>(&m_3dCtx.m_ctx11.m_dev)) || !m_3dCtx.m_ctx11.m_dev ||
                FAILED(tempCtx.As<ID3D11DeviceContext1>(&m_3dCtx.m_ctx11.m_devCtx)) || !m_3dCtx.m_ctx11.m_devCtx ||
                FAILED(m_3dCtx.m_ctx11.m_dev.As<IDXGIDevice2>(&device)) || !device)
            {
                MessageBoxW(nullptr, L"Windows 7 users should install 'Platform Update for Windows 7':\n"
                                     L"https://www.microsoft.com/en-us/download/details.aspx?id=36805",
                                     L"IDXGIDevice2 interface error", MB_OK | MB_ICONERROR);
                exit(1);
            }

            /* Obtain DXGI Factory */
            ComPtr<IDXGIAdapter> adapter;
            device->GetParent(__uuidof(IDXGIAdapter), &adapter);
            adapter->GetParent(__uuidof(IDXGIFactory2), &m_3dCtx.m_ctx11.m_dxFactory);

            m_3dCtx.m_ctx11.m_anisotropy = std::min(m_3dCtx.m_ctx11.m_anisotropy, uint32_t(16));

            /* Build default sampler here */
            CD3D11_SAMPLER_DESC sampDesc(D3D11_DEFAULT);
            sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
            sampDesc.MaxAnisotropy = m_3dCtx.m_ctx11.m_anisotropy;
            m_3dCtx.m_ctx11.m_dev->CreateSamplerState(&sampDesc, &m_3dCtx.m_ctx11.m_ss[0]);

            sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
            sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
            sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
            m_3dCtx.m_ctx11.m_dev->CreateSamplerState(&sampDesc, &m_3dCtx.m_ctx11.m_ss[1]);

            sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            m_3dCtx.m_ctx11.m_dev->CreateSamplerState(&sampDesc, &m_3dCtx.m_ctx11.m_ss[2]);

            Log.report(logvisor::Info, "initialized D3D11 renderer");
            return;
        }

#if BOO_HAS_VULKAN
        if (useVulkan)
        {
            HMODULE vulkanLib = LoadLibraryW(L"vulkan-1.dll");
            if (vulkanLib)
            {
                m_getVkProc = (PFN_vkGetInstanceProcAddr)GetProcAddress(vulkanLib, "vkGetInstanceProcAddr");
                if (m_getVkProc)
                {
                    /* Check device support for vulkan */
                    vk::init_dispatch_table_top(PFN_vkGetInstanceProcAddr(m_getVkProc));
                    if (g_VulkanContext.m_instance == VK_NULL_HANDLE)
                    {
                        auto appName = getUniqueName();
                        if (g_VulkanContext.initVulkan(WCSTMBS(appName.data()).c_str()))
                        {
                            vk::init_dispatch_table_middle(g_VulkanContext.m_instance, false);
                            if (g_VulkanContext.enumerateDevices())
                            {
                                /* Obtain DXGI Factory */
                                HRESULT hr = MyCreateDXGIFactory1(__uuidof(IDXGIFactory1), &m_3dCtx.m_vulkanDxFactory);
                                if (FAILED(hr))
                                    Log.report(logvisor::Fatal, "unable to create DXGI factory");

                                Log.report(logvisor::Info, "initialized Vulkan renderer");
                                return;
                            }
                        }
                    }
                }
            }
        }
#endif

        /* Finally try OpenGL */
        {
            /* Obtain DXGI Factory */
            HRESULT hr = MyCreateDXGIFactory1(__uuidof(IDXGIFactory1), &m_3dCtx.m_ctxOgl.m_dxFactory);
            if (FAILED(hr))
                Log.report(logvisor::Fatal, "unable to create DXGI factory");

            Log.report(logvisor::Info, "initialized OpenGL renderer");
            return;
        }

        Log.report(logvisor::Fatal, "system doesn't support Vulkan, D3D11, or OpenGL");
    }

    EPlatformType getPlatformType() const
    {
        return EPlatformType::Win32;
    }

    LRESULT winHwndHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        /* Lookup boo window instance */
        auto search = m_allWindows.find(hwnd);
        if (search == m_allWindows.end())
            return DefWindowProc(hwnd, uMsg, wParam, lParam);;

        std::shared_ptr<IWindow> window = search->second.lock();
        if (!window)
            return DefWindowProc(hwnd, uMsg, wParam, lParam);

        switch (uMsg)
        {
            case WM_CREATE:
                return 0;

            case WM_DEVICECHANGE:
                return DeviceFinder::winDevChangedHandler(wParam, lParam);

            case WM_CLOSE:
            case WM_SIZE:
            case WM_MOVING:
            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYUP:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_XBUTTONDOWN:
            case WM_XBUTTONUP:
            case WM_MOUSEMOVE:
            case WM_MOUSELEAVE:
            case WM_NCMOUSELEAVE:
            case WM_MOUSEHOVER:
            case WM_NCMOUSEHOVER:
            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
            case WM_CHAR:
            case WM_UNICHAR:
            {
                HWNDEvent eventData(uMsg, wParam, lParam);
                window->_incomingEvent(&eventData);
            }

            default:
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    int run()
    {
        g_mainThreadId = GetCurrentThreadId();

        /* Spawn client thread */
        int clientReturn = 0;
        std::thread clientThread([&]()
        {
            std::string thrName = WCSTMBS(getFriendlyName().data()) + " Client Thread";
            logvisor::RegisterThreadName(thrName.c_str());
            CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
            clientReturn = m_callback.appMain(this);
            PostThreadMessage(g_mainThreadId, WM_USER+1, 0, 0);
        });

        /* Pump messages */
        MSG msg = {0};
        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (!msg.hwnd)
            {
                /* PostThreadMessage events */
                switch (msg.message)
                {
                case WM_USER:
                {
                    /* New-window message (coalesced onto main thread) */
                    std::unique_lock<std::mutex> lk(m_nwmt);
                    SystemStringView* title = reinterpret_cast<SystemStringView*>(msg.wParam);
                    m_mwret = newWindow(*title);
                    lk.unlock();
                    m_nwcv.notify_one();
                    continue;
                }
                case WM_USER+1:
                    /* Quit message from client thread */
                    PostQuitMessage(0);
                    continue;
                case WM_USER+2:
                    /* SetCursor call from client thread */
                    SetCursor(HCURSOR(msg.wParam));
                    continue;
                case WM_USER+3:
                    /* ImmSetOpenStatus call from client thread */
                    ImmSetOpenStatus(HIMC(msg.wParam), BOOL(msg.lParam));
                    continue;
                case WM_USER+4:
                    /* ImmSetCompositionWindow call from client thread */
                    ImmSetCompositionWindow(HIMC(msg.wParam), LPCOMPOSITIONFORM(msg.lParam));
                    continue;
                default: break;
                }
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        m_callback.appQuitting(this);
        clientThread.join();
        return clientReturn;
    }

    SystemStringView getUniqueName() const
    {
        return m_uniqueName;
    }

    SystemStringView getFriendlyName() const
    {
        return m_friendlyName;
    }

    SystemStringView getProcessName() const
    {
        return m_pname;
    }

    const std::vector<SystemString>& getArgs() const
    {
        return m_args;
    }

    std::mutex m_nwmt;
    std::condition_variable m_nwcv;
    std::shared_ptr<IWindow> m_mwret;
    std::shared_ptr<IWindow> newWindow(SystemStringView title)
    {
        if (GetCurrentThreadId() != g_mainThreadId)
        {
            std::unique_lock<std::mutex> lk(m_nwmt);
            if (!PostThreadMessage(g_mainThreadId, WM_USER, WPARAM(&title), 0))
                Log.report(logvisor::Fatal, "PostThreadMessage error");
            m_nwcv.wait(lk);
            std::shared_ptr<IWindow> ret = std::move(m_mwret);
            m_mwret.reset();
            return ret;
        }

        std::shared_ptr<IWindow> window = _WindowWin32New(title, m_3dCtx);
        HWND hwnd = HWND(window->getPlatformHandle());
        m_allWindows[hwnd] = window;
        return window;
    }
};

IApplication* APP = NULL;
int ApplicationRun(IApplication::EPlatformType platform,
                   IApplicationCallback& cb,
                   SystemStringView uniqueName,
                   SystemStringView friendlyName,
                   SystemStringView pname,
                   const std::vector<SystemString>& args,
                   std::string_view gfxApi,
                   uint32_t samples,
                   uint32_t anisotropy,
                   bool singleInstance)
{
    std::string thrName = WCSTMBS(friendlyName.data()) + " Main Thread";
    logvisor::RegisterThreadName(thrName.c_str());
    if (APP)
        return 1;
    if (platform != IApplication::EPlatformType::Win32 &&
        platform != IApplication::EPlatformType::Auto)
        return 1;

#if _WIN32_WINNT_WINBLUE
    /* HI-DPI support */
    HMODULE shcoreLib = LoadLibraryW(L"Shcore.dll");
    if (shcoreLib)
        MyGetScaleFactorForMonitor =
            (PFN_GetScaleFactorForMonitor)GetProcAddress(shcoreLib, "GetScaleFactorForMonitor");
#endif

    WIN32_CURSORS.m_arrow = LoadCursor(nullptr, IDC_ARROW);
    WIN32_CURSORS.m_hResize = LoadCursor(nullptr, IDC_SIZEWE);
    WIN32_CURSORS.m_vResize = LoadCursor(nullptr, IDC_SIZENS);
    WIN32_CURSORS.m_ibeam = LoadCursor(nullptr, IDC_IBEAM);
    WIN32_CURSORS.m_crosshairs = LoadCursor(nullptr, IDC_CROSS);
    WIN32_CURSORS.m_wait = LoadCursor(nullptr, IDC_WAIT);

    /* One class for *all* boo windows */
    WNDCLASS wndClass =
    {
        0,
        WindowProc,
        0,
        0,
        GetModuleHandle(nullptr),
        0,
        0,
        0,
        0,
        L"BooWindow"
    };
    wndClass.hIcon = LoadIconW(wndClass.hInstance, MAKEINTRESOURCEW(101));
    wndClass.hCursor = WIN32_CURSORS.m_arrow;
    ATOM a = RegisterClassW(&wndClass);

    APP = new ApplicationWin32(cb, uniqueName, friendlyName, pname, args,
                               gfxApi, samples, anisotropy, singleInstance);
    return APP->run();
}

}

static const DEV_BROADCAST_DEVICEINTERFACE HOTPLUG_CONF =
{
    sizeof(DEV_BROADCAST_DEVICEINTERFACE),
    DBT_DEVTYP_DEVICEINTERFACE,
    0,
    0
};
static bool HOTPLUG_REGISTERED = false;
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!HOTPLUG_REGISTERED && uMsg == WM_CREATE)
    {
        /* Register hotplug notification with windows */
        RegisterDeviceNotification(hwnd, (LPVOID)&HOTPLUG_CONF,
                                   DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
        HOTPLUG_REGISTERED = true;
    }
    return static_cast<boo::ApplicationWin32*>(boo::APP)->winHwndHandler(hwnd, uMsg, wParam, lParam);
}

