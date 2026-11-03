#pragma once
// =====================================================================
//  Overlay.h  -  VOLD LINE Transparent DirectX11 ImGui Overlay
// =====================================================================

#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <cmath>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// ImGui needs this to receive mouse and keyboard input from our window
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// =====================================================================
//  Vector2  -  a point on your SCREEN (x, y)
// =====================================================================
struct Vector2
{
    float x = 0.0f;
    float y = 0.0f;

    Vector2() = default;
    Vector2(float x, float y) : x(x), y(y) {}

    Vector2 operator+(const Vector2& other) const { return Vector2(x + other.x, y + other.y); }
    Vector2 operator-(const Vector2& other) const { return Vector2(x - other.x, y - other.y); }
    Vector2 operator*(float scale)          const { return Vector2(x * scale, y * scale); }
    Vector2 operator/(float scale)          const { return Vector2(x / scale, y / scale); }

    // How long this vector is
    float Length() const { return sqrtf(x * x + y * y); }

    // How far away another point is
    float Distance(const Vector2& other) const { return (*this - other).Length(); }
};

// =====================================================================
//  Vector3  -  a position in the 3D GAME WORLD (x, y, z)
// =====================================================================
struct Vector3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vector3() = default;
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& other) const { return Vector3(x + other.x, y + other.y, z + other.z); }
    Vector3 operator-(const Vector3& other) const { return Vector3(x - other.x, y - other.y, z - other.z); }
    Vector3 operator*(float scale)          const { return Vector3(x * scale, y * scale, z * scale); }
    Vector3 operator/(float scale)          const { return Vector3(x / scale, y / scale, z / scale); }

    float Length() const { return sqrtf(x * x + y * y + z * z); }
    float Distance(const Vector3& other) const { return (*this - other).Length(); }
};

struct Matrix4x4
{
    float m[4][4];
};

// =====================================================================
//  Colors
// =====================================================================

// Make your own color: Color(red, green, blue, alpha)  - each from 0 to 255
inline ImU32 Color(int r, int g, int b, int a = 255)
{
    return IM_COL32(r, g, b, a);
}

namespace Colors
{
    const ImU32 White = IM_COL32(255, 255, 255, 255);
    const ImU32 Black = IM_COL32(0, 0, 0, 255);
    const ImU32 Red = IM_COL32(255, 0, 0, 255);
    const ImU32 Green = IM_COL32(0, 255, 0, 255);
    const ImU32 Blue = IM_COL32(0, 120, 255, 255);
    const ImU32 Yellow = IM_COL32(255, 255, 0, 255);
    const ImU32 Orange = IM_COL32(255, 165, 0, 255);
    const ImU32 Purple = IM_COL32(170, 0, 255, 255);
    const ImU32 Cyan = IM_COL32(0, 255, 255, 255);
}

// =====================================================================
//  Drawing functions  -  call these inside YOUR drawing function
// =====================================================================

// Draws a line from one point to another
inline void DrawLine(Vector2 from, Vector2 to, ImU32 color = Colors::White, float thickness = 1.0f)
{
    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(from.x, from.y), ImVec2(to.x, to.y), color, thickness);
}

// Draws an empty box. 'topLeft' is the top-left corner of the box.
inline void DrawBox(Vector2 topLeft, float width, float height, ImU32 color = Colors::White, float thickness = 1.0f)
{
    ImGui::GetBackgroundDrawList()->AddRect(
        ImVec2(topLeft.x, topLeft.y),
        ImVec2(topLeft.x + width, topLeft.y + height),
        color, 0.0f, 0, thickness);
}

// Draws a box filled with color (tip: use a low alpha like Color(0,0,0,100) for see-through)
inline void DrawFilledBox(Vector2 topLeft, float width, float height, ImU32 color = Colors::White)
{
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        ImVec2(topLeft.x, topLeft.y),
        ImVec2(topLeft.x + width, topLeft.y + height),
        color);
}

// Draws an outlined box (colored box with a 1px inner and outer black border for crisp contrast)
inline void DrawOutlinedBox(Vector2 topLeft, float width, float height, ImU32 color = Colors::White, float thickness = 1.0f)
{
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    drawList->AddRect(ImVec2(topLeft.x - 1.0f, topLeft.y - 1.0f), ImVec2(topLeft.x + width + 1.0f, topLeft.y + height + 1.0f), Colors::Black, 0.0f, 0, thickness);
    drawList->AddRect(ImVec2(topLeft.x, topLeft.y), ImVec2(topLeft.x + width, topLeft.y + height), color, 0.0f, 0, thickness);
    drawList->AddRect(ImVec2(topLeft.x + 1.0f, topLeft.y + 1.0f), ImVec2(topLeft.x + width - 1.0f, topLeft.y + height - 1.0f), Colors::Black, 0.0f, 0, thickness);
}

