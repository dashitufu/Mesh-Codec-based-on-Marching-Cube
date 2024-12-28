#pragma once

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>

extern "C" _declspec(dllexport) void Test(void *p);
extern "C" _declspec(dllexport) void* pInit_Env();
extern "C" _declspec(dllexport) void Send_Data(void* pEnv, unsigned char* pBuffer, int iPoint_Count, unsigned long long tTime_Stamp);
