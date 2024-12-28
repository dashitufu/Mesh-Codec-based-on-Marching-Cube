// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
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
void Decoder_Test()
{
    printf("Decoder");
}
int iDecode_Test_1()
{
    return 100;
}
void Get_Data_2(void* poEnv, unsigned char Buffer[], int* piPoint_Count, unsigned long long* ptTime_Stamp,int *pCentroid)
{
    Get_Data_1(poEnv, Buffer, piPoint_Count, ptTime_Stamp, pCentroid);
}
void Get_Data(void* poEnv, unsigned char Buffer[], int* piPoint_Count, unsigned long long* ptTime_Stamp)
{
    Get_Data_1(poEnv, Buffer, piPoint_Count, ptTime_Stamp);
}
void* pInit_Decoder_Env()
{//初始化整个解码环境
//return:   成功返回一个指针，往后按照这个指针来调用
//          若失败返回NULL
    return pInit_Env_1();
}
void Free_Decoder_Env(void *p)
{//释放整个解码环境
    Free_Env_1(p);
}