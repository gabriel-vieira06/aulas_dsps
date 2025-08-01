#include "F28x_Project.h"

#include "sogi.h"
#include "CLA_config.h"

// CABEÇALHO DE FUNÇÕES
void setup_dac(void);
void setup_adc(void);
void setup_epwm(void);
void setup_gpio(void);
__interrupt void ISR_TIMER1(void);
__interrupt void ISR_ADC(void);

// VARIÁVEIS GLOBAIS
uint32_t count = 0, index = 0;

SPLL_SOGI v_pll;
float vrede = 0;
float vsync = 0;
float phase = 0;
float ampl = 0.5;

volatile SPLL_SOGI cla1_pll;
volatile float vrede_CLA;
volatile float phase_CLA = 0;
volatile float ampl_CLA = 0.5;
volatile uint32_t count_task = 0;

#pragma DATA_SECTION(vrede_CLA, "Cla1ToCpuMsgRAM");

#pragma DATA_SECTION(cla1_pll, "CLADataLS0");
#pragma DATA_SECTION(phase_CLA, "CLADataLS0");
#pragma DATA_SECTION(ampl_CLA, "CLADataLS0");
#pragma DATA_SECTION(count_task, "CLADataLS0");

float plot1[512], plot2[512];
float *padc1 = &vrede;
float *padc2 = &vsync;

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

    PieVectTable.TIMER1_INT       = &ISR_TIMER1;
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;          // HABILITA INTERRUPÇÃO COLUNA 7

    PieVectTable.ADCA1_INT        = &ISR_ADC;
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;          // HABILITA INTERRUPÇÃO COLUNA 1

    EDIS;

    IER |= M_INT1;   // HABILITA INTERRUPÇÃO LINHA 1
    IER |= M_INT13;  // HABILITA INTERRUPÇÃO LINHA 13

    InitCpuTimers();

    // SETUP TIMER1
    ConfigCpuTimer(&CpuTimer1, 200, 1000000);
    CpuTimer1Regs.TCR.all = 0x4000;

    // SETUP DAC
    setup_dac();

    // SETUP ADC
    setup_adc();

    // SETUP PWM
    setup_epwm();

    // SETUP GPIO
    setup_gpio();

    // SETUP CLA
    CLA1_ConfigCLAMemory();
    CLA1_InitCpu1Cla1();

    SOGI_init(60, 32.5520833E-06, &v_pll);
    SOGI_coeff_update(32.5520833E-06, 376.99112, 0.7, &v_pll);

    SOGI_init(60, 32.5520833E-06, &cla1_pll);
    SOGI_coeff_update(32.5520833E-06, 376.99112, 0.7, &cla1_pll);

    EINT;   // HABILITA AS INTERRUPÇÕES GLOBAIS
    ERTM;   // HABILITA AS INTERRUPÇÕES REALTIME

    while(1){}
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

void setup_adc(void)
{
    Uint16 acqps;

    // DETERMINA A JANELA DE AQUISIÇÃO MÍNIMA EM SYSCLKS BASEADO NA RESOLUÇÃO
    // 12 BITS => 15 CLKS == 15ns
    // 16 BITS => 64 CLKS == 320ns
    acqps = (ADC_RESOLUTION_12BIT == AdcaRegs.ADCCTL2.bit.RESOLUTION) ? 14 : 63;

    EALLOW;

    CpuSysRegs.PCLKCR13.bit.ADC_A     = 1;
    AdcaRegs.ADCCTL2.bit.PRESCALE     = 6;

    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);

    AdcaRegs.ADCCTL1.bit.INTPULSEPOS  = 1;   // SETA PULSO UM CICLO ANTES DO RESULTADO
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ     = 1;   // ENERGIZA ADC
    DELAY_US(1000);

    AdcaRegs.ADCSOC0CTL.bit.CHSEL     = 3;      // ADCINA3 - J3-26
    AdcaRegs.ADCSOC0CTL.bit.ACQPS     = acqps;  // JANELA DE AMOSTRAGEM É 15 CICLOS DE SYSCLK
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL   = 0x17;   // SELECIONA EPWM10 COMO FONTE DO GATILHO DA CONVERSÃO

    AdcaRegs.ADCSOC1CTL.bit.CHSEL     = 4;      // ADCINA4 - J7-69
    AdcaRegs.ADCSOC1CTL.bit.ACQPS     = acqps;
    AdcaRegs.ADCSOC1CTL.bit.TRIGSEL   = 0x17;   // SELECIONA EPWM10 COMO FONTE DO GATILHO DA CONVERSÃO

    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 0x01;   // AO FIM DE SOC1 SERÁ SETADO A FLAG INT1
    AdcaRegs.ADCINTSEL1N2.bit.INT1E   = 1;      // HABILITA FLAG INT1
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;      // GARANTE QUE INT1 ESTÁ ZERADA

    EDIS;
}

