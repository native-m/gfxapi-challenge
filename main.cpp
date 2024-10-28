#include "main.h"
#include <fstream>
#include <filesystem>

static bool running = true;
constexpr int window_width = 640;
constexpr int window_height = 480;

std::vector<std::byte> load_file(const char* filepath) {
    auto size = std::filesystem::file_size(filepath);
    std::fstream file(filepath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return {};
    }
    std::vector<std::byte> contents(size);
    file.read((char*)contents.data(), size);
    file.close();
    return contents;
}

LRESULT wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_CLOSE:
            running = false;
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

int main() {
    WNDCLASSEX wcx {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW,
        .lpfnWndProc = wnd_proc,
        .hInstance = GetModuleHandle(nullptr),
        .hIcon = LoadIcon(NULL, IDI_APPLICATION),
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH),
        .lpszMenuName = nullptr,
        .lpszClassName = L"window_class",
        .hIconSm = LoadIcon(NULL, IDI_APPLICATION),
    };

    if (!RegisterClassEx(&wcx)) {
        return false;
    }

    int sc_width = GetSystemMetrics(SM_CXSCREEN);
    int sc_height = GetSystemMetrics(SM_CYSCREEN);
    int window_x = (sc_width - window_width) / 2;
    int window_y = (sc_height - window_height) / 2;
    RECT window_rect { .left = window_x, .top = window_y, .right = window_x + 640, .bottom = window_y + 480 };
    AdjustWindowRectEx(&window_rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);

    HWND hwnd = CreateWindowEx(WS_EX_APPWINDOW, L"window_class", L"Triangle", WS_OVERLAPPEDWINDOW, window_x, window_y,
                              window_rect.right - window_rect.left, window_rect.bottom - window_rect.top,
                              nullptr, nullptr, wcx.hInstance, nullptr);
    ShowWindow(hwnd, SW_SHOW);
    initialize(hwnd);

    while (running) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        render();
    }

    shutdown();
    UnregisterClass(L"window_class", wcx.hInstance);

    return 0;
}
