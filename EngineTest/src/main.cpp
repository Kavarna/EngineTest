#include <Logger.h>
#include "Application.h"

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
    Logger::Close();
    return 0;
}
