#include "Application.h"
#include "MaterialManager.h"
#include "TextureManager.h"
#include "PipelineManager.h"
#include "Utils/RayTracingStructures.h"
#include "Utils/Utils.h"

#include "imgui/imgui.h"

Application::Application() : 
    mSceneLight((unsigned int)Direct3D::kBufferCount)
{
}

Application::~Application()
{
}

bool Application::OnInit(ID3D12GraphicsCommandList *initializationCmdList, ID3D12CommandAllocator *cmdAllocator)
{
    mSceneLight.SetAmbientColor(0.02f, 0.02f, 0.02f, 1.0f);
    CHECK(InitModels(initializationCmdList, cmdAllocator), false, "Cannot init all models");
    CHECK(InitRaytracing(), false, "Cannot initialize raytracing");
    return true;
}

bool Application::OnUpdate(FrameResources *frameResources, float dt)
{
    ReactToKeyPresses(dt);
    mSceneLight.UpdateLightsBuffer(frameResources->LightsBuffer);
    return true;
}

bool Application::OnRender(ID3D12GraphicsCommandList *cmdList_, FrameResources *frameResources)
{
    ID3D12GraphicsCommandList4* cmdList;
    CHECK_HR(cmdList_->QueryInterface(IID_PPV_ARGS(&cmdList)), false);

    auto d3d = Direct3D::Get();
    auto pipelineManager = PipelineManager::Get();
    FLOAT backgroundColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };

    cmdList->RSSetViewports(1, &mViewport);
    cmdList->RSSetScissorRects(1, &mScissors);

    auto backbufferHandle = d3d->GetBackbufferHandle();
    auto dsvHandle = d3d->GetDSVHandle();

    cmdList->ClearRenderTargetView(backbufferHandle, backgroundColor, 0, nullptr);
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    cmdList->OMSetRenderTargets(1, &backbufferHandle, TRUE, &dsvHandle);

    D3D12_DISPATCH_RAYS_DESC raytrace = {};
    raytrace.Width = mClientWidth;
    raytrace.Height = mClientHeight;
    raytrace.Depth = 1;
    
    raytrace.RayGenerationShaderRecord.StartAddress = mShaderTable.GetGPUVirtualAddress();
    raytrace.RayGenerationShaderRecord.SizeInBytes = mShaderTableEntrySize;

    size_t missOffset = mShaderTableEntrySize;
    raytrace.MissShaderTable.StartAddress = mShaderTable.GetGPUVirtualAddress() + missOffset;
    raytrace.MissShaderTable.SizeInBytes = mShaderTableEntrySize;
    raytrace.MissShaderTable.StrideInBytes = mShaderTableEntrySize;

    size_t hitOffset = 2 * mShaderTableEntrySize;
    raytrace.HitGroupTable.StartAddress = mShaderTable.GetGPUVirtualAddress() + hitOffset;
    raytrace.HitGroupTable.SizeInBytes = mShaderTableEntrySize;
    raytrace.HitGroupTable.StrideInBytes = mShaderTableEntrySize;

    auto emptyRootSignature = pipelineManager->GetRootSignature(RootSignatureType::Empty);
    cmdList->SetComputeRootSignature(emptyRootSignature.Get());
    cmdList->SetPipelineState1(mRtStateObject.Get());
    
    ID3D12DescriptorHeap* heaps[] = { mDescriptorHeap.Get() };
    cmdList->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);

    cmdList->DispatchRays(&raytrace);


    auto currentBackbufferResource = d3d->GetCurrentBackbufferResource();
    
    d3d->Transition(cmdList, currentBackbufferResource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
    d3d->Transition(cmdList, mRaytracingResultResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    
    cmdList->CopyResource(currentBackbufferResource.Get(), mRaytracingResultResource.Get());
    
    d3d->Transition(cmdList, mRaytracingResultResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    d3d->Transition(cmdList, currentBackbufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);

    cmdList->Release();

    return true;
}

bool Application::OnRenderGUI()
{
    return true;
}

bool Application::OnResize()
{
    mViewport.Width = (FLOAT)mClientWidth;
    mViewport.Height = (FLOAT)mClientHeight;
    mViewport.TopLeftX = 0;
    mViewport.TopLeftY = 0;
    mViewport.MinDepth = 0.0f;
    mViewport.MaxDepth = 1.0f;

    mScissors.left = 0;
    mScissors.top = 0;
    mScissors.right = mClientWidth;
    mScissors.bottom = mClientWidth;


    DirectX::XMFLOAT3 oldCameraPosition;
    DirectX::XMStoreFloat3(&oldCameraPosition, mCamera.GetPosition());
    mCamera.Create(oldCameraPosition, (float)mClientWidth / mClientHeight);

    return true;
}

void Application::OnClose()
{
}

std::unordered_map<uuids::uuid, uint32_t> Application::GetInstanceCount()
{
    return std::unordered_map<uuids::uuid, uint32_t>();
}

uint32_t Application::GetPassCount()
{
    return 1;
}

uint32_t Application::GetModelCount()
{
    return (uint32_t)mModels.size();
}

ID3D12PipelineState *Application::GetBeginFramePipeline()
{
    return nullptr;
}

void Application::ReactToKeyPresses(float dt)
{
    auto kb = mKeyboard->GetState();
    auto mouse = mMouse->GetState();

    if (kb.Escape)
    {
        PostQuitMessage(0);
    }

    if (!mMenuActive)
    {
        if (kb.W)
        {
            mCamera.MoveForward(dt);
        }
        if (kb.S)
        {
            mCamera.MoveBackward(dt);
        }
        if (kb.D)
        {
            mCamera.MoveRight(dt);
        }
        if (kb.A)
        {
            mCamera.MoveLeft(dt);
        }

        mouse.x = Math::clamp(mouse.x, -25, 25);
        mouse.y = Math::clamp(mouse.y, -25, 25);
        mCamera.Update(dt, (float)mouse.x, (float)mouse.y);
    }
    else
    {
        mCamera.Update(dt, 0.0f, 0.0f);
    }

    static bool bRightClick = false;
    if (mouse.rightButton && !bRightClick)
    {
        bRightClick = true;
        if (mMenuActive)
        {
            mMouse->SetMode(DirectX::Mouse::Mode::MODE_RELATIVE);
            while (ShowCursor(FALSE) > 0);
        }
        else
        {
            mMouse->SetMode(DirectX::Mouse::Mode::MODE_ABSOLUTE);
            while (ShowCursor(TRUE) <= 0);
        }
        mMenuActive = !mMenuActive;
    }
    else if (!mouse.rightButton)
        bRightClick = false;
}


bool Application::InitModels(ID3D12GraphicsCommandList* initializationCmdList, ID3D12CommandAllocator* cmdAllocator)
{
    auto d3d = Direct3D::Get();
    auto materialManager = MaterialManager::Get();

    ID3D12GraphicsCommandList4* cmdList = nullptr;
    CHECK_HR(initializationCmdList->QueryInterface(__uuidof(ID3D12GraphicsCommandList4), (void**)&cmdList), false);

    CHECK_HR(cmdAllocator->Reset(), false);
    CHECK_HR(cmdList->Reset(cmdAllocator, nullptr), false);

    mModels.emplace_back(Direct3D::kBufferCount, 0);
    CHECK(mModels.back().Create("Resources\\Suzanne.obj"), false, "Unable to load Suzanne");
    // CHECK(mModels.back().Create(Model::ModelType::Triangle), false, "Unable to load triangle");
    mModels.back().Scale(0.5f, 0.5f, 0.5f);
    mModels.back().Translate(2.0f, 0.0f, 0.0f);
    
    uint32_t firstIntance = mModels.back().AddInstance(InstanceInfo()).Get();
    uint32_t secondInstance = mModels.back().AddInstance(InstanceInfo()).Get();
    
    mModels.back().Scale(0.5f, 0.5f, 0.5f, firstIntance);
    mModels.back().Translate(0.0f, 0.0f, 0.0f, firstIntance);

    mModels.back().Scale(0.5f, 0.5f, 0.5f, secondInstance);
    mModels.back().Translate(-2.0f, 0.0f, 0.0f, secondInstance);

    ComPtr<ID3D12Resource> intermediaryResources1[2];
    CHECK(Model::InitBuffers(cmdList, intermediaryResources1), false, "Unable to initialize buffers for models");
    
    mModels.back().BuildBottomLevelAccelerationStructure(cmdList);
    Model::BuildTopLevelAccelerationStructure(cmdList);
    
    mCamera.Create({ 0.0f, 0.0f, -3.0f }, (float)mClientWidth / mClientHeight);

    CHECK_HR(cmdList->Close(), false);
    d3d->Flush(cmdList, mFence.Get(), ++mCurrentFrame);

    cmdList->Release();

    return true;
}

bool Application::InitRaytracing()
{
    CHECK(InitRaytracingPipelineObject(), false, "Unable to initialze raytracing pipeline object");
    CHECK(InitRaytracingResources(), false, "Unable to initialize raytracing resources");
    CHECK(InitShaderTable(), false, "Unable to initialize raytracing shader table");

    return true;
}

bool Application::InitShaderTable()
{
    mShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    mShaderTableEntrySize += 8;
    mShaderTableEntrySize = Math::AlignUp(mShaderTableEntrySize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
    uint32_t shaderTableSize = mShaderTableEntrySize * 3;

    mShaderTable.Init(shaderTableSize);
    mShaderTable.GetResource()->SetName(L"Shader table");

    ComPtr<ID3D12StateObjectProperties> props;
    mRtStateObject.As(&props);

    uint8_t* mappedMemory = mShaderTable.GetMappedMemory();

    // Entry 0 - ray gen shader
    memcpy(mappedMemory, props->GetShaderIdentifier(kRaygenShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *(uint64_t*)(mappedMemory + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = mDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
    mappedMemory += mShaderTableEntrySize;

    // Entry 1 - miss shader
    memcpy(mappedMemory, props->GetShaderIdentifier(kMissShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    mappedMemory += mShaderTableEntrySize;

    // Entry 2 - hit program
    memcpy(mappedMemory, props->GetShaderIdentifier(kHitGroupName), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    return true;
}

bool Application::InitRaytracingPipelineObject()
{
    auto d3d = Direct3D::Get();
    auto d3dDevice = d3d->GetD3D12Device();
    ComPtr<ID3D12Device5> d3dDevice5;
    CHECK_HR(d3dDevice.As(&d3dDevice5), false);

    auto shadersBlob = Utils::CompileLibrary(L"Shaders\\Basic.rt.hlsl", L"lib_6_3");
    CHECK(shadersBlob, false, "Unable to compile Basic.rt.hlsl");

    std::array<D3D12_STATE_SUBOBJECT, 12> subobjects = {};
    uint32_t index = 0;

    const wchar_t* entrypoints[] = { kRaygenShader, kMissShader, kClosestHit };
    DxilLibrary library(shadersBlob, entrypoints, ARRAYSIZE(entrypoints));
    subobjects[index++] = library; // 0

    HitGroup hitGroup(nullptr, kClosestHit, kHitGroupName);
    subobjects[index++] = hitGroup; // 1

    CD3DX12_DESCRIPTOR_RANGE rayGenRanges[2];
    rayGenRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, 0);
    rayGenRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 1);

    CD3DX12_ROOT_PARAMETER rayGenParameters[1];
    rayGenParameters[0].InitAsDescriptorTable(ARRAYSIZE(rayGenRanges), rayGenRanges);

    D3D12_ROOT_SIGNATURE_DESC rayGenLocalRootSignatureDesc = {};
    rayGenLocalRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    rayGenLocalRootSignatureDesc.NumParameters = ARRAYSIZE(rayGenParameters);
    rayGenLocalRootSignatureDesc.pParameters = rayGenParameters;

    LocalRootSignature rayGenLocalRootSignature(rayGenLocalRootSignatureDesc);
    subobjects[index] = rayGenLocalRootSignature;

    uint32_t rayGenRootSignatureIndex = index++; // 2

    const wchar_t* rayGenShader[] =
    {
        kRaygenShader
    };
    ExportAssociation rayGenExportAssociation(ARRAYSIZE(rayGenShader), rayGenShader, subobjects[rayGenRootSignatureIndex]);
    subobjects[index++] = rayGenExportAssociation; // 3

    D3D12_ROOT_SIGNATURE_DESC emptyLocalRootSignatureDesc = {};
    emptyLocalRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    emptyLocalRootSignatureDesc.NumParameters = 0;
    emptyLocalRootSignatureDesc.pParameters = nullptr;
    LocalRootSignature missClosestHitRootSignature(emptyLocalRootSignatureDesc);
    subobjects[index] = missClosestHitRootSignature; // 4
    uint32_t missClosestHitRootSignatureIndex = index++;

    const wchar_t* emptyRootSignatureAssociated[] =
    {
        kClosestHit, kMissShader
    };
    ExportAssociation missClosestHitExportAssociation(ARRAYSIZE(emptyRootSignatureAssociated), emptyRootSignatureAssociated, subobjects[missClosestHitRootSignatureIndex]);
    subobjects[index++] = missClosestHitExportAssociation; // 5

    ShaderConfig shaderConfig(2 * sizeof(float), 3 * sizeof(float));
    subobjects[index] = shaderConfig; // 6
    uint32_t shaderConfigIndex = index++;

    const wchar_t* shaderConfigExports[] =
    {
        kClosestHit, kMissShader, kRaygenShader
    };
    ExportAssociation shaderConfigExportAssociation(ARRAYSIZE(shaderConfigExports), shaderConfigExports, subobjects[shaderConfigIndex]);
    subobjects[index++] = shaderConfigExportAssociation; // 7

    PipelineConfig pipelineConfig(1);
    subobjects[index++] = pipelineConfig; // 8

    D3D12_ROOT_SIGNATURE_DESC globalRootSignatureDesc = {};
    GlobalRootSignature globalRootSignature(globalRootSignatureDesc);
    subobjects[index++] = globalRootSignature; // 9

    D3D12_STATE_OBJECT_DESC objectDesc = {};
    objectDesc.NumSubobjects = index;
    objectDesc.pSubobjects = subobjects.data();
    objectDesc.Type = D3D12_STATE_OBJECT_TYPE::D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    CHECK_HR(d3dDevice5->CreateStateObject(&objectDesc, IID_PPV_ARGS(&mRtStateObject)), false);

    return true;
}

bool Application::InitRaytracingResources()
{
    auto d3d = Direct3D::Get();
    auto device = d3d->GetD3D12Device();

    CD3DX12_HEAP_PROPERTIES defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R8G8B8A8_UNORM, mClientWidth, mClientHeight);
    resourceDesc.MipLevels = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    CHECK_HR(device->CreateCommittedResource(
        &defaultHeap, D3D12_HEAP_FLAG_NONE,
        &resourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(&mRaytracingResultResource)
    ), false);
    mRaytracingResultResource->SetName(L"Raytracing texture");

    auto heapResult = d3d->CreateDescriptorHeap(2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    CHECK(heapResult.Valid(), false, "Unable to initialize raytracing descriptor heap");
    mDescriptorHeap = heapResult.Get();
    auto incrementSize = d3d->GetDescriptorIncrementSize<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>();
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Texture2D.MipSlice = 0;
    uavDesc.Texture2D.PlaneSlice = 0;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    d3d->CreateUnorderedAccessView(mRaytracingResultResource.Get(), uavDesc, cpuHandle);
    cpuHandle.Offset(1, incrementSize);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    auto tlasBuffer = Model::GetTLASBuffer();
    srvDesc.RaytracingAccelerationStructure.Location = tlasBuffer->GetGPUVirtualAddress();
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    d3d->CreateShaderResourceView(nullptr, srvDesc, cpuHandle);

    return true;
}
