/*
 * CLA_config.h
 *
 *  Created on: 29 de jul de 2025
 *      Author: Vieira
 */

#ifndef CLA_CONFIG_H_
#define CLA_CONFIG_H_

#include "CLA.h"
#include "F28x_Project.h"

void CLA1_ConfigCLAMemory(void);
void CLA1_InitCpu1Cla1(void);

__interrupt void CLA1_isr1(void);
__interrupt void CLA1_isr2(void);
__interrupt void CLA1_isr3(void);
__interrupt void CLA1_isr4(void);
__interrupt void CLA1_isr5(void);
__interrupt void CLA1_isr6(void);
__interrupt void CLA1_isr7(void);
__interrupt void CLA1_isr8(void);

#endif /* CLA_CONFIG_H_ */
