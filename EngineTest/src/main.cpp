#include <Logger.h>
#include "Application.h"

#include <dxgidebug.h>

void DXGIMemoryCheck()
{
    ComPtr<IDXGIDebug> debugInterface;
    CHECKRET_HR(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debugInterface)));
    debugInterface->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
}


int main(int argc, char *argv[])
{
    try
    {
        Logger::Init();
        Application app;
        CHECK(app.Init(GetModuleHandle(NULL)), 0, "Cannot initialize application");
        app.Run();
    }
    catch (...)
    {
    }
    DXGIMemoryCheck();
    Logger::Close();

    return 0;
}
