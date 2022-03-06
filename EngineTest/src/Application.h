#pragma once


#include "Engine.h"



class Application : public Engine
{
    static constexpr const wchar_t* kRaygenShader = L"rayGen";
    static constexpr const wchar_t* kMissShader = L"miss";
    static constexpr const wchar_t* kClosestHit = L"chs";
    static constexpr const wchar_t* kHitGroupName = L"HitGroup";
public:
    Application();
    ~Application();

public:
    // Inherited via Engine
    virtual bool OnInit(ID3D12GraphicsCommandList *initializationCmdList, ID3D12CommandAllocator *cmdAllocator) override;
    virtual bool OnUpdate(FrameResources *frameResources, float dt) override;
    virtual bool OnRender(ID3D12GraphicsCommandList *cmdList, FrameResources *frameResources) override;
    virtual bool OnRenderGUI() override;
    virtual bool OnResize() override;
    virtual void OnClose() override;
    virtual std::unordered_map<uuids::uuid, uint32_t> GetInstanceCount() override;
    virtual uint32_t GetPassCount() override;

    virtual uint32_t GetModelCount() override;
    virtual ID3D12PipelineState *GetBeginFramePipeline() override;

private:
    void ReactToKeyPresses(float dt);

    bool InitModels(ID3D12GraphicsCommandList* initializationCmdList, ID3D12CommandAllocator* cmdAllocator);

private:
    bool InitRaytracing();
    bool InitRaytracingPipelineObject();
    bool InitShaderTable();

private:
    std::vector<Model> mModels;

    SceneLight mSceneLight;

    D3D12_VIEWPORT mViewport;
    D3D12_RECT mScissors;

    Camera mCamera;

    bool mMenuActive = true;

    ComPtr<ID3D12StateObject> mRtStateObject;
    UploadBuffer<unsigned char> mShaderTable;
    uint32_t mShaderTableEntrySize;
};
