#include "F28x_Project.h"
#include "math.h"

// CABEÇALHO DE FUNÇÕES
void setup_dac(void);
void setup_adc(void);
void setup_epwm(void);
void setup_gpio(void);
__interrupt void ISR_TIMER1(void);
__interrupt void ISR_ADC(void);

// VARIÁVEIS GLOBAIS
uint16_t index = 0, offset1 = 0, offset2 = 0, senotable[400], costable[400];
uint16_t indexadc = 0, adc1 = 0, adc2 = 0, plot1[400], plot2[400];
uint16_t *adc1point = &adc1;
uint16_t *adc2point = &adc2;

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
    ConfigCpuTimer(&CpuTimer1, 200, 50);
    CpuTimer1Regs.TCR.all = 0x4000;

    // SETUP DAC
    setup_dac();

    // SETUP ADC
    setup_adc();

    // SETUP PWM
    setup_epwm();

    // SETUP GPIO
    setup_gpio();

    // GERA TABELAS SENO E COSSENO
    for (index = 0; index < 400; index++){
        senotable[index] = (uint16_t)(1000.0*(1.0 + sin(6.28318531/400.0*(float)index)));
        costable[index] =  (uint16_t)(1000.0*(1.0 + cos(6.28318531/400.0*(float)index)));
    }

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

    EPwm10Regs.TBPRD               = 5000;             // LAB ADC
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

    // LARGURA DO PULSO DE 75%
    EPwm1Regs.CMPA.bit.CMPA        = EPwm1Regs.TBPRD/4;

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

    // -----------
    // SETUP EPWM2
    // ------------
    CpuSysRegs.PCLKCR2.bit.EPWM2     = 1;            // HABILITA CLOCK

    EPwm2Regs.TBPRD                = 50000;              // PERIODO = Fosc/2*Fpwm OU Fosc/4*Fpwm
    EPwm2Regs.TBPHS.bit.TBPHS      = 0;                  // FASE
    EPwm2Regs.TBCTL.bit.SYNCOSEL   = TB_SYNC_DISABLE;
    EPwm2Regs.TBCTR                = 0x0000;
    EPwm2Regs.TBCTL.bit.CTRMODE    = TB_COUNT_UPDOWN;
    EPwm2Regs.TBCTL.bit.PHSEN      = TB_DISABLE;
    EPwm2Regs.TBCTL.bit.HSPCLKDIV  = TB_DIV1;
    EPwm2Regs.TBCTL.bit.CLKDIV     = TB_DIV1;

    // LARGURA DO PULSO DE 50%
    EPwm2Regs.CMPA.bit.CMPA        = EPwm2Regs.TBPRD/2;

    EPwm2Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;     // CARREGA REGISTRADORES A CADA ZERO
    EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO_PRD;
    EPwm2Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;     // CARREGA REGISTRADORES A CADA ZERO
    EPwm2Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO_PRD;

    EPwm2Regs.AQCTLA.bit.PRD = AQ_NO_ACTION;       // CARREGA AÇÕES PARA EPWM1A
    EPwm2Regs.AQCTLA.bit.ZRO = AQ_NO_ACTION;
    EPwm2Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm2Regs.AQCTLA.bit.CAD = AQ_SET;

    // ------------
    // FIM SETUP EPWM2
    // ------------

    // -----------
    // SETUP EPWM3
    // ------------
    CpuSysRegs.PCLKCR2.bit.EPWM3     = 1;            // HABILITA CLOCK

    EPwm3Regs.TBPRD                = 50000;              // PERIODO = Fosc/2*Fpwm OU Fosc/4*Fpwm
    EPwm3Regs.TBPHS.bit.TBPHS      = 0;                  // FASE
    EPwm3Regs.TBCTL.bit.SYNCOSEL   = TB_SYNC_DISABLE;
    EPwm3Regs.TBCTR                = 0x0000;
    EPwm3Regs.TBCTL.bit.CTRMODE    = TB_COUNT_UPDOWN;
    EPwm3Regs.TBCTL.bit.PHSEN      = TB_DISABLE;
    EPwm3Regs.TBCTL.bit.HSPCLKDIV  = TB_DIV1;
    EPwm3Regs.TBCTL.bit.CLKDIV     = TB_DIV1;

    // LARGURA DO PULSO DE 25%
    EPwm3Regs.CMPA.bit.CMPA        = 3*EPwm3Regs.TBPRD/4;

    EPwm3Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;     // CARREGA REGISTRADORES A CADA ZERO
    EPwm3Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO_PRD;
    EPwm3Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;     // CARREGA REGISTRADORES A CADA ZERO
    EPwm3Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO_PRD;

    EPwm3Regs.AQCTLA.bit.PRD = AQ_NO_ACTION;       // CARREGA AÇÕES PARA EPWM1A
    EPwm3Regs.AQCTLA.bit.ZRO = AQ_NO_ACTION;
    EPwm3Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm3Regs.AQCTLA.bit.CAD = AQ_SET;

    // ------------
    // FIM SETUP EPWM3
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

    // IO 0 - EPWM2A (J4 - 38)
    GpioCtrlRegs.GPAGMUX1.bit.GPIO2 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO2  = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO2   = 1;
    GpioCtrlRegs.GPACSEL1.bit.GPIO2 = GPIO_MUX_CPU1;

    // IO 0 - EPWM3A (J4 - 36)
    GpioCtrlRegs.GPAGMUX1.bit.GPIO4 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO4  = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO4   = 1;
    GpioCtrlRegs.GPACSEL1.bit.GPIO4 = GPIO_MUX_CPU1;

    EDIS;
}

__interrupt void ISR_TIMER1(void)
{
    index = (index == 399) ? 0 : (index + 1);
    DacaRegs.DACVALS.all = offset1 + senotable[index];
    DacbRegs.DACVALS.all = offset2 + costable[index];

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

__interrupt void ISR_ADC(void)
{
    adc1 = AdcaResultRegs.ADCRESULT0;
    adc2 = AdcaResultRegs.ADCRESULT1;

    indexadc = (indexadc == 399) ? 0 : (indexadc + 1);
    plot1[indexadc] = *adc1point;
    plot2[indexadc] = *adc2point;

    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}