// Draws tactical corner brackets around a bounding box
inline void DrawCornerBox(Vector2 topLeft, float width, float height, ImU32 color = Colors::White, float thickness = 1.0f)
{
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    float lineW = width / 4.0f;
    float lineH = height / 4.0f;
    float x = topLeft.x;
    float y = topLeft.y;

    // Top-Left
    drawList->AddLine(ImVec2(x, y), ImVec2(x + lineW, y), color, thickness);
    drawList->AddLine(ImVec2(x, y), ImVec2(x, y + lineH), color, thickness);
    // Top-Right
    drawList->AddLine(ImVec2(x + width, y), ImVec2(x + width - lineW, y), color, thickness);
    drawList->AddLine(ImVec2(x + width, y), ImVec2(x + width, y + lineH), color, thickness);
    // Bottom-Left
    drawList->AddLine(ImVec2(x, y + height), ImVec2(x + lineW, y + height), color, thickness);
    drawList->AddLine(ImVec2(x, y + height), ImVec2(x, y + height - lineH), color, thickness);
    // Bottom-Right
    drawList->AddLine(ImVec2(x + width, y + height), ImVec2(x + width - lineW, y + height), color, thickness);
    drawList->AddLine(ImVec2(x + width, y + height), ImVec2(x + width, y + height - lineH), color, thickness);
}

// Draws an empty circle
inline void DrawCircle(Vector2 center, float radius, ImU32 color = Colors::White, float thickness = 1.0f)
{
    ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(center.x, center.y), radius, color, 0, thickness);
}

// Helper to draw a solid filled circle / dot
inline void DrawFilledCircle(Vector2 center, float radius, ImU32 color = Colors::White)
{
    ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(center.x, center.y), radius, color);
}

// Draws text on the screen
inline void DrawString(Vector2 position, const char* text, ImU32 color = Colors::White)
{
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(position.x, position.y), color, text);
}

// Draws text with an outline shadow for high legibility against bright backgrounds
inline void DrawOutlinedString(Vector2 position, const char* text, ImU32 color = Colors::White, ImU32 outlineColor = Colors::Black)
{
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec2 pos(position.x, position.y);
    drawList->AddText(ImVec2(pos.x - 1.0f, pos.y), outlineColor, text);
    drawList->AddText(ImVec2(pos.x + 1.0f, pos.y), outlineColor, text);
    drawList->AddText(ImVec2(pos.x, pos.y - 1.0f), outlineColor, text);
    drawList->AddText(ImVec2(pos.x, pos.y + 1.0f), outlineColor, text);
    drawList->AddText(pos, color, text);
}

// Returns the size of your screen, e.g. (1920, 1080)
inline Vector2 GetScreenSize()
{
    return Vector2((float)GetSystemMetrics(SM_CXSCREEN), (float)GetSystemMetrics(SM_CYSCREEN));
}

// =====================================================================
//  Internal overlay state & window management
// =====================================================================
namespace OverlayInternal
{
    inline HWND                    window = nullptr;
    inline ID3D11Device* device = nullptr;
    inline ID3D11DeviceContext* context = nullptr;
    inline IDXGISwapChain* swapChain = nullptr;
    inline ID3D11RenderTargetView* renderTarget = nullptr;

    inline HWND previousWindow = nullptr;
    inline int  g_MenuToggleKey = VK_INSERT;
    inline bool g_ShowMenu = false;
    inline bool g_ShouldExit = false;
    inline bool g_IsBindingKey = false;

    inline LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        switch (msg)
        {
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) // disable the ALT menu
                return 0;
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    inline bool CreateOverlayWindow()
    {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_CLASSDC;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"VoldLineOverlayClass";
        RegisterClassExW(&wc);

        Vector2 screen = GetScreenSize();

        // TOPMOST     = always on top of other windows
        // LAYERED     = allows transparency
        // TRANSPARENT = mouse clicks go through the window
        // TOOLWINDOW  = hides it from the taskbar
        // NOACTIVATE  = prevents stealing focus and minimizing the game
        window = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            wc.lpszClassName, L"VOLD LINE", WS_POPUP,
            0, 0, (int)screen.x, (int)screen.y,
            nullptr, nullptr, wc.hInstance, nullptr);

        if (!window)
            return false;

        SetLayeredWindowAttributes(window, RGB(0, 0, 0), 255, LWA_ALPHA);

        // Makes the black background fully see-through
        MARGINS margins = { -1 };
        DwmExtendFrameIntoClientArea(window, &margins);

