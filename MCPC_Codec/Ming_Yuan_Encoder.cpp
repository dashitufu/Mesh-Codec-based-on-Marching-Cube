#include "MCPC_Codec.h"
#define START_FRAME_NO	0
typedef struct Semaphore_For_Thread {
	short s;	//信号量
	CRITICAL_SECTION m_oLock;	//排斥量, 由操作系统提供
}Semaphore_For_Thread;
typedef struct Init_Setting {
	char m_Save_Path[256];
	char m_Send_To_IP[32];
	int m_iSend_To_Port;
	int m_iEncoder_Count;
	int m_bSave_Bin;
	int m_bSave_Org;
	int m_bColor_Transform;
	int m_bSend_To_Output_Queue;
	int m_iQp;			//用于量化
	float m_fMC_Size;	//一个MC Cube的边长
	float m_fScale;		//与MC量化级别等价，另一种表达形式而已
	int m_iSample_Count;		//临时变量，一共装入多少个样本
}Init_Setting;

typedef struct Input_Video_Queue_Item {
	int m_iPoint_Count;
	//int m_iFace_Count;
	unsigned long long m_tTime_Stamp;
	int m_iFrame_No;	//输出要排序，输入不用，就是干
	float m_Box_Start[3];
	float m_fScale;
	unsigned char* m_pBuffer;
	int m_bIs_Pending;   //是否还在处理数据中，可能正在写，也可能正在读
}Input_Video_Queue_Item;
typedef struct Input_Video_Queue {
	Semaphore_For_Thread m_oS;
	int m_iHead, m_iEnd;
	int m_iCount;				//当前队列里的Buffer_Item数
	int m_iBuffer_Item_Count;			//一共可以放多少个Buffer_Item
	int m_iMax_Point_Count;		//最大点数限制
	Input_Video_Queue_Item* m_pBuffer;
}Input_Video_Queue;

typedef struct Output_Video_Queue_Item {
	unsigned long long m_tTime_Stamp;
	int m_iFrame_No;
	int m_iSize;
	int m_bHas_Data;			//由于是排序队列，此标志表示是否已经有数据，入队出队则修改
	float m_fScale;
	float m_Box_Start[3];
	unsigned char* m_pBuffer;
}Output_Video_Queue_Item;
typedef struct Output_Video_Queue {
	Semaphore_For_Thread m_oS;
	int m_iHead, m_iEnd;
	int m_iCount;				//当前队列里的Buffer_Item数
	int m_iCur_Frame_No;		//当前队头的Frame_No
	int m_iBuffer_Item_Count;	//一共可以放多少个Buffer_Item,由于要照顾Encoder，此数字为Encoder的n倍
	int m_iMax_Item_Size;		//一共可以放多少个Buffer_Item
	Output_Video_Queue_Item* m_pBuffer;
}Output_Video_Queue;

typedef struct Encoder_Env {
	Init_Setting m_oInit_Setting;
	Input_Video_Queue m_oIn_Video_Queue;
	Output_Video_Queue m_oOut_Video_Queue;
	PCC_Encoder* m_pEncoder[16];
}Encoder_Env;
typedef struct Encode_Param {
	PCC_Encoder* m_poEncoder;
	Encoder_Env* m_poEnv;
}Encode_Param;

