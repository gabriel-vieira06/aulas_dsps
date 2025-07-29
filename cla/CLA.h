/*
 * CLA.h
 *
 *  Created on: 29 de jul de 2025
 *      Author: Vieira
 */

#ifndef CLA_H_
#define CLA_H_

#include "F2837xD_Cla_defines.h"
#include "CLAMath.h"
#include "F28x_Project.h"
#include "sogi.h"

extern volatile float vrede_CLA;
extern volatile SPLL_SOGI cla1_pll;
extern volatile float phase_CLA;
extern volatile float ampl_CLA;
extern volatile uint32_t count_task;

__interrupt void CLA1Task1();
__interrupt void CLA1Task2();
__interrupt void CLA1Task3();
__interrupt void CLA1Task4();
__interrupt void CLA1Task5();
__interrupt void CLA1Task6();
__interrupt void CLA1Task7();
__interrupt void CLA1Task8();

#endif /* CLA_H_ */
