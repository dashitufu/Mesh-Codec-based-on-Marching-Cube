#pragma once

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>

extern "C" _declspec(dllexport) void Decoder_Test();
extern "C" _declspec(dllexport) int iDecode_Test_1();
extern "C" _declspec(dllexport) void* pInit_Decoder_Env();
extern "C" _declspec(dllexport) void Free_Decoder_Env(void *poEnv);
extern "C" _declspec(dllexport) void Get_Data(void* poEnv, unsigned char Buffer[], int* piPoint_Count, unsigned long long* ptTime_Stamp);
extern "C" _declspec(dllexport) void Get_Data_2(void* poEnv, unsigned char Buffer[], int* piPoint_Count, unsigned long long* ptTime_Stamp,int *pCentroid);
