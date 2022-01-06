#include "Application.h"
#include "MaterialManager.h"
#include "TextureManager.h"
#include "PipelineManager.h"

#include "imgui/imgui.h"

Application::Application() : 
    mSceneLight((unsigned int)Direct3D::kBufferCount)
{
}

bool Application::OnInit(ID3D12GraphicsCommandList *initializationCmdList, ID3D12CommandAllocator *cmdAllocator)
{
    mSceneLight.SetAmbientColor(0.02f, 0.02f, 0.02f, 1.0f);

    return true;
}

bool Application::OnUpdate(FrameResources *frameResources, float dt)
{
    ReactToKeyPresses(dt);
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