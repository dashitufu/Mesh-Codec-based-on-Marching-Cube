// Call_Decoder_Dll_Test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "MCPC_Decoder_Dll.h"
#pragma comment(lib, "MCPC_Decoder_Dll.lib")
int main()
{
	unsigned char* pBuffer = (unsigned char*)malloc(10000000);
	int iPoint_Count;
	unsigned long long tTime_Stamp;

	while (1)
	{
		void* p = pInit_Decoder_Env();
		int Centroid[3], i = 0;
		if (!p)
			return 0;
		while (1)
		{
			//Get_Data(p, pBuffer, &iPoint_Count, &tTime_Stamp);
			//Get_Data_2(p, pBuffer, &iPoint_Count, &tTime_Stamp, Centroid);
			//printf("Point Count:%d Centroid:(%d %d %d)\n", iPoint_Count,Centroid[0],Centroid[1],Centroid[2]);
			//bSave("c:\\tmp\\1.ply", pBuffer, iPoint_Count);
			if (i++ >= 1)
				break;
		}
		Free_Decoder_Env(p);
	}
}