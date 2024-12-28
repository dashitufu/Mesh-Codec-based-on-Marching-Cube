//将所有的网络线程有的没的全放在这实现了事
#include "MCPC_Decoder.h"
#include "winsock.h"

typedef struct Semaphore_For_Thread {
	short s;	//信号量
	CRITICAL_SECTION m_oLock;	//排斥量, 由操作系统提供
}Semaphore_For_Thread;
typedef struct Init_Setting_1 {
	char m_Save_Path[256];
	char m_Recv_From_IP[32];
	int m_iRecv_From_Port;
	int m_iDecoder_Count;
	int m_bColor_Transform;
	int m_bSave_New;
}Init_Setting_1;
typedef struct Video_Input_Queue_Item {
	unsigned long long m_tTime_Stamp;
	int m_iFrame_No;
	int m_iSize;
	float m_fScale;
	float m_Box_Start[3];
	unsigned char* m_pBuffer;
}Video_Input_Queue_Item;
typedef struct Video_Input_Queue {
	Semaphore_For_Thread m_oS;
	int m_iHead, m_iEnd;
	int m_iCount;				//当前队列里的Buffer_Item数
	int m_iBuffer_Item_Count;	//一共可以放多少个Buffer_Item
	int m_iMax_Item_Size;			//一共可以放多少个Buffer_Item
	Video_Input_Queue_Item* m_pBuffer;
}Input_Queue;

typedef struct Video_Output_Queue_Item {
	unsigned long long m_tTime_Stamp;
	int m_iFrame_No;
	int m_iSize;
	int m_iPoint_Count;
	float m_fScale;
	//float m_Box_Start[3];
	int m_Centroid[3];
	unsigned char* m_pBuffer;
}Video_Output_Queue_Item;

typedef struct Video_Output_Queue {
	Semaphore_For_Thread m_oS;
	int m_iHead, m_iEnd;
	int m_iCount;				//当前队列里的Buffer_Item数
	int m_iBuffer_Item_Count;	//一共可以放多少个Buffer_Item
	int m_iMax_Item_Size;			//一共可以放多少个Buffer_Item
	Video_Output_Queue_Item* m_pBuffer;
}Video_Output_Queue;

typedef struct Decoder_Env {
	Init_Setting_1 m_oInit_Setting;
	Video_Input_Queue m_oIn_Video_Queue;
	Video_Output_Queue m_oOut_Video_Queue;
	int m_bStop;		//0表示正在干； //1表示通知停
	//unsigned int m_Decode_Thread_Stop[31];	//最多多少个并发Codec
	unsigned int m_bRecv_Thread_Stop;

	void* m_pDecode_Param;
	//PCC_Decoder* m_pDecoder[16];
}Decoder_Env;
typedef struct Decode_Param {
	PCC_Decoder* m_poDecoder;
	Decoder_Env* m_poEnv;
	int m_iIndex;	//指明这是第几个Decoder
	int m_bStop;		//是否已经停止
}Decode_Param;
void Init_Semaphore_For_Thread(Semaphore_For_Thread* ps, short s)
{
	InitializeCriticalSection(&ps->m_oLock);
	ps->s = s;
}
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
int bGet_Value(char* pText, int iSize, const char* pKey, char* pValue)
{//Purpose: 从pText中找到pKey, 然后得到其Value. pText的格式形如 Key=Value
 //Return: 0 if Fail, 1 if success
	char* pPos, * pEnd, * pValue1;
	char* pPos_Space, * pCur;
	int bError;
	while (1)
	{
		pPos = strstr(pText, pKey);	//寻找pKey串位置
		if (!pPos)
			return 0;
		pPos_Space = strstr(pPos, "=");
		if (!pPos_Space)
			return 0;
		for (bError = 0, pCur = pPos + strlen(pKey); pCur < pPos_Space; pCur++)
		{
			if (*pCur != ' ')
			{
				bError = 1;
				break;
			}
		}
		if (bError)
		{
			pText = pPos + 1;
			continue;
		}
		else
			pPos = pPos_Space + 1;
		//pPos++;
		while (*pPos == ' ' || *pPos == '\t')
			pPos++;
		for (pValue1 = pValue, pEnd = pPos; *pEnd != '\n' && *pEnd != '\r' && *pEnd != 0; pEnd++, pValue1++)
			*pValue1 = *pEnd;
		*pValue1 = 0;
		break;
	}
	return 1;
}
int bLoad_Setting(char* pcFile, Init_Setting_1* poInit_Setting)
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

	if (!bGet_Value(pcText, iResult, "Recv From IP", poInit_Setting->m_Recv_From_IP))
	{
		printf("Fail to load setting Recv From IP\n");
		bRet = 0;
		goto END;
	}
	if (!bGet_Value(pcText, iResult, "Recv From Port", Value))
	{
		printf("Fail to load setting Recv From Port\n");
		bRet = 0;
		goto END;
	}
	poInit_Setting->m_iRecv_From_Port = atoi(Value);
		
	if (!bGet_Value(pcText, iResult, "Decoder Count", Value))
	{
		printf("Fail to load setting Decoder Count\n");
		bRet = 0;
		goto END;
	}
	poInit_Setting->m_iDecoder_Count = atoi(Value);

	if (!bGet_Value(pcText, iResult, "Color Transform", Value))
	{
		printf("Fail to load setting Color Transform\n");
		bRet = 0;
		goto END;
	}
	poInit_Setting->m_bColor_Transform = atoi(Value);

	if (!bGet_Value(pcText, iResult, "Save New", Value))
	{
		printf("Fail to load setting Save New\n");
		bRet = 0;
		goto END;
	}
	poInit_Setting->m_bSave_New = atoi(Value);

	if (!bGet_Value(pcText, iResult, "Save Path", poInit_Setting->m_Save_Path))
	{
		printf("Fail to load setting Save Path\n");
		bRet = 0;
		goto END;
	}
