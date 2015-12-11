#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>
#include <Windows.h>
#include <stdlib.h>

#define DIRECTINPUT_VERSION 0x800
#include <ddraw.h>
#include <dinput.h>
#include <dsound.h>

#include "hook.h"
#include "watcall.h"

#define assert(X) {if (!(X)) __debugbreak();}

#define FULLSCREEN 1

namespace {

struct unknown1_t {
    int    a_;
    void * b_;
};

// Call fallouts debug log function
__declspec(naked)
void call_debug_log(const char * fmt, ...) {
    static const void * dst = (void*) 0x4B3008;
    __asm JMP dst
}

#define GLOBAL(TYPE, NAME, OFFSET)  TYPE & NAME = *(TYPE *)OFFSET
#define STRING(NAME, OFFSET) const char * NAME = (const char *)OFFSET

// Winmain Globals
STRING(gWN95Mutex,     0x4FE214 );

GLOBAL(HANDLE,         gMutex,         0x53A294);
GLOBAL(HINSTANCE,      gHInstance,     0x53A284);
GLOBAL(int,            gCmdShow,       0x53A28C);
GLOBAL(const char*,    gCmdArgs,       0x53A288);
GLOBAL(int,            g53A290,        0x53A290);

// DirectX Modules
GLOBAL(HMODULE,        gHModDDraw,     0x53A298);
GLOBAL(HMODULE,        gHModDInput,    0x53A29C);
GLOBAL(HMODULE,        gHModDSound,    0x53A2A0);

// DirectX COM Factory Functions
typedef HRESULT (__stdcall * DirectDrawCreate_t)(GUID * lpGUID,
                                                 LPDIRECTDRAW * lplpDD,
                                                 IUnknown * pUnkOuter );
GLOBAL(void*,          gFPDDCreate,    0x53A274);
GLOBAL(void*,          gFPDICreate,    0x53A278);
GLOBAL(void*,          gFPDSCreate,    0x53A27C);

// DirectX Instances
GLOBAL(IDirectDraw*,        gHDDraw,     0x539DE0);
GLOBAL(IDirectInputA*,      gHDInput,    0x53A278);
GLOBAL(IDirectSound*,       gHDSound,    0x53A27C);

GLOBAL(IDirectDrawPalette*, gPalette,    0x539DEC);
GLOBAL(IDirectDrawSurface*, gSurface,    0x539DE4);

// Window Instance
GLOBAL(HWND,           gWindow,        0x53A280);

} // namespace {}

// original @ 0x4B57F8
int impl_4B57F8(int width, int height, int bpp) {

    if (gHDDraw != nullptr ) {
        watcall<0x4B5EE8>();
        watcall<0x4B5A0C>(0ul);
        assert(!"todo");
        return -1;
    }

    if (DirectDrawCreate_t(gFPDDCreate)(nullptr,
                                        &gHDDraw, 
                                        nullptr) != DD_OK) {
        return -1;
    }

    DWORD coop_level = (FULLSCREEN)
                     ? DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE
                     : DDSCL_NORMAL;

    if (gHDDraw->SetCooperativeLevel(gWindow,
                                     coop_level) != DD_OK) {
        return -1;
    }

    if (gHDDraw->SetDisplayMode(width,
                                height,
                                bpp) != DD_OK) {
        return -1;
    }

    DDSURFACEDESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize         = sizeof(desc);
    desc.dwFlags        = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    if (gHDDraw->CreateSurface(&desc,
                               &gSurface,
                               nullptr) != DD_OK) {
        return -1;
    }

    *(IDirectDrawSurface**)0x539DE8 = gSurface;

    PALETTEENTRY temp[256];

    for (uint32_t i=0; i<256; ++i) {
        temp[i].peRed    = i;
        temp[i].peGreen  = i;
        temp[i].peBlue   = i;
        temp[i].peFlags  = 0;
    }

    if (bpp == 8) {
        if (gHDDraw->CreatePalette(DDPCAPS_ALLOW256 | DDPCAPS_8BIT,
                                   temp,
                                   &gPalette,
                                   nullptr) != DD_OK) {
            return -1;
        }
        if (gSurface->SetPalette(gPalette) != DD_OK) {
            return -1;
        }
    }
    else {
        assert(!"todo");
    }

    return 0;
}

__declspec (naked)
int hook_4B57F8() {
    // push callee save
    __asm push esi
    __asm push edi
    __asm push ecx
    __asm push ebx
    // args
    __asm push ebx
    __asm push edx
    __asm push eax
    __asm call impl_4B57F8
    // pop callee save
    __asm pop ebx
    __asm pop ecx
    __asm pop edi
    __asm pop esi
    // return result in eax
    __asm ret
}