void setup_epwm(void)
{
    EALLOW;

    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0;

    // ------------
    // SETUP EPWM10
    // ------------

    CpuSysRegs.PCLKCR2.bit.EPWM10  = 1;                // HABILITA CLOCK

    EPwm10Regs.TBPRD               = 3255;             // LAB ADC
    EPwm10Regs.TBPHS.bit.TBPHS     = 0;                // FASE É 0
    EPwm10Regs.TBCTL.bit.SYNCOSEL  = TB_CTR_ZERO;
    EPwm10Regs.TBCTR               = 0x0000;           // LIMPA CONTADOR
    EPwm10Regs.TBCTL.bit.CTRMODE   = TB_COUNT_UPDOWN;  // CONTA PRA CIMA E PARA BAIXO
    EPwm10Regs.TBCTL.bit.PHSEN     = TB_DISABLE;       // DESABILITA CARGA DE FASE
    EPwm10Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;          // DIVISOR DE CLOCK EM RELAÇÃO A SYSCLKOUT
    EPwm10Regs.TBCTL.bit.CLKDIV    = TB_DIV1;

    // LARGURA DO PULSO DE 50%
    EPwm10Regs.CMPA.bit.CMPA = EPwm10Regs.TBPRD >> 1;

    EPwm10Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;     // CARREGA REGISTRADORES A CADA ZERO
    EPwm10Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO_PRD;
    EPwm10Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;     // CARREGA REGISTRADORES A CADA ZERO
    EPwm10Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO_PRD;

    EPwm10Regs.AQCTLA.bit.PRD = AQ_NO_ACTION;       // CARREGA AÇÕES PARA EPWM10A
    EPwm10Regs.AQCTLA.bit.ZRO = AQ_NO_ACTION;
    EPwm10Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm10Regs.AQCTLA.bit.CAD = AQ_SET;

    // ACIONA ADC
    EPwm10Regs.ETSEL.bit.SOCAEN  = 1;                // HABILITA SOC NO GRUPO A
    EPwm10Regs.ETSEL.bit.SOCASEL = ET_CTR_PRDZERO;   // DISPARA ADC NO TOPO
    EPwm10Regs.ETPS.bit.SOCAPRD  = ET_1ST;           // ACIONA EM CADA EVENTO

    // ------------
    // FIM SETUP EPWM10
    // ------------

    // -----------
    // SETUP EPWM1
    // ------------
    CpuSysRegs.PCLKCR2.bit.EPWM1     = 1;            // HABILITA CLOCK

    EPwm1Regs.TBPRD                = 50000;              // PERIODO = Fosc/2*Fpwm OU Fosc/4*Fpwm
    EPwm1Regs.TBPHS.bit.TBPHS      = 0;                  // FASE
    EPwm1Regs.TBCTL.bit.SYNCOSEL   = TB_SYNC_DISABLE;
    EPwm1Regs.TBCTR                = 0x0000;
    EPwm1Regs.TBCTL.bit.CTRMODE    = TB_COUNT_UPDOWN;
    EPwm1Regs.TBCTL.bit.PHSEN      = TB_DISABLE;
    EPwm1Regs.TBCTL.bit.HSPCLKDIV  = TB_DIV1;
    EPwm1Regs.TBCTL.bit.CLKDIV     = TB_DIV1;

    // LARGURA DO PULSO DE 50%
    EPwm1Regs.CMPA.bit.CMPA        = EPwm1Regs.TBPRD >> 1;

    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;     // CARREGA REGISTRADORES A CADA ZERO
    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO_PRD;
    EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;     // CARREGA REGISTRADORES A CADA ZERO
    EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO_PRD;

    EPwm1Regs.AQCTLA.bit.PRD = AQ_NO_ACTION;       // CARREGA AÇÕES PARA EPWM1A
    EPwm1Regs.AQCTLA.bit.ZRO = AQ_NO_ACTION;
    EPwm1Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm1Regs.AQCTLA.bit.CAD = AQ_SET;

    EPwm1Regs.DBCTL.bit.POLSEL   = DB_ACTV_HIC;     // ACTIVE HIGH COMPLEMENTARY
    EPwm1Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;  // HABILITA DEAD-BAND
    EPwm1Regs.DBFED.bit.DBFED    = 44;              // TIME = DBxED*2*TBclk
    EPwm1Regs.DBRED.bit.DBRED    = 9;

    // ------------
    // FIM SETUP EPWM1
    // ------------

    // ------------
    // SETUP EPWM7
    // ------------

    CpuSysRegs.PCLKCR2.bit.EPWM7     = 1;            // HABILITA CLOCK

    EPwm7Regs.TBPRD                = 3255;              // PERIODO = Fosc/2*Fpwm OU Fosc/4*Fpwm
    EPwm7Regs.TBPHS.bit.TBPHS      = 0;                 // FASE
    EPwm7Regs.TBCTL.bit.SYNCOSEL   = TB_CTR_ZERO;
    EPwm7Regs.TBCTR                = 0x0000;
    EPwm7Regs.TBCTL.bit.CTRMODE    = TB_COUNT_UPDOWN;
    EPwm7Regs.TBCTL.bit.PHSEN      = TB_DISABLE;
    EPwm7Regs.TBCTL.bit.HSPCLKDIV  = TB_DIV1;
    EPwm7Regs.TBCTL.bit.CLKDIV     = TB_DIV1;

    EPwm7Regs.CMPA.bit.CMPA        = 0;

    EPwm7Regs.TBCTL.bit.PRDLD      = TB_SHADOW;
    EPwm7Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;     // CARREGA REGISTRADORES A CADA ZERO
    EPwm7Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO_PRD;

    EPwm7Regs.AQCTLA.bit.PRD = AQ_NO_ACTION;       // CARREGA AÇÕES PARA EPWM7A
    EPwm7Regs.AQCTLA.bit.ZRO = AQ_NO_ACTION;
    EPwm7Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm7Regs.AQCTLA.bit.CAD = AQ_SET;

    EPwm7Regs.DBCTL.bit.POLSEL   = DB_ACTV_HIC;     // ACTIVE HIGH COMPLEMENTARY
    EPwm7Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;  // HABILITA DEAD-BAND
    EPwm7Regs.DBFED.bit.DBFED    = 44;              // TIME = DBxED*2*TBclk
    EPwm7Regs.DBRED.bit.DBRED    = 9;

    // ------------
    // FIM SETUP EPWM7
    // ------------

    // ------------
    // SETUP EPWM8
    // ------------

    CpuSysRegs.PCLKCR2.bit.EPWM8     = 1;            // HABILITA CLOCK

    EPwm8Regs.TBPRD                = 3255;              // PERIODO = Fosc/2*Fpwm OU Fosc/4*Fpwm
    EPwm8Regs.TBPHS.bit.TBPHS      = 0;                 // FASE
    EPwm8Regs.TBCTL.bit.SYNCOSEL   = TB_SYNC_IN;
    EPwm8Regs.TBCTR                = 0x0000;
    EPwm8Regs.TBCTL.bit.CTRMODE    = TB_COUNT_UPDOWN;
    EPwm8Regs.TBCTL.bit.PHSEN      = TB_ENABLE;
    EPwm8Regs.TBCTL.bit.PHSDIR     = TB_DOWN;
    EPwm8Regs.TBCTL.bit.HSPCLKDIV  = TB_DIV1;
    EPwm8Regs.TBCTL.bit.CLKDIV     = TB_DIV1;

    // LARGURA DO PULSO DE 50%
    EPwm8Regs.CMPA.bit.CMPA        = 0;

    EPwm8Regs.TBCTL.bit.PRDLD      = TB_SHADOW;
    EPwm8Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;     // CARREGA REGISTRADORES A CADA ZERO
    EPwm8Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO_PRD;

    EPwm8Regs.AQCTLA.bit.PRD = AQ_NO_ACTION;       // CARREGA AÇÕES PARA EPWM8A
    EPwm8Regs.AQCTLA.bit.ZRO = AQ_NO_ACTION;
    EPwm8Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm8Regs.AQCTLA.bit.CAD = AQ_SET;

    EPwm8Regs.DBCTL.bit.POLSEL   = DB_ACTV_HIC;     // ACTIVE HIGH COMPLEMENTARY
    EPwm8Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;  // HABILITA DEAD-BAND
    EPwm8Regs.DBFED.bit.DBFED    = 44;              // TIME = DBxED*2*TBclk
    EPwm8Regs.DBRED.bit.DBRED    = 9;

    // ------------
    // FIM SETUP EPWM8
    // ------------

    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;

    EDIS;
}

