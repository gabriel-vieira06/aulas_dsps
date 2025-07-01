
#include "F28x_Project.h"

// CABEÇALHO DE FUNÇÕES
void setup_gpio(void);

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

    // SETUP DO GPIO
    setup_gpio();

    EINT;   // HABILITA AS INTERRUPÇÕES GLOBAIS
    ERTM;   // HABILITA AS INTERRUPÇÕES REALTIME

    GpioDataRegs.GPBDAT.bit.GPIO34 = 0;
    GpioDataRegs.GPADAT.bit.GPIO31 = 1;

    while(1)
    {
        for(count = 0; count < 0x00FFFFFF; count++){}
        GpioDataRegs.GPBTOGGLE.bit.GPIO34 = 1;
        GpioDataRegs.GPATOGGLE.bit.GPIO31 = 1;
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
