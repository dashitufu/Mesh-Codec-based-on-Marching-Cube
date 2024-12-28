#include "MCPC_Decoder.h"
static void Test_1()
{
	char File[256];
	unsigned char* pBuffer = NULL;
	int iSize, iMC_Count,iPoint_Count;
	MC* pMC;
	BitPtr oBitPtr;

	Init_Codec_Env();
	PCC_Decoder* poDecoder = (PCC_Decoder*)malloc(sizeof(PCC_Decoder));
	Init_Decoder(poDecoder, 100000, Pre_Process_Type::Full_Search);
	poDecoder->m_bDecode_Geometry_Only = 1;

	sprintf(File, "c:\\tmp\\temp\\0000.bin");
	Load_File(File, poDecoder, &pBuffer, &iSize);
	unsigned int tStart = GetTickCount();
	bDecode(poDecoder, pBuffer, iSize, 0);

	Decode_MC(poDecoder, pBuffer, iSize, poDecoder->m_oGeometry_Brick.m_pPoint, poDecoder->m_oGeometry_Brick.m_oHeader.geom_num_points, &pMC, &iMC_Count,&iPoint_Count,&oBitPtr);
	Free(&poDecoder->m_oMem_Mgr, poDecoder->m_oGeometry_Brick.m_pPoint);

	Decode_Color(poDecoder, &oBitPtr, pMC, iMC_Count, iPoint_Count, 8,0);

	printf("%d ms\n", GetTickCount() - tStart);
	bSave("c:\\tmp\\Temp\\New_0000.ply", poDecoder->m_oGeometry_Brick.m_pPoint, iPoint_Count,1);
	//bSave("c:\\tmp\\temp\\New_0077.ply", pMC, iMC_Count, 256, 0);

	Free_Decoder(poDecoder);
	Free_Codec_Env();
	free(poDecoder);
}
void Test_2(void *p)
{
	unsigned char* pBuffer = (unsigned char*)malloc(10000000);
	int iPoint_Count;
	unsigned long long tTime_Stamp;
	int iFrame_No = 0;
	char File[256];

	while (1)
	{
		Get_Data_1(p, pBuffer, &iPoint_Count, &tTime_Stamp);

		//sprintf(File, "c:\\tmp\\temp\\Dest_%04d.ply");
		//bSave(File, pBuffer, iPoint_Count);
		Free_Env_1(p);
		break;
	}
	free(pBuffer);
	return;
}

int main()
{
	//普通解码文件
	//Test_1();
	//return 0;
	
	//此处调用dll获取数据
	void *p=pInit_Env_1();
	Test_2(p);
	
	//Sleep(INFINITE);
	_CrtDumpMemoryLeaks();
	return 0;
}