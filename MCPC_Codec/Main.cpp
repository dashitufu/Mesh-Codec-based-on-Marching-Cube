#include "MCPC_Codec.h"
static void Test_1()
{
    MC* pMC;
    PCC_Point* pPoint = NULL;
    int i, iMC_Count, iPoint_Count;
    char File[256];
    unsigned int tStart;
    Mem_Mgr* poMem_Mgr;
    Init_Codec_Env(1);
    Free_Codec_Env();
    PCC_Encoder* poEncoder = (PCC_Encoder*)malloc(sizeof(PCC_Encoder));
    Init_Encoder(poEncoder, 100000, 1, 1, 0, 22, Pre_Process_Type::Full_Search);	//此处只做最基本的初值
    poMem_Mgr = &poEncoder->m_oMem_Mgr;
    //bRead_PLY_File("d:\\chi0000.ply", &poEncoder->m_oMem_Mgr,&pPoint,&iPoint_Count);
    //for (i = 0; i < 100; i++)
    i = 0;
    {
        sprintf(File, "d:\\sample\\chi\\chi%04d.ply", i);
        if (!bRead_PLY_File(File, &poEncoder->m_oMem_Mgr, &pPoint, &iPoint_Count) || iPoint_Count == 0)
        {
            printf("File:%s Point Count : %d\n", File, iPoint_Count);
            return;
        }
        tStart = GetTickCount();
        
        bGen_MC(&poEncoder->m_oMem_Mgr, pPoint, iPoint_Count, 0.005333334f, 255.f, &pMC, &iMC_Count);
        
        Free(poMem_Mgr, pPoint);
        pPoint = (PCC_Point*)pMalloc(poMem_Mgr, iMC_Count * sizeof(PCC_Point));
        Reset_Point(poEncoder, pPoint, pMC, iMC_Count);
        
        Encode(poEncoder);
        
        Resort_MC(&poEncoder->m_oMem_Mgr, poEncoder->m_oGeometry_Brick.m_pOrder_Point, poEncoder->m_oGeometry_Brick.m_oHeader.geom_num_points, &pMC, &iMC_Count);
        //此处开始干MC内的点
        Encode_MC(poEncoder, pMC, iMC_Count,NULL);
        
        Free(poMem_Mgr,pPoint);
        Free(poMem_Mgr, poEncoder->m_oGeometry_Brick.m_pOrder_Point);
        
        Encode_Color_1(poEncoder, pMC, iMC_Count, iPoint_Count, 8);

        printf("Points:%d %ums Size:%d\n",iPoint_Count, GetTickCount() - tStart, poEncoder->m_iOut_Data_Size);
        
        sprintf(File, "c:\\tmp\\temp\\%04d.bin", i);
        if (!bSave(poEncoder, File))
            return;
        sprintf(File, "c:\\tmp\\temp\\Org_%04d.ply", i);
        bSave(File, pMC, iMC_Count, (float)256.f, 1,poEncoder->m_oSPS.seq_bounding_box.seq_bounding_box_xyz);
        Free_All(&poEncoder->m_oMem_Mgr);
        //return 0;
    }
    Free_Encoder(poEncoder);
    printf("End\n");
}
int main()
{
    Test_1();
    //return 0;

    //Ming_Yuan_Test();
    _CrtDumpMemoryLeaks();
    return 0;
}