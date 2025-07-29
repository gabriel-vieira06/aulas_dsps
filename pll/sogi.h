/*
 * sogi.h
 *
 *  Created on: 18/02/2018
 *      Author: waner
 */

#ifndef SOGI_H_
#define SOGI_H_


typedef struct{
    float   osg_k;
    float   osg_x;
    float   osg_y;
    float   osg_b0;
    float   osg_b2;
    float   osg_a1;
    float   osg_a2;
    float   osg_qb0;
    float   osg_qb1;
    float   osg_qb2;
}SPLL_SOGI_OSG_COEFF;

typedef struct{
    float   B1_lf;
    float   B0_lf;
    float   A1_lf;
}SPLL_SOGI_LPF_COEFF;

typedef struct{
    float u[3];         //in 1ph AC Signal measured and normalized
    float osg_u[3];     // Estimated Grid Voltage
    float osg_qu[3];    // Estimated Orthogonal Grid Voltage
    float u_Q[2];       //out Q axis component of the estimated grid
    float u_D[2];       //out D axis component of the estimated grid
    float ylf[2];
    float fo;       //out Instantaneous Grid Frequency PLL
    float fn;       //in Grid Frequency Nominal
    float theta[2]; //out grid phase angle
    float cos_;     //out Cos(grid phase angle)
    float sin_;     //Sin(grid phase angle)
    float delta_T;  // 1/Frequency of calling the PLL routine
    float delta_fn;
    SPLL_SOGI_OSG_COEFF osg_coeff;
    SPLL_SOGI_LPF_COEFF lpf_coeff;
}SPLL_SOGI;

void SOGI_init(float Grid_freq, float DELTA_T, SPLL_SOGI *spll_obj);
void SOGI_coeff_update(float delta_T, float wn, float k, SPLL_SOGI *spll);
void SPLL_SOGI_CALC(SPLL_SOGI *spll_obj);
void SOGI_CALC(SPLL_SOGI *obj);
//SPLL_SOGI  sogi_pll, sogi_vS, sogi_iL, sogi_1h, sogi_3h, sogi_5h, sogi_7h, sogi_9h;

#endif /* NANOGRID_BGIC_SOGI_H_ */

