/*******************************************************************************
* File Name: user_interface.c
*
* Version: 1.0
*
* Description:
*  This file contains user interface related source.
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


#include "user_interface.h"

/*******************************************************************************
* Function Name: void InitUserInterface(void)
********************************************************************************
*
* Summary:
*   Initialization the user interface: LEDs, SW2, etc. 
*
*******************************************************************************/
void InitUserInterface(void)
{
    /* Initialize wakeup pin for Hibernate */
    Cy_SysPm_SetHibernateWakeupSource(CY_SYSPM_HIBERNATE_PIN1_LOW);
    
    /* Initialize LEDs */
    DisableAllLeds();
}


/*******************************************************************************
* Function Name: UpdateLedState
********************************************************************************
*
* Summary:
*  This function updates LED status based on current BLE state.
*
*******************************************************************************/
void UpdateLedState(void)
{
#if(CYDEV_VDDD_MV >= RGB_LED_MIN_VOLTAGE_MV)
    if(Cy_BLE_GetAdvertisementState() == CY_BLE_ADV_STATE_ADVERTISING)
    {
        /* In advertising state, turn off disconnect indication LED */
        Disconnect_LED_Write(LED_OFF);
        Simulation_LED_Write(LED_OFF);

        /* Blink advertising indication LED */
        Advertising_LED_INV();
    }
    else if(Cy_BLE_GetNumOfActiveConn() == 0u)
    {
        /* If in disconnected state, turn on disconnect indication LED and turn off advertising LED */
        Disconnect_LED_Write(LED_ON);
        Advertising_LED_Write(LED_OFF);
        Simulation_LED_Write(LED_OFF);
    }
    else 
    {
        /* In connected state, turn off disconnect indication and advertising indication LEDs */
        Disconnect_LED_Write(LED_OFF);
        Advertising_LED_Write(LED_OFF);
        if((Cy_BLE_GetConnectionState(appConnHandle) == CY_BLE_CONN_STATE_CLIENT_DISCOVERED) &&
           (cy_ble_serverInfo[Cy_BLE_GetDiscoveryIdx(appConnHandle)][CY_BLE_SRVI_CTS].range.startHandle != 0u))
        {
            Simulation_LED_Write(LED_ON);
        }
        else
        {
            Simulation_LED_Write(LED_OFF);
        }

    }
#else
    if(Cy_BLE_GetAdvertisementState() == CY_BLE_ADV_STATE_ADVERTISING)
    {
        /* Blink advertising indication LED */
        LED5_INV();
    }
    else if(Cy_BLE_GetNumOfActiveConn() == 0u)
    {
        /* If in disconnected state, turn on disconnect indication LED */
        LED5_Write(LED_ON);
    }
    else 
    {
        /* In connected state, turn off disconnect indication LED */
        LED5_Write(LED_OFF);
    }
#endif /* #if(CYDEV_VDDD_MV >= RGB_LED_MIN_VOLTAGE_MV) */    
}
/* [] END OF FILE */