void Lock_Semaphore_For_Thread(Semaphore_For_Thread* ps, int iThreadID)
{//简单地加个排斥锁了事，注意：这个函数在单线程中不会阻塞，例：
	if (!ps)
		printf("Error");
	EnterCriticalSection(&ps->m_oLock);
	return;
}
void Unlock_Semaphore_For_Thread(Semaphore_For_Thread* ps, int iThreadID)
{
	LeaveCriticalSection(&ps->m_oLock);
	return;
}
void Delete_Semaphore_For_Thread(Semaphore_For_Thread* ps)
{
	DeleteCriticalSection(&ps->m_oLock);
}
void Init_Semaphore_For_Thread(Semaphore_For_Thread* ps, short s)
{
	InitializeCriticalSection(&ps->m_oLock);
	ps->s = s;
}
int bLoad_Setting(char* pcFile, Init_Setting* poInit_Setting)
{
	char* pcText = NULL, Value[256];
	FILE* pFile = fopen(pcFile, "rb");
	int bRet = 1, iResult;
	iResult = (int)iGet_File_Length(pcFile);
	if (!pFile)
	{
		bRet = 0;
		printf("Fail to open:%s\n", pcFile);
		goto END;
	}
	pcText = (char*)malloc(iResult + 1);
	iResult = (int)fread(pcText, 1, iResult, pFile);
	pcText[iResult] = 0;

	if (!bGet_Value(pcText, iResult, "Send To IP", poInit_Setting->m_Send_To_IP))
	{
		printf("Fail to load setting Send To IP\n");
		bRet = 0;
		goto END;
	}

	if (!bGet_Value(pcText, iResult, "Send To Port", Value))
	{
		printf("Fail to load setting Send To Port\n");
		bRet = 0;
		goto END;
	}
	poInit_Setting->m_iSend_To_Port = atoi(Value);

	if (!bGet_Value(pcText, iResult, "Encoder Count", Value))
	{
		printf("Fail to load setting Decoder Count\n");
		bRet = 0;
		goto END;
	}
	poInit_Setting->m_iEncoder_Count = atoi(Value);

	if (!bGet_Value(pcText, iResult, "Save Bin", Value))
	{
		printf("Fail to load setting Save Bin\n");
		bRet = 0;
		goto END;
	}
	poInit_Setting->m_bSave_Bin = atoi(Value);

	if (!bGet_Value(pcText, iResult, "Color Transform", Value))
	{
		printf("Fail to load setting Color Transform\n");
		bRet = 0;
		goto END;
	}
	poInit_Setting->m_bColor_Transform = atoi(Value);

	if (!bGet_Value(pcText, iResult, "MC Size", Value))
	{
		printf("Fail to load setting MC Size\n");
		bRet = 0;
		goto END;
	}
	poInit_Setting->m_fMC_Size = (float)atof(Value);

	if (!bGet_Value(pcText, iResult, "Scale", Value))
	{
		printf("Fail to load setting Scale\n");
		bRet = 0;
		goto END;
	}
	poInit_Setting->m_fScale = (float)atof(Value);

	if (!bGet_Value(pcText, iResult, "Save Bin", Value))
	{
		printf("Fail to load Save Bin\n");
		bRet = 0;
		goto END;
	}
	poInit_Setting->m_bSave_Bin = atoi(Value);

	if (!bGet_Value(pcText, iResult, "Save Org", Value))
	{
		printf("Fail to load Save Org\n");
		bRet = 0;
		goto END;
	}
	poInit_Setting->m_bSave_Org = atoi(Value);

	if (!bGet_Value(pcText, iResult, "Save Path", poInit_Setting->m_Save_Path))
	{
		printf("Fail to load Save Path\n");
		bRet = 0;
		goto END;
	}

	if (!bGet_Value(pcText, iResult, "QP", Value))
	{
		printf("Fail to load QP\n");
		bRet = 0;
		goto END;
	}
	poInit_Setting->m_iQp = atoi(Value);

	if (!bGet_Value(pcText, iResult, "Send To Output Queue", Value))
	{
		printf("Fail to load Send To Output Queue\n");
		bRet = 0;
		goto END;
	}
	poInit_Setting->m_bSend_To_Output_Queue = atoi(Value);

	if (!bGet_Value(pcText, iResult, "Sample Count", Value))
	{
		printf("Fail to load Send To Sample Count\n");
		bRet = 0;
		goto END;
	}
	poInit_Setting->m_iSample_Count = atoi(Value);

END:
	if (pcText)
		free(pcText);
	if (pFile)
		fclose(pFile);
	return bRet;
}
void Init_Queue(Input_Video_Queue* poQueue, int iBuffer_Item_Count, int iMax_Point_Count)
{
	int i;
	Input_Video_Queue_Item* poItem;
	Init_Semaphore_For_Thread(&poQueue->m_oS, 1);
	poQueue->m_iBuffer_Item_Count = iBuffer_Item_Count;
	poQueue->m_iCount = poQueue->m_iEnd = poQueue->m_iHead = 0;
	poQueue->m_pBuffer = (Input_Video_Queue_Item*)malloc(sizeof(Input_Video_Queue_Item) * iBuffer_Item_Count);
	poQueue->m_iMax_Point_Count = iMax_Point_Count;
	for (i = 0; i < iBuffer_Item_Count; i++)
	{
		poItem = &poQueue->m_pBuffer[i];
		poItem->m_pBuffer = (unsigned char*)malloc((int)iMax_Point_Count * 15 );
		poItem->m_iPoint_Count = 0;
		poItem->m_tTime_Stamp = 0;
		poItem->m_bIs_Pending = 0;
	}
	return;
}
Input_Video_Queue_Item* poIn_Queue(Input_Video_Queue* poQueue)
{
	Lock_Semaphore_For_Thread(&poQueue->m_oS,0);
	if (poQueue->m_iCount == poQueue->m_iBuffer_Item_Count)
	{
		//printf("Queue is full\n");
		Unlock_Semaphore_For_Thread(&poQueue->m_oS,0);
		return NULL;
	}
	Input_Video_Queue_Item* poItem = &poQueue->m_pBuffer[poQueue->m_iEnd];
	if (poItem->m_bIs_Pending)
	{
		//printf("Queue is full\n");
		Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);
		return NULL;
	}
	poItem->m_bIs_Pending = 1;    //开始使用，写入数据后再置0
	poQueue->m_iEnd = (poQueue->m_iEnd + 1) % poQueue->m_iBuffer_Item_Count;
	poQueue->m_iCount++;

	Unlock_Semaphore_For_Thread(&poQueue->m_oS,0);
	return poItem;
}
int bIn_Queue(Input_Video_Queue* poQueue, unsigned char* pSource, int iSize, unsigned long long tTime_Stamp,int iPoint_Count)
{
	Input_Video_Queue_Item oNew_Item = { 0 }, * poItem;
	
	Lock_Semaphore_For_Thread(&poQueue->m_oS, 0);
	if (poQueue->m_iCount == poQueue->m_iBuffer_Item_Count)
	{//满座，走人
		Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);
		return 0;
	}
	poItem = &poQueue->m_pBuffer[poQueue->m_iEnd];	//先占坑
	poQueue->m_iEnd = (poQueue->m_iEnd + 1) % poQueue->m_iBuffer_Item_Count;

	memcpy(poItem->m_pBuffer, pSource, iSize);
	
	poItem->m_tTime_Stamp = tTime_Stamp;	// oNew_Item.m_tTime_Stamp;
	poItem->m_iPoint_Count = iPoint_Count;	// oNew_Item.m_iPoint_Count;
	poQueue->m_iCount++;
	Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);
	return 1;
}
void Data_Finish(Input_Video_Queue* poQueue, Input_Video_Queue_Item* poItem)
{
	poItem->m_bIs_Pending = 0;
}
void Init_Queue(Output_Video_Queue* poQueue, int iBuffer_Item_Count, int iItem_Size)
{
	int i;
	Output_Video_Queue_Item* poItem;
	Init_Semaphore_For_Thread(&poQueue->m_oS, 1);
	poQueue->m_iBuffer_Item_Count = iBuffer_Item_Count;
	poQueue->m_iCur_Frame_No = poQueue->m_iEnd = poQueue->m_iHead = 0;
	poQueue->m_iMax_Item_Size = iItem_Size;
	poQueue->m_iCount = 0;
	poQueue->m_pBuffer = (Output_Video_Queue_Item*)malloc(sizeof(Input_Video_Queue_Item) * iBuffer_Item_Count);
	for (i = 0; i < iBuffer_Item_Count; i++)
	{
		poItem = &poQueue->m_pBuffer[i];
		poItem->m_pBuffer = (unsigned char*)malloc(iItem_Size);
		poItem->m_tTime_Stamp = 0;
		poItem->m_bHas_Data = 0;
		poItem->m_Box_Start[0] = poItem->m_Box_Start[1] = poItem->m_Box_Start[2] = 0;
		poItem->m_iSize = 0;
		poItem->m_iFrame_No = 0;
	}

	return;
}
int bLoad(const char* pcFile, unsigned char* pBuffer,int *piPoint_Count,int iMax_Buffer_Size)
{//简单写个装入数据 x,y,z r,g,b 一点15字节
	FILE* pFile = fopen(pcFile, "rb");
	unsigned char* pCur = pBuffer;
	int i,iPoint_Count;
	if (!pFile)
	{
		printf("Fail to open %s\n", pcFile);
		return 0;
	}
	bRead_PLY_Header(pFile, NULL, NULL, NULL, NULL, NULL, &iPoint_Count, NULL, NULL);

	////老方法，存粹为了好debug
	//for (i = 0; i < iPoint_Count; i++)
	//{
	//	fread(pCur, 1, 3 * sizeof(float), pFile);
	//	pCur += 3 * sizeof(float);
	//	//暂时不需要Normal,向前推进3个float
	//	fseek(pFile, 3 * sizeof(float), SEEK_CUR);
	//	fread(pCur, 1, 3, pFile);
	//	pCur += 3;
	//}

	//新方法，一口气全load进来
	unsigned char* pCur_Source, * pCur_Dest;
	int j,iResult,iPoint_Left;
	pCur_Source = pCur_Dest = pBuffer;

	if (iPoint_Count * 27 > iMax_Buffer_Size)
	{
		iPoint_Left = iPoint_Count;
		iPoint_Count = iMax_Buffer_Size / 27;
		iPoint_Left -= iPoint_Count;
	}else
		iPoint_Left = 0;

	iResult=(int)fread(pCur_Source, 1, iPoint_Count * 27, pFile);
	for (i = 0; i < iPoint_Count; i++)
	{
		for(j=0;j<3;j++,pCur_Dest+=4,pCur_Source+=4)
			*(float*)pCur_Dest = *(float*)pCur_Source;
		//暂时不需要Normal,向前推进3个float
		pCur_Source += 3 * 4;
		*(unsigned int*)pCur_Dest = *(unsigned int*)pCur_Source;
		pCur_Dest += 3;
		pCur_Source += 3;
	}
	pCur_Source = pCur_Dest;
	if (pBuffer + iMax_Buffer_Size - pCur_Source < iPoint_Left * 27)
	{
		printf("Insufficient memory in bLoad\n");
		return 0;
	}
	if(iPoint_Left)
		iResult = (int)fread(pCur_Source, 1, iPoint_Left * 27, pFile);
	for (i = 0; i < iPoint_Left; i++)
	{
		for (j = 0; j < 3; j++, pCur_Dest += 4, pCur_Source += 4)
			*(float*)pCur_Dest = *(float*)pCur_Source;
		//暂时不需要Normal,向前推进3个float
		pCur_Source += 3 * 4;
		*(unsigned int*)pCur_Dest = *(unsigned int*)pCur_Source;
		pCur_Dest += 3;
		pCur_Source += 3;
	}

	if (piPoint_Count)
		*piPoint_Count = iPoint_Count+iPoint_Left;
	fclose(pFile);
	return 1;
}
void Send_Data_1(void* pEnv, unsigned char* pBuffer, int iPoint_Count,unsigned long long tTime_Stamp)
{
	Encoder_Env* poEnv = (Encoder_Env*)pEnv;
	Input_Video_Queue* poQueue = &poEnv->m_oIn_Video_Queue;
	int iMax_Point_Count = poQueue->m_iMax_Point_Count;
	int iResult, iSize, iFrame_No = 0;

	iSize = iPoint_Count * 15;
	while (!(iResult = bIn_Queue(poQueue, pBuffer, iSize, tTime_Stamp, iPoint_Count)))
	{
		//printf("Wait\n");
		Sleep(1);
	}
}
ULONG __stdcall Gen_Data_Thread(LPVOID pParam)
{//这个线程只是假装上游推数据
//#define SAMPLE_COUNT 199
	Encoder_Env* poEnv = (Encoder_Env*)pParam;
	Input_Video_Queue* poQueue = &poEnv->m_oIn_Video_Queue;
	int iMax_Point_Count = poQueue->m_iMax_Point_Count;
	int iResult,iSize,iFrame_No = START_FRAME_NO,iFrame_Count=0;

	//unsigned char* pBuffer = (unsigned char*)malloc(poQueue->m_iMax_Point_Count * 27);

	char File[256];
	int iPoint_Count,iSample_Count=poEnv->m_oInit_Setting.m_iSample_Count,
		iMax_Buffer_Size= poQueue->m_iMax_Point_Count * 15;
	unsigned long long tTime_Stamp=0;
	Input_Video_Queue_Item* poHead;
	tTime_Stamp = GetTickCount();
	//循环拿数据
	while (1)
	{
		sprintf(File, "D:\\Sample\\chi\\chi%04d.ply", iFrame_No);		
		while ( !(poHead = poIn_Queue(poQueue)))
			Sleep(1);

		bLoad(File, poHead->m_pBuffer,&iPoint_Count, iMax_Buffer_Size);
		iSize = iPoint_Count * 15;
		poHead->m_tTime_Stamp = tTime_Stamp;
		poHead->m_iPoint_Count = iPoint_Count;
		Data_Finish(poQueue, poHead);
		
		iFrame_No = (iFrame_No + 1) % iSample_Count;
		//if (iFrame_Count++ > 1000)
			//break;
	}
	printf("%d\n", GetTickCount() - tTime_Stamp);
	return 0;
//#undef SAMPLE_COUNT
}
Input_Video_Queue_Item* poOut_Queue(Input_Video_Queue* poQueue)
{
	Input_Video_Queue_Item* poHead;
	if (!poQueue->m_iCount)
		return NULL; //连锁都懒得锁
	Lock_Semaphore_For_Thread(&poQueue->m_oS,0);
	if (!poQueue->m_iCount || (poHead = &poQueue->m_pBuffer[poQueue->m_iHead])->m_bIs_Pending)
	{
		Unlock_Semaphore_For_Thread(&poQueue->m_oS,0);
		return NULL;    //未可
	}
	poQueue->m_iHead = (poQueue->m_iHead + 1) % poQueue->m_iBuffer_Item_Count;
	poQueue->m_iCount--;
	poHead->m_bIs_Pending = 1;

	Unlock_Semaphore_For_Thread(&poQueue->m_oS,0);
	return poHead;
}
int bOut_Queue(Input_Video_Queue* poQueue,Input_Video_Queue_Item* poItem)
{//将内容抄到poItem，出队队头
	Input_Video_Queue_Item* poHead;
	int iSize;
	Lock_Semaphore_For_Thread(&poQueue->m_oS, 0);
	if (poQueue->m_iCount == 0)
	{//没活，走人
		Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);
		return 0;
	}
	poHead = &poQueue->m_pBuffer[poQueue->m_iHead];
	poItem->m_tTime_Stamp = poHead->m_tTime_Stamp;
	poItem->m_iPoint_Count = poHead->m_iPoint_Count;
	poItem->m_iFrame_No = poHead->m_iFrame_No;
	poItem->m_fScale = poHead->m_fScale;
	iSize = poItem->m_iPoint_Count*15;
	memcpy(poItem->m_pBuffer, poHead->m_pBuffer, iSize);
	poQueue->m_iHead = (poQueue->m_iHead + 1) % poQueue->m_iBuffer_Item_Count;
	poQueue->m_iCount--;
	Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);

	return 1;
}
int bIn_Queue(Output_Video_Queue* poQueue, unsigned char* pSource, int iSize, Input_Video_Queue_Item* poInput_Item)
{//多路生产，一路消费 poInput_Item带来来自最初相机捕捉的信息
	Output_Video_Queue_Item* poItem;
	Lock_Semaphore_For_Thread(&poQueue->m_oS, 0);
	if (poQueue->m_iCount == poQueue->m_iBuffer_Item_Count)
	{//满座，走人
		Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);
		return 0;
	}
	poItem = &poQueue->m_pBuffer[poQueue->m_iEnd];	//先占坑
	if (iSize > poQueue->m_iMax_Item_Size)
	{
		printf("Data Size exceed max_item_size\n");
		assert(false);
	}
	memcpy(poItem->m_pBuffer, pSource, iSize);
	poItem->m_tTime_Stamp = poInput_Item->m_tTime_Stamp;
	poItem->m_iFrame_No = poInput_Item->m_iFrame_No;
	poItem->m_iSize = iSize;
	poItem->m_bHas_Data = 1;
	poItem->m_Box_Start[0] = poInput_Item->m_Box_Start[0];
	poItem->m_Box_Start[1] = poInput_Item->m_Box_Start[1];
	poItem->m_Box_Start[2] = poInput_Item->m_Box_Start[2];
	poItem->m_fScale = poInput_Item->m_fScale;
	poQueue->m_iCount++;
	poQueue->m_iEnd = (poQueue->m_iEnd + 1) % poQueue->m_iBuffer_Item_Count;
	Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);
	return 1;
}
int bOut_Queue_1(Output_Video_Queue* poQueue, Output_Video_Queue_Item* poItem)
{
	Output_Video_Queue_Item* poHead;
	char* pCur;

	Lock_Semaphore_For_Thread(&poQueue->m_oS, 0);
	if (poQueue->m_iCount == 0)
	{//没活，走人
		Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);
		return 0;
	}
	poHead = &poQueue->m_pBuffer[poQueue->m_iHead];
	poItem->m_tTime_Stamp = poHead->m_tTime_Stamp;
	poItem->m_iFrame_No = poHead->m_iFrame_No;
	poItem->m_Box_Start[0] = poHead->m_Box_Start[0];
	poItem->m_Box_Start[1] = poHead->m_Box_Start[1];
	poItem->m_Box_Start[2] = poHead->m_Box_Start[2];
	poItem->m_fScale = poHead->m_fScale;
	poHead->m_bHas_Data = 0;
	if (poHead->m_iSize >= poQueue->m_iMax_Item_Size)
	{
		printf("Data size exceeds max_item_size\n");
		assert(false);
	}
	memcpy(poItem->m_pBuffer, poHead->m_pBuffer, poHead->m_iSize);
	pCur = (char*)&poItem->m_pBuffer[poHead->m_iSize];
	*pCur++ = 0;	//简单表示以上二进制已经结束，文本开始
	sprintf(pCur, "Type=Video\r\n");
	sprintf(pCur + strlen(pCur), "Time Stamp=%I64d\r\n", poItem->m_tTime_Stamp);

	assert(strlen(pCur) < 1024);
	pCur += strlen(pCur);
	poItem->m_iSize =(int)( pCur - (char*)poItem->m_pBuffer);
	poQueue->m_iCount--;
	poQueue->m_iHead = (poQueue->m_iHead + 1) % poQueue->m_iBuffer_Item_Count;
	Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);
	return 1;
}

