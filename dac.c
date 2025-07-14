#include "F28x_Project.h"
#include "math.h"

// CABEÇALHO DE FUNÇÕES
__interrupt void ISR_TIMER1(void);
void setup_dac(void);

// VARIÁVEIS GLOBAIS
uint16_t index = 0, offset1 = 0, offset2 = 0, senotable[400], costable[400];

void main(void)
{

    InitSysCtrl();  // INICIALIZA CPU
    DINT;           // DESABILITA INTERRUPÇÕES

    InitPieCtrl();

    IER = 0x0000;   // DESABILITA INTERRUPÇÕES DA CPU
    IFR = 0x0000;   // LIMPA AS FLAGS DE INTERRUPÇÃO

    InitPieVectTable();

    // INTERRUPÇÕES
    EALLOW;

    PieVectTable.TIMER1_INT = &ISR_TIMER1;
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1; // HABILITA INTERRUPÇÃO COLUNA 7

    EDIS;

    IER |= M_INT13; // HABILITA INTERRUPÇÃO LINHA 13

    InitCpuTimers();

    // SETUP TIMER1
    ConfigCpuTimer(&CpuTimer1, 200, 50);
    CpuTimer1Regs.TCR.all = 0x4000;

    // SETUP DAC
    setup_dac();

    // GERA TABELAS SENO E COSSENO
    for (index = 0; index < 400; index++){
        senotable[index] = (uint16_t)(1000.0*(1.0 + sin(6.28318531/400.0*(float)index)));
        costable[index] =  (uint16_t)(1000.0*(1.0 + cos(6.28318531/400.0*(float)index)));
    }

    index = 0;

    EINT;   // HABILITA AS INTERRUPÇÕES GLOBAIS
    ERTM;   // HABILITA AS INTERRUPÇÕES REALTIME


    while(1){}
}

__interrupt void ISR_TIMER1(void)
{
  
    index = (index == 399) ? 0 : (index + 1);
    DacaRegs.DACVALS.all = offset1 + senotable[index];
    DacbRegs.DACVALS.all = offset2 + costable[index];

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

void setup_dac(void)
{

    EALLOW; // HABILITA ESCRITA EM REGISTRADORES PROTEGIDOS

    // PIN 70
    DacaRegs.DACCTL.all = 0x0001;       // BIT0 = 3v3 (0) ou 3v (1)
    DacaRegs.DACVALS.all = 0x0800;      // SET DAC A TO MID RANGE
    DacaRegs.DACOUTEN.bit.DACOUTEN = 1; // HABILITA DAC A COMO OUTPUT
    DacaRegs.DACLOCK.all = 0x0000;      // LOCK = 1 IMPEDE ALTERAÇÕES

    // PIN 30
    DacbRegs.DACCTL.all = 0x0001;       // BIT0 = 3v3 (0) ou 3v (1)
    DacbRegs.DACVALS.all = 0x0800;      // SET DAC B TO MID RANGE
    DacbRegs.DACOUTEN.bit.DACOUTEN = 1; // HABILITA DAC B COMO OUTPUT
    DacbRegs.DACLOCK.all = 0x0000;      // LOCK = 1 IMPEDE ALTERAÇÕES

    EDIS;   // DESABILITA ESCRITA EM REGISTRADORES PROTEGIDOS
}
