// Call_Dll_Test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <iostream>
#include "MCPC_Encoder_Dll.h"
#pragma comment(lib, "MCPC_Encoder_Dll.lib")
int bGet_Line(FILE* pFile, char* pLine)
{
	char* pCur = pLine;
	do {
		*pCur = fgetc(pFile);
	} while (*pCur == '\n' || *pCur == '\r');
	if (*pCur == EOF)
		return 0;
	while (*pCur != '\n' && *pCur != '\r' && *pCur != EOF)
	{
		pCur++;
		*pCur = fgetc(pFile);
	}
	*pCur = 0;
	return 1;
}
int bRead_PLY_Header(FILE* pFile, int* piPos_Float_Double, int* pbHas_Normal, int* pbHas_Color,
	int* pbHas_Alpha, int* pbHas_Face, int* piPoint_Count, int* piFace_Count, int* pbText)
{
	int iPos_Float_Double = 0, bHas_Normal = 0, bHas_Color = 0, bHas_Alpha = 0,
		bHas_Face = 0, iPoint_Count = 0, iFace_Count = 0, bText = 0;	//此处全是参数
	int iResult, bRet = 1;
	char Line[256];
	//先全部设置缺省值
	if (piPos_Float_Double)
		*piPos_Float_Double = 0;
	if (pbHas_Normal)
		*pbHas_Normal = 0;
	if (pbHas_Color)
		*pbHas_Color = 0;
	if (pbHas_Alpha)
		*pbHas_Alpha = 0;
	if (pbHas_Face)
		*pbHas_Face = 0;
	if (piPoint_Count)
		*piPoint_Count = 0;
	if (piFace_Count)
		*piFace_Count = 0;
	if (pbText)
		*pbText = 0;

	while (bGet_Line(pFile, Line))
	{
		if (strstr(Line, "element vertex"))
		{//element vertex字段
			iResult = sscanf(Line + strlen("element vertex") + 1, "%d", &iPoint_Count);
			if (iResult < 1)
			{
				printf("No vertex\n");
				bRet = 0;
				goto END;
			}
		}
		else if (_stricmp(Line, "end_header") == 0)
		{//头读完了
			break;
		}
		else if (_stricmp(Line, "format binary_little_endian 1.0") == 0)
			bText = 0;
		else if (_stricmp(Line, "format ascii 1.0") == 0)
			bText = 1;
		else if (_stricmp(Line, "property uchar red") == 0)
			bHas_Color = 1;
		else if (strstr(Line, "property uchar alpha"))
			bHas_Alpha = 1;
		else if (strstr(Line, "property float x"))
			iPos_Float_Double = 0;
		else if (strstr(Line, "property double x"))
			iPos_Float_Double = 1;
		else if (strstr(Line, "property float nx"))
			bHas_Normal = 1;
		else if (strstr(Line, "element face"))
		{
			iResult = sscanf(Line + strlen("element face") + 1, "%d", &iFace_Count);
			if (iResult < 1)
			{
				printf("No Face\n");
				bRet = 0;
				goto END;
			}
			if (iFace_Count)
				bHas_Face = 1;
		}
	}
	if (piPos_Float_Double)
		*piPos_Float_Double = iPos_Float_Double;
	if (pbHas_Normal)
		*pbHas_Normal = bHas_Normal;
	if (pbHas_Color)
		*pbHas_Color = bHas_Color;
	if (pbHas_Alpha)
		*pbHas_Alpha = bHas_Alpha;
	if (pbHas_Face)
		*pbHas_Face = bHas_Face;
	if (piPoint_Count)
		*piPoint_Count = iPoint_Count;
	if (piFace_Count)
		*piFace_Count = iFace_Count;
	if (pbText)
		*pbText = bText;
END:
	return bRet;
}
int bLoad(const char* pcFile, unsigned char* pBuffer, int* piPoint_Count)
{//简单写个装入数据 x,y,z r,g,b 一点15字节
	FILE* pFile = fopen(pcFile, "rb");
	unsigned char* pCur = pBuffer;
	int i, iPoint_Count;
	if (!pFile)
	{
		printf("Fail to open %s\n", pcFile);
		return 0;
	}
	bRead_PLY_Header(pFile, NULL, NULL, NULL, NULL, NULL, &iPoint_Count, NULL, NULL);

	for (i = 0; i < iPoint_Count; i++)
	{
		fread(pCur, 1, 3 * sizeof(float), pFile);
		pCur += 3 * sizeof(float);
		//暂时不需要Normal,向前推进3个float
		fseek(pFile, 3 * sizeof(float), SEEK_CUR);
		fread(pCur, 1, 3, pFile);
		pCur += 3;
	}
	if (piPoint_Count)
		*piPoint_Count = iPoint_Count;
	fclose(pFile);
	return 1;
}
void Send_Data_Test(void *poEnv)
{
	unsigned char* pBuffer = (unsigned char*)malloc(500000 * 15);

	char File[256];
	int iPoint_Count,iFrame_No,iSize;
	unsigned long long tTime_Stamp = 0;
	//循环拿数据
	iFrame_No = 0;
	while (1)
	{
		sprintf(File, "D:\\Sample\\chi\\chi%04d.ply", iFrame_No);
		bLoad(File, pBuffer, &iPoint_Count);
		iSize = iPoint_Count * 15;
		Send_Data(poEnv,pBuffer, iPoint_Count, 0);
		
		iFrame_No = (iFrame_No + 1) % 199;
		if (iFrame_No == 0)
			break;
		//printf("Frame No:%d\n", iFrame_No);
	}
}
int main()
{
    void *p=pInit_Env();
	Send_Data_Test(p);
    return 0;
}