void setup_gpio(void)
{
    EALLOW;

    // IO 0 - EPWM1A (J4 - 40)
    GpioCtrlRegs.GPAGMUX1.bit.GPIO0 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO0  = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO0   = 1;
    GpioCtrlRegs.GPACSEL1.bit.GPIO0 = GPIO_MUX_CPU1;

    // IO 1 - EPWM1B (J4 - 39)
    GpioCtrlRegs.GPAGMUX1.bit.GPIO1 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO1  = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO1   = 1;
    GpioCtrlRegs.GPACSEL1.bit.GPIO1 = GPIO_MUX_CPU1;

    // PWM 7A DAC3 (J8 - 72)
    GpioCtrlRegs.GPEGMUX2.bit.GPIO157 = 0;
    GpioCtrlRegs.GPEMUX2.bit.GPIO157  = 1;
    GpioCtrlRegs.GPEPUD.bit.GPIO157   = 1;

    // PWM 8A DAC1 (J4 - 32)
    GpioCtrlRegs.GPEGMUX2.bit.GPIO159 = 0;
    GpioCtrlRegs.GPEMUX2.bit.GPIO159  = 1;
    GpioCtrlRegs.GPEPUD.bit.GPIO159   = 1;

    // GPIO 14 (J8 - 74)
    GpioCtrlRegs.GPAGMUX1.bit.GPIO14 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO14 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO14 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO14 = 1;
    GpioCtrlRegs.GPACSEL2.bit.GPIO14 = GPIO_MUX_CPU1;

    // GPIO 15 (J8 - 73)
    GpioCtrlRegs.GPAGMUX1.bit.GPIO15 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO15 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO15 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO15 = 1;
    GpioCtrlRegs.GPACSEL2.bit.GPIO15 = GPIO_MUX_CPU1CLA;

    EDIS;
}

