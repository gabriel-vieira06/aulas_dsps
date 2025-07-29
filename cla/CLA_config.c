/*
 * CLA_config.c
 *
 *  Created on: 29 de jul de 2025
 *      Author: Vieira
 */

#include "CLA_config.h"

void CLA1_ConfigCLAMemory(void)
{
    extern uint32_t Cla1funcsRunStart, Cla1funcsLoadStart, Cla1funcsLoadSize;
    EALLOW;
#ifdef _FLASH
    memcpy((uint32_t *)&Cla1funcsRunStart, (uint32_t *)&Cla1funcsLoadStart, (uin32_t *)&Cla1funcsLoadSize);
#endif //_FLASH

    MemCfgRegs.MSGxINIT.bit.INIT_CLA1TOCPU = 1;
    while(MemCfgRegs.MSGxINITDONE.bit.INITDONE_CLA1TOCPU != 1){};
    MemCfgRegs.MSGxINIT.bit.INIT_CPUTOCLA1 = 1;
    while(MemCfgRegs.MSGxINITDONE.bit.INITDONE_CPUTOCLA1 != 1){};

    MemCfgRegs.LSxMSEL.bit.MSEL_LS5 = 1;
    MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS5 = 1;
    MemCfgRegs.LSxMSEL.bit.MSEL_LS0 = 1;
    MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS0 = 1;
    MemCfgRegs.LSxMSEL.bit.MSEL_LS1 = 1;
    MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS1 = 1;
    EDIS;
}

void CLA1_InitCpu1Cla1(void)
{
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.CLA1 = 1;
    CpuSysRegs.PCLKCR0.bit.DMA = 1;

    Cla1Regs.MVECT1 = (uint16_t)(&CLA1Task1);
    Cla1Regs.MVECT2 = (uint16_t)(&CLA1Task2);
    Cla1Regs.MVECT3 = (uint16_t)(&CLA1Task3);
    Cla1Regs.MVECT4 = (uint16_t)(&CLA1Task4);
    Cla1Regs.MVECT5 = (uint16_t)(&CLA1Task5);
    Cla1Regs.MVECT6 = (uint16_t)(&CLA1Task6);
    Cla1Regs.MVECT7 = (uint16_t)(&CLA1Task7);
    Cla1Regs.MVECT8 = (uint16_t)(&CLA1Task8);

    Cla1Regs.MCTL.bit.IACKE = 1;
    Cla1Regs.MIER.all = M_INT1;

    PieVectTable.CLA1_1_INT = &CLA1_isr1;
    PieVectTable.CLA1_2_INT = &CLA1_isr2;
    PieVectTable.CLA1_3_INT = &CLA1_isr3;
    PieVectTable.CLA1_4_INT = &CLA1_isr4;
    PieVectTable.CLA1_5_INT = &CLA1_isr5;
    PieVectTable.CLA1_6_INT = &CLA1_isr6;
    PieVectTable.CLA1_7_INT = &CLA1_isr7;
    PieVectTable.CLA1_8_INT = &CLA1_isr8;

    DmaClaSrcSelRegs.CLA1TASKSRCSEL1.bit.TASK1 = 1;
    DmaClaSrcSelRegs.CLA1TASKSRCSELLOCK.bit.CLA1TASKSRCSEL1 = 0;
    DmaClaSrcSelRegs.CLA1TASKSRCSELLOCK.bit.CLA1TASKSRCSEL2 = 0;

    EDIS;


}