END:
	if (pcText)
		free(pcText);
	if (pFile)
		fclose(pFile);
	return bRet;
}
void Free_Queue(Video_Output_Queue* poQueue)
{
	int i;
	Video_Output_Queue_Item* poItem;
	for (i = 0; i < poQueue->m_iBuffer_Item_Count; i++)
	{
		poItem = &poQueue->m_pBuffer[i];
		if (poItem->m_pBuffer)
			free(poItem->m_pBuffer);
	}
	free(poQueue->m_pBuffer);
	Delete_Semaphore_For_Thread(&poQueue->m_oS);
	return;
}
void Init_Queue(Video_Output_Queue* poQueue, int iBuffer_Item_Count, int iItem_Size)
{
	int i;
	Video_Output_Queue_Item* poItem;
	Init_Semaphore_For_Thread(&poQueue->m_oS, 1);
	poQueue->m_iBuffer_Item_Count = iBuffer_Item_Count;
	poQueue->m_iCount = poQueue->m_iEnd = poQueue->m_iHead = 0;
	poQueue->m_iMax_Item_Size = iItem_Size;
	poQueue->m_pBuffer = (Video_Output_Queue_Item*)malloc(sizeof(Video_Output_Queue_Item) * iBuffer_Item_Count);
	for (i = 0; i < iBuffer_Item_Count; i++)
	{
		poItem = &poQueue->m_pBuffer[i];
		poItem->m_pBuffer = (unsigned char*)malloc(iItem_Size);
		poItem->m_tTime_Stamp = 0;
	}
	return;
}
void Free_Queue(Video_Input_Queue* poQueue)
{
	int i;
	Video_Input_Queue_Item* poItem;
	for (i = 0; i < poQueue->m_iBuffer_Item_Count; i++)
	{
		poItem = &poQueue->m_pBuffer[i];
		if (poItem->m_pBuffer)
			free(poItem->m_pBuffer);
	}
	free(poQueue->m_pBuffer);
	Delete_Semaphore_For_Thread(&poQueue->m_oS);
	return;
}
void Init_Queue(Video_Input_Queue* poQueue, int iBuffer_Item_Count, int iItem_Size)
{
	int i;
	Video_Input_Queue_Item* poItem;
	Init_Semaphore_For_Thread(&poQueue->m_oS, 1);
	poQueue->m_iBuffer_Item_Count = iBuffer_Item_Count;
	poQueue->m_iCount = poQueue->m_iEnd = poQueue->m_iHead = 0;
	poQueue->m_iMax_Item_Size = iItem_Size;
	poQueue->m_pBuffer = (Video_Input_Queue_Item*)malloc(sizeof(Video_Input_Queue_Item) * iBuffer_Item_Count);
	for (i = 0; i < iBuffer_Item_Count; i++)
	{
		poItem = &poQueue->m_pBuffer[i];
		poItem->m_pBuffer = (unsigned char*)malloc(iItem_Size);
		poItem->m_tTime_Stamp = 0;
	}
	return;
}
int bIn_Queue(Video_Output_Queue* poQueue,unsigned char* pSource, int iPoint_Count,unsigned long long tTime_Stamp,int Centroid[3])
{
	Video_Output_Queue_Item* poItem;
	int iSize = iPoint_Count * 15;
	if (iSize > poQueue->m_iMax_Item_Size)
		return 0;
	Lock_Semaphore_For_Thread(&poQueue->m_oS, 0);
	if (poQueue->m_iCount == poQueue->m_iBuffer_Item_Count)
	{//满座，走人
		Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);
		return 0;
	}
	poItem = &poQueue->m_pBuffer[poQueue->m_iEnd];	//先占坑
	poQueue->m_iEnd = (poQueue->m_iEnd + 1) % poQueue->m_iBuffer_Item_Count;
	memcpy(poItem->m_pBuffer, pSource, iSize);
	poItem->m_iPoint_Count = iPoint_Count;
	poItem->m_tTime_Stamp = tTime_Stamp;
	poItem->m_Centroid[0] = Centroid[0];
	poItem->m_Centroid[1] = Centroid[1];
	poItem->m_Centroid[2] = Centroid[2];
	poQueue->m_iCount++;

	Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);
	return 1;
}
int bIn_Queue(Video_Input_Queue* poQueue, int iSize, unsigned char* pSource, unsigned int iFrame_No)
{//多路生产，一路消费
	Video_Input_Queue_Item* poItem;
	char* pCur, Value[256];
	int iLen;
	if (iSize > poQueue->m_iMax_Item_Size)
		return 0;
	Lock_Semaphore_For_Thread(&poQueue->m_oS, 0);
	if (poQueue->m_iCount == poQueue->m_iBuffer_Item_Count)
	{//满座，走人
		Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);
		return 0;
	}
	poItem = &poQueue->m_pBuffer[poQueue->m_iEnd];	//先占坑
	poQueue->m_iEnd = (poQueue->m_iEnd + 1) % poQueue->m_iBuffer_Item_Count;

	memcpy(poItem->m_pBuffer, pSource, iSize);
	pCur = (char*)&poItem->m_pBuffer[iSize];
	*pCur-- = 0;
	while (pCur > (char*)poItem->m_pBuffer)
	{
		if (*pCur == 0)
			break;
		pCur--;
	}
	poItem->m_iSize = (int)(pCur - (char*)poItem->m_pBuffer);
	pCur++;
	iLen = (int)strlen(pCur);

	if (!bGet_Value(pCur, iLen, "Time Stamp", Value))
	{//没有信息，应该丢弃
		printf("No Time Stamp found");
		goto END;
	}
	poItem->m_tTime_Stamp = _atoi64(Value);

	//if (!bGet_Value(pCur, iLen, "Box Start", Value))
	//{//没有信息，应该丢弃
	//	printf("No Box Start found");
	//	goto END;
	//}
	//iResult = sscanf(Value, "%f %f %f", &poItem->m_Box_Start[0], &poItem->m_Box_Start[1], &poItem->m_Box_Start[2]);
	//if (iResult < 3)
	//{
	//	printf("No Box Start found");
	//	goto END;
	//}
	//if (!bGet_Value(pCur, iLen, "Scale", Value))
	//{//没有信息，应该丢弃
	//	printf("No Scale found");
	//	goto END;
	//}
	//poItem->m_fScale = (float)atof(Value);

	//if (!bGet_Value(pCur, iLen, "Frame No", Value))
	//{//没有信息，应该丢弃
	//	printf("No Frame No found");
	//	goto END;
	//}
	//poItem->m_iFrame_No = atoi(Value);
	poQueue->m_iCount++;