__interrupt void ISR_TIMER1(void)
{

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

__interrupt void ISR_ADC(void)
{
    GpioDataRegs.GPADAT.bit.GPIO14 = 1;
    vrede = 0.0005*((int)AdcaResultRegs.ADCRESULT0 - 0x7FF);
    v_pll.u[0] = vrede;
    SPLL_SOGI_CALC(&v_pll);

    vsync = v_pll.sin_;

    EPwm7Regs.CMPA.bit.CMPA = (uint16_t) (1627.0 * (1.0 + ampl * __sin(v_pll.theta[1] + phase)));

    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
    GpioDataRegs.GPADAT.bit.GPIO14 = 0;
}

__interrupt void CLA1_isr1(void)
{
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP11;
}
__interrupt void CLA1_isr2(void)
{
    asm(" ESTOP0");
}
__interrupt void CLA1_isr3(void)
{
    asm(" ESTOP0");
}
__interrupt void CLA1_isr4(void)
{
    asm(" ESTOP0");
}

__interrupt void CLA1_isr5(void)
{
    asm(" ESTOP0");
}

__interrupt void CLA1_isr6(void)
{
    asm(" ESTOP0");
}

__interrupt void CLA1_isr7(void)
{
    asm(" ESTOP0");
}

__interrupt void CLA1_isr8(void)
{
    asm(" ESTOP0");
}

