/*
 * sogi.c
 *
 *  Created on: 18/02/2018
 *      Author: waner
 */
#include "sogi.h"
#include "math.h"
void SOGI_init(float Grid_freq, float DELTA_T, SPLL_SOGI *spll_obj){
    spll_obj->u[0] = 0.0;
    spll_obj->u[1] = 0.0;
    spll_obj->u[2] = 0.0;

    spll_obj->osg_u[0] = 0.0;
    spll_obj->osg_u[1] = 0.0;
    spll_obj->osg_u[2] = 0.0;

    spll_obj->osg_qu[0] = 0.0;
    spll_obj->osg_qu[1] = 0.0;
    spll_obj->osg_qu[2] = 0.0;

    spll_obj->u_Q[0] = 0.0;
    spll_obj->u_Q[1] = 0.0;

    spll_obj->u_D[0] = 0.0;
    spll_obj->u_D[1] = 0.0;

    spll_obj->ylf[0] = 0.0;
    spll_obj->ylf[1] = 0.0;

    spll_obj->fo = 0.0;
    spll_obj->fn = Grid_freq;

    spll_obj->theta[0] = 0.0;
    spll_obj->theta[1] = 0.0;

    spll_obj->sin_ = 0.0;
    spll_obj->cos_ = 0.0;
    spll_obj->delta_fn = 0.0;
    // loop filter coefficients for 30720Hz
    spll_obj->lpf_coeff.B0_lf = 223.21446;
    spll_obj->lpf_coeff.B1_lf = -222.4062;
    spll_obj->lpf_coeff.A1_lf = -1.0;

    spll_obj->delta_T = DELTA_T;
}

void SOGI_coeff_update(float delta_T, float wn, float k, SPLL_SOGI *spll){
    float osgx,osgy,temp;
    spll->osg_coeff.osg_k = k;
    osgx=(2.0*0.5*wn*delta_T);
    spll->osg_coeff.osg_x = osgx;
    osgy = wn*delta_T*wn*delta_T;
    spll->osg_coeff.osg_y = osgy;
    temp = 1.0/(osgx+osgy+4.0);
    spll->osg_coeff.osg_b0 = osgx*temp;
    spll->osg_coeff.osg_b2 = (-1.0)*spll->osg_coeff.osg_b0;
    spll->osg_coeff.osg_a1 = (2.0*(4.0-osgy))*temp;
    spll->osg_coeff.osg_a2 = (osgx-osgy-4)*temp;
    spll->osg_coeff.osg_qb0 = (0.5*osgy)*temp;
    spll->osg_coeff.osg_qb1 = spll->osg_coeff.osg_qb0*(2.0);
    spll->osg_coeff.osg_qb2 = spll->osg_coeff.osg_qb0;
}

void SPLL_SOGI_CALC(SPLL_SOGI *spll_obj){
    // Update the spll_obj->u[0] with the grid value before calling this routine
    // Orthogonal Signal Generator   //
    spll_obj->osg_u[0] = (spll_obj->osg_coeff.osg_b0*(spll_obj->u[0]-spll_obj->u[2])) + (spll_obj->osg_coeff.osg_a1*spll_obj->osg_u[1]) + (spll_obj->osg_coeff.osg_a2*spll_obj->osg_u[2]);
      spll_obj->osg_u[0] = (spll_obj->osg_coeff.osg_b0*(spll_obj->u[0]-spll_obj->u[2])) + (spll_obj->osg_coeff.osg_a1*spll_obj->osg_u[1]) + (spll_obj->osg_coeff.osg_a2*spll_obj->osg_u[2]);
    spll_obj->osg_u[2] = spll_obj->osg_u[1];
    spll_obj->osg_u[1] = spll_obj->osg_u[0];
    spll_obj->osg_qu[0] = (spll_obj->osg_coeff.osg_qb0*spll_obj->u[0]) + (spll_obj->osg_coeff.osg_qb1*spll_obj->u[1]) + (spll_obj->osg_coeff.osg_qb2*spll_obj->u[2]) + (spll_obj->osg_coeff.osg_a1*spll_obj->osg_qu[1]) + (spll_obj->osg_coeff.osg_a2*spll_obj->osg_qu[2]);
    spll_obj->osg_qu[2] = spll_obj->osg_qu[1];
    spll_obj->osg_qu[1] = spll_obj->osg_qu[0];
    spll_obj->u[2] = spll_obj->u[1];
    spll_obj->u[1] = spll_obj->u[0];
    // Park Transform from alpha beta to d-q axis            //
    spll_obj->u_Q[0] = (spll_obj->cos_*spll_obj->osg_u[0]) + (spll_obj->sin_*spll_obj->osg_qu[0]);
    spll_obj->u_D[0] = (spll_obj->cos_*spll_obj->osg_qu[0]) - (spll_obj->sin_*spll_obj->osg_u[0]);
    // Loop Filter                     //
    spll_obj->ylf[0]=spll_obj->ylf[1] + (spll_obj->lpf_coeff.B0_lf*spll_obj->u_Q[0]) + (spll_obj->lpf_coeff.B1_lf*spll_obj->u_Q[1]);
    spll_obj->ylf[1]=spll_obj->ylf[0];
    spll_obj->u_Q[1]=spll_obj->u_Q[0];

    spll_obj->fo=spll_obj->fn+spll_obj->ylf[0];
    spll_obj->theta[0]=spll_obj->theta[1] + (spll_obj->fo*spll_obj->delta_T)*6.2831853072;

    if(spll_obj->theta[0] > 6.2831853072)
        spll_obj->theta[0] -= 6.2831853072;
    else if(spll_obj->theta[0] < -6.2831853072)
        spll_obj->theta[0] += 6.2831853072;

    spll_obj->theta[1] = spll_obj->theta[0];
    spll_obj->sin_ = __sin(spll_obj->theta[0]);
    spll_obj->cos_ = __cos(spll_obj->theta[0]);

    //spll_obj->sin_ = sin(spll_obj->theta[0]);
    //spll_obj->cos_ = cos(spll_obj->theta[0]);

}

void SOGI_CALC(SPLL_SOGI *obj){
    // Orthogonal Signal Generator   //
    obj->osg_u[0] = (obj->osg_coeff.osg_b0*(obj->u[0]-obj->u[2])) + (obj->osg_coeff.osg_a1*obj->osg_u[1]) + (obj->osg_coeff.osg_a2*obj->osg_u[2]);
    obj->osg_u[2] = obj->osg_u[1];
    obj->osg_u[1] = obj->osg_u[0];

    obj->osg_qu[0] = (obj->osg_coeff.osg_qb0*obj->u[0]) + (obj->osg_coeff.osg_qb1*obj->u[1]) + (obj->osg_coeff.osg_qb2*obj->u[2]) + (obj->osg_coeff.osg_a1*obj->osg_qu[1]) + (obj->osg_coeff.osg_a2*obj->osg_qu[2]);
    obj->osg_qu[2] = obj->osg_qu[1];
    obj->osg_qu[1] = obj->osg_qu[0];

    obj->u[2] = obj->u[1];
    obj->u[1] = obj->u[0];

    // Park Transform from alpha beta to d-q axis            //
    obj->u_Q[0] = (obj->cos_*obj->osg_u[0]) + (obj->sin_*obj->osg_qu[0]);
    obj->u_D[0] = (obj->cos_*obj->osg_qu[0]) - (obj->sin_*obj->osg_u[0]);
}

/////////////////////////////////////////////////SOGI PLL



