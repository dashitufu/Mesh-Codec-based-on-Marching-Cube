// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{//这个作用未明，别手贱
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
void Test(void *p)
{
    Ming_Yuan_Test();
}
void Send_Data(void* pEnv, unsigned char* pBuffer, int iPoint_Count, unsigned long long tTime_Stamp)
{
    Send_Data_1(pEnv, pBuffer, iPoint_Count, tTime_Stamp);
}
void *pInit_Env()
{//初始化整个编码环境
//return:   成功返回一个指针，往后按照这个指针来调用
//          若失败返回NULL
    return pInit_Env_1();
}