ULONG __stdcall Video_Encode_Thread(LPVOID pParam)
{//这里编码
	Encode_Param* poParam = (Encode_Param*)pParam;
	PCC_Encoder* poEncoder = poParam->m_poEncoder;

	Input_Video_Queue* poIn_Queue = &poParam->m_poEnv->m_oIn_Video_Queue;
	Output_Video_Queue* poOutput_Queue = &poParam->m_poEnv->m_oOut_Video_Queue;
	Init_Setting* poInit_Setting = &poParam->m_poEnv->m_oInit_Setting;
	Input_Video_Queue_Item oItem,*poHead;
	float fMC_Size, fScale;
	unsigned int tStart;
	int iPoint_Count;
	int iFrame_No = START_FRAME_NO;
	char File[256];

	fMC_Size = poParam->m_poEnv->m_oInit_Setting.m_fMC_Size;
	fScale = poParam->m_poEnv->m_oInit_Setting.m_fScale;
	//oItem.m_pBuffer = (unsigned char*)malloc(poIn_Queue->m_iMax_Point_Count * 15);
	while (1)
	{
		while (!(poHead = poOut_Queue(poIn_Queue)))
			Sleep(1);
		if (poHead->m_iPoint_Count < 1000)
		{
			printf("Point count<1000\n");
			Data_Finish(poIn_Queue, poHead);
			continue;
		}
		tStart = GetTickCount();
		MCPC_Encode(poEncoder, poHead->m_pBuffer, poHead->m_iPoint_Count,fMC_Size,fScale,&iPoint_Count);
		Data_Finish(poIn_Queue, poHead);

		if (poInit_Setting->m_bSave_Org)
		{
			sprintf(File, "%s\\Src_%04d.ply", poInit_Setting->m_Save_Path, iFrame_No);
			bSave(File, poEncoder->m_oGeometry_Brick.m_pOrder_Point, iPoint_Count, 1);
		}
		if (poInit_Setting->m_bSave_Bin)
		{
			sprintf(File, "%s\\%04d.bin", poInit_Setting->m_Save_Path, iFrame_No);
			//printf("%s\n", File);
			bSave(poEncoder, File);
		}
		iFrame_No++;
		printf("Encode Time:%d\n", (int)(GetTickCount() - tStart));
		if(poInit_Setting->m_bSend_To_Output_Queue)
			while (!bIn_Queue(poOutput_Queue, poEncoder->m_pOut_Buffer, poEncoder->m_iOut_Data_Size, &oItem))
				Sleep(1);
		//Disp_Mem(&poEncoder->m_oMem_Mgr);
		Free_All(&poEncoder->m_oMem_Mgr);
	}
	return 0;
}
SOCKET iInit_Connection(char ServerAddress[], int iServerPort)
{//Purpose: init a connection to the remote socket server
//return:	Not 0 if successful;0 if fail
	int iResult;
	SOCKET iSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (!iSocket)
	{
		printf("fail to get socket:%d\n", GetLastError());
		return 0;
	}

	sockaddr_in oServer;

	oServer.sin_addr.s_addr = inet_addr(ServerAddress);
	oServer.sin_family = AF_INET;
	oServer.sin_port = htons(iServerPort);
	iResult = connect(iSocket, (sockaddr*)&oServer, sizeof(oServer));

	if (iResult == SOCKET_ERROR)
	{
		iResult = GetLastError();
		closesocket(iSocket);
		SetLastError(0);
		iSocket = 0;
		return 0;
	}
	return iSocket;
}
ULONG __stdcall Send_Data_Thread(LPVOID pParam)
{
	int /*bHas_Data,*/iResult;
	SOCKET iSocket = 0;
	Encoder_Env* poEnv = (Encoder_Env*)pParam;
	Output_Video_Queue* poVideo_Queue = &poEnv->m_oOut_Video_Queue;
	Output_Video_Queue_Item oVideo_Item;
	unsigned int iStart_Code = 0xFFFFFFFF;

	//这里搞个socket
	while (!(iSocket = iInit_Connection(poEnv->m_oInit_Setting.m_Send_To_IP, poEnv->m_oInit_Setting.m_iSend_To_Port)))
		printf("重连:%s\n", poEnv->m_oInit_Setting.m_Send_To_IP);

	oVideo_Item.m_pBuffer = (unsigned char*)malloc(poVideo_Queue->m_iMax_Item_Size);
	//取队列一个Item
	while (1)
	{
		if (bOut_Queue_1(poVideo_Queue, &oVideo_Item))
		{
			iResult = send(iSocket, (char*)&iStart_Code, 4, 0);
			iResult = send(iSocket, (char*)&oVideo_Item.m_iSize, 4, 0);
			iResult = send(iSocket, (char*)oVideo_Item.m_pBuffer, oVideo_Item.m_iSize, 0);
			if (iResult != oVideo_Item.m_iSize)
			{
				do {
					printf("重连:%s\n", poEnv->m_oInit_Setting.m_Send_To_IP);
				} while (!(iSocket = iInit_Connection(poEnv->m_oInit_Setting.m_Send_To_IP, poEnv->m_oInit_Setting.m_iSend_To_Port)));
					
			}
		}else
			Sleep(1);
	}
	return 0;
}
void *pInit_Env_1()
{//iCodec_Count: 同时开几个Codec
	WSADATA oData;
	int i, iMax_Point_Count = 500000;
	Encoder_Env *poEnv=(Encoder_Env*)malloc(sizeof(Encoder_Env));
	PCC_Encoder* poEncoder;
	Encode_Param *pEncode_Param=(Encode_Param*)malloc(16*sizeof(Encode_Param));
	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &oData);
	if (!bLoad_Setting((char*)"Setting.ini", &poEnv->m_oInit_Setting))
		exit(0);
	
	Init_Codec_Env(poEnv->m_oInit_Setting.m_bColor_Transform);	//需要YUV颜色空间才需要开辟表

	Init_Queue(&poEnv->m_oIn_Video_Queue, poEnv->m_oInit_Setting.m_iEncoder_Count+2, iMax_Point_Count);
	Init_Queue(&poEnv->m_oOut_Video_Queue, 2, 1000000);
	
	//这个线程测试用，运行时不用
	CloseHandle(CreateThread(0, 0, Gen_Data_Thread, poEnv, 0, 0));
	
	CloseHandle(CreateThread(0, 0, Send_Data_Thread, poEnv, 0, 0));

	for (i = 0; i < poEnv->m_oInit_Setting.m_iEncoder_Count; i++)
	{
		poEnv->m_pEncoder[i] = poEncoder = (PCC_Encoder*)malloc(sizeof(PCC_Encoder));
		Init_Encoder(poEncoder, 100000, 1, poEnv->m_oInit_Setting.m_bColor_Transform, 0, poEnv->m_oInit_Setting.m_iQp, Pre_Process_Type::Full_Search);		//此处只做最基本的初值
		pEncode_Param[i].m_poEncoder = poEncoder;
		pEncode_Param[i].m_poEnv = poEnv;
		CloseHandle(CreateThread(0, 0, Video_Encode_Thread, &pEncode_Param[i], 0, 0));
	}

	//运行时不用
	//Sleep(INFINITE);
	return poEnv;
}
void Ming_Yuan_Test()
{
	pInit_Env_1();
	Sleep(INFINITE);
	return;
}