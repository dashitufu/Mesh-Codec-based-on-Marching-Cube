#pragma once
#include "windows.h"
#include "PCC_Decoder.h"
extern unsigned char(*YUV_2_RGB)[256][256][3];
typedef struct MC_Point {
	union {
		struct {
			unsigned char m_Color[3];
			unsigned char m_iPos;
		};
		unsigned int m_iColor_Pos;
	};
}MC_Point;

typedef struct MC {	//����ָһ��MC
	union {		//������һ������
		float Pos_f[3];
		int Pos_i[3];
		PCC_Pos m_oPos;
	};
	MC_Point m_Point[3];			//����Ժ�һ��MC���ֻ�������㣬�ֱ�������������
	unsigned int m_iOccupancy : 3;	//zyx ���У� x����0λ
	unsigned int m_iNext : 29;		//ɢ�б�������������
}MC;
void MCPC_Decode(PCC_Decoder* poDecoder, unsigned char* pBuffer, int iSize, unsigned char* pOut_Buffer, int iBuffer_Size, int* piPoint_Count, int bColor_Transform, int Centroid[3]);
void Decode_MC(PCC_Decoder* poDecoder, unsigned char* pBuffer, int iBuffer_Size, PCC_Point Point[], int iPoint_Count, MC** ppMC, int* piMC_Count, int* piPoint_Count, BitPtr* poBitPtr);
void Decode_Color(PCC_Decoder* poDecoder, BitPtr* poBitPtr, MC* pMC, int iMC_Count, int iPoint_Count, int iMC_Shift_Bit, int bColor_Transform);
void Get_Data_1(void* poEnv, unsigned char* pBuffer, int* piPoint_Count, unsigned long long* ptTime_Stamp, int* pCentroid=NULL);
int bSave(const char* pcFile, MC* pMC, int iMC_Count, float fScale, int bColor);
int bSave(const char* pcFile, unsigned char* pBuffer, int iPoint_Count, int bText=1, int bColor=1);
int bSave(const char* pcFile, PCC_Point Point[], int iPoint_Count, int bColor=1);

void* pInit_Env_1();
void Free_Env_1(void* p);