#include "Config.h"
#include "EngineMain.h"

constexpr const char* GAME_NAME = "Pixel Trader";

#if HS_WINDOWS
    //------------------------------------------------------------------------------
    int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
    {
        int result = hs::EngineMainWin32(instance, prevInstance, cmdLine, GAME_NAME);
        return result;
    }
#elif HS_LINUX
    //------------------------------------------------------------------------------
    int main(int argc, char** argv)
    {
        int result = hs::EngineMainLinux(argc, argv, GAME_NAME);
        return result;
    }
#endif

