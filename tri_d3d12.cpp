#include "main.h"
#include <dxgi1_4.h>
#include <d3d12.h>
#include <iostream>

using PfnCreateDXGIFactory2 = HRESULT(WINAPI*)(UINT flags, REFIID riid, _COM_Outptr_ void** ppFactory);

static constexpr uint32_t buffer_count = 2;

static ID3D12Device* device;
static ID3D12CommandQueue* queue;
static IDXGISwapChain3* swapchain;
static ID3D12RootSignature* root_signature;
static ID3D12PipelineState* pso;
static ID3D12DescriptorHeap* rtv_heap;
static ID3D12Resource* render_targets[buffer_count];
static D3D12_CPU_DESCRIPTOR_HANDLE rtv[buffer_count];

// Vertex buffer info
static ID3D12Resource* vtx_buffer;
static D3D12_VERTEX_BUFFER_VIEW vtx_buffer_view;

// Vulkan-like approach
static ID3D12CommandAllocator* cmd_allocator[buffer_count];
static ID3D12GraphicsCommandList* cmd_list;
static ID3D12Fence* fence;
static HANDLE fence_event;
static uint64_t fence_value[2] {};

static uint32_t frame_index = 0;

static Vertex tri_vtx[3] = {
    { 0.0f, 0.5f, 0, 255, 0, 255 },
    { 0.5f, -0.5f, 0, 0, 255, 255 },
    { -0.5f, -0.5f, 255, 0, 0, 255 },
};

template <typename T>
constexpr T* ptr_of(const T&& v) {
    return &v;
}

void wait_for_device();

void initialize(HWND hwnd) {
    HMODULE dxgi_dll = LoadLibrary(L"dxgi.dll");
    HMODULE d3d12_dll = LoadLibrary(L"d3d12.dll");
    PfnCreateDXGIFactory2  create_dxgi_factory2 = (PfnCreateDXGIFactory2)GetProcAddress(dxgi_dll, "CreateDXGIFactory2");
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE serialize_root_signature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(d3d12_dll, "D3D12SerializeRootSignature");
    PFN_D3D12_GET_DEBUG_INTERFACE get_debug_interface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12_dll, "D3D12GetDebugInterface");
    PFN_D3D12_CREATE_DEVICE create_device = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12_dll, "D3D12CreateDevice");

    ID3D12Debug* debug;
    HRESULT hr = get_debug_interface(IID_PPV_ARGS(&debug));
    assert(SUCCEEDED(hr));
    debug->EnableDebugLayer();
    debug->Release();

    IDXGIFactory2* factory;
    hr = create_dxgi_factory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory));
    assert(SUCCEEDED(hr));

    // Take whatever device in index 0
    IDXGIAdapter1* adapter;
    hr = factory->EnumAdapters1(0, &adapter);
    assert(SUCCEEDED(hr));

    DXGI_ADAPTER_DESC1 adapter_desc;
    adapter->GetDesc1(&adapter_desc);
    std::wcout << L"Adapter: " << adapter_desc.Description << std::endl;

    hr = create_device(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));
    assert(SUCCEEDED(hr));

    D3D12_COMMAND_QUEUE_DESC queue_desc {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
    };
    hr = device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue));
    assert(SUCCEEDED(hr));

    // Create swapchain
    IDXGISwapChain1* new_swapchain;
    DXGI_SWAP_CHAIN_DESC1 swapchain_desc {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {.Count = 1},
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = buffer_count,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = 0,
    };
    hr = factory->CreateSwapChainForHwnd(queue, hwnd, &swapchain_desc, nullptr, nullptr, &new_swapchain);
    assert(SUCCEEDED(hr));
    new_swapchain->QueryInterface(&swapchain);
    new_swapchain->Release();
    frame_index = swapchain->GetCurrentBackBufferIndex();


    D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = buffer_count,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    };
    hr = device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&rtv_heap));
    assert(SUCCEEDED(hr));
    uint32_t rtv_incr = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create frame buffers
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor = rtv_heap->GetCPUDescriptorHandleForHeapStart();
    for (uint32_t i = 0; i < buffer_count; i++) {
        ID3D12Resource* rtv_resource;
        swapchain->GetBuffer(i, IID_PPV_ARGS(&rtv_resource));
        device->CreateRenderTargetView(rtv_resource, nullptr, rtv_descriptor);
        rtv[i] = rtv_descriptor;
        rtv_descriptor.ptr += rtv_incr;
        render_targets[i] = rtv_resource;

        hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmd_allocator[i]));
        assert(SUCCEEDED(hr));
    }

    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_allocator[frame_index], nullptr, IID_PPV_ARGS(&cmd_list));
    assert(SUCCEEDED(hr));
    cmd_list->Close();

    // Create vertex buffer
    D3D12_HEAP_PROPERTIES heap_props {
        .Type = D3D12_HEAP_TYPE_UPLOAD,
    };

    D3D12_RESOURCE_DESC resource_desc {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Width = sizeof(tri_vtx),
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {.Count = 1},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
    };

    hr = device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc,
                                         D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                         IID_PPV_ARGS(&vtx_buffer));
    assert(SUCCEEDED(hr));

    // Upload vertex data to vertex buffer
    void* buffer_ptr;
    D3D12_RANGE range {};
    vtx_buffer->Map(0, &range, &buffer_ptr);
    std::memcpy(buffer_ptr, tri_vtx, sizeof(tri_vtx));
    vtx_buffer->Unmap(0, nullptr);

    vtx_buffer_view = {
        .BufferLocation = vtx_buffer->GetGPUVirtualAddress(),
        .SizeInBytes = sizeof(tri_vtx),
        .StrideInBytes = sizeof(Vertex),
    };

    // Create root signature
    ID3D10Blob* rs_blob;
    ID3D10Blob* rs_error_blob;
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc {
        .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
    };
    serialize_root_signature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rs_blob, &rs_error_blob);
    hr = device->CreateRootSignature(0, rs_blob->GetBufferPointer(), rs_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature));
    assert(SUCCEEDED(hr));
    rs_blob->Release();
    if (rs_error_blob)
        rs_error_blob->Release();

    std::vector<std::byte> vs_content = load_file("shaders/shader_vs.hlsl.dxil");
    assert(vs_content.size() != 0);
    assert(SUCCEEDED(hr));

    std::vector<std::byte> ps_content = load_file("shaders/shader_ps.hlsl.dxil");
    assert(ps_content.size() != 0);
    assert(SUCCEEDED(hr));

    D3D12_INPUT_ELEMENT_DESC elem[2] {
        {
            .SemanticName = "POSITION",
            .SemanticIndex = 0,
            .Format = DXGI_FORMAT_R32G32_FLOAT,
            .InputSlot = 0,
            .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
        },
        {
            .SemanticName = "COLOR",
            .SemanticIndex = 0,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .InputSlot = 0,
            .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
        },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc {
        .pRootSignature = root_signature,
        .VS = {.pShaderBytecode = vs_content.data(), .BytecodeLength = vs_content.size() },
        .PS = {.pShaderBytecode = ps_content.data(), .BytecodeLength = ps_content.size() },
        .BlendState = {
            .RenderTarget = {
                {
                    .SrcBlend = D3D12_BLEND_ONE,
                    .DestBlend = D3D12_BLEND_ZERO,
                    .BlendOp = D3D12_BLEND_OP_ADD,
                    .SrcBlendAlpha = D3D12_BLEND_ONE,
                    .DestBlendAlpha = D3D12_BLEND_ZERO,
                    .BlendOpAlpha = D3D12_BLEND_OP_ADD,
                    .LogicOp = D3D12_LOGIC_OP_NOOP,
                    .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
                }
            },
        },
        .SampleMask = UINT_MAX,
        .RasterizerState = {
            .FillMode = D3D12_FILL_MODE_SOLID,
            .CullMode = D3D12_CULL_MODE_NONE,
            .DepthClipEnable = TRUE,
        },
        .DepthStencilState = {
            .DepthEnable = FALSE,
            .StencilEnable = FALSE,
        },
        .InputLayout = {.pInputElementDescs = elem, .NumElements = 2},
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = { swapchain_desc.Format, },
        .SampleDesc = {.Count = 1 },
    };
    hr = device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pso));
    assert(SUCCEEDED(hr));

    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    assert(SUCCEEDED(hr));
    fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    //wait_for_device();
}

void wait_for_device() {
    queue->Signal(fence, fence_value[frame_index]);
    fence->SetEventOnCompletion(fence_value[frame_index], fence_event);
    WaitForSingleObjectEx(fence_event, INFINITE, FALSE);
    fence_value[frame_index]++;
}

void shutdown() {
    wait_for_device();
    CloseHandle(fence_event);
    fence->Release();
    pso->Release();
    root_signature->Release();
    vtx_buffer->Release();
    cmd_list->Release();
    for (uint32_t i = 0; i < buffer_count; i++) {
        cmd_allocator[i]->Release();
        render_targets[i]->Release();
    }
    rtv_heap->Release();
    swapchain->Release();
    queue->Release();
    device->Release();
}

void render() {
    cmd_allocator[frame_index]->Reset();
    cmd_list->Reset(cmd_allocator[frame_index], nullptr);
    cmd_list->SetPipelineState(pso);
    cmd_list->SetGraphicsRootSignature(root_signature);

    D3D12_VIEWPORT viewport {
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = 640.0f,
        .Height = 480.0f,
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    };
    cmd_list->RSSetViewports(1, &viewport);

    RECT scissor {
       .left = 0,
       .top = 0,
       .right = 640,
       .bottom = 480,
    };
    cmd_list->RSSetScissorRects(1, &scissor);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv[frame_index];
    D3D12_RESOURCE_BARRIER barrier {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition = {
            .pResource = render_targets[frame_index],
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
            .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
        }
    };
    cmd_list->ResourceBarrier(1, &barrier);
    cmd_list->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);

    float clear[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cmd_list->ClearRenderTargetView(rtv_handle, clear, 0, nullptr);
    cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd_list->IASetVertexBuffers(0, 1, &vtx_buffer_view);
    cmd_list->DrawInstanced(3, 1, 0, 0);

    barrier = {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition = {
            .pResource = render_targets[frame_index],
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
            .StateAfter = D3D12_RESOURCE_STATE_PRESENT,
        }
    };
    cmd_list->ResourceBarrier(1, &barrier);
    cmd_list->Close();

    ID3D12CommandList* command_list = cmd_list;
    queue->ExecuteCommandLists(1, &command_list);
    swapchain->Present(1, 0);

    uint64_t current_fence_value = fence_value[frame_index];
    queue->Signal(fence, current_fence_value);

    frame_index = swapchain->GetCurrentBackBufferIndex();
    uint64_t completed_value = fence->GetCompletedValue();
    if (completed_value < fence_value[frame_index]) {
        fence->SetEventOnCompletion(fence_value[frame_index], fence_event);
        WaitForSingleObjectEx(fence_event, INFINITE, FALSE);
    }

    fence_value[frame_index] = current_fence_value + 1;
}
