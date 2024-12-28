// MCPC_Codec.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "stdio.h"
#include "PCC_Encoder.h"
#include "MCPC_Codec.h"
void Write_PLY_Header(FILE* pFile, int iPoint_Count,int bHas_Color)
{
	char Header[256];

	sprintf(Header, "ply\r\n");
	sprintf(Header + strlen(Header), "format ascii 1.0\r\n");
	sprintf(Header + strlen(Header), "comment HQYT generated\r\n");
	sprintf(Header + strlen(Header), "element vertex %d\r\n", iPoint_Count);
	
	sprintf(Header + strlen(Header), "property float x\r\n");
	sprintf(Header + strlen(Header), "property float y\r\n");
	sprintf(Header + strlen(Header), "property float z\r\n");
	
	if (bHas_Color)
	{
		sprintf(Header + strlen(Header), "property uchar red\r\n");
		sprintf(Header + strlen(Header), "property uchar green\r\n");
		sprintf(Header + strlen(Header), "property uchar blue\r\n");
	}
	sprintf(Header + strlen(Header), "end_header\r\n");
	fwrite(Header, 1, strlen(Header), pFile);
	return;
}
int bRead_PLY_Header(FILE* pFile, int* piPos_Float_Double, int* pbHas_Normal,int* pbHas_Color, 
	int* pbHas_Alpha, int* pbHas_Face, int* piPoint_Count, int* piFace_Count,int *pbText)
{
	int iPos_Float_Double=0, bHas_Normal=0, bHas_Color=0, bHas_Alpha=0,
		bHas_Face=0, iPoint_Count=0, iFace_Count=0,bText=0;	//此处全是参数
	int iResult,bRet=1;
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

int bRead_PLY_File(const char* pcFile, Mem_Mgr* poMem_Mgr, PCC_Point **ppPoint,int *piPoint_Count)
{//为了简化，MC的边长不再统计分析，直接从上游流下来 fMC_Size
	//MC* pMC;
	int i,bRet=1;
	PCC_Point oPoint,*pPoint;

	FILE* pFile = fopen(pcFile, "rb");
	int iPos_Float_Double, bHas_Normal,	bHas_Color, bHas_Alpha,
		bHas_Face, iPoint_Count,iFace_Count,bText;	//此处全是参数

	if (!pFile)
	{
		printf("Fail to open:%s\n", pcFile);
		return 0;
	}
	if (!bRead_PLY_Header(pFile, &iPos_Float_Double, &bHas_Normal, &bHas_Color, &bHas_Alpha, &bHas_Face, &iPoint_Count, &iFace_Count, &bText))
	{
		printf("Fail to read header\n");
		goto END;
	}

	pPoint=(PCC_Point*)pMalloc_At_End(poMem_Mgr, (iPoint_Count + 1) * sizeof(MC));
	for (i = 1; i <= iPoint_Count; i++)
	{
		if (bText)
		{
			char Line[256],*pCur=Line,Word[32];
			bGet_Line(pFile, Line);
			Get_Word(&pCur, Word);
			oPoint.Pos_f[0] = (float)atof(Word);
			Get_Word(&pCur, Word);
			oPoint.Pos_f[1] = (float)atof(Word);
			Get_Word(&pCur, Word);
			oPoint.Pos_f[2] = (float)atof(Word);
			if (bHas_Normal)
			{//暂时不需要Normal
				Get_Word(&pCur, Word);
				Get_Word(&pCur, Word);
				Get_Word(&pCur, Word);
			}
			if (bHas_Color)
			{
				Get_Word(&pCur, Word);
				oPoint.m_Color[0] = atoi(Word);
				Get_Word(&pCur, Word);
				oPoint.m_Color[1] = atoi(Word);
				Get_Word(&pCur, Word);
				oPoint.m_Color[2] = atoi(Word);
				if (bHas_Alpha)
				{
					Get_Word(&pCur, Word);
					oPoint.m_Color[3] = atoi(Word);
				}
			}
		}else
		{
			if (iPos_Float_Double == 0)
			{//float position
				fread(oPoint.Pos_f, 1, 3 * sizeof(float), pFile);
			}
			else
			{//double
				double fValue;
				fread(&fValue, 1, sizeof(double), pFile);
				oPoint.Pos_f[0] = (float)fValue;
				fread(&fValue, 1, sizeof(double), pFile);
				oPoint.Pos_f[1] = (float)fValue;
				fread(&fValue, 1, sizeof(double), pFile);
				oPoint.Pos_f[2] = (float)fValue;
			}
			if (bHas_Normal)
			{//暂时不需要Normal
				fseek(pFile, 3 * sizeof(float), SEEK_CUR);
			}
			if (bHas_Color)
			{
				if (bHas_Alpha)
					fread(oPoint.m_Color, 1, 4, pFile);
				else
					fread(oPoint.m_Color, 1, 3, pFile);
			}
		}
		pPoint[i] = oPoint;		
	}
	if (ppPoint)
		*ppPoint = pPoint;
	if (piPoint_Count)
		*piPoint_Count = iPoint_Count;
END:
	//printf("bRet:%d Point Count:%d \n", bRet, iPoint_Count);
	if (pFile)
		fclose(pFile);
	return bRet;
}
int bGen_MC(Mem_Mgr *poMem_Mgr,PCC_Point Point[], int iPoint_Count,float fMC_Size, float fScale,MC** ppMC, int* piMC_Count)
{//分析点云，将所有点放入MC
	float eps = fMC_Size / 256.f;
	float fHalf_MC_Size = fMC_Size / 2.f;
	PCC_Point oPoint;
	PCC_Pos oPos;
	MC* pMC = NULL,oMC,*poItem_Exist;
	int i,j,iDim,iValue,iValue_1,iCount,iMC_Count,iCur_Item=1;

	int* pHash_Table, iHash_Size, iHash_Pos, iPos;

	pMC = (MC*)pMalloc(poMem_Mgr, (iPoint_Count + 1) * sizeof(MC));
	iHash_Size = iPoint_Count;
	pHash_Table = (int*)pMalloc(poMem_Mgr, iHash_Size * sizeof(int));
	memset(pHash_Table, 0, iHash_Size * sizeof(int));
	for (i = 1; i <= iPoint_Count; i++)
	{
		oPoint = Point[i];
		//取出数据后，此处可以分析MC了
		iCount = 0;
		
		for (iValue_1=j = 0; j < 3; j++)
		{//将点的三个坐标掏出来，分析一下靠在那条边上
			if (oPoint.Pos_f[j] >= 0)
			{
				iValue =(int)( oPoint.Pos_f[j] / fMC_Size);
				oPos.Pos[j] = iValue;
				if (oPoint.Pos_f[j] - iValue * fMC_Size < eps)
					iCount++;
				else if ((iValue + 1) * fMC_Size - oPoint.Pos_f[j] < eps)
				{
					oPos.Pos[j]++;
					iCount++;
				}else
				{//此时，iValue是点落在某一轴上的量化值
					iValue =(int) ((oPoint.Pos_f[j] - iValue * fMC_Size) * fScale /fMC_Size +0.5) ;
					iValue_1 = Clip(iValue);
					iDim = j;
				}
			}else //<0
			{
				printf("Not implemented\n");
				return 0;
			}
		}
		//if (oPos.Pos[0] == 235 && oPos.Pos[1] == 376 && oPos.Pos[2] == 3)
			//printf("i:%d x:%f y:%f z:%f iDim:%d\n", i,oPoint.Pos_f[0], oPoint.Pos_f[1], oPoint.Pos_f[2],iDim);
			//printf("Here");
		if (iCount < 2)
		{
			printf("err");
			return 0;
		}

		//加入散列表
		iHash_Pos = (unsigned int)((oPos.Pos[0] ^ (oPos.Pos[1] << 8) ^ (oPos.Pos[2] << 16))) % iHash_Size;
		if (iPos = pHash_Table[iHash_Pos])
		{//散列表有东西
			do {
				poItem_Exist = &pMC[iPos];
				if (poItem_Exist->Pos_i[0] == oPos.Pos[0] && poItem_Exist->Pos_i[1] == oPos.Pos[1] && poItem_Exist->Pos_i[2] == oPos.Pos[2])
				{//重复了，此点不加入散列表
					poItem_Exist->m_Point[iDim].m_iColor_Pos = oPoint.m_iColor;
					poItem_Exist->m_Point[iDim].m_iPos = iValue_1;
					//if (iPos == 1)
						//printf("Here");
					poItem_Exist->m_iOccupancy |= (1 << iDim);
					goto HASH_END;
				}
			} while (iPos = poItem_Exist->m_iNext);
		}
		oMC.m_oPos = oPos;
		oMC.m_Point[iDim].m_iColor_Pos = oPoint.m_iColor;
		oMC.m_Point[iDim].m_iPos = iValue;
		oMC.m_iNext = pHash_Table[iHash_Pos];
		oMC.m_iOccupancy = 1 << iDim;
		pMC[iCur_Item] = oMC;
		pHash_Table[iHash_Pos] = iCur_Item++;
	HASH_END:
		;
	}

	iMC_Count = iCur_Item - 1;
	if (pHash_Table)
		Free(poMem_Mgr, pHash_Table);
	pShrink(poMem_Mgr, pMC, (iMC_Count + 1) * sizeof(MC));
	if (ppMC)
		*ppMC = pMC;
	if (piMC_Count)
		*piMC_Count = iMC_Count;
	return 1;
}
int bSave(const char* pcFile, PCC_Point Point[], int iPoint_Count, int bHas_Color)
{
	FILE* pFile = fopen(pcFile, "wb");
	int i;
	PCC_Point oPoint;

	if (!pFile)
	{
		printf("Fail to save:%s\n", pcFile);
		return 0;
	}
		
	Write_PLY_Header(pFile, iPoint_Count, bHas_Color);
	for (i = 1; i <= iPoint_Count; i++)
	{
		oPoint = Point[i];
		fprintf(pFile, "%d %d %d", oPoint.Pos_i[0], oPoint.Pos_i[1], oPoint.Pos_i[2]);
		if (bHas_Color)
			fprintf(pFile, " %d %d %d \r\n", oPoint.m_Color[0], oPoint.m_Color[1], oPoint.m_Color[2]);
		else
			fprintf(pFile, "\r\n");
	}
	fclose(pFile);

	return 1;
}
int bSave(const char* pcFile, MC* pMC, int iMC_Count, float fScale,int bColor,int Min[3])
{//将MC的位置xfScale,然后再
	FILE* pFile = fopen(pcFile, "wb");
	int i,j,k,iValue,iPoint_Count=0;
	MC oMC;
	for (i = 1; i <= iMC_Count; i++)
		iPoint_Count += Bit_1_Count[pMC[i].m_iOccupancy];

	Write_PLY_Header(pFile, iPoint_Count,bColor);
	for (i = 1; i <= iMC_Count; i++)
	{
		oMC = pMC[i];
		for (j = 0; j < 3; j++)
		{
			if (oMC.m_iOccupancy & (1 << j))
			{//有东西，存起来
				//if (i == iMC_Count)
					//printf("Here");
				int Temp[3];
				for (k = 0; k < 3; k++)
				{
					iValue = (int)( (oMC.Pos_i[k]+Min[k])  * fScale +0.5);
					if (k == j)
						iValue += oMC.m_Point[j].m_iPos;
					
					Temp[k] = iValue;
					fprintf(pFile, "%d ", iValue);
				}
				//if (Temp[0] == 57630 && Temp[1] == 14025 && Temp[2] == 65280)
					//printf("Here");
				if(bColor)
					for (k = 0; k < 3; k++)
						fprintf(pFile, "%d ", oMC.m_Point[j].m_Color[k]);
				fprintf(pFile, "\r\n");
			}
		}
	}
	fclose(pFile);
	return 1;
}
//void Gen_Point(PCC_Encoder* poEncoder, PCC_Point* ppPoint, int* piPoint_Count, MC* pMC, int iMC_Count)
//{
//	int i, j,iPoint_Count, iMax = 0, Min[3] = { 0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF }, Max[3] = { -0x7FFFFFFF,-0x7FFFFFFF,-0x7FFFFFFF };
//	PCC_Point* pPoint;
//	PCC_Pos oPos;
//	PCC_Geometry_Brick* poBrick = &poEncoder->m_oGeometry_Brick;
//	PCC_SPS* poSPS = &poEncoder->m_oSPS;
//	PCC_APS* poAPS = &poEncoder->m_oAPS;
//	Mem_Mgr* poMem_Mgr = &poEncoder->m_oMem_Mgr;
//	
//	pPoint = (PCC_Point*)pMalloc(poMem_Mgr, iMC_Count *sizeof(PCC_Point));
//	oPos = pMC[1].m_oPos;
//	Min[0] = Max[0] = oPos.Pos[0];
//	Min[1] = Max[1] = oPos.Pos[1];
//	Min[2] = Max[2] = oPos.Pos[2];
//
//	poBrick->m_oHeader.geom_num_points = iMC_Count;
//	for (i = 2; i <= iMC_Count; i++)
//	{
//		oPos = pMC[i].m_oPos;
//
//		if (Min[0] > oPos.Pos[0])
//			Min[0] = oPos.Pos[0];
//		else if (Max[0] < (int)oPos.Pos[0])
//			Max[0] = (int)oPos.Pos[0];
//
//		if (Min[1] > oPos.Pos[1])
//			Min[1] = oPos.Pos[1];
//		else if (Max[1] < (int)oPos.Pos[1])
//			Max[1] = (int)oPos.Pos[1];
//
//		if (Min[2] > oPos.Pos[2])
//			Min[2] = oPos.Pos[2];
//		else if (Max[2] < (int)oPos.Pos[2])
//			Max[2] = (int)oPos.Pos[2];
//	}
//	return;
//}
void Reset_Point(PCC_Encoder *poEncoder,PCC_Point Point[], MC* pMC, int iMC_Count)
{//仅将MC作为点，重新做一遍
	int i,iMax = 0,Min[3] = { 0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF }, Max[3] = { -0x7FFFFFFF,-0x7FFFFFFF,-0x7FFFFFFF };
	PCC_Pos oPos;
	PCC_Geometry_Brick* poBrick = &poEncoder->m_oGeometry_Brick;
	PCC_SPS* poSPS = &poEncoder->m_oSPS;
	PCC_APS* poAPS = &poEncoder->m_oAPS;

	oPos = pMC[1].m_oPos;
	Min[0] = Max[0] = oPos.Pos[0];
	Min[1] = Max[1] = oPos.Pos[1];
	Min[2] = Max[2] = oPos.Pos[2];

	poBrick->m_oHeader.geom_num_points = iMC_Count;
	for (i = 2; i <=iMC_Count; i++)
	{
		oPos = pMC[i].m_oPos;
		
		if (Min[0] > oPos.Pos[0])
			Min[0] = oPos.Pos[0];
		else if (Max[0] < (int)oPos.Pos[0])
			Max[0] = (int)oPos.Pos[0];

		if (Min[1] > oPos.Pos[1])
			Min[1] = oPos.Pos[1];
		else if (Max[1] < (int)oPos.Pos[1])
			Max[1] = (int)oPos.Pos[1];

		if (Min[2] > oPos.Pos[2])
			Min[2] = oPos.Pos[2];
		else if (Max[2] < (int)oPos.Pos[2])
			Max[2] = (int)oPos.Pos[2];
	}

	for (i = 0; i < 3; i++)
	{
		if (iMax < Max[i])
			iMax = Max[i];
		poSPS->seq_bounding_box.seq_bounding_box_whd[i] = Max[i] - Min[i] + 1;
	}
	int* pPos;
	//再整一遍，调整坐标
	for (i = 1; i <= iMC_Count; i++)
	{
		pPos = pMC[i].Pos_i;
		pPos[0] -= Min[0];
		pPos[1] -= Min[1];
		pPos[2] -= Min[2];
		//if (pPos[0] == 234 && pPos[1] == 25 && pPos[2] == 237)
			//printf("here");
		Point[i].Pos = pMC[i].m_oPos;
	}

	iMax = 0;
	for (i = 0; i < 3; i++)
	{
		Max[i] -= Min[i];
		if (iMax < Max[i])
			iMax = Max[i];
	}
	poBrick->m_iMax_Shift = 20 - (int)ceil(log2((float)(Max[2] + 2)));
	poBrick->m_oHeader.geom_max_node_size_log2 = (int)ceil(log2((float)(iMax + 1)));
	poSPS->seq_bounding_box.seq_bounding_box_xyz[0] = Min[0];
	poSPS->seq_bounding_box.seq_bounding_box_xyz[1] = Min[1];
	poSPS->seq_bounding_box.seq_bounding_box_xyz[2] = Min[2];

	pShrink(&poEncoder->m_oMem_Mgr, Point, iMC_Count * sizeof(PCC_Point));
	poBrick->m_pOrder_Point = (PCC_Point*)pMalloc(&poEncoder->m_oMem_Mgr, sizeof(PCC_Point) * (iMC_Count+1));
	poBrick->m_pPoint = Point;

	poEncoder->m_iOut_Buffer_Size = iMC_Count * 6;
	poEncoder->m_pOut_Buffer = (unsigned char*)pMalloc(&poEncoder->m_oMem_Mgr, poEncoder->m_iOut_Buffer_Size);

	return;
}
void Resort_MC(Mem_Mgr *poMem_Mgr,PCC_Point* poPoint, int iPoint_Count, MC **ppMC,int *piMC_Count)
{//将原来的MC重排为Morton有序
	int i,iMC_Count=iPoint_Count;
	MC* pOrg_MC = *ppMC, * pNew_MC = (MC*)pMalloc(poMem_Mgr, iMC_Count * sizeof(MC));
	for (i = 1; i <= iPoint_Count; i++)
		pNew_MC[i] = pOrg_MC[poPoint[i].m_iOrg_Index];
	
	Free(poMem_Mgr, pOrg_MC);
	if(ppMC)
		*ppMC = pNew_MC;
	if(piMC_Count)
		*piMC_Count = iMC_Count;
	return;
}


void Build_MC_Hash_Table(MC* pMC, int iMC_Count,int Hash_Table[], int iHash_Size)
{
	int i,iHash_Pos;
	PCC_Pos oPos;
	memset(Hash_Table, 0, iHash_Size * sizeof(int));
	for (i = 1; i <= iMC_Count; i++)
	{
		oPos = pMC[i].m_oPos;
		iHash_Pos = (unsigned int)((oPos.Pos[0] ^ (oPos.Pos[1] << 8) ^ (oPos.Pos[2] << 16))) % iHash_Size;
		pMC[i].m_iNext = Hash_Table[iHash_Pos];
		Hash_Table[iHash_Pos] = i;
	}
	return;
}
MC* poFind_Item(MC Item[], PCC_Pos oPos, int Hash_Table[], int iHash_Size)
{//从散列表中根据给定的x,y,z找出一个Item
	int iPos, iHash_Pos;
	MC oItem_Exist;
	iHash_Pos = (unsigned int)((oPos.Pos[0] ^ (oPos.Pos[1] << 8) ^ (oPos.Pos[2] << 16))) % iHash_Size;
	if (iPos = Hash_Table[iHash_Pos])
	{//散列表有东西
		do {
			oItem_Exist = Item[iPos];
			if (oItem_Exist.Pos_i[0] == oPos.Pos[0] && oItem_Exist.Pos_i[1] == oPos.Pos[1] && oItem_Exist.Pos_i[2] == oPos.Pos[2])
			{//重复了，此点不加入散列表
				return &Item[iPos];
			}
		} while (iPos = oItem_Exist.m_iNext);
	}
	//printf("查无此坐标：%d %d %d\n", oPos.Pos[0], oPos.Pos[1], oPos.Pos[2] );
	return NULL;
}
void Encode_MC(PCC_Encoder *poEncoder,MC* pMC, int iMC_Count,int *piPoint_Count)
{//对MC的三条边占位进行熵编码
	MC* poMC,*Found[3];
	PCC_Pos oPos;
	int i,j,iOccupancy,iCount=0,iHash_Size = iMC_Count,iPoint_Count=0;
	int* pHash_Table = (int*)pMalloc(&poEncoder->m_oMem_Mgr, iHash_Size * sizeof(int));
	Build_MC_Hash_Table(pMC, iMC_Count, pHash_Table, iHash_Size);

	////测试一下
	//for (i = 1; i <= iMC_Count; i++)
	//{
	//	poMC = &pMC[i];
	//	poFound = poFind_Item(pMC,poMC->m_oPos, pHash_Table, iHash_Size);
	//	if (poMC->Pos_i[0] != poFound->Pos_i[0] ||
	//		poMC->Pos_i[1] != poFound->Pos_i[1] ||
	//		poMC->Pos_i[2] != poFound->Pos_i[2])
	//		printf("err");
	//}

	//先搞一组Bit Modal，理论上，如果取3组邻居，每组有3个Occupancy
	BitPtr oBitPtr;
	Arithmetic_Encoder oCabac;
	AdaptiveBitModel *poModal, MC_Occupancy_Modal[8][8][8][3];
	AdaptiveBitModel MC_Edge_Pos_High[3][3][3];	//[2]时表示没有邻居

	//AdaptiveBitModel Residual[7];	//位面Residual，可以进一步试验，似乎效果不错

	Init_BitPtr(&oBitPtr, poEncoder->m_pOut_Buffer+poEncoder->m_iOut_Data_Size, poEncoder->m_iOut_Buffer_Size- poEncoder->m_iOut_Data_Size);
	oBitPtr.m_iCur += 4;	//留着回填数据用

	Init_Arithmetic_Codec(&oCabac, &oBitPtr);

	//可以不要
	/*for (i = 0; i < 8 * 8 * 8 * 3; i++)
	{
		poModal = &((AdaptiveBitModel*)MC_Occupancy_Modal)[i];
		Init_Bit_Context_Modal(poModal);
	}*/
	//FILE* pFile = fopen("c:\\tmp\\encode.txt", "wb");
	for (i = 1; i <= iMC_Count; i++)
	{
		poMC = &pMC[i];
		
		for (j = 0; j < 3; j++)
		{
			oPos = poMC->m_oPos;
			oPos.Pos[j]--;
			Found[j] = poFind_Item(pMC, oPos, pHash_Table, iHash_Size);
		}
		//if (i == iMC_Count - 1)
			//printf("here");
		iOccupancy = poMC->m_iOccupancy;
		for (j = 0; j < 3; j++,iOccupancy >>= 1)
		{
			poModal = &MC_Occupancy_Modal[Found[0] ? Found[0]->m_iOccupancy : 0][Found[1] ? Found[1]->m_iOccupancy : 0][Found[2] ? Found[2]->m_iOccupancy : 0][j];
			//fprintf(pFile, "modal:%d ", (int)(poModal - (AdaptiveBitModel*)MC_Occupancy_Modal));
			//fprintf(pFile, "Bit:%d ", iOccupancy & 1);
			Encode_Bit(iOccupancy & 1, poModal, &oCabac);
			if (iOccupancy & 1)
			{//该轴有值
				iPoint_Count++;
				//poModal = &MC_Edge_Pos_High[Found[0] ? Found[0]->m_Point[0].m_iPos >> 7 : 0][Found[1] ? Found[1]->m_Point[1].m_iPos >> 7 : 0][Found[2] ? Found[2]->m_Point[2].m_iPos >> 7 : 0];
				int Index[3],k;
				for (k = 0; k < 3; k++)
				{
					if (!Found[k] || k == j || ! (Found[k]->m_iOccupancy&(1<<j)) )
						Index[k] = 2;
					else
						Index[k] = Found[k]->m_Point[j].m_iPos >> 7;
				}
				poModal = &MC_Edge_Pos_High[Index[0]][Index[1]][Index[2]];
				Encode_Bit(poMC->m_Point[j].m_iPos>>7, poModal, &oCabac);
				//fprintf(pFile,"Hbit modal:%d Bit:%d ", (int)(poModal - (AdaptiveBitModel*)MC_Edge_Pos_High), poMC->m_Point[j].m_iPos >> 7);
			}
		}
		//fprintf(pFile,"\r\n");
	}
	//fclose(pFile);

	Flush_Cabac(&oCabac);
	oBitPtr.m_iCur += oCabac.offset;	//oCabac.offset正是输出缓冲区大小
	poEncoder->m_iOut_Data_Size += oBitPtr.m_iCur;
	oBitPtr.m_iBitPtr=oBitPtr.m_iCur = 0;
	Write_Len(&oBitPtr, oCabac.offset);
	//printf("Total:%d\n", oCabac.offset);
	Free(&poEncoder->m_oMem_Mgr, pHash_Table);

	//接着将所有点的低七位写入
	Init_BitPtr(&oBitPtr, poEncoder->m_pOut_Buffer + poEncoder->m_iOut_Data_Size, poEncoder->m_iOut_Buffer_Size - poEncoder->m_iOut_Data_Size);

	//Temp code
	//Init_Arithmetic_Codec(&oCabac, &oBitPtr);

	for (i = 1; i <= iMC_Count; i++)
	{
		poMC = &pMC[i];
		for (j = 0; j < 3; j++)
		{
			if (poMC->m_iOccupancy & (1 << j))
			{
				iCount++;
				
				////此处用bit_plane 有点用，未知普遍性
				////0.74
				//Encode_Bit((poMC->m_Point[j].m_iPos & 64)>0 ,&Residual[0], &oCabac);
				////0.76
				//Encode_Bit((poMC->m_Point[j].m_iPos & 32) > 0, &Residual[1], &oCabac);
				////0.807
				//Encode_Bit((poMC->m_Point[j].m_iPos & 16) > 0, &Residual[2], &oCabac);
				////0.905
				//Encode_Bit((poMC->m_Point[j].m_iPos & 8) > 0, &Residual[3], &oCabac);
				//1.001
				//Encode_Bit((poMC->m_Point[j].m_iPos & 4) > 0, &Residual[4], &oCabac);
				//1.026
				//Encode_Bit((poMC->m_Point[j].m_iPos & 2) > 0, &Residual[5], &oCabac);
				//1.027
				//Encode_Bit((poMC->m_Point[j].m_iPos & 1) > 0, &Residual[6], &oCabac);

				WriteBits3(oBitPtr,7, poMC->m_Point[j].m_iPos & 0x7F);
				//WriteBits(&oBitPtr, 7, poMC->m_Point[j].m_iPos & 0x7F);
			}
		}
	}
	//补完字节
	if (oBitPtr.m_iBitPtr)
	{
		oBitPtr.m_iBitPtr = 0;
		oBitPtr.m_iCur++;
	}
	poEncoder->m_iOut_Data_Size += oBitPtr.m_iCur;
	if (piPoint_Count)
		*piPoint_Count = iPoint_Count;
	//printf("Total:%d\n", oBitPtr.m_iCur);
	return;
}
int bGen_MC(Mem_Mgr* poMem_Mgr, unsigned char *pBuffer, int iPoint_Count, float fMC_Size, float fScale, MC** ppMC, int* piMC_Count,int bColor_Transform)
{//分析点云，将所有点放入MC
	float eps = fMC_Size / 256.f;
	float fHalf_MC_Size = fMC_Size / 2.f;
	PCC_Point oPoint,*pCur_Point;
	unsigned char* pCur,*pColor;
	PCC_Pos oPos;
	MC* pMC = NULL, oMC, * poItem_Exist;
	int i, j, iDim, iValue, iValue_1, iCount, iMC_Count, iCur_Item = 1;

	int* pHash_Table, iHash_Size, iHash_Pos, iPos;
	
	pMC = (MC*)pMalloc(poMem_Mgr, (iPoint_Count + 1) * sizeof(MC));
	iHash_Size = iPoint_Count;
	pHash_Table = (int*)pMalloc(poMem_Mgr, iHash_Size * sizeof(int));
	memset(pHash_Table, 0, iHash_Size * sizeof(int));

	pCur = pBuffer;
	pCur_Point = (PCC_Point*)pCur;
	for (i = 1; i <= iPoint_Count; i++)
	{
		oPoint = *pCur_Point;
		//取出数据后，此处可以分析MC了
		iCount = 0;

		for (iValue_1 = j = 0; j < 3; j++)
		{//将点的三个坐标掏出来，分析一下靠在那条边上
			if (oPoint.Pos_f[j] >= 0)
			{
				iValue = (int)(oPoint.Pos_f[j] / fMC_Size);
				oPos.Pos[j] = iValue;
				if (oPoint.Pos_f[j] - iValue * fMC_Size < eps)
					iCount++;
				else if ((iValue + 1) * fMC_Size - oPoint.Pos_f[j] < eps)
				{
					oPos.Pos[j]++;
					iCount++;
				}
				else
				{//此时，iValue是点落在某一轴上的量化值
					iValue = (int)((oPoint.Pos_f[j] - iValue * fMC_Size) * fScale / fMC_Size + 0.5);
					iValue_1 = Clip(iValue);
					iDim = j;
				}
			}
			else //<0
			{
				printf("Not implemented\n");
				return 0;
			}
		}
		//if (oPos.Pos[0] == 235 && oPos.Pos[1] == 376 && oPos.Pos[2] == 3)
			//printf("i:%d x:%f y:%f z:%f iDim:%d\n", i,oPoint.Pos_f[0], oPoint.Pos_f[1], oPoint.Pos_f[2],iDim);
			//printf("Here");
		if (iCount < 2)
		{
			printf("err");
			return 0;
		}

		//加入散列表
		iHash_Pos = (unsigned int)((oPos.Pos[0] ^ (oPos.Pos[1] << 8) ^ (oPos.Pos[2] << 16))) % iHash_Size;
		if (iPos = pHash_Table[iHash_Pos])
		{//散列表有东西
			do {
				poItem_Exist = &pMC[iPos];
				if (poItem_Exist->Pos_i[0] == oPos.Pos[0] && poItem_Exist->Pos_i[1] == oPos.Pos[1] && poItem_Exist->Pos_i[2] == oPos.Pos[2])
				{//重复了，此点不加入散列表
					//若靠在0上则需要判断是否存在重复点
					if (!iValue_1)
					{
						for (j = 0; j < 3; j++)
						{
							if (j!=iDim && poItem_Exist->m_iOccupancy & (1 << j) && poItem_Exist->m_Point[j].m_iPos == 0)
							{
								//printf("*");
								goto HASH_END;	//存在重复点，靠在0点上，所以这点不加
							}
						}
					}
					if (bColor_Transform)
					{
						pColor = RGB_2_YUV[oPoint.m_Color[0]][oPoint.m_Color[1]][oPoint.m_Color[2]];
						oPoint.m_Color[0] = pColor[0];
						oPoint.m_Color[1] = pColor[1];
						oPoint.m_Color[2] = pColor[2];
						//pColor = YUV_2_RGB[pColor[0]][pColor[1]][pColor[2]];
					}
					poItem_Exist->m_Point[iDim].m_iColor_Pos = oPoint.m_iColor;
					poItem_Exist->m_Point[iDim].m_iPos = iValue_1;
					poItem_Exist->m_iOccupancy |= (1 << iDim);
					goto HASH_END;
				}
			} while (iPos = poItem_Exist->m_iNext);
		}
		oMC.m_oPos = oPos;
		
		if (bColor_Transform)
		{
			//if (oPoint.m_Color[0] == 195 && oPoint.m_Color[1] == 201 && oPoint.m_Color[2] == 215)
				//printf("Here");
			pColor = RGB_2_YUV[oPoint.m_Color[0]][oPoint.m_Color[1]][oPoint.m_Color[2]];
			oPoint.m_Color[0] = pColor[0];
			oPoint.m_Color[1] = pColor[1];
			oPoint.m_Color[2] = pColor[2];
			//pColor = YUV_2_RGB[pColor[0]][pColor[1]][pColor[2]];
		}

		oMC.m_Point[iDim].m_iColor_Pos = oPoint.m_iColor;
		oMC.m_Point[iDim].m_iPos = iValue;
		oMC.m_iNext = pHash_Table[iHash_Pos];
		oMC.m_iOccupancy = 1 << iDim;
		pMC[iCur_Item] = oMC;
		pHash_Table[iHash_Pos] = iCur_Item++;
	HASH_END:
		;

		pCur += 15;
		pCur_Point = (PCC_Point*)pCur;
	}

	iMC_Count = iCur_Item - 1;
	if (pHash_Table)
		Free(poMem_Mgr, pHash_Table);
	pShrink(poMem_Mgr, pMC, (iMC_Count + 1) * sizeof(MC));
	if (ppMC)
		*ppMC = pMC;
	if (piMC_Count)
		*piMC_Count = iMC_Count;
	return 1;
}
void Pre_Process(Mem_Mgr *poMem_Mgr,MC *pMC,int iMC_Count, PCC_Predictor** ppPredictor,int iPoint_Count,int iMax_Neighbour_MC_Count,int iMC_Shift_Bit)
{//对每一个MC寻找邻近MC
	PCC_Point oPoint,Point_Neighbour[12*3];	//从周围26个邻近MC中选择临近点
	PCC_Predictor *poPredictor,*pPredictor = (PCC_Predictor*)pMalloc(poMem_Mgr, (iPoint_Count + 1) * sizeof(PCC_Predictor));
	int i,j,k,l,m,bSort,iCur_Point = 0,iNeighbour_MC_Count,iNeighbour_Point_Count;
	unsigned int iWeight;
	MC oMC, oItem_Exist;//最高取到周围26个邻近MC
	PCC_Point oPos;
	static int Delta[][3] = { {-1,0,0},{0,-1,0},{0,0,-1},
		{ 1,0,-1 },{ 1,-1,0 },{ 0,1,-1 },{ 0,-1,1 },{ 0, -1, -1 },{ -1,1,0 },{ -1,0,1 },{ -1,0,-1 },{ -1,-1,0 } };
	int* pDelta;
	int iHash_Size = iMC_Count, * pHash_Table, iHash_Pos, iPos;
	pHash_Table = (int*)pMalloc(poMem_Mgr,iHash_Size * sizeof(int));
	memset(pHash_Table, 0, iHash_Size * sizeof(int));

	for (l=0,i = 1; i <=iMC_Count; i++)
	{
		if (i == 87654)
			printf("Here");

		oMC = pMC[i];
		iNeighbour_MC_Count = iNeighbour_Point_Count=0;
		for (j = 0; j < 12 && iNeighbour_MC_Count<=iMax_Neighbour_MC_Count; j++)
		{//头三种模式都是来自相邻面，根本不必排序，能填满就算胜利
			oPos.Pos = oMC.m_oPos;
			pDelta = Delta[j];
			oPos.Pos_i[0] += pDelta[0];
			oPos.Pos_i[1] += pDelta[1];
			oPos.Pos_i[2] += pDelta[2];
			iHash_Pos = (unsigned int)((oPos.Pos_i[0] ^ (oPos.Pos_i[1] << 8) ^ (oPos.Pos_i[2] << 16))) % iHash_Size;
			iPos = pHash_Table[iHash_Pos];
			if (iPos)
			{
				do {
					oItem_Exist = pMC[iPos];
					if (oItem_Exist.Pos_i[0] == oPos.Pos_i[0] && oItem_Exist.Pos_i[1] == oPos.Pos_i[1] && oItem_Exist.Pos_i[2] == oPos.Pos_i[2])
					{//找到
						//MC_Neighbour[iNeighbour_MC_Count++] = oItem_Exist;
						oItem_Exist.Pos_i[0] <<= iMC_Shift_Bit;
						oItem_Exist.Pos_i[1] <<= iMC_Shift_Bit;
						oItem_Exist.Pos_i[2] <<= iMC_Shift_Bit;
						for (k = 0; k < 3; k++,oItem_Exist.m_iOccupancy>>=1)
						{
							if (oItem_Exist.m_iOccupancy & 1)
							{//x轴上有东西
								oPoint.Pos = oItem_Exist.m_oPos;
								oPoint.Pos_i[k] += oItem_Exist.m_Point[k].m_iPos;
								oPoint.m_iColor = oItem_Exist.m_Point[k].m_iColor_Pos;
								Point_Neighbour[iNeighbour_Point_Count++] = oPoint;
							}
						}
						iNeighbour_MC_Count++;
						goto HASH_END;
					}
				} while (iPos = oItem_Exist.m_iNext);
			}
		HASH_END:
			;
		}
		//将当前MC加入散列表
		iHash_Pos = (unsigned int)((oMC.Pos_i[0] ^ (oMC.Pos_i[1] << 8) ^ (oMC.Pos_i[2] << 16))) % iHash_Size;
		oMC.m_iNext = pHash_Table[iHash_Pos];
		pHash_Table[iHash_Pos] = i;
		//printf("%d %d %d\n", oMC.Pos_i[0], oMC.Pos_i[1], oMC.Pos_i[2]);
		oMC.Pos_i[0] <<= iMC_Shift_Bit;
		oMC.Pos_i[1] <<= iMC_Shift_Bit;
		oMC.Pos_i[2] <<= iMC_Shift_Bit;
		//printf("iNeighbour_Point_Count:%d\n", iNeighbour_Point_Count);
		for (j = 0; j < 3; j++,oMC.m_iOccupancy>>=1)
		{
			if (oMC.m_iOccupancy & 1)
			{//该轴有点

				poPredictor = &pPredictor[l++];
				poPredictor->predMode = poPredictor->m_iNeightbour_Count = 0;

				oPos.Pos = oMC.m_oPos;
				oPos.Pos_i[j] += oMC.m_Point[j].m_iPos;
				oPos.m_iColor = oMC.m_Point[j].m_iColor_Pos;
				for (k = 0; k < iNeighbour_Point_Count; k++)
				{
					oPoint = Point_Neighbour[k];
					iGet_Distance_1(oPos.Pos_i,oPoint.Pos_i,iWeight);
					bSort = 0;
					if (poPredictor->m_iNeightbour_Count < 3)
					{
						poPredictor->m_Neighbour[poPredictor->m_iNeightbour_Count++] = { (int)oPoint.m_iColor,(unsigned int)iWeight };
						bSort = 1;
					}else if (iWeight < poPredictor->m_Neighbour[3 - 1].m_iWeight)
					{
						poPredictor->m_Neighbour[3 - 1] = { k,(unsigned int)iWeight };
						bSort = 1;
					}
					//根据TMC3, 最优点放在Neighbour[0]，如果更优则换出 干完以后还要排序
					if (bSort)
					{
						for (m = poPredictor->m_iNeightbour_Count - 1; m > 0; m--)
						{
							PCC_Neighbour oPre = poPredictor->m_Neighbour[m], oNext = poPredictor->m_Neighbour[m - 1];
							//先权重，后morton轨迹
							if (oPre.m_iWeight <= oNext.m_iWeight)
							{
								poPredictor->m_Neighbour[m] = oNext;
								poPredictor->m_Neighbour[m - 1] = oPre;
							}
							else
								break;	//这里很绝妙，短路退出
						}
					}
				}
				//末了，将自己也加入到Point_Neighbour
				Point_Neighbour[iNeighbour_Point_Count++] = oPos;
			}
		}
	}
	if (ppPredictor)
		*ppPredictor = pPredictor;
	Free(poMem_Mgr, pHash_Table);
	return;
}
void Predict_Color(int predMode, PCC_Predictor* poPredictor, unsigned char Pred_Color[3])
{//预测颜色，结果放在Pred_Color
	unsigned char* pColor;
	int i, w, predicted[3] = { 0,0,0 };
	if (predMode > poPredictor->m_iNeightbour_Count)
		Pred_Color[0] = Pred_Color[1] = Pred_Color[2] = 0;
	else if (predMode > 0)
	{
		//pColor = Point[poPredictor->m_Neighbour[predMode - 1].m_iIndex].m_Color;
		pColor = poPredictor->m_Neighbour[predMode - 1].m_Color;
		for (i = 0; i < 3; i++)
			Pred_Color[i] = pColor[i];
	}
	else
	{
		for (i = 0; i < poPredictor->m_iNeightbour_Count; ++i)
		{
			//pColor = Point[poPredictor->m_Neighbour[i].m_iIndex].m_Color;
			pColor = poPredictor->m_Neighbour[i].m_Color;

			w = poPredictor->m_Neighbour[i].m_iWeight;
			for (size_t k = 0; k < 3; ++k)
				predicted[k] += w * pColor[k];
		}
		for (uint32_t k = 0; k < 3; ++k)
		{
			divExp2RoundHalfInf_1(predicted[k], Pred_Color[k])
		}
	}
}
int iGet_predMode(PCC_Predictor* poPredictor,unsigned int iColor, int qstep[3], int adaptive_prediction_threshold, int max_num_direct_predictors)
{
	PCC_Predictor oPredictor;// = *poPredictor;
	int i, iValue, attrResidualQuant[3], idxBits;
	unsigned char* attrPred, Pred_Color[3];
	double best_score, score;
	PCC_Color oColor;

	oPredictor = *poPredictor;
	computeWeights_1(oPredictor);//此处也许可以写成宏
	if (oPredictor.m_iNeightbour_Count > 1)
	{
		//oColor.m_iColor = Point[oPredictor.m_Neighbour[0].m_iIndex].m_iColor;
		oColor.m_iColor = oPredictor.m_Neighbour[0].m_iColor;

		int minValue[3] = { oColor.m_Color[0], oColor.m_Color[1], oColor.m_Color[2] }, maxValue[3] = { oColor.m_Color[0], oColor.m_Color[1], oColor.m_Color[2] };
		for (i = 1; i < oPredictor.m_iNeightbour_Count; ++i)
		{
			//oColor.m_iColor = Point[oPredictor.m_Neighbour[i].m_iIndex].m_iColor;
			oColor.m_iColor = oPredictor.m_Neighbour[i].m_iColor;

			for (size_t k = 0; k < 3; ++k)
			{
				if (oColor.m_Color[k] < minValue[k])
					minValue[k] = oColor.m_Color[k];
				if (oColor.m_Color[k] > maxValue[k])
					maxValue[k] = oColor.m_Color[k];
			}
		}
		oPredictor.maxDiff = maxValue[0] - minValue[0];
		for (i = 1; i < 3; i++)
		{
			iValue = maxValue[i] - minValue[i];
			if (iValue > oPredictor.maxDiff)
				oPredictor.maxDiff = iValue;
		}
		if (oPredictor.maxDiff > adaptive_prediction_threshold)
		{
			//oColor.m_iColor = Point[iPredictor_Index + 1].m_iColor;
			oColor.m_iColor = iColor;

			//Predict_Color(Point, oPredictor.predMode, &oPredictor, Pred_Color);
			Predict_Color(oPredictor.predMode, &oPredictor, Pred_Color);
			computeColorResiduals_1(oColor.m_Color, Pred_Color, qstep, attrResidualQuant);
			best_score = 0.01 * (double)(qstep[0] >> 8) + attrResidualQuant[0] + attrResidualQuant[1] + attrResidualQuant[2];
			for (int i = 0; i < oPredictor.m_iNeightbour_Count; i++)
			{
				if (i == max_num_direct_predictors)
					break;
				//attrPred = Point[oPredictor.m_Neighbour[i].m_iIndex].m_Color;
				attrPred = oPredictor.m_Neighbour[i].m_Color;

				computeColorResiduals_1(oColor.m_Color, attrPred, qstep, attrResidualQuant);
				idxBits = i + (i == max_num_direct_predictors - 1 ? 1 : 2);
				score = 0.01f * idxBits * (qstep[0] >> 8) + attrResidualQuant[0] + attrResidualQuant[1] + attrResidualQuant[2];
				if (score < best_score)
				{
					best_score = score;
					oPredictor.predMode = i + 1;
				}
			}
		}
	}
	else
		oPredictor.maxDiff = -1;
	*poPredictor = oPredictor;

	return oPredictor.predMode;
}
void Gen_Point(Mem_Mgr* poMem_Mgr, MC* pMC, int iMC_Count,PCC_Point **ppPoint,int iPoint_Count, int iMC_Shift_Bit,int Min[3])
{
	int i, j,iCur_Item=1;
	PCC_Point oPoint,* pPoint = (PCC_Point*)pMalloc(poMem_Mgr, (iPoint_Count + 1) * sizeof(PCC_Point));
	PCC_Pos oPos;
	MC oMC;

	for (i = 1; i <= iMC_Count; i++)
	{
		oMC = pMC[i];
		oPos.Pos[0] = (oMC.Pos_i[0]+Min[0]) << iMC_Shift_Bit;
		oPos.Pos[1] = (oMC.Pos_i[1]+Min[1]) << iMC_Shift_Bit;
		oPos.Pos[2] = (oMC.Pos_i[2]+Min[2]) << iMC_Shift_Bit;
		for (j = 0; j < 3; j++)
		{
			if (oMC.m_iOccupancy & (1 << j))
			{
				oPoint.Pos = oPos;
				oPoint.Pos_i[j] += oMC.m_Point[j].m_iPos;
				oPoint.m_iColor = oMC.m_Point[j].m_iColor_Pos;
				pPoint[iCur_Item++] = oPoint;
			}
		}
	}
	if (ppPoint)
		*ppPoint = pPoint;
	return;
}
void Pre_Process(PCC_Point *Point,int iPoint_Count, PCC_Predictor Predictor[], int iSearch_Range, int iMax_Neighbour_Count)
{
	int i, j, k, iTo, bSort;
	unsigned int iWeight;
	PCC_Predictor oPredictor, * poPredictor;
	PCC_Neighbour* poBest;
	PCC_Pos oPos, oSearch_Pos;
	Point++;

	for (i = 0; i < iPoint_Count; i++)
	{
		iTo = i - iSearch_Range;
		if (iTo < 0)
			iTo = 0;
		poPredictor = &Predictor[i];
		oPredictor.m_iNeightbour_Count = oPredictor.predMode = 0;
		oPos = Point[i].Pos;
		//if (i==4075)
			//printf("Point:%d %d %d %d\n",i, oPos.Pos[0], oPos.Pos[1], oPos.Pos[2]);
		poBest = &oPredictor.m_Neighbour[iMax_Neighbour_Count - 1];
		for (k = i - 1; k >= iTo; k--)
		{
			oSearch_Pos = Point[k].Pos;
			iGet_Distance_1(oPos.Pos, oSearch_Pos.Pos, iWeight);
			bSort = 0;
			if (oPredictor.m_iNeightbour_Count < iMax_Neighbour_Count)
			{
				oPredictor.m_Neighbour[oPredictor.m_iNeightbour_Count].m_iIndex = k + 1;
				oPredictor.m_Neighbour[oPredictor.m_iNeightbour_Count++].m_iWeight = iWeight;
				bSort = 1;
			}
			else
			{//
				if (iWeight < (int)poBest->m_iWeight)
				{
					poBest->m_iWeight = iWeight;
					poBest->m_iIndex = k + 1;
					bSort = 1;
				}
			}
			//根据TMC3, 最优点放在Neighbour[0]，如果更优则换出 干完以后还要排序
			if (bSort)
			{
				oPredictor.m_Neighbour;
				for (j = oPredictor.m_iNeightbour_Count - 1; j > 0; j--)
				{
					PCC_Neighbour oPre = oPredictor.m_Neighbour[j], oNext = oPredictor.m_Neighbour[j - 1];
					//if (oPre.m_iWeight < oNext.m_iWeight || (oPre.m_iWeight == oNext.m_iWeight && oPre.m_iIndex < oNext.m_iIndex))
					if (oPre.m_iWeight <= oNext.m_iWeight)
					{
						//swap(oPredictor.m_Neighbour[j], oPredictor.m_Neighbour[j - 1]);
						oPredictor.m_Neighbour[j] = oNext;
						oPredictor.m_Neighbour[j - 1] = oPre;
					}
					else
						break;	//这里很绝妙，短路退出
				}
			}
		}
		*poPredictor = oPredictor;
	}
	return;
}
void Encode_Color_1(PCC_Encoder* poEncoder, MC* pMC, int iMC_Count,int iPoint_Count, int iMC_Shift_Bit)
{//老方法全搜索
	PCC_Attribute_Brick* poBrick = &poEncoder->m_oAttribute_Brick;
	Mem_Mgr* poMem_Mgr = &poEncoder->m_oMem_Mgr;
	PCC_Point* pPoint;
	PCC_Predictor *poCur_Predictor,*poPredictor = (PCC_Predictor*)pMalloc(poMem_Mgr, (iPoint_Count + 1) * sizeof(PCC_Predictor));
	PCC_APS* poAPS = &poEncoder->m_oAPS;
	int i,k, iCur_Zero_Item,iValue,zero_cnt = 0, qstep[3], iMax_Neighbour_Count = poAPS->num_pred_nearest_neighbours, iZero_Item_Count = 0, predMode;
	int adaptive_prediction_threshold = poAPS->adaptive_prediction_threshold,
		max_num_direct_predictors = poAPS->max_num_direct_predictors;
	unsigned short Value[4] = { 0 };
	BitPtr oBitPtr, oLen_Pos;
	Arithmetic_Encoder oCabac;
	PCCResidualsEncoder* poModal = &poEncoder->m_oResidual_Modal;
	Zero_Count_Item* pZero_Count, * pCur_Zero_Item;
	unsigned char Pred_Color[3], * pColor, reconstructedQuantAttValue[3];
	int quantPredAttValue, delta, reconstructedDelta;

	Init_BitPtr(&oBitPtr, poEncoder->m_pOut_Buffer + poEncoder->m_iOut_Data_Size, poEncoder->m_iOut_Buffer_Size - poEncoder->m_iOut_Data_Size);
	WriteBits(&oBitPtr, 8, PCC_TYPE_APS);
	Write_APS(&oBitPtr, &poEncoder->m_oAPS);
	Write_Brick_Header(&oBitPtr, &oLen_Pos, &poBrick->m_oHeader);
	Init_Arithmetic_Codec(&oCabac, &oBitPtr);
	Init_Residual_Encoder(poModal);

	Gen_Point(poMem_Mgr, pMC, iMC_Count,&pPoint, iPoint_Count,iMC_Shift_Bit,poEncoder->m_oSPS.seq_bounding_box.seq_bounding_box_xyz);
	Pre_Process(pPoint,iPoint_Count, poPredictor, poAPS->search_range, iMax_Neighbour_Count);

	pZero_Count = (Zero_Count_Item*)pMalloc(poMem_Mgr, (iPoint_Count) * sizeof(Zero_Count_Item));
	Get_qstep(qstep, poAPS, &poBrick->m_oHeader);
	iCur_Node = 0;
	pCur_Zero_Item = &pZero_Count[0];

	unsigned long long iDelta_Total = 0;
	for (i = 0; i < iPoint_Count; i++)
	{
		poCur_Predictor = &poPredictor[i];
		pColor = pPoint[i + 1].m_Color;
	
		predMode = iGet_predMode(poCur_Predictor, pPoint, i, qstep, adaptive_prediction_threshold, max_num_direct_predictors);
		Predict_Color(pPoint, predMode, poCur_Predictor, Pred_Color);
		//printf("%d %d %d ", pPoint[i + 1].Pos_i[0], pPoint[i + 1].Pos_i[1], pPoint[i + 1].Pos_i[2]);
		for (k = 0; k < 3; k++)
		{
			quantPredAttValue = Pred_Color[k];
			PCCQuantization_1(pColor[k] - quantPredAttValue, qstep[k], delta);
			PCCInverseQuantization_1(delta, qstep[k], reconstructedDelta);
			iValue = quantPredAttValue + reconstructedDelta;
			reconstructedQuantAttValue[k] = Clip(iValue);
			iDelta_Total += abs(delta);
			//printf("Correct:%d Error:%d ",pColor[k], reconstructedQuantAttValue[k]);
			IntToUInt_1(delta, Value[k]);
		}
		//printf("\r\n");

		iValue = (reconstructedQuantAttValue[0] - pColor[0]) * (reconstructedQuantAttValue[0] - pColor[0]);
		*(unsigned int*)pColor = *(unsigned int*)reconstructedQuantAttValue;
		*(unsigned long long*)poCur_Predictor->m_Residual = *(unsigned long long*)Value;
		if (*(unsigned long long*)Value)
		{
			if (zero_cnt)
			{
				pCur_Zero_Item->m_iStart = i - (pCur_Zero_Item->m_iCount = zero_cnt);
				pCur_Zero_Item++;
				zero_cnt = 0;
			}
		}
		else
			++zero_cnt;
		iCur_Node++;
	}

	if (zero_cnt)
	{
		pCur_Zero_Item->m_iStart = i - (pCur_Zero_Item->m_iCount = zero_cnt);
		pCur_Zero_Item++;
		zero_cnt = 0;
	}
	//此处开始编码
	iZero_Item_Count = (int)(pCur_Zero_Item - pZero_Count);
	iCur_Zero_Item = 0;
	pCur_Zero_Item = &pZero_Count[iCur_Zero_Item++];
	zero_cnt = pCur_Zero_Item->m_iStart ? 0 : pCur_Zero_Item->m_iCount;
	encodeZeroCnt(zero_cnt, iPoint_Count, poModal->ctxZeroCnt, &oCabac);
	iCur_Node = 0;

	for (i = 0; i < iPoint_Count; i++)
	{
		poCur_Predictor = &poPredictor[i];
		if (poCur_Predictor->maxDiff > adaptive_prediction_threshold)
			encodePredMode(poCur_Predictor->predMode, max_num_direct_predictors, poModal->ctxPredMode, &oCabac);
		if (zero_cnt > 0)
		{
			zero_cnt--;
			if (!zero_cnt)
				pCur_Zero_Item = &pZero_Count[iCur_Zero_Item++];
		}
		else
		{
			Encode_Residual(poCur_Predictor->m_Residual, poModal, &oCabac);

			if (i + 1 == pCur_Zero_Item->m_iStart)
				zero_cnt = pCur_Zero_Item->m_iCount;
			if (i != iPoint_Count - 1)
				encodeZeroCnt_1(zero_cnt, iPoint_Count, poModal->ctxZeroCnt, &oCabac);
		}
		iCur_Node++;
	}

	Flush_Cabac(&oCabac);
	oBitPtr.m_iCur += oCabac.offset;
	Write_Len(&oLen_Pos, oBitPtr.m_iCur - oLen_Pos.m_iCur - 4);//回填Brick 长度,不算长度所占4字节，故此-4
	poEncoder->m_iOut_Data_Size += oBitPtr.m_iCur;

	Free(poMem_Mgr, poPredictor);
	Free(poMem_Mgr, pZero_Count);
	poEncoder->m_oGeometry_Brick.m_pOrder_Point = pPoint;
	//Disp_Mem(poMem_Mgr);
	return;
}
void Encode_Color(PCC_Encoder *poEncoder, MC* pMC, int iMC_Count, int iPoint_Count,int iMC_Shift_Bit)
{//此处应参考TMC3的颜色编码，先实现三点最近邻寻找。未遂，暂时没工夫去对比数据。
	PCC_Predictor* poCur_Predictor,* pPredictor = NULL;
	Mem_Mgr* poMem_Mgr = &poEncoder->m_oMem_Mgr;
	BitPtr oLen_Pos, oBitPtr;
	PCC_APS* poAPS = &poEncoder->m_oAPS;
	int adaptive_prediction_threshold = poAPS->adaptive_prediction_threshold,
		max_num_direct_predictors = poAPS->max_num_direct_predictors;
	Arithmetic_Encoder* poCabac = &poEncoder->m_oCabac;
	PCC_Attribute_Brick_Header *poHeader= &poEncoder->m_oAttribute_Brick.m_oHeader;
	PCCResidualsEncoder* poModal= &poEncoder->m_oResidual_Modal;
	Zero_Count_Item* pZero_Count, * pCur_Zero_Item;
	int i, j, k,l, iZero_Item_Count, iCur_Zero_Item, zero_cnt = 0, iValue, qstep[3], predMode;
	unsigned char Pred_Color[3],*pColor, reconstructedQuantAttValue[3];
	int quantPredAttValue, delta, reconstructedDelta;
	unsigned short Value[4] = { 0 };

	MC oMC;
	PCC_Point oPoint;

	Init_BitPtr(&oBitPtr, poEncoder->m_pOut_Buffer + poEncoder->m_iOut_Data_Size, poEncoder->m_iOut_Buffer_Size - poEncoder->m_iOut_Data_Size);
	Write_Brick_Header(&oBitPtr, &oLen_Pos, poHeader);
	Init_Arithmetic_Codec(poCabac, &oBitPtr);
	Init_Residual_Encoder(poModal);

	unsigned int tStart = GetTickCount();
	Pre_Process(poMem_Mgr, pMC, iMC_Count, &pPredictor,iPoint_Count,6,iMC_Shift_Bit);
	printf("Pre Process:%d\n", GetTickCount() - tStart);

	pZero_Count = (Zero_Count_Item*)pMalloc(poMem_Mgr, (iPoint_Count) * sizeof(Zero_Count_Item));
	Get_qstep(qstep, poAPS, poHeader);
	iCur_Node = 0;
	pCur_Zero_Item = &pZero_Count[0];

	unsigned long long iDelta_Total = 0;
	for (poCur_Predictor=&pPredictor[0],l=0, i = 1; i <=iMC_Count; i++)
	{
		oMC = pMC[i];
		oMC.Pos_i[0] <<= iMC_Shift_Bit;
		oMC.Pos_i[1] <<= iMC_Shift_Bit;
		oMC.Pos_i[2] <<= iMC_Shift_Bit;
		for (j = 0; j < 3; j++)
		{
			//if (i == 78 && j == 0)
				//printf("Here");

			if (oMC.m_iOccupancy & (1 << j))
			{
				oPoint.Pos = oMC.m_oPos;
				oPoint.Pos_i[j] += oMC.m_Point[j].m_iPos;
				oPoint.m_iColor = oMC.m_Point[j].m_iColor_Pos;

				//此处可以算一把
				pColor = oPoint.m_Color;
				predMode = iGet_predMode(poCur_Predictor, oPoint.m_iColor, qstep, adaptive_prediction_threshold, max_num_direct_predictors);
				Predict_Color(predMode, poCur_Predictor, Pred_Color);

				for (k = 0; k < 3; k++)
				{
					quantPredAttValue = Pred_Color[k];
					PCCQuantization_1(pColor[k] - quantPredAttValue, qstep[k], delta);
					PCCInverseQuantization_1(delta, qstep[k], reconstructedDelta);
					iValue = quantPredAttValue + reconstructedDelta;
					reconstructedQuantAttValue[k] = Clip(iValue);
					IntToUInt_1(delta, Value[k]);
				}
				iValue = (reconstructedQuantAttValue[0] - pColor[0]) * (reconstructedQuantAttValue[0] - pColor[0]);
				*(unsigned int*)pColor = *(unsigned int*)reconstructedQuantAttValue;
				*(unsigned long long*)poCur_Predictor->m_Residual = *(unsigned long long*)Value;
				if (*(unsigned long long*)Value)
				{
					if (zero_cnt)
					{
						pCur_Zero_Item->m_iStart = l - (pCur_Zero_Item->m_iCount = zero_cnt);
						//if(l<100)
						//printf("Zero Start:%d Count:%d\n", pCur_Zero_Item->m_iStart, pCur_Zero_Item->m_iCount);
						pCur_Zero_Item++;
						zero_cnt = 0;
					}
				}
				else
					++zero_cnt;
				poCur_Predictor++;
				l++;
			}
		}
	}//Zero_Count算完

	if (zero_cnt)
	{
		pCur_Zero_Item->m_iStart = i - (pCur_Zero_Item->m_iCount = zero_cnt);
		pCur_Zero_Item++;
		zero_cnt = 0;
	}

	//此处开始编码
	iZero_Item_Count = (int)(pCur_Zero_Item - pZero_Count);
	iCur_Zero_Item = 0;
	pCur_Zero_Item = &pZero_Count[iCur_Zero_Item++];
	zero_cnt = pCur_Zero_Item->m_iStart ? 0 : pCur_Zero_Item->m_iCount;
	encodeZeroCnt(zero_cnt, iPoint_Count, poModal->ctxZeroCnt, poCabac);

	//int iMax = 0;
	for (poCur_Predictor = &pPredictor[0],k=0, i = 1; i <= iMC_Count; i++)
	{
		oMC = pMC[i];
		oMC.Pos_i[0] <<= iMC_Shift_Bit;
		oMC.Pos_i[1] <<= iMC_Shift_Bit;
		oMC.Pos_i[2] <<= iMC_Shift_Bit;
		for (j = 0; j < 3; j++)
		{
			//if (i == 35 && j == 0)
				//printf("Here");
			if (oMC.m_iOccupancy & (1 << j))
			{
				if (poCur_Predictor->maxDiff > adaptive_prediction_threshold)
					encodePredMode(poCur_Predictor->predMode, max_num_direct_predictors, poModal->ctxPredMode, poCabac);
				if (zero_cnt > 0)
				{
					zero_cnt--;
					if (!zero_cnt)
						pCur_Zero_Item = &pZero_Count[iCur_Zero_Item++];
				}else
				{
					Encode_Residual(poCur_Predictor->m_Residual, poModal, poCabac);
					//if (poCur_Predictor->m_Residual[0] >= 65533)
						//printf("i:%d j:%d %d %d %d\n",i,j, poCur_Predictor->m_Residual[0], poCur_Predictor->m_Residual[1], poCur_Predictor->m_Residual[2]);
					//if (poCur_Predictor->m_Residual[0]==508 /* iMax*/)
					//	iMax = poCur_Predictor->m_Residual[0];
					
					if (k + 1 == pCur_Zero_Item->m_iStart)
						zero_cnt = pCur_Zero_Item->m_iCount;
					if (k != iPoint_Count - 1)
						encodeZeroCnt_1(zero_cnt, iPoint_Count, poModal->ctxZeroCnt, poCabac);
				}
				poCur_Predictor++;
				k++;
			}
		}
	}

	Flush_Cabac(poCabac);
	oBitPtr.m_iCur += poCabac->offset;
	Write_Len(&oLen_Pos, oBitPtr.m_iCur - oLen_Pos.m_iCur - 4);//回填Brick 长度,不算长度所占4字节，故此-4

	return;
}
void MCPC_Encode(PCC_Encoder* poEncoder, unsigned char* pBuffer, int iPoint_Count, float fMC_Size,float fScale,int *piPoint_Count)
{//简化接口，此处直接对Buffer进行处理
	MC* pMC;
	PCC_Point* pPoint;
	int iMC_Count;
	Mem_Mgr* poMem_Mgr = &poEncoder->m_oMem_Mgr;
	
	bGen_MC(poMem_Mgr, pBuffer, iPoint_Count, fMC_Size, fScale, &pMC, &iMC_Count, poEncoder->m_oAPS.colorTransform);
	iPoint_Count = iMC_Count;
	pPoint = (PCC_Point*)pMalloc(&poEncoder->m_oMem_Mgr,(iPoint_Count+1) * sizeof(PCC_Point));
	//此处已经完成了Max,Min的调整，所有的MC已经落入到一个规范的Bounding Box
	Reset_Point(poEncoder, pPoint, pMC, iMC_Count);
	
	Encode(poEncoder);
	Resort_MC(poMem_Mgr, poEncoder->m_oGeometry_Brick.m_pOrder_Point, poEncoder->m_oGeometry_Brick.m_oHeader.geom_num_points, &pMC, &iMC_Count);
	
	Free(poMem_Mgr, poEncoder->m_oGeometry_Brick.m_pPoint);
	Free(poMem_Mgr, poEncoder->m_oGeometry_Brick.m_pOrder_Point);
	
	//此处开始干MC内的点
	Encode_MC(poEncoder, pMC, iMC_Count,&iPoint_Count);

	//应该在此处做Attribute Coding	
	Encode_Color_1(poEncoder, pMC, iMC_Count,iPoint_Count,8);
	
	if (piPoint_Count)
		*piPoint_Count = iPoint_Count;
	return;
}