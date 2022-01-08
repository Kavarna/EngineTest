#include "FrameResources.h"

#include "Direct3D.h"
#include "TextureManager.h"

bool FrameResources::Init(uint32_t numObjects, uint32_t numPasses, uint32_t numMaterials, uint32_t width, uint32_t height,
                          const std::unordered_map<uuids::uuid, uint32_t>& instancesCountPerObject)
{
    auto d3d = Direct3D::Get();

    auto allocatorResult = d3d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
    CHECK(allocatorResult.Valid(), false, "Unable to create a command allocator for frame resources");
    CommandAllocator = allocatorResult.Get();
    CommandAllocator->SetName(L"Frame resource allocator");

    //if (numObjects > 0)
    //{
    //    CHECK(PerObjectBuffers.Init(numObjects, true), false,
    //          "Unable to initialize per object buffer with {} elements", numObjects);
    //}
    CHECK(HUDBuffers.Init(1, true), false, "Unable to initialize HUD buffers");
    if (numPasses > 0)
    {
        CHECK(PerPassBuffers.Init(numPasses, true), false,
              "Unable to initialize per pass buffer with {} elements", numPasses);
    }

    if (numMaterials > 0)
    {
        CHECK(MaterialsBuffers.Init(numMaterials, true), false,
              "Unable to initialize material buffer with {} elements", numMaterials);
    }
    CHECK(LightsBuffer.Init(1, true), false,
          "Unable to initialize lights buffer with {} elements", 1);

    CHECK(VertexBatchRenderer.Create(), false, "Failed to create vertex batch renderer");
    CHECK(HUDBatchRenderer.Create(), false, "Failed to create HUD batch renderer");

    CD3DX12_RESOURCE_DESC backbufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        Direct3D::kBackbufferFormat, width / kBlurScale, height / kBlurScale, 1, 0, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    CD3DX12_HEAP_PROPERTIES defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = Direct3D::kBackbufferFormat;
    clearValue.Color[0] = 0.0f;
    clearValue.Color[1] = 0.0f;
    clearValue.Color[2] = 0.0f;
    clearValue.Color[3] = 1.0f;

    auto indexResult = TextureManager::Get()->AddTexture(backbufferDesc, defaultHeap, D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                         D3D12_HEAP_FLAG_NONE, &clearValue);
    CHECK(indexResult.Valid(), false, "Cannot created render target views");
    BlurRenderTargetIndex = indexResult.Get();

    CD3DX12_RESOURCE_DESC depthDesc = backbufferDesc;
    depthDesc.Format = Direct3D::kDepthStencilFormat;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    clearValue.Format = Direct3D::kDepthStencilFormat;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;
    indexResult = TextureManager::Get()->AddTexture(depthDesc, defaultHeap, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                    D3D12_HEAP_FLAG_NONE, &clearValue);
    CHECK(indexResult.Valid(), false, "Cannot create depth stencil view");
    BlurDepthStencilIndex = indexResult.Get();

    for (const auto &it : instancesCountPerObject)
    {
        CHECK(InstanceBuffer[std::get<0>(it)].Init(it.second), false,
            "Unable to create instance buffer ({} instances) for object {}", std::get<1>(it), std::get<0>(it));
    }

    SHOWINFO("Successfully created a frame resource");
    return true;
}

bool FrameResources::OnResize(uint32_t width, uint32_t height)
{
    CD3DX12_RESOURCE_DESC backbufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        Direct3D::kBackbufferFormat, width / kBlurScale, height / kBlurScale, 1, 0, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    CD3DX12_HEAP_PROPERTIES defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = Direct3D::kBackbufferFormat;
    clearValue.Color[0] = 0.0f;
    clearValue.Color[1] = 0.0f;
    clearValue.Color[2] = 0.0f;
    clearValue.Color[3] = 1.0f;

    CHECK(TextureManager::Get()->UpdateTexture(BlurRenderTargetIndex, backbufferDesc, defaultHeap,
                                               D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_HEAP_FLAG_NONE, &clearValue),
          false, "Unable to update render target resource");

    CD3DX12_RESOURCE_DESC depthDesc = backbufferDesc;
    depthDesc.Format = Direct3D::kDepthStencilFormat;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    clearValue.Format = Direct3D::kDepthStencilFormat;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;
    CHECK(TextureManager::Get()->UpdateTexture(BlurDepthStencilIndex, depthDesc, defaultHeap, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                               D3D12_HEAP_FLAG_NONE, &clearValue),
          false, "Unable to update depth stencil resource");

    return true;
}
