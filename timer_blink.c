
#include "F28x_Project.h"

// CABEÇALHO DE FUNÇÕES
void setup_gpio(void);
__interrupt void ISR_TIMER0(void);
__interrupt void ISR_TIMER1(void);

// VARIÁVEIS GLOBAIS
uint32_t count = 0;

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

    PieVectTable.TIMER0_INT = &ISR_TIMER0;
    PieVectTable.TIMER1_INT = &ISR_TIMER1;
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1; // HABILITA INTERRUPÇÃO COLUNA 7

    EDIS;

    IER |= M_INT1;  // HABILITA INTERRUPÇÃO LINHA 1
    IER |= M_INT13; // HABILITA INTERRUPÇÃO LINHA 13

    InitCpuTimers();

    // SETUP TIMER0
    ConfigCpuTimer(&CpuTimer0, 200, 1000000);
    CpuTimer0Regs.TCR.all = 0x4000;

    // SETUP TIMER1
    ConfigCpuTimer(&CpuTimer1, 200, 500000);
    CpuTimer1Regs.TCR.all = 0x4000;

    // SETUP DO GPIO
    setup_gpio();

    EINT;   // HABILITA AS INTERRUPÇÕES GLOBAIS
    ERTM;   // HABILITA AS INTERRUPÇÕES REALTIME

    GpioDataRegs.GPBDAT.bit.GPIO34 = 0;
    GpioDataRegs.GPADAT.bit.GPIO31 = 1;

    while(1)
    {

    }
}

void setup_gpio(void)
{
    EALLOW; // HABILITA ESCRITA EM REGISTRADORES PROTEGIDOS

    // GPIO34
    GpioCtrlRegs.GPBGMUX1.bit.GPIO34 = 0;   // GPIO
    GpioCtrlRegs.GPBPUD.bit.GPIO34 = 1;     // PULLUP
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1;     // OUTPUT -- VALIDO PARA GPIOS
    GpioCtrlRegs.GPBCSEL1.bit.GPIO34 = GPIO_MUX_CPU1; // CPU1 MASTER -- DEFAULT

    // GPIO31
    GpioCtrlRegs.GPAGMUX2.bit.GPIO31 = 0;   // GPIO
    GpioCtrlRegs.GPAPUD.bit.GPIO31 = 1;     // PULLUP
    GpioCtrlRegs.GPADIR.bit.GPIO31 = 1;     // OUTPUT -- VALIDO PARA GPIOS

    EDIS;   // DESABILITA ESCRITA EM REGISTRADORES PROTEGIDOS
}

__interrupt void ISR_TIMER0(void)
{
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
    GpioDataRegs.GPBTOGGLE.bit.GPIO34 = 1;
}

__interrupt void ISR_TIMER1(void)
{
    GpioDataRegs.GPATOGGLE.bit.GPIO31 = 1;
}
