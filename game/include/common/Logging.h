#pragma once
#include "Config.h"

namespace hs
{

struct Mat44;

//------------------------------------------------------------------------------
enum class LogLevel
{
    Info,
    Warning,
    Error
};

//------------------------------------------------------------------------------
void Log(LogLevel level, const char* formatString, ...);

//------------------------------------------------------------------------------
void Mat44ToString(const Mat44& m, char* buff);

//------------------------------------------------------------------------------
void LogMat44(const Mat44& m);

}

#if VKR_DEBUG
    #define DBG_LOG(msg, ...) hs::Log(hs::LogLevel::Info, msg, __VA_ARGS__)
#else
    #define DBG_LOG(msg, ...)
#endif