// original @ 0x4725E8
int impl_4725E8(int a, void * b) {

    // Create autorun mutex
    if (watcall<int, 0x413450>(a, b) == 0) {
        return 1;
    }

    // Startup subsystems
    if (watcall<int, 0x4728C9>(a, b) != 1) {
        return 1;
    }

    watcall<0x4460C0>(0, 1);
    watcall<0x4460C0>(2, 0);

    // Load color.pal ?
    if (watcall<int, 0x472F80>() != 0) {
        // Abort
        watcall<0x4483E4>();
        watcall<0x472B3C>();
        watcall<0x43B654>();
        watcall<0x413490>();
        return 0;
    }

    int local = 1;

    // 0x47263C
    do {
        watcall<0x4B6548>();
        watcall<0x448338>("07desert", 0xB, &local, "language_filter");
        watcall<0x473378>(1, "preferences");
        watcall<0x4B4B88>();

        local = watcall<int, 0x47341C>();
        watcall<0x4B4D70>();

        assert(!"todo");
    }
    while (false);

    return 0;
}

// original @ 0x4C9F8C
LRESULT __stdcall window_proc(HWND hwnd,
                              UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam) {

    /*
    // Fall through to original fallout winproc handler
    const WNDPROC old_proc = (WNDPROC) 0x4C9F8C;
    return old_proc(hwnd, uMsg, wParam, lParam);
    */

    switch (uMsg) {
    case (WM_ACTIVATEAPP): {
        int *dw_53A290 = (int*) 0x53A290;
        *dw_53A290 = wParam;

        // Window is being activated
        if (wParam == TRUE) {
            watcall<0x4B43FC, int>(1);
            int * dw_672170 = (int*) 0x672170;
            watcall<0x4C3654, int>(*dw_672170);
        }
        // Window is being deactivated
        else {
            watcall<0x4B43FC, int>(0);
        }
        return 0;
        }

    case (WM_PAINT): {
        RECT wndRect;
        if (GetUpdateRect(hwnd, &wndRect, 0) == TRUE) {
            int32_t size[] = {
                wndRect.left,
                wndRect.top,
                wndRect.right-1,
                wndRect.bottom-1
            };
            watcall<0x4C3654, int>((int)&size);
        }
        }
        break;

    case (WM_SETCURSOR):
        if (hwnd == gWindow) {
            SetCursor(0);
            return 1;
        }
        break;

    case (WM_DESTROY):
        watcall<0x4CF800, int>(0);
        break;

    case (WM_SYSCOMMAND):
        if (wParam == SC_SCREENSAVE ||
            wParam == SC_MONITORPOWER) {
            return 0;
        }
    }

    // Pass message to default message handler
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

// original @ 0x4C9DF4
int check_win_version() {

    // Get os info
    OSVERSIONINFOA info;
    ZeroMemory(&info, sizeof(info));
    if (GetVersionExA(&info) != TRUE) {
        return 1;
    }

    do {
        // 1 = Windows 95
        if (info.dwPlatformId < 1)
            break;

        // 4 = Major version
        if (info.dwMajorVersion < 4)
            break;

        // All is well
        return 1;
    }
    while (false);

    // Display error message
    const char * capt = (const char *)0x4FE22C;
    const char * text = (const char *)0x4FE244;
    MessageBoxA(nullptr, text, capt, 0x10);

    // Fail
    return 0;
}

// original @ 0x4C9D84
int init_wndclass(HINSTANCE hinst) {

    // Create window class structure
    WNDCLASSA wndClass;
    ZeroMemory(&wndClass, sizeof(wndClass));

    // Fill window class properties
    wndClass.hIcon          = LoadIconA(hinst, (const char *)0x63);
    wndClass.lpszClassName  = "GNW95 Class";
    wndClass.lpfnWndProc    = window_proc;
    wndClass.hInstance      = hinst;
    wndClass.style          = CS_HREDRAW | CS_VREDRAW;
    wndClass.hbrBackground  = (HBRUSH) GetStockObject(BLACK_BRUSH);

    // Register the window class
    int res = RegisterClassA(&wndClass);
    return res & 0xffff;
}

// original @ 0x4B5738
int create_window() {

    // Early exit if the window already exists
    if (gWindow != nullptr) {
        return 0;
    }

#if FULLSCREEN
    // Fullscreen (Original)
    const DWORD style1 = WS_EX_TOPMOST;
    const DWORD style2 = WS_SYSMENU | WS_VISIBLE | WS_POPUP;
    const DWORD width  = GetSystemMetrics(SM_CXSCREEN);
    const DWORD height = GetSystemMetrics(SM_CYSCREEN);
    const DWORD x      = 0;
    const DWORD y      = 0;
#else
    // Windowed mode (Forced to full screen later...)
    const DWORD style1 = WS_EX_CLIENTEDGE;
    const DWORD style2 = WS_SYSMENU | WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    const DWORD width  = 640;
    const DWORD height = 480;
    const DWORD x      = CW_USEDEFAULT;
    const DWORD y      = CW_USEDEFAULT;
#endif

    // Get the window name
    const char * wnd_name = (const char *) 0x6B0760;

    // Create the main app window
    gWindow = CreateWindowExA(style1,
                              "GNW95 Class",
                              wnd_name,
                              style2,
                              x,
                              y,
                              width,
                              height,
                              nullptr,
                              nullptr,
                              gHInstance,
                              nullptr);

    if (gWindow == nullptr) {
        // Fail
        return -1;
    }

    // Run winproc and bring into focus
    UpdateWindow(gWindow);
    SetFocus(gWindow);

    // Success
    return 0;
}

__declspec(naked)
int hook_create_window() {
    // Note: watcom code calls into this function
    // callee save
    __asm PUSH EDX
    __asm PUSH ECX
    __asm PUSH EBX
    __asm PUSH ESI
    __asm PUSH EDI
    // Bounce into our create window
    __asm CALL create_window
    // restore callee save
    __asm POP EDI
    __asm POP ESI
    __asm POP EBX
    __asm POP ECX
    __asm POP EDX
    // return with EAX
    __asm RET
}

// original @ 0x4C9E60
int init_directx() {

    do {
        // Load direct draw library
        gHModDDraw = LoadLibraryA("DDRAW.DLL");
        if (! gHModDDraw)
            break;
        gFPDDCreate = (DirectDrawCreate_t) GetProcAddress(gHModDDraw, "DirectDrawCreate");
        if (! gFPDDCreate)
            break;

        // Load direct input library
        gHModDInput = LoadLibraryA("DINPUT.DLL");
        if (! gHModDInput)
            break;
        gFPDICreate = GetProcAddress(gHModDInput, "DirectInputCreateA");
        if (! gFPDICreate)
            break;

        // Load direct sound library
        gHModDSound = LoadLibraryA("DSOUND.DLL");
        if (! gHModDSound)
            break;
        gFPDSCreate = GetProcAddress(gHModDSound, "DirectSoundCreate");
        if (! gFPDSCreate)
            break;

        // Unknown?
        void * callback = (void*) 0x4C9F18;
        watcall<0x4CD0F1>(callback);

        // Success
        return 1;
    }
    while (false);

    // Free any loaded modules
    watcall<0x4C9F18>();

    // Show error message
    const char * capt = (const char *) 0x4FE2EC;
    const char * text = (const char *) 0x4FE304;
    MessageBoxA(0, capt, text, MB_ICONHAND);

    // Fail
    return 0;
}

template <typename type_t>
type_t read(uint32_t offset) {
    type_t val = 0;
    memcpy(&val, (void*)offset, sizeof(type_t));
    return val;
}

void place_hooks() {

    // Hook the create window function
    hook((void*)0x4B5738, (void*)hook_create_window);

    // Hook the direct draw init function
    hook((void*)0x4B57F8, (void*)hook_4B57F8);

    // Enable the debug log (debug.log)
    // note: must export env var $DEBUGACTIVE="log"
    if (getenv("DEBUGACTIVE")) {
        watcall<0x4B2E50>();
        call_debug_log("Fallout is hooked!");
    }

#if 0
    // gHDDraw->SetCooperativeLevel(gWindow, DDSCL_NORMAL)
    if (read<uint16_t>(0x4B5850) == /* push 11h */ 0x116A) {
        uint16_t data = /* push 08h */ 0x086A;
        patch((void*)0x4B5850, &data, sizeof(data));
    }

    // Nop out gHDDraw->SetDisplayMode()
    nop(0x4B586C, 0x4B587A - 0x4B586C);
#endif
}

// original @ 0x4C9C90
int CALLBACK fo_WinMain(_In_ HINSTANCE hInstance,
                        _In_ HINSTANCE hPrevInstance,
                        _In_ LPSTR     lpCmdLine,
                        _In_ int       nCmdShow){

    // Place additional hooks
    place_hooks();

    // Create global mutex
    gMutex = CreateMutexA(nullptr, TRUE, gWN95Mutex);
    if (GetLastError == 0) {
        return 0;
    }

    // Hide mouse pointer
    ShowCursor(false);

    // Register window class
    if (init_wndclass(hInstance) == 0) {
        CloseHandle(gMutex);
        return 0;
    }

    // Check for compatable windows version
    if (check_win_version() == 0) {
        CloseHandle(gMutex);
        return 0;
    }

    // Load directx modules and exported create functions
    if (init_directx() != 1) {
        CloseHandle(gMutex);
        return 0;
    }

    // Save global winmain params
    gHInstance = hInstance;
    gCmdArgs   = lpCmdLine;
    gCmdShow   = nCmdShow;

    // Unknown?
    unknown1_t local;
    ZeroMemory(&local, sizeof(local));
    if (watcall<int, 0x4CE0B4>(&local, lpCmdLine) == 0) {
        CloseHandle(gMutex);
        return 1;
    }

    // Unknown?
    watcall<0x4CE420>(1, 0x4C9F84, hInstance);
    watcall<0x4CE420>(3, 0x4C9F84, hInstance);
    watcall<0x4CE420>(5, 0x4C9F84, (void*) 1);

    // Unknown?
    g53A290 = 1;

    // Main game jumpoff point
#if 0
    impl_4725E8(local.a_, local.b_);
#else
    watcall<0x4725E8>(local.a_, local.b_);
#endif

    // Cleanup
    watcall<0x4CE24C>(&local);

    return 0;
}
