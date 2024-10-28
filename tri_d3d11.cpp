#include "main.h"

#include <dxgi1_2.h>
#include <d3d11.h>;

static IDXGISwapChain1* swapchain {};
static ID3D11Device* device {};
static ID3D11DeviceContext* ctx {};
static ID3D11RenderTargetView* swapchain_rtv;
static ID3D11VertexShader* vs;
static ID3D11PixelShader* ps;
static ID3D11Buffer* vtx_buffer;
static ID3D11InputLayout* input_layout;
static ID3D11RasterizerState* raster_state;

static Vertex tri_vtx[3] = {
    { 0.0f, 0.5f, 0, 255, 0, 255 },
    { 0.5f, -0.5f, 0, 0, 255, 255 },
    { -0.5f, -0.5f, 255, 0, 0, 255 },
};

void initialize(HWND hwnd) {
    HMODULE d3d11_dll = LoadLibrary(L"d3d11.dll");
    assert(d3d11_dll != nullptr);

    PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN create_device_and_swapchain =
        (PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN)GetProcAddress(d3d11_dll, "D3D11CreateDeviceAndSwapChain");
    assert(create_device_and_swapchain);

    DXGI_SWAP_CHAIN_DESC swapchain_desc {
        .BufferDesc = {
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        },
        .SampleDesc = {.Count = 1},
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 2,
        .OutputWindow = hwnd,
        .Windowed = TRUE,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = 0,
    };

    const D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
    const UINT device_flags = D3D11_CREATE_DEVICE_DEBUG;
    IDXGISwapChain* new_swapchain {};
    HRESULT hr = create_device_and_swapchain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, device_flags, &feature_level, 1, D3D11_SDK_VERSION,
                                             &swapchain_desc, &new_swapchain, &device, nullptr, &ctx);
    assert(SUCCEEDED(hr));
    new_swapchain->QueryInterface(&swapchain);
    new_swapchain->Release();

    ID3D11Texture2D* backbuffer {};
    swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
    assert(backbuffer != nullptr);

    hr = device->CreateRenderTargetView(backbuffer, nullptr, &swapchain_rtv);
    assert(SUCCEEDED(hr));
    backbuffer->Release();

    std::vector<std::byte> vs_content = load_file("shaders/shader_vs.hlsl.dxbc");
    assert(vs_content.size() != 0);
    hr = device->CreateVertexShader(vs_content.data(), vs_content.size(), nullptr, &vs);
    assert(SUCCEEDED(hr));

    std::vector<std::byte> ps_content = load_file("shaders/shader_ps.hlsl.dxbc");
    assert(ps_content.size() != 0);
    hr = device->CreatePixelShader(ps_content.data(), ps_content.size(), nullptr, &ps);
    assert(SUCCEEDED(hr));

    D3D11_BUFFER_DESC vtx_buffer_desc {
        .ByteWidth = sizeof(tri_vtx),
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_VERTEX_BUFFER,
    };

    D3D11_SUBRESOURCE_DATA vtx_buffer_data {
        .pSysMem = tri_vtx,
        .SysMemPitch = sizeof(tri_vtx),
    };
    hr = device->CreateBuffer(&vtx_buffer_desc, &vtx_buffer_data, &vtx_buffer);
    assert(SUCCEEDED(hr));

    D3D11_INPUT_ELEMENT_DESC elem[2] {
        {
            .SemanticName = "POSITION",
            .SemanticIndex = 0,
            .Format = DXGI_FORMAT_R32G32_FLOAT,
            .InputSlot = 0,
            .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
        },
        {
            .SemanticName = "COLOR",
            .SemanticIndex = 0,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .InputSlot = 0,
            .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
        },
    };
    hr = device->CreateInputLayout(elem, 2, vs_content.data(), vs_content.size(), &input_layout);
    assert(SUCCEEDED(hr));

    D3D11_RASTERIZER_DESC rasterizer {
        .FillMode = D3D11_FILL_SOLID,
        .CullMode = D3D11_CULL_NONE,
        .DepthClipEnable = TRUE,
    };
    hr = device->CreateRasterizerState(&rasterizer, &raster_state);
    assert(SUCCEEDED(hr));
}

void shutdown() {
    vtx_buffer->Release();
    ps->Release();
    vs->Release();
    swapchain_rtv->Release();
    swapchain->Release();
    ctx->Release();
    device->Release();
}

void render() {
    float clear[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    ctx->ClearRenderTargetView(swapchain_rtv, clear);
    ctx->OMSetRenderTargets(1, &swapchain_rtv, nullptr);
    ctx->VSSetShader(vs, nullptr, 0);
    ctx->PSSetShader(ps, nullptr, 0);
    ctx->IASetInputLayout(input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    uint32_t stride = sizeof(Vertex);
    uint32_t offset = 0;
    ctx->IASetVertexBuffers(0, 1, &vtx_buffer, &stride, &offset);
    ctx->RSSetState(raster_state);

    D3D11_VIEWPORT viewport {
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = 640.0f,
        .Height = 480.0f,
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    };
    ctx->RSSetViewports(1, &viewport);

    RECT scissor {
       .left = 0,
       .top = 0,
       .right = 640,
       .bottom = 480,
    };
    ctx->RSSetScissorRects(1, &scissor);
    ctx->Draw(3, 0);

    swapchain->Present(1, 0);
}