        ShowWindow(window, SW_SHOWDEFAULT);
        UpdateWindow(window);
        return true;
    }

    inline bool CreateDirectX()
    {
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 2;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = window;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
        D3D_FEATURE_LEVEL featureLevel;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            levels, 2, D3D11_SDK_VERSION, &sd, &swapChain, &device, &featureLevel, &context);

        if (hr == DXGI_ERROR_UNSUPPORTED)
            hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
                levels, 2, D3D11_SDK_VERSION, &sd, &swapChain, &device, &featureLevel, &context);

        if (FAILED(hr))
            return false;

        ID3D11Texture2D* backBuffer = nullptr;
        swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        device->CreateRenderTargetView(backBuffer, nullptr, &renderTarget);
        backBuffer->Release();
        return true;
    }

    inline void Cleanup()
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        if (renderTarget) { renderTarget->Release(); renderTarget = nullptr; }
        if (swapChain) { swapChain->Release();    swapChain = nullptr; }
        if (context) { context->Release();      context = nullptr; }
        if (device) { device->Release();       device = nullptr; }

        DestroyWindow(window);
        UnregisterClassW(L"SimpleOverlayClass", GetModuleHandleW(nullptr));
    }

    // Click-through ON  = clicks go to the game
    // Click-through OFF = clicks go to our menu
    inline void SetClickThrough(bool enabled)
    {
        LONG_PTR style = GetWindowLongPtrW(window, GWL_EXSTYLE);
        if (enabled) style |= WS_EX_TRANSPARENT;
        else         style &= ~WS_EX_TRANSPARENT;
        SetWindowLongPtrW(window, GWL_EXSTYLE, style);
        SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    // Opens or closes the menu, properly routing clicks and focus
    inline void SetMenuState(bool open)
    {
        if (g_ShowMenu == open) return;
        g_ShowMenu = open;
        SetClickThrough(!g_ShowMenu);

        if (g_ShowMenu)
        {
            previousWindow = GetForegroundWindow();
            SetWindowLongPtrW(window, GWL_EXSTYLE, GetWindowLongPtrW(window, GWL_EXSTYLE) & ~WS_EX_NOACTIVATE);
            SetActiveWindow(window);
        }
        else
        {
            SetWindowLongPtrW(window, GWL_EXSTYLE, GetWindowLongPtrW(window, GWL_EXSTYLE) | WS_EX_NOACTIVATE);
            if (previousWindow)
            {
                SetForegroundWindow(previousWindow);
            }
        }
    }

    inline void ToggleMenu()
    {
        SetMenuState(!g_ShowMenu);
    }

    inline bool KeyPressed(int key, bool& wasDown)
    {
        bool isDown = (GetAsyncKeyState(key) & 0x8000) != 0;
        bool pressed = isDown && !wasDown;
        wasDown = isDown;
        return pressed;
    }
}

// =====================================================================
//  RunOverlay  -  Starts the overlay loop with drawing & menu callbacks
// =====================================================================
inline void RunOverlay(void (*drawFunction)(), void (*menuFunction)() = nullptr)
{
    using namespace OverlayInternal;

    SetProcessDPIAware();

    if (!CreateOverlayWindow()) { MessageBoxA(nullptr, "Failed to create overlay window", "Error", MB_OK); return; }
    if (!CreateDirectX()) { MessageBoxA(nullptr, "Failed to create DirectX", "Error", MB_OK); return; }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, context);

    bool running = true;
    bool menuKeyWasDown = false;
    bool endWasDown = false;

    while (running && !g_ShouldExit)
    {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT)
                running = false;
        }
        if (!running || g_ShouldExit)
            break;

        if (KeyPressed(VK_END, endWasDown))
            break;

        if (!g_IsBindingKey)
        {
            if (KeyPressed(g_MenuToggleKey, menuKeyWasDown))
            {
                ToggleMenu();
            }
        }

        ImGui::GetIO().MouseDrawCursor = g_ShowMenu;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (g_ShowMenu)
        {
            if (menuFunction)
            {
                menuFunction();
            }
            else
            {
                ImGui::Begin("Overlay Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
                ImGui::Text("Menu Toggle Key = show/hide menu   END = exit");
                ImGui::Separator();
                if (ImGui::Button("Exit"))
                    running = false;
                ImGui::End();
            }
        }

        if (drawFunction)
            drawFunction();

        ImGui::Render();
        const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        context->OMSetRenderTargets(1, &renderTarget, nullptr);
        context->ClearRenderTargetView(renderTarget, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        swapChain->Present(1, 0);
    }

    Cleanup();
}