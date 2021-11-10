#include "Application.h"
#include "MaterialManager.h"
#include "TextureManager.h"
#include "PipelineManager.h"

Application::Application() : 
    mSceneLight((unsigned int)Direct3D::kBufferCount)
{
}

bool Application::OnInit(ID3D12GraphicsCommandList *initializationCmdList, ID3D12CommandAllocator *cmdAllocator)
{
    mSceneLight.SetAmbientColor(0.2f, 0.2f, 0.2f, 1.0f);
    mSceneLight.AddDirectionalLight("Sun", { -1.0f,-1.0f, 1.0f }, { 0.6f, 0.6f, 0.6f });
    
    CHECK(InitModels(initializationCmdList, cmdAllocator), false, "Unable to initialize all models");

    return true;
}

bool Application::OnUpdate(FrameResources *frameResources, float dt)
{
    ReactToKeyPresses(dt);
    UpdateCamera(frameResources);
    UpdateModels(frameResources);
    mSceneLight.UpdateLightsBuffer(frameResources->LightsBuffer);
    return true;
}

bool Application::OnRender(ID3D12GraphicsCommandList *cmdList, FrameResources *frameResources)
{
    auto d3d = Direct3D::Get();
    auto pipelineManager = PipelineManager::Get();
    FLOAT backgroundColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    auto pipelineSignatureResult = pipelineManager->GetPipelineAndRootSignature(PipelineType::MaterialLight);
    CHECK(pipelineSignatureResult.Valid(), false, "Unable to retrieve pipeline and root signature");
    auto [pipeline, rootSignature] = pipelineSignatureResult.Get();

    cmdList->RSSetViewports(1, &mViewport);
    cmdList->RSSetScissorRects(1, &mScissors);

    cmdList->SetPipelineState(pipeline);
    cmdList->SetGraphicsRootSignature(rootSignature);

    auto backbufferHandle = d3d->GetBackbufferHandle();
    auto dsvHandle = d3d->GetDSVHandle();

    cmdList->ClearRenderTargetView(backbufferHandle, backgroundColor, 0, nullptr);
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    cmdList->OMSetRenderTargets(1, &backbufferHandle, TRUE, &dsvHandle);

    Model::Bind(cmdList);

    mGrid->SetShouldRender(false);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    RenderModels(cmdList, frameResources,
                 [](Model * m) -> bool
                 {
                     auto oldShouldRender = m->ShouldRender();
                     m->SetShouldRender(false);
                     return oldShouldRender;
                 });

    
    auto pipelineResult = pipelineManager->GetPipeline(PipelineType::Terrain);
    CHECK(pipelineResult.Valid(), false, "Unable to get terrain pipeline");
    pipeline = pipelineResult.Get();
    cmdList->SetPipelineState(pipeline);
    mGrid->SetShouldRender(true);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
    RenderModels(cmdList, frameResources,
                 [](Model *m) -> bool
                 {
                     auto oldShouldRender = m->ShouldRender();
                     m->SetShouldRender(true);
                     return oldShouldRender;
                 });

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


    mCamera.Create(mCamera.GetPosition(), (float)mClientWidth / mClientHeight);

    return true;
}

std::unordered_map<void *, uint32_t> Application::GetInstanceCount()
{
    return std::unordered_map<void *, uint32_t>();
}

uint32_t Application::GetModelCount()
{
    return (uint32_t)mModels.size();
}

ID3D12PipelineState *Application::GetBeginFramePipeline()
{
    return nullptr;
}

bool Application::InitModels(ID3D12GraphicsCommandList *initializationCmdList, ID3D12CommandAllocator *cmdAllocator)
{
    auto d3d = Direct3D::Get();
    auto materialManager = MaterialManager::Get();
    CHECK_HR(cmdAllocator->Reset(), false);
    CHECK_HR(initializationCmdList->Reset(cmdAllocator, nullptr), false);

    mModels.emplace_back(Direct3D::kBufferCount, 0);
    CHECK(mModels.back().Create("Resources\\Suzanne.obj"), false, "Unable to load Suzanne");
    mModels.back().Translate(2.0f, 0.0f, 0.0f);

    mModels.emplace_back(Direct3D::kBufferCount, 1);
    CHECK(mModels.back().Create("Resources\\Cube.obj"), false, "Unable to load Cube");
    mModels.back().Translate(-2.0f, 0.0f, 0.0f);

    mModels.emplace_back(Direct3D::kBufferCount, 2);
    Model::GridInitializationInfo gridInfo;
    gridInfo.width = 10.f;
    gridInfo.depth = 10.f;
    gridInfo.N = 5;
    gridInfo.M = 5;
    CHECK(mModels.back().CreatePrimitive(gridInfo), false, "Unable to create grid");
    mModels.back().SetMaterial(materialManager->AddDefaultMaterial(Direct3D::kBufferCount));
    mModels.back().SetMaterial(mModels[0].GetMaterial());
    mModels.back().Translate(0.0f, -1.0f, 0.0f);
    mModels.back().Scale(5.f);
    mGrid = &mModels.back();

    ComPtr<ID3D12Resource> intermediaryResources[2];
    CHECK(Model::InitBuffers(initializationCmdList, intermediaryResources), false, "Unable to initialize buffers for models");

    mCamera.Create({ 0.0f, 0.0f, -3.0f }, (float)mClientWidth / mClientHeight);

    CHECK_HR(initializationCmdList->Close(), false);
    d3d->Flush(initializationCmdList, mFence.Get(), ++mCurrentFrame);

    return true;
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

void Application::UpdateCamera(FrameResources *frameResources)
{
    if (mCamera.DirtyFrames > 0)
    {
        auto mappedMemory = frameResources->PerPassBuffers.GetMappedMemory();
        mappedMemory->View = DirectX::XMMatrixTranspose(mCamera.GetView());
        mappedMemory->Projection = DirectX::XMMatrixTranspose(mCamera.GetProjection());

        mappedMemory->CameraPosition = mCamera.GetPosition();

        mCamera.DirtyFrames--;
    }
}

void Application::UpdateModels(FrameResources *frameResources)
{
    for (auto &model : mModels)
    {
        if (model.DirtyFrames > 0)
        {
            auto mappedMemory = frameResources->PerObjectBuffers.GetMappedMemory(model.ConstantBufferIndex);
            mappedMemory->World = DirectX::XMMatrixTranspose(model.GetWorld());
            mappedMemory->TexWorld = DirectX::XMMatrixTranspose(model.GetTexWorld());
            model.DirtyFrames--;
        }
    }
}

void Application::RenderModels(ID3D12GraphicsCommandList *cmdList, FrameResources *frameResources, std::function<bool(Model *)> callback)
{
    auto textureManager = TextureManager::Get();

    cmdList->SetGraphicsRootConstantBufferView(1, frameResources->PerPassBuffers.GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(3, frameResources->LightsBuffer.GetGPUVirtualAddress());

    cmdList->SetDescriptorHeaps(1, textureManager->GetSrvUavDescriptorHeap().GetAddressOf());

    for (unsigned int i = 0; i < mModels.size(); ++i)
    {
        CHECKCONT(callback(&mModels[i]), "");
        auto perObjectBufferAddress = frameResources->PerObjectBuffers.GetGPUVirtualAddress();
        perObjectBufferAddress += mModels[i].ConstantBufferIndex * frameResources->PerObjectBuffers.GetElementSize();
        cmdList->SetGraphicsRootConstantBufferView(0, perObjectBufferAddress);

        auto materialBufferAddress = frameResources->MaterialsBuffers.GetGPUVirtualAddress();
        const auto *objectMaterial = mModels[i].GetMaterial();
        materialBufferAddress += (uint64_t)objectMaterial->ConstantBufferIndex * frameResources->MaterialsBuffers.GetElementSize();
        cmdList->SetGraphicsRootConstantBufferView(2, materialBufferAddress);

        if (objectMaterial->GetTextureIndex() != -1)
        {
            auto textureSRVResult = textureManager->GetGPUDescriptorSrvHandleForTextureIndex(objectMaterial->GetTextureIndex());
            cmdList->SetGraphicsRootDescriptorTable(
                4, textureSRVResult.Get());
        }
        cmdList->DrawIndexedInstanced(mModels[i].GetIndexCount(),
                                           1, // Number of instances
                                           mModels[i].GetStartIndexLocation(),
                                           mModels[i].GetBaseVertexLocation(),
                                           0); // Start InstanceLocation
    }
}
