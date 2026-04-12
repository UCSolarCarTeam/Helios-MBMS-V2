/*
 * ReadGPIO.h
 *
 *  Created on: Apr 11, 2026
 *      Author: m
 */

#ifndef INC_READGPIO_H_
#define INC_READGPIO_H_

#include "cmsis_os.h"
#include "main.h"


// Change when electrical tells you to.
#define MPS_ACTIVE 1
#define ESD_ACTIVE 1

#define _12V_CAN_EN_ACTIVE 1

#define MAIN_CNTR_AUX_ACTIVE 1
#define COMMON_CNTR_ACTIVE 1
#define DCDC1_EN_ACTIVE 1
#define _14V_CHARGE_EN_ACTIVE 1
#define NDCDC_FAULT_ACTIVE 1
#define _12V_CRITICAL_FAULT_ACTIVE 1
#define _14V_CHARGER_FAULT_ACTIVE 1
#define _12V_CRITICAL_UC_ACTIVE 1

#define read_DCDC1_EN()							 HAL_GPIO_ReadPin(DCDC1_EN_GPIO_Port, DCDC1_EN_Pin)
#define	read_14V_Charge_EN()				     HAL_GPIO_ReadPin(_14V_Charge_EN_GPIO_Port, _14V_Charge_EN_Pin)
#define	read_nDCDC_Fault()	    			 	 HAL_GPIO_ReadPin(nDCDC_Fault_GPIO_Port, nDCDC_Fault_Pin)
#define	read_12V_Critical_Fault() 				 HAL_GPIO_ReadPin(_12V_Critical_Fault_GPIO_Port,_12V_Critical_Fault_Pin)
#define	read_14V_Charger_Fault() 				 HAL_GPIO_ReadPin(_14V_Charger_Fault_GPIO_Port, _14V_Charger_Fault_Pin)
#define	read_12V_Critical_UC()					 HAL_GPIO_ReadPin(_12V_Critical_UC_GPIO_Port, _12V_Critical_UC_Pin)

#define read_MPS()								 HAL_GPIO_ReadPin(MPS_GPIO_Port, MPS_Pin)
#define read_ESD()								 HAL_GPIO_ReadPin(ESD_GPIO_Port, ESD_Pin)
#define read_Main_CNTR_Aux()					 HAL_GPIO_ReadPin(Main_CNTR_Aux_GPIO_Port, Main_CNTR_Aux_Pin)
#define read_Common_CNTR_Aux()					 HAL_GPIO_ReadPin(Common_CNTR_Aux_GPIO_Port, Common_CNTR_Aux_Pin)


#endif /* INC_READGPIO_H_ */