END:
	Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);
	return 1;
}
int bOut_Queue(Video_Output_Queue* poQueue, unsigned char *pOut_Buffer,int *piPoint_Count,unsigned long long *ptTime_Stamp,int *pCentroid=NULL)
{
	Video_Output_Queue_Item* poHead;
	int iSize;
	Lock_Semaphore_For_Thread(&poQueue->m_oS, 0);
	if (poQueue->m_iCount == 0)
	{//没活，走人
		Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);
		return 0;
	}
	poHead = &poQueue->m_pBuffer[poQueue->m_iHead];
	iSize = poHead->m_iPoint_Count * 15;
	*ptTime_Stamp = poHead->m_tTime_Stamp;
	*piPoint_Count = poHead->m_iPoint_Count;
	if (pCentroid)
	{
		pCentroid[0] = poHead->m_Centroid[0];
		pCentroid[1] = poHead->m_Centroid[1];
		pCentroid[2] = poHead->m_Centroid[2];
	}
	/*poItem->m_iFrame_No = poHead->m_iFrame_No;
	poItem->m_Box_Start[0] = poHead->m_Box_Start[0];
	poItem->m_Box_Start[1] = poHead->m_Box_Start[1];
	poItem->m_Box_Start[2] = poHead->m_Box_Start[2];
	poItem->m_fScale = poHead->m_fScale;*/

	memcpy(pOut_Buffer, poHead->m_pBuffer, iSize);
	poQueue->m_iHead = (poQueue->m_iHead + 1) % poQueue->m_iBuffer_Item_Count;
	poQueue->m_iCount--;
	Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);
	return 1;
}
int bOut_Queue(Video_Input_Queue* poQueue, Video_Input_Queue_Item* poItem)
{//将内容抄到poItem，出队队头
	Video_Input_Queue_Item* poHead;
	Lock_Semaphore_For_Thread(&poQueue->m_oS, 0);
	if (poQueue->m_iCount == 0)
	{//没活，走人
		Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);
		return 0;
	}
	poHead = &poQueue->m_pBuffer[poQueue->m_iHead];
	poItem->m_tTime_Stamp = poHead->m_tTime_Stamp;
	poItem->m_iSize = poHead->m_iSize;
	if (poItem->m_iSize >= poQueue->m_iMax_Item_Size)
	{
		printf("Data size exceeds max_item_size\n");
		assert(false);
	}
	poItem->m_iFrame_No = poHead->m_iFrame_No;
	poItem->m_Box_Start[0] = poHead->m_Box_Start[0];
	poItem->m_Box_Start[1] = poHead->m_Box_Start[1];
	poItem->m_Box_Start[2] = poHead->m_Box_Start[2];
	poItem->m_fScale = poHead->m_fScale;

	memcpy(poItem->m_pBuffer, poHead->m_pBuffer, poItem->m_iSize);
	poQueue->m_iHead = (poQueue->m_iHead + 1) % poQueue->m_iBuffer_Item_Count;
	poQueue->m_iCount--;
	Unlock_Semaphore_For_Thread(&poQueue->m_oS, 0);
	return 1;
}
void Init_Codec_Env(int bColor_Transform)
{//为编码器初始化一些表，一些全局常量
	//一下两表并不高明，乃大内存，很有可能存在查找超出Cache的问题
	if (bColor_Transform)
		Gen_YUY2RGB_Map();

	//生成由Occupancy到占位个数的表，[Occupancy]即可返回最多8个的表
	unsigned char* pCur;
	int iOccupancy, i;
	memset(&Occupancy_Pos[0], 0, 256 * 8 * sizeof(unsigned char));
	for (iOccupancy = 0; iOccupancy < 256; iOccupancy++)
	{
		pCur = Occupancy_Pos[iOccupancy];
		for (i = 0; i < 8; i++)
		{
			if ((iOccupancy >> i) & 1)
				*pCur++ = i;
		}
	}
	return;
}

ULONG __stdcall Video_Decode_Thread(LPVOID pParam)
{
	Decode_Param* poParam = (Decode_Param*)pParam;
	if (!poParam->m_poEnv)
	{
		printf("err");
		return 0;
	}
	Video_Input_Queue* poIn_Queue = &poParam->m_poEnv->m_oIn_Video_Queue;
	Video_Output_Queue* poOut_Queue = &poParam->m_poEnv->m_oOut_Video_Queue;
	
	PCC_Decoder* poDecoder = poParam->m_poDecoder;
	Video_Input_Queue_Item oInput_Item;
	Video_Output_Queue_Item oOutput_Item;
	Init_Setting_1* poInit_Setting = &poParam->m_poEnv->m_oInit_Setting;
	int* pbStop = &poParam->m_poEnv->m_bStop;
	
	oInput_Item.m_pBuffer = (unsigned char*)malloc(poIn_Queue->m_iMax_Item_Size);
	oOutput_Item.m_pBuffer = (unsigned char*)malloc(poOut_Queue->m_iMax_Item_Size);
	unsigned int tStart;
	
	int iFrame_No = 0;
	char File[256];
	//while (1)
	while(!(*pbStop))
	{
		while (!bOut_Queue(poIn_Queue, &oInput_Item) && !(*pbStop))
			Sleep(1);

		tStart = GetTickCount();
		MCPC_Decode(poDecoder, oInput_Item.m_pBuffer, oInput_Item.m_iSize,oOutput_Item.m_pBuffer,oOutput_Item.m_iSize,&oOutput_Item.m_iPoint_Count,poInit_Setting->m_bColor_Transform, oOutput_Item.m_Centroid);
		
		if (poInit_Setting->m_bSave_New)
		{
			sprintf(File, "%s\\Dest_%04d.ply",poInit_Setting->m_Save_Path, iFrame_No);
			bSave(File, oOutput_Item.m_pBuffer, oOutput_Item.m_iPoint_Count);
			//printf("Frame No:%d File:%s\n", iFrame_No, File);
			iFrame_No++;
		}
		
		while (!bIn_Queue(poOut_Queue, oOutput_Item.m_pBuffer, oOutput_Item.m_iPoint_Count,oInput_Item.m_tTime_Stamp,oOutput_Item.m_Centroid) && !(*pbStop))
			Sleep(1);

		printf("Decode time:%dms\n",(int)(GetTickCount()-tStart));
	}

	//释放内存
	if (oInput_Item.m_pBuffer)
		free(oInput_Item.m_pBuffer);
	if (oOutput_Item.m_pBuffer)
		free(oOutput_Item.m_pBuffer);
	//poParam->m_poEnv->m_Decode_Thread_Stop[poParam->m_iIndex] = 1;
	poParam->m_bStop = 1;
	return 0;
}
SOCKET iListern(char IP[], int iPort)
{
	SOCKET iSocket;
	int iResult, iValue;
	sockaddr_in oListen;
	WSADATA oData;
	iResult = WSAStartup(MAKEWORD(2, 2), &oData);
	if (iResult != 0)
	{
		printf("Fail to WSAStartup\n");
		return 0;	//Error occurs;
	}
	iSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (!iSocket)
	{
		printf("Fail to get socket\n");
		return 0;	//error occurs;
	}
	iValue = 10000;	//侦听最长等待时间
	iResult = setsockopt(iSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&iValue, sizeof(iValue));
	if (iResult == SOCKET_ERROR)
	{
		iResult = GetLastError();
		return 0;
	}
	oListen.sin_addr.s_addr = INADDR_ANY; //绑定所有网卡
	oListen.sin_port = htons(iPort); //主机端口

	oListen.sin_family = AF_INET;
	iResult = bind(iSocket, (sockaddr*)&oListen, sizeof(oListen));
	if (iResult == SOCKET_ERROR)	//-1
	{
		iResult = GetLastError();
		printf("Fail to bind to the IP and port defined by .ini file\n");
		return 0;
	}
	iResult = listen(iSocket, 800);
	if (iResult == SOCKET_ERROR)
	{
		printf("Fail to listen\n");
		return 0;
	}
	else
	{
		printf("Start listening...\n");
	}
	return iSocket;
}
int iRecvEx(SOCKET iSocket, char* buf, int iLen)
{//purpose: pack the recv so as to improve the performance.
//Return: count of bytes if successful; SOCKET_ERROR if network error
	int i, j, iRemain, iResult;
	i = iRemain = iLen;
	j = 0;
	if (!iSocket)
		return 0;
	while (i > 0)
	{
		iResult = recv(iSocket, buf + j, iRemain, 0);
		if (!iResult || iResult == SOCKET_ERROR)
			return SOCKET_ERROR;
		j += iResult;
		i -= iResult;
		iRemain = i;
	}
	return iLen;
}
ULONG __stdcall Recv_Thread(LPVOID pParam)
{
	Decoder_Env* poEnv = (Decoder_Env*)pParam;
	SOCKET iSocket, iSocketClient;
	sockaddr_in oClient;
	Video_Input_Queue* poVideo_Queue = &poEnv->m_oIn_Video_Queue;
	Video_Input_Queue_Item oItem = { 0 };
	int iResult, iLen, iData_Size, iVideo_Frame_No = 0;
	char* pCur,Type[32];

	oItem.m_pBuffer = (unsigned char*)malloc( poEnv->m_oIn_Video_Queue.m_iMax_Item_Size);

	iSocket = iListern(poEnv->m_oInit_Setting.m_Recv_From_IP, poEnv->m_oInit_Setting.m_iRecv_From_Port);
	iSocketClient = 0;
	while (!poEnv->m_bStop)
	//while(1)
	{
		iLen = sizeof(sockaddr);
		if (!iSocketClient)
		{
			iSocketClient = accept(iSocket, (sockaddr*)&oClient, &iLen);
			if (iSocketClient == INVALID_SOCKET)
			{
				iSocketClient = 0;
				continue;
			}
		}
		if (iRecvEx(iSocketClient, (char*)oItem.m_pBuffer, 8)==SOCKET_ERROR)
		{
			closesocket(iSocketClient);
			iSocketClient = 0;
		}
		if (*(unsigned int*)oItem.m_pBuffer == 0xFFFFFFFF)
		{//是了
			oItem.m_iSize = *(int*)&oItem.m_pBuffer[4];
			iResult = iRecvEx(iSocketClient, (char*)oItem.m_pBuffer, oItem.m_iSize);

			//先分离出Type
			oItem.m_pBuffer[oItem.m_iSize] = 0;
			pCur = (char*)&oItem.m_pBuffer[oItem.m_iSize - 1];
			while (*pCur != 0)
				pCur--;
			iData_Size = (int)((unsigned char*)pCur - oItem.m_pBuffer);
			if (iData_Size <= 0)
				continue;
			pCur++;
			Type[0] = 0;
			bGet_Value(pCur, (int)strlen(pCur), "Type", Type);

			if (_stricmp(Type, "Video") == 0)
			{//视频入队
				while (oItem.m_iSize < 0 || !bIn_Queue(poVideo_Queue, oItem.m_iSize, oItem.m_pBuffer, iVideo_Frame_No) && !poEnv->m_bStop)
				{
					//printf("Wait Decoder\n");
					Sleep(1);
				}
				//printf("In Queue\n");
				iVideo_Frame_No++;
			}
		}
	}

	//释放
	if (iSocketClient)
		closesocket(iSocketClient);
	if(iSocket)
	{
		WSAAsyncSelect(iSocket, NULL, FD_CLOSE, 0);
		shutdown(iSocket, 2);
		closesocket(iSocket);
	}
		
	free(oItem.m_pBuffer);
	poEnv->m_bRecv_Thread_Stop = 1;
	

	return 0;
}
//void Get_Data_1(void* poEnv, unsigned char* pBuffer, int* piPoint_Count, unsigned long long* ptTime_Stamp,int Centroid[3])
//{//这里已经算是一个接口
//	Video_Output_Queue* poQueue = &((Decoder_Env*)poEnv)->m_oOut_Video_Queue;
//	while (!bOut_Queue(poQueue, pBuffer, piPoint_Count, ptTime_Stamp))
//		Sleep(1);
//	//Sleep(INFINITE);
//}
void Get_Data_1(void* poEnv, unsigned char* pBuffer, int* piPoint_Count, unsigned long long* ptTime_Stamp,int *pCentroid)
{//这里已经算是一个接口
	Video_Output_Queue* poQueue = &((Decoder_Env*)poEnv)->m_oOut_Video_Queue;
	while (!bOut_Queue(poQueue, pBuffer, piPoint_Count, ptTime_Stamp,pCentroid))
		Sleep(1);
	//Sleep(INFINITE);
}
void Free_Env_1(void* p)
{
	Decoder_Env* poEnv =(Decoder_Env*)p;
	poEnv->m_bStop = 1;

	//先等Recv_Thread停止
	while (!poEnv->m_bRecv_Thread_Stop)
		Sleep(1);
	//printf("Recv_Thread Stop\n");
	for (int i = 0; i < poEnv->m_oInit_Setting.m_iDecoder_Count; i++)
	{
		//while (!poEnv->m_Decode_Thread_Stop[i])
		while(! ((Decode_Param*)poEnv->m_pDecode_Param)[i].m_bStop)
			Sleep(1);
		Free_Decoder(((Decode_Param*)poEnv->m_pDecode_Param)[i].m_poDecoder);
		free(((Decode_Param*)poEnv->m_pDecode_Param)[i].m_poDecoder);
	}
	
	//printf("Decode Threads Stop\n");
	Free_Queue(&poEnv->m_oIn_Video_Queue);
	Free_Queue(&poEnv->m_oOut_Video_Queue);
	free(poEnv->m_pDecode_Param);
	free(poEnv);
	
	Free_Codec_Env();
	return;
}
void* pInit_Env_1()
{
	WSADATA oData;
	int i,iResult, iMax_Point_Count = 500000;
	Decoder_Env* poEnv = (Decoder_Env*)malloc(sizeof(Decoder_Env));
	PCC_Decoder *poDecoder;
	//这里小心，千万不能用数组，因为出去就没了
	Decode_Param *pDecode_Param=(Decode_Param*)malloc(16*sizeof(Decode_Param));
	memset(poEnv, 0, sizeof(Decoder_Env));
	poEnv->m_pDecode_Param = pDecode_Param;

	iResult = WSAStartup(MAKEWORD(2, 2), &oData);
	if (!bLoad_Setting((char*)"Setting.ini", &poEnv->m_oInit_Setting))
	{
		printf("Fail to load setting.ini\n");
		return NULL;
	}
	Init_Codec_Env(poEnv->m_oInit_Setting.m_bColor_Transform);	//需要YUV颜色空间才需要开辟表
	
	Init_Queue(&poEnv->m_oIn_Video_Queue, 2, 2 * 1024 * 1024);
	Init_Queue(&poEnv->m_oOut_Video_Queue, 2, iMax_Point_Count*15);

	CloseHandle(CreateThread(0, 0, Recv_Thread, poEnv, 0, 0));

	for (i = 0; i < poEnv->m_oInit_Setting.m_iDecoder_Count; i++)
	{
		poDecoder = (PCC_Decoder*)malloc(sizeof(PCC_Decoder));
		Init_Decoder(poDecoder, 100000, Pre_Process_Type::Full_Search);		//此处只做最基本的初值
		pDecode_Param[i] = { poDecoder,poEnv,i,0 };
		/*pDecode_Param[i].m_poDecoder = poDecoder;
		pDecode_Param[i].m_poEnv = poEnv;
		pDecode_Param[i].m_iIndex = i;
		pDecode_Param[i].m_bStop = 0;*/

		CloseHandle(CreateThread(0, 0, Video_Decode_Thread, &pDecode_Param[i], 0, 0));
	}
	return poEnv;
}