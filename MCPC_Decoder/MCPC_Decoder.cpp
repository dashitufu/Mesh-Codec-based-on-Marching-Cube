// MCPC_Decoder.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "MCPC_Decoder.h"

void Init_BitPtr(BitPtr* poBitPtr, unsigned char* pBuffer, int iSize)
{
	poBitPtr->m_iBitPtr = poBitPtr->m_iCur = 0;
	poBitPtr->m_iEnd = iSize;
	poBitPtr->m_pBuffer = pBuffer;
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
void Decode_MC(PCC_Decoder *poDecoder,unsigned char *pBuffer,int iBuffer_Size,PCC_Point Point[],int iPoint_Count,MC **ppMC,int *piMC_Count,int *piPoint_Count,BitPtr *poBitPtr)
{//在这解码MC里三条边的occupancy，若有occupancy，解最高位
	int i,j,iOccupancy,iMC_Count=iPoint_Count,iLen,iCur_Item=1;
	MC* pMC, oMC = { 0 },*Found[3];
	PCC_Pos oPos;
	Mem_Mgr* poMem_Mgr = &poDecoder->m_oMem_Mgr;
	BitPtr oBitPtr;

	Free(poMem_Mgr, poDecoder->m_pOut_Buffer);
	pMC = (MC*)pMalloc_At_End(poMem_Mgr, (iMC_Count + 1) * sizeof(MC));
	
	//初始化散列表
	int* pHash_Table, iHash_Pos,iHash_Size= iMC_Count;
	pHash_Table = (int*)pMalloc(poMem_Mgr, iHash_Size * sizeof(int));
	memset(pHash_Table, 0, iHash_Size * sizeof(int));

	Init_BitPtr(&oBitPtr, pBuffer + poDecoder->m_iDecode_Cur,iBuffer_Size- poDecoder->m_iDecode_Cur);
	iLen = iRead_Len(&oBitPtr);
	//以下这句影响到oCabac的m_iLength
	oBitPtr.m_iEnd = oBitPtr.m_iCur + iLen;

	Arithmetic_Decoder oCabac;
	AdaptiveBitModel* poModal, MC_Occupancy_Modal[8][8][8][3];
	AdaptiveBitModel MC_Edge_Pos_High[3][3][3];	//[2]时表示没有邻居

	Init_Arithmetic_Decoder(&oCabac, &oBitPtr);
	//FILE* pFile = fopen("c:\\tmp\\decode.txt", "wb");
	for (i = 1; i <= iMC_Count; i++)
	{
		oMC.m_oPos = Point[i].m_oPos;
		//先找到三个邻居用于预测
		for (j = 0; j < 3; j++)
		{
			oPos = oMC.m_oPos;
			oPos.Pos[j]--;
			Found[j] = poFind_Item(pMC, oPos, pHash_Table, iHash_Size);
		}
		iOccupancy = 0;
		//if (i == iMC_Count )
			//printf("Here");
		for (j = 0; j < 3; j++)
		{
			poModal = &MC_Occupancy_Modal[Found[0] ? Found[0]->m_iOccupancy : 0][Found[1] ? Found[1]->m_iOccupancy : 0][Found[2] ? Found[2]->m_iOccupancy : 0][j];
			//fprintf(pFile,"modal:%d ", (int)(poModal - (AdaptiveBitModel*)MC_Occupancy_Modal));
			//Decode Bit
			int iBit= iDecode(poModal, &oCabac);
			//fprintf(pFile, "Bit:%d ", iBit);
			if (iBit)
			{//边上有点
				iOccupancy |= (1 << j);
				//解开最高位
				//poModal = &MC_Edge_Pos_High[Found[0] ? Found[0]->m_Point[0].m_iPos: 0][Found[1] ? Found[1]->m_Point[1].m_iPos: 0][Found[2] ? Found[2]->m_Point[2].m_iPos : 0];
				int Index[3], k;
				for (k = 0; k < 3; k++)
				{
					if (!Found[k] || k == j || !(Found[k]->m_iOccupancy & (1 << j)))
						Index[k] = 2;
					else
						Index[k] = Found[k]->m_Point[j].m_iPos;
				}
				poModal = &MC_Edge_Pos_High[Index[0]][Index[1]][Index[2]];

				oMC.m_Point[j].m_iPos = iDecode(poModal, &oCabac);
				//fprintf(pFile,"Hbit modal:%d Bit:%d ", (int)(poModal - (AdaptiveBitModel*)MC_Edge_Pos_High), oMC.m_Point[j].m_iPos);
			}
			
		}
		//fprintf(pFile,"\r\n");

		oMC.m_iOccupancy = iOccupancy;

		//将本MC加入散列表
		iHash_Pos = (unsigned int)((oMC.Pos_i[0] ^ (oMC.Pos_i[1] << 8) ^ (oMC.Pos_i[2] << 16))) % iHash_Size;
		oMC.m_iNext = pHash_Table[iHash_Pos];
		pHash_Table[iHash_Pos] = i;
		pMC[i] = oMC;
	}
	//fclose(pFile);
	Free(poMem_Mgr, pHash_Table);

	oBitPtr.m_iCur += iLen-4;
	oBitPtr.m_iEnd = iBuffer_Size - poDecoder->m_iDecode_Cur;
	//然后从这里开始装入残差
	for (iPoint_Count=0,i = 1; i <= iMC_Count; i++)
	{
		oMC = pMC[i];
		for (j = 0; j < 3; j++)
		{
			if (oMC.m_iOccupancy & (1 << j))
			{//有东西，读入
				oMC.m_Point[j].m_iPos = (oMC.m_Point[j].m_iPos << 7) + iGetBits1(&oBitPtr, 7);
				iPoint_Count++;
			}
		}
		pMC[i] = oMC;
	}
	if (oBitPtr.m_iBitPtr)
		oBitPtr.m_iCur++;
	oBitPtr.m_iBitPtr = 0;

	if (ppMC)
		*ppMC = pMC;
	if (piMC_Count)
		*piMC_Count = iMC_Count;
	if (piPoint_Count)
		*piPoint_Count = iPoint_Count;
	if (poBitPtr)
		*poBitPtr = oBitPtr;

	return;
}
void Write_PLY_Header(FILE* pFile, int iPoint_Count, int bHas_Color)
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
int bSave(const char* pcFile, unsigned char* pBuffer, int iPoint_Count,int bText,int bColor)
{//进来是裸数据，存盘成ply
	FILE* pFile = fopen(pcFile, "wb");
	int i,j;
	unsigned char* pCur = pBuffer;
	if (!pFile)
	{
		printf("Fail to save:%s\n", pcFile);
		return 0;
	}
	Write_PLY_Header(pFile, iPoint_Count, bColor);
	for (i = 0; i < iPoint_Count; i++)
	{
		for (j = 0; j < 3; j++,pCur+=4)
			fprintf(pFile, "%d ", *(int*)pCur);
		if (bColor)
		{
			fprintf(pFile, "%d %d %d \r\n", pCur[0], pCur[1], pCur[2]);
			pCur += 3;
		}else
			fprintf(pFile, "\r\n");
		
		
	}
	
	fclose(pFile);
	return 1;
}
int bSave(const char* pcFile, PCC_Point Point[], int iPoint_Count, int bColor)
{
	FILE* pFile = fopen(pcFile, "wb");
	int i;
	PCC_Point oPoint;

	if (!pFile)
		return 0;
	Write_PLY_Header(pFile, iPoint_Count, bColor);
	for (i = 1; i <= iPoint_Count; i++)
	{
		oPoint = Point[i];
		fprintf(pFile, "%d %d %d", oPoint.Pos[0], oPoint.Pos[1], oPoint.Pos[2] );
		if (bColor)
			fprintf(pFile, " %d %d %d \r\n", oPoint.m_Color[0], oPoint.m_Color[1], oPoint.m_Color[2]);
		else
			fprintf(pFile, "\r\n");
	}
	fclose(pFile);

	return 1;
}
int bSave(const char* pcFile, MC* pMC, int iMC_Count, float fScale,int bColor)
{//将MC的位置xfScale,然后再
	FILE* pFile = fopen(pcFile, "wb");
	int i, j, k, iValue, iPoint_Count = 0;
	MC oMC;
	for (i = 1; i <= iMC_Count; i++)
		iPoint_Count += Bit_1_Count[pMC[i].m_iOccupancy];

	Write_PLY_Header(pFile, iPoint_Count, bColor);
	for (i = 1; i <= iMC_Count; i++)
	{
		oMC = pMC[i];
		for (j = 0; j < 3; j++)
		{
			if (oMC.m_iOccupancy & (1 << j))
			{//有东西，存起来
				/*if (i == iMC_Count)
					printf("Here");*/
				for (k = 0; k < 3; k++)
				{
					iValue = (int)(oMC.Pos_i[k] * fScale + 0.5);
					if (k == j)
						iValue += oMC.m_Point[j].m_iPos;
					if(k==2)
						fprintf(pFile, "%d", iValue);
					else
						fprintf(pFile, "%d ", iValue);
				}
				if (bColor)
				{
					for (k = 0; k < 3; k++)
						fprintf(pFile, " %d ", oMC.m_Point[j].m_Color[k]);
				}
				fprintf(pFile, "\r\n");
			}
		}
	}
	fclose(pFile);
	return 1;
}
void Gen_Data(PCC_Point Point[], int iPoint_Count, unsigned char* pOut_Data,int Centroid[3])
{
	int i;
	unsigned char* pCur = pOut_Data;
	long long Total[3] = { 0,0,0 };
	PCC_Point* poPoint = &Point[1];
	for (i = 1; i <= iPoint_Count; i++)
	{
		Total[0] += poPoint->Pos[0];
		Total[1] += poPoint->Pos[1];
		Total[2] += poPoint->Pos[2];
		*(PCC_Point*)pCur = *poPoint++;
		pCur += 15;
	}
	Centroid[0] = (int)(((float)Total[0] / iPoint_Count) + 0.5f);
	Centroid[1] = (int)(((float)Total[1] / iPoint_Count) + 0.5f);
	Centroid[2] = (int)(((float)Total[2] / iPoint_Count) + 0.5f);
	return;
}
void Gen_Data(MC *pMC,int iMC_Count,unsigned char *pOut_Data,int *piPoint_Count)
{
	int i,j,iPoint_Count;
	MC oMC;
	PCC_Pos oPos;
	unsigned char* pCur = pOut_Data;
	
	for (iPoint_Count=0,i = 1; i <= iMC_Count; i++)
	{
		oMC = pMC[i];
		oMC.Pos_i[0] <<= 8;
		oMC.Pos_i[1] <<= 8;
		oMC.Pos_i[2] <<= 8;
		
		for (j = 0; j < 3; j++)
		{
			if (oMC.m_iOccupancy & (1 << j))
			{//有东西，存起来
				oPos = oMC.m_oPos;
				oPos.Pos[j] += oMC.m_Point[j].m_iPos;
				*(PCC_Pos*)pCur = oPos;
				pCur += 12;
				*(unsigned int*)pCur =oMC.m_Point[j].m_iColor_Pos;
				pCur += 3;
				iPoint_Count++;
			}
		}
	}
	if (piPoint_Count)
		*piPoint_Count = iPoint_Count;
	return;
}
void Pre_Process(Mem_Mgr* poMem_Mgr, MC* pMC, int iMC_Count, PCC_Predictor** ppPredictor, int iPoint_Count, int iMax_Neighbour_MC_Count, int iMC_Shift_Bit)
{//对每一个MC寻找邻近MC
	PCC_Point oPoint, Point_Neighbour[12 * 3];	//从周围26个邻近MC中选择临近点
	PCC_Predictor* poPredictor, * pPredictor = (PCC_Predictor*)pMalloc(poMem_Mgr, (iPoint_Count + 1) * sizeof(PCC_Predictor));
	int i, j, k, l, bSort, iCur_Point = 0, iNeighbour_MC_Count, iNeighbour_Point_Count;
	unsigned int iWeight;
	MC oMC, oItem_Exist;//最高取到周围26个邻近MC
	PCC_Pos oPos;
	static int Delta[][3] = { {-1,0,0},{0,-1,0},{0,0,-1},
		{ 1,0,-1 },{ 1,-1,0 },{ 0,1,-1 },{ 0,-1,1 },{ 0, -1, -1 },{ -1,1,0 },{ -1,0,1 },{ -1,0,-1 },{ -1,-1,0 } };
	int* pDelta;
	int iHash_Size = iMC_Count, * pHash_Table, iHash_Pos, iPos;
	pHash_Table = (int*)pMalloc(poMem_Mgr, iHash_Size * sizeof(int));
	memset(pHash_Table, 0, iHash_Size * sizeof(int));

	for (l = 0, i = 1; i <= iMC_Count; i++)
	{
		oMC = pMC[i];
		iNeighbour_MC_Count = iNeighbour_Point_Count = 0;
		for (j = 0; j < 12 && iNeighbour_MC_Count <= iMax_Neighbour_MC_Count; j++)
		{//头三种模式都是来自相邻面，根本不必排序，能填满就算胜利
			oPos = oMC.m_oPos;
			pDelta = Delta[j];
			oPos.Pos[0] += pDelta[0];
			oPos.Pos[1] += pDelta[1];
			oPos.Pos[2] += pDelta[2];
			iHash_Pos = (unsigned int)((oPos.Pos[0] ^ (oPos.Pos[1] << 8) ^ (oPos.Pos[2] << 16))) % iHash_Size;
			iPos = pHash_Table[iHash_Pos];
			if (iPos)
			{
				do {
					oItem_Exist = pMC[iPos];
					if (oItem_Exist.Pos_i[0] == oPos.Pos[0] && oItem_Exist.Pos_i[1] == oPos.Pos[1] && oItem_Exist.Pos_i[2] == oPos.Pos[2])
					{//找到
						//MC_Neighbour[iNeighbour_MC_Count++] = oItem_Exist;
						oItem_Exist.Pos_i[0] <<= iMC_Shift_Bit;
						oItem_Exist.Pos_i[1] <<= iMC_Shift_Bit;
						oItem_Exist.Pos_i[2] <<= iMC_Shift_Bit;
						for (k = 0; k < 3; k++, oItem_Exist.m_iOccupancy >>= 1)
						{
							if (oItem_Exist.m_iOccupancy & 1)
							{//x轴上有东西
								oPoint.m_oPos = oItem_Exist.m_oPos;
								oPoint.Pos[k] += oItem_Exist.m_Point[k].m_iPos;
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
		for (j = 0; j < 3; j++, oMC.m_iOccupancy >>= 1)
		{
			if (oMC.m_iOccupancy & 1)
			{//该轴有点
				poPredictor = &pPredictor[l++];
				poPredictor->m_iNeighbour_Count = 0;
				oPos = oMC.m_oPos;
				oPos.Pos[j] += oMC.m_Point[j].m_iPos;
				for (k = 0; k < iNeighbour_Point_Count; k++)
				{
					oPoint = Point_Neighbour[k];
					iGet_Distance_1(oPos.Pos, oPoint.Pos, iWeight);
					bSort = 0;
					if (poPredictor->m_iNeighbour_Count < 3)
					{
						poPredictor->m_Neighbour[poPredictor->m_iNeighbour_Count++] = { (int)oPoint.m_iColor,(unsigned int)iWeight };
						bSort = 1;
					}
					else if (iWeight < poPredictor->m_Neighbour[3 - 1].m_iWeight)
					{
						poPredictor->m_Neighbour[3 - 1] = { k,(unsigned int)iWeight };
						bSort = 1;
					}
					//根据TMC3, 最优点放在Neighbour[0]，如果更优则换出 干完以后还要排序
					if (bSort)
					{
						for (j = poPredictor->m_iNeighbour_Count - 1; j > 0; j--)
						{
							PCC_Neighbour oPre = poPredictor->m_Neighbour[j], oNext = poPredictor->m_Neighbour[j - 1];
							//先权重，后morton轨迹
							if (oPre.m_iWeight <= oNext.m_iWeight)
							{
								poPredictor->m_Neighbour[j] = oNext;
								poPredictor->m_Neighbour[j - 1] = oPre;
							}
							else
								break;	//这里很绝妙，短路退出
						}
					}
				}
			}
		}
	}
	if (ppPredictor)
		*ppPredictor = pPredictor;
	Free(poMem_Mgr, pHash_Table);
	return;
}
//int iGet_predMode(PCC_Predictor* poPredictor,Arithmetic_Decoder* poCabac, int adaptive_prediction_threshold,
//	int max_num_direct_predictors, AdaptiveBitModel CtxPredMode[])
//{//在取没一点之前，会有个predMode，语义未明
//	PCC_Predictor oPredictor = *poPredictor;
//	unsigned char* pColor;
//	int i, iMax_Diff, iValue;
//
//	//if (iCur_Node == 1)
//		//printf("Here");
//	//在此重算weight，原来的weight是距离，现在是真正的权值
//	computeWeights_1(oPredictor);//此处也许可以写成宏
//	if (oPredictor.m_iNeighbour_Count > 1)
//	{
//		int minValue[3] = { 0xFFF, 0xFFF, 0xFFF }, maxValue[3] = { -1, -1, -1 };
//		for (i = 0; i < (int)oPredictor.m_iNeighbour_Count; ++i)
//		{
//			//pColor = Point[oPredictor.m_Neighbour[i].m_iIndex].m_Color;
//			pColor = oPredictor.m_Neighbour[i].m_Color;
//			for (size_t k = 0; k < 3; ++k)
//			{
//				if (pColor[k] < minValue[k])
//					minValue[k] = pColor[k];
//				if (pColor[k] > maxValue[k])
//					maxValue[k] = pColor[k];
//			}
//		}
//		for (iMax_Diff = -1, i = 0; i < 3; i++)
//		{
//			iValue = maxValue[i] - minValue[i];
//			if (iValue > iMax_Diff)
//				iMax_Diff = iValue;
//		}
//		*poPredictor = oPredictor;
//		if (iMax_Diff > adaptive_prediction_threshold)
//			return decodePredMode(max_num_direct_predictors, poCabac, CtxPredMode);
//	}
//	*poPredictor = oPredictor;
//	return 0;
//}
void Gen_Point(Mem_Mgr* poMem_Mgr, MC* pMC, int iMC_Count, PCC_Point** ppPoint, int iPoint_Count, int iMC_Shift_Bit,int Min[3])
{
	int i, j, iCur_Item = 1;
	PCC_Point oPoint, * pPoint = (PCC_Point*)pMalloc(poMem_Mgr, (iPoint_Count + 1) * sizeof(PCC_Point));
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
				oPoint.m_oPos = oPos;
				oPoint.Pos[j] += oMC.m_Point[j].m_iPos;
				oPoint.m_iColor = oMC.m_Point[j].m_iColor_Pos;
				pPoint[iCur_Item++] = oPoint;
			}
		}
	}
	if (ppPoint)
		*ppPoint = pPoint;
	return;
}
void Pre_Process(Mem_Mgr* poMem_Mgr,PCC_Point *pPoint, int iPoint_Count,PCC_Predictor Predictor[], int iSearch_Range, int iMax_Neighbour_Count)
{
	int i, j, k, iTo, bSort;
	unsigned int iWeight;
	PCC_Predictor oPredictor, * poPredictor;
	PCC_Neighbour* poBest;
	PCC_Pos oPos, oSearch_Pos;
	pPoint++;

	for (i = 0; i < iPoint_Count; i++)
	{
		iTo = i - iSearch_Range;
		if (iTo < 0)
			iTo = 0;
		poPredictor = &Predictor[i];
		oPredictor.m_iNeighbour_Count = 0;
		oPos = pPoint[i].m_oPos;
		poBest = &oPredictor.m_Neighbour[iMax_Neighbour_Count - 1];
		for (k = i - 1; k >= iTo; k--)
		{
			oSearch_Pos = pPoint[k].m_oPos;
			iGet_Distance_1(oPos.Pos, oSearch_Pos.Pos, iWeight);
			bSort = 0;
			if (oPredictor.m_iNeighbour_Count < iMax_Neighbour_Count)
			{
				oPredictor.m_Neighbour[oPredictor.m_iNeighbour_Count].m_iIndex = k + 1;
				oPredictor.m_Neighbour[oPredictor.m_iNeighbour_Count++].m_iWeight = iWeight;
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
				for (j = oPredictor.m_iNeighbour_Count - 1; j > 0; j--)
				{
					PCC_Neighbour oPre = oPredictor.m_Neighbour[j], oNext = oPredictor.m_Neighbour[j - 1];
					if (oPre.m_iWeight <= oNext.m_iWeight)
					{
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
int bDecode_Color_1(BitPtr* poBitPtr, PCC_Decoder* poDecoder, MC* pMC, int iMC_Count, int iPoint_Count)
{
	PCC_Attribute_Brick* poBrick = &poDecoder->m_oAttribute_Brick;
	PCC_APS* poAPS = &poDecoder->m_oAPS;
	PCC_SPS* poSPS = &poDecoder->m_oSPS;
	int i, iLast_Point, zero_cnt, iValue, Value[3], Pred_Color[3], qstep[3], predMode;
	int iMax_Neighbour_Count = poAPS->num_pred_nearest_neighbours;
	int adaptive_prediction_threshold = poAPS->adaptive_prediction_threshold,
		max_num_direct_predictors = poAPS->max_num_direct_predictors;

	Arithmetic_Decoder* poCabac = &poDecoder->m_oCabac;
	PCCResidualsDecoder* poModal = &poDecoder->m_oResidual_Modal;
	Mem_Mgr* poMem_Mgr = &poDecoder->m_oMem_Mgr;
	PCC_Predictor* poCur_Predictor, * poPredictor;	// = (PCC_Predictor*)pMalloc(poMem_Mgr, (iPoint_Count + 1) * sizeof(PCC_Predictor));
	PCC_Point* pPoint,* poCur_Point;
	unsigned int Bounding_Box_xyz[4] = { (unsigned int)poSPS->seq_bounding_box.seq_bounding_box_xyz[0],(unsigned int)poSPS->seq_bounding_box.seq_bounding_box_xyz[1],(unsigned int)poSPS->seq_bounding_box.seq_bounding_box_xyz[2],0 };
	PCC_Color oColor;

	Init_Arithmetic_Codec(poCabac, poBitPtr, poBrick->m_iLen, poBrick->m_oHeader.m_iLen);
	Init_PCCResidualsDecoder(poModal);

	Gen_Point(poMem_Mgr, pMC, iMC_Count, &pPoint, iPoint_Count, 8, poDecoder->m_oSPS.seq_bounding_box.seq_bounding_box_xyz);
	//后面再也不用MC了，此处可以释放了事
	Free(poMem_Mgr, pMC);

	poPredictor = (PCC_Predictor*)pMalloc(poMem_Mgr, (iPoint_Count + 1) * sizeof(PCC_Predictor));
	//Disp_Mem(poMem_Mgr);
	Pre_Process(poMem_Mgr,pPoint,iPoint_Count, poPredictor, poAPS->search_range, iMax_Neighbour_Count);

	Get_qstep(qstep, poAPS, &poBrick->m_oHeader);
	iGet_predMode(poPredictor, pPoint, poCabac, poAPS->adaptive_prediction_threshold, poAPS->max_num_direct_predictors, poModal->ctxPredMode);
	zero_cnt = iGet_zero_cnt(poCabac, poModal->ctxZeroCnt, iPoint_Count);
	poCur_Point = pPoint + 1;
	unsigned char* pColor;
	iLast_Point = iPoint_Count - 1;

	for (i = 0; i < iPoint_Count; i++, poCur_Point++)
	{
		//if (i == 1)
			//printf("Here");

		poCur_Predictor = &poPredictor[i];
		predMode = iGet_predMode_1(poCur_Predictor, pPoint, poCabac, adaptive_prediction_threshold, max_num_direct_predictors, poModal->ctxPredMode);
		if (zero_cnt)
		{
			Value[0] = Value[1] = Value[2] = 0;
			zero_cnt--;
		}
		else
		{
			Decode(poModal, poCabac, Value);
			if (i < iLast_Point)
				zero_cnt = iGet_zero_cnt(poCabac, poModal->ctxZeroCnt, iPoint_Count);
		}
		Predict_Color(pPoint, predMode, poCur_Predictor, Pred_Color);
		iInverseQuantization_1(Value[0], qstep[0], Pred_Color[0], iValue);
		pColor = poCur_Point->m_Color;
		pColor[0] = Clip(iValue);

		iInverseQuantization_1(Value[1], qstep[1], Pred_Color[1], iValue);
		pColor[1] = Clip(iValue);
		iInverseQuantization_1(Value[2], qstep[2], Pred_Color[2], iValue);
		pColor[2] = Clip(iValue);
#if _DEBUG
		iCur_Node++;
#endif

	}

	if (poAPS->colorTransform)
	{
		for (poCur_Point = pPoint + 1, i = 1; i <= iPoint_Count; i++, poCur_Point++)
		{
			//poCur_Point->Pos[0] += Bounding_Box_xyz[0];
			//poCur_Point->Pos[1] += Bounding_Box_xyz[1];
			//poCur_Point->Pos[2] += Bounding_Box_xyz[2];
			oColor.m_iColor = poCur_Point->m_oColor.m_iColor;
			poCur_Point->m_oColor.m_iColor = *(unsigned int*)YUV_2_RGB[oColor.m_Color[0]][oColor.m_Color[1]][oColor.m_Color[2]];
		}
	}else
	{
		/*for (poCur_Point = pPoint + 1, i = 1; i <= iPoint_Count; i++, poCur_Point++)
		{
			poCur_Point->Pos[0] += Bounding_Box_xyz[0];
			poCur_Point->Pos[1] += Bounding_Box_xyz[1];
			poCur_Point->Pos[2] += Bounding_Box_xyz[2];
		}*/
	}

	poDecoder->m_oGeometry_Brick.m_pPoint = pPoint;
	//poDecoder->m_iOut_Data_Size = (int)(poCur_Point - pPoint) * 9;
	Free(&poDecoder->m_oMem_Mgr, poPredictor);
	
	//Disp_Mem(poMem_Mgr);
	return 1;
}
//int bDecode_Color(BitPtr* poBitPtr, PCC_Decoder* poDecoder,MC *pMC,int iMC_Count,int iPoint_Count)
//{
//	PCC_Attribute_Brick* poBrick = &poDecoder->m_oAttribute_Brick;
//	PCC_APS* poAPS = &poDecoder->m_oAPS;
//	int i, iLast_Point, zero_cnt, iValue, Value[3], Pred_Color[3], qstep[3], predMode;
//	PCC_Predictor* pPredictor;
//	Arithmetic_Decoder* poCabac = &poDecoder->m_oCabac;
//	PCCResidualsDecoder* poModal = &poDecoder->m_oResidual_Modal;
//	Mem_Mgr* poMem_Mgr = &poDecoder->m_oMem_Mgr;
//	PCC_Point* pPoint;
//	Disp_Mem(poMem_Mgr);
//
//	Gen_Point(poMem_Mgr, pMC, iMC_Count, &pPoint, iPoint_Count,8);
//	Pre_Process(&poDecoder->m_oMem_Mgr, pMC, iMC_Count, &pPredictor, iPoint_Count, 6, 8);
//
//	Get_qstep(qstep, poAPS, &poBrick->m_oHeader);
//	//iGet_predMode(pPredictor, poCabac, poAPS->adaptive_prediction_threshold, poAPS->max_num_direct_predictors, poModal->ctxPredMode);
//
//	return 1;
//}
void Decode_Color(PCC_Decoder* poDecoder,BitPtr *poBitPtr, MC* pMC, int iMC_Count, int iPoint_Count,int iMC_Shift_Bit,int bColor_Transform)
{//解码后的数据放在poDecoder->m_oGeometry_Brick.m_pPoint
	int i, bRet = 1;
	BitPtr oOrg = *poBitPtr;
	PCC_Attribute_Brick* poBrick = &poDecoder->m_oAttribute_Brick;
	poBrick->m_iLen = 0;
	int iType;
	iType = iGetBits1(poBitPtr, 8);
	if (iType == PCC_TYPE_APS)
		Get_APS(poBitPtr, &poDecoder->m_oAPS, bColor_Transform);
	else
	{
		printf("Fail to Get_AP\n");
		getchar();
		return;
	}
		
	iType = iGetBits1(poBitPtr, 8);
	if (iType == PCC_TYPE_ATTRIBUTE_BRICK)
	{
		for (i = 0; i < 4; i++)
			poBrick->m_iLen = (poBrick->m_iLen << 8) + iGetBits1(poBitPtr, 8);
		if (!poBrick->m_iLen)
			return;
	}
	else
	{
		printf("Fail to get PCC_TYPE_ATTRIBUTE_BRICK in Deocde_Color\n");
		getchar();
		return;
	}
	
	Get_Brick_Header(poBitPtr, &poBrick->m_oHeader);

	if (!bDecode_Color_1(poBitPtr, poDecoder, pMC, iMC_Count, iPoint_Count))
		bRet = 0;
	//Disp_Mem(&poDecoder->m_oMem_Mgr);
	poBitPtr->m_iCur = oOrg.m_iCur + poBrick->m_iLen + 4;
	poBitPtr->m_iBitPtr = 0;
	return;
}
void MCPC_Decode(PCC_Decoder* poDecoder, unsigned char* pBuffer,int iSize,unsigned char *pOut_Buffer,int iBuffer_Size,int *piPoint_Count,int bColor_Transform,int Centroid[3])
{
	int iMC_Count,iPoint_Count;
	MC* pMC;
	BitPtr oBitPtr;
	poDecoder->m_bDecode_Geometry_Only = 1;
	bDecode(poDecoder, pBuffer, iSize, 0);
	
	Decode_MC(poDecoder, pBuffer, iSize, poDecoder->m_oGeometry_Brick.m_pPoint, poDecoder->m_oGeometry_Brick.m_oHeader.geom_num_points, &pMC, &iMC_Count,&iPoint_Count,&oBitPtr);
	Free(&poDecoder->m_oMem_Mgr, poDecoder->m_oGeometry_Brick.m_pPoint);
	//Disp_Mem(&poDecoder->m_oMem_Mgr);
	
	//解码颜色
	Decode_Color(poDecoder,&oBitPtr, pMC, iMC_Count, iPoint_Count, 8, bColor_Transform);
	
	Gen_Data(poDecoder->m_oGeometry_Brick.m_pPoint,iPoint_Count,pOut_Buffer,Centroid);
	if (piPoint_Count)
		*piPoint_Count = iPoint_Count;

	Free_All(&poDecoder->m_oMem_Mgr);
}