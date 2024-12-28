#include "PCC_Encoder.h"
typedef struct MC_Point {
	/*union {
		float Pos_f[3];
		int Pos_i[3];
		PCC_Pos m_oPos;
	};
	union {
		unsigned int m_iColor;
		unsigned char m_Color[4];
	};*/
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
	unsigned int m_iNext:29;		//ɢ�б�������������
}MC;
int bGen_MC(Mem_Mgr* poMem_Mgr, PCC_Point Point[], int iPoint_Count, float fMC_Size, float fScale, MC** ppMC, int* piMC_Count);
int bRead_PLY_Header(FILE* pFile, int* piPos_Float_Double, int* pbHas_Normal, int* pbHas_Color,
	int* pbHas_Alpha, int* pbHas_Face, int* piPoint_Count, int* piFace_Count, int* pbText);
int bRead_PLY_File(const char* pcFile, Mem_Mgr* poMem_Mgr, PCC_Point** ppPoint, int* piPoint_Count);
int bSave(const char* pcFile, MC* pMC, int iMC_Count, float fScale, int bColor, int Min[3]);
void Reset_Point(PCC_Encoder* poEncoder, PCC_Point Point[], MC* pMC, int iMC_Count);
void Resort_MC(Mem_Mgr* poMem_Mgr, PCC_Point* poPoint, int iPoint_Count, MC** ppMC, int* piMC_Count);
void Encode_MC(PCC_Encoder* poEncoder, MC* pMC, int iMC_Count, int* piPoint_Count);
void Encode_Color_1(PCC_Encoder* poEncoder, MC* pMC, int iMC_Count, int iPoint_Count, int iMC_Shift_Bit);

void Ming_Yuan_Test();
void* pInit_Env_1();
void MCPC_Encode(PCC_Encoder* poEncoder, unsigned char* pBuffer, int iPoint_Count, float fMC_Size, float fScale, int* piPoint_Count);
int bSave(const char* pcFile, PCC_Point Point[], int iPoint_Count, int bHas_Color);
void Send_Data_1(void* pEnv, unsigned char* pBuffer, int iPoint_Count, unsigned long long tTime_Stamp);

