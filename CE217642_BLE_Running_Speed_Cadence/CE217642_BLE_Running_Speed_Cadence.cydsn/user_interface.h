/*******************************************************************************
* File Name: user_interface.h
*
* Version: 1.0
*
* Description:
*  Contains the function prototypes and constants for user interface related source.
*
* Hardware Dependency:
*  CY8CKIT-062 PSoC6 BLE Pioneer Kit
*
********************************************************************************
* Copyright (2018), Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/


#include "common.h"


/***************************************
*           API Constants
***************************************/
#define LED_ON                          (0u)
#define LED_OFF                         (1u)
#define RGB_LED_MIN_VOLTAGE_MV          (2700u)
#define SW2_PRESS_TIME_DEL_BOND_LIST    (4u)

/* Constants for buttonState */
#define BUTTON_IS_PRESSED                   (1u)
#define BUTTON_IS_NOT_PRESSED               (0u)


/***************************************
*    Function Prototypes
***************************************/
void InitUserInterface(void);
void UpdateLedState(void);
bool ButtonSW2GetStatus(void);
void ButtonSW2SetStatus(bool Status);

/***************************************
*        Macros
***************************************/

/* Set Disconnect LED  */
#define Disconnect_LED_Write(value)                                                 \
    do{                                                                             \
        Cy_GPIO_Write(Disconnect_LED_0_PORT, Disconnect_LED_0_NUM, value);          \
    }while(0u)
        
/* Set Advertising LED  */
#define Advertising_LED_Write(value)                                                \
    do{                                                                             \
        Cy_GPIO_Write(Advertising_LED_0_PORT, Advertising_LED_0_NUM, value);        \
    }while(0u)

#define Advertising_LED_INV()  Cy_GPIO_Inv(Advertising_LED_0_PORT, Advertising_LED_0_NUM) 

/* Set Running LED */
#define Running_LED_Write(value)                                                    \
    do{                                                                             \
        Cy_GPIO_Write(Running_LED_0_PORT, Running_LED_0_NUM, value);                \
    }while(0u)

#define Running_LED_INV()  Cy_GPIO_Inv(Running_LED_0_PORT, Running_LED_0_NUM)        

/* Set LED5  */
#define LED5_Write(value)                                                           \
    do{                                                                             \
        Cy_GPIO_Write(Disconnect_LED_0_PORT, Disconnect_LED_0_NUM, value);          \
    }while(0u)

#define LED5_INV()  Cy_GPIO_Inv(Disconnect_LED_0_PORT, Disconnect_LED_0_NUM)        

/* Read SW2 pin */
#define SW2_Read()  Cy_GPIO_Read(SW2_0_PORT, SW2_0_NUM)

#define SW2_EnableInt() \
                do{       \
                    Cy_GPIO_ClearInterrupt(SW2_0_PORT, SW2_0_NUM); \
                    Cy_GPIO_SetInterruptMask(SW2_0_PORT, SW2_0_NUM, 0x01u);\
                }while(0u)
#define SW2_DisableInt() \
                do{       \
                    Cy_GPIO_SetInterruptMask(SW2_0_PORT, SW2_0_NUM, 0x00u);\
                    Cy_GPIO_ClearInterrupt(SW2_0_PORT, SW2_0_NUM);\
                }while(0u)
#define SW2_ClearInt()  Cy_GPIO_ClearInterrupt(SW2_0_PORT, SW2_0_NUM);

/* Leds operations */
#define EnableAllLeds()                                                             \
    do{                                                                             \
        Disconnect_LED_Write(LED_ON);                                               \
        Advertising_LED_Write(LED_ON);                                              \
        Running_LED_Write(LED_ON);                                                  \
    }while(0u)                                                                       
                                                                                    
#define DisableAllLeds()                                                            \
    do {                                                                            \
        Disconnect_LED_Write(LED_OFF);                                              \
        Advertising_LED_Write(LED_OFF);                                             \
        Running_LED_Write(LED_OFF);                                                 \
    } while(0u)
    
/* [] END OF FILE */
