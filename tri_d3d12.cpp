#include "main.h"
#include <d3d12.h>
// otw

void initialize(HWND hwnd) {
    HMODULE d3d12_dll = LoadLibrary(L"d3d12.dll");
    PFN_D3D12_CREATE_DEVICE create_device = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12_dll, "D3D12CreateDevice");
}

void shutdown() {

}

void render() {

}
