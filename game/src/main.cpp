#include "EngineMain.h"

//------------------------------------------------------------------------------
int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
    int result = hs::EngineMain(instance, prevInstance, cmdLine, showCmd);
    return result;
}