/*******************************************************************************
* File Name: host_main.c
*
* Version 1.0
*
* Description:
*  This is the source code for the BLE Environmental Sensing example 
*  project.
*
* Hardware Dependency:
*  CY8CKIT-062 PSoC6 BLE Pioneer Kit
*
******************************************************************************
* Copyright (2018), Cypress Semiconductor Corporation.
******************************************************************************
* This software is owned by Cypress Semiconductor Corporation (Cypress) and is
* protected by and subject to worldwide patent protection (United States and
* foreign), United States copyright laws and international treaty provisions.
* Cypress hereby grants to licensee a personal, non-exclusive, non-transferable
* license to copy, use, modify, create derivative works of, and compile the
* Cypress Source Code and derivative works for the sole purpose of creating
* custom software in support of licensee product to be used only in conjunction
* with a Cypress integrated circuit as specified in the applicable agreement.
* Any reproduction, modification, translation, compilation, or representation of
* this software except as specified above is prohibited without the express
* written permission of Cypress.
*
* Disclaimer: CYPRESS MAKES NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, WITH
* REGARD TO THIS MATERIAL, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* Cypress reserves the right to make changes without further notice to the
* materials described herein. Cypress does not assume any liability arising out
* of the application or use of any product or circuit described herein. Cypress
* does not authorize its products for use as critical components in life-support
* systems where a malfunction or failure may reasonably be expected to result in
* significant injury to the user. The inclusion of Cypress' product in a life-
* support systems application implies that the manufacturer assumes all risk of
* such use and in doing so indemnifies Cypress against all charges. Use may be
* limited by and subject to the applicable Cypress software license agreement.
*****************************************************************************/

#include "user_interface.h"
#include "common.h"

/* Global Variables */
static cy_stc_ble_conn_handle_t appConnHandle;
static volatile uint32_t        mainTimer  = 1u;
static volatile uint32_t        prevMainTimer;
static bool                     isButtonPressed = false;
static bool                     isIndicationPending = false;

/* Contains the value of the Change Index which is advertised in the 
* Service Data AD field. By the spec it should be a pseudo-random value
* but in this project, for simplicity, instead of a pseudo-random value,
* a value incremented from an old value will be used.
*/
cy_stc_ble_ess_characteristic_data_t    humidity;
cy_stc_ble_ess_characteristic_data_t    windSpeed[SIZE_2_BYTES];

/*******************************************************************************
* Function Name: AppCallBack
********************************************************************************
*
* Summary:
*   This is an event callback function to receive events from the BLE Component.
*
* Parameters:
*   event - the event code
*   eventParam - the event parameters
*
*******************************************************************************/
void AppCallBack(uint32_t event, void* eventParam)
{
    cy_en_ble_api_result_t apiResult;
    uint8_t i;
    static cy_stc_ble_gap_sec_key_info_t keyInfo =
    {
        .localKeysFlag    = CY_BLE_GAP_SMP_INIT_ENC_KEY_DIST | 
                            CY_BLE_GAP_SMP_INIT_IRK_KEY_DIST | 
                            CY_BLE_GAP_SMP_INIT_CSRK_KEY_DIST,
        .exchangeKeysFlag = CY_BLE_GAP_SMP_INIT_ENC_KEY_DIST | 
                            CY_BLE_GAP_SMP_INIT_IRK_KEY_DIST | 
                            CY_BLE_GAP_SMP_INIT_CSRK_KEY_DIST |
                            CY_BLE_GAP_SMP_RESP_ENC_KEY_DIST |
                            CY_BLE_GAP_SMP_RESP_IRK_KEY_DIST |
                            CY_BLE_GAP_SMP_RESP_CSRK_KEY_DIST,
    };

    switch (event)
    {
        /**********************************************************
        *                       General Events
        ***********************************************************/
        case CY_BLE_EVT_STACK_ON: /* This event is received when the component is Started */
            DBG_PRINTF("CY_BLE_EVT_STACK_ON, StartAdvertisement \r\n");   
            
            /* Enter into discoverable mode so that remote can find it */
            apiResult = Cy_BLE_GAPP_StartAdvertisement(CY_BLE_ADVERTISING_FAST, CY_BLE_PERIPHERAL_CONFIGURATION_0_INDEX);
            if(apiResult != CY_BLE_SUCCESS)
            {
                DBG_PRINTF("Cy_BLE_GAPP_StartAdvertisement API Error: 0x%x \r\n", apiResult);
            }
            
            /* Generates the security keys */
            apiResult = Cy_BLE_GAP_GenerateKeys(&keyInfo);
            if(apiResult != CY_BLE_SUCCESS)
            {
                DBG_PRINTF("Cy_BLE_GAP_GenerateKeys API Error: 0x%x \r\n", apiResult);
            }
           
            /* Display Bond list */
            App_DisplayBondList();
            break;
            
        case CY_BLE_EVT_TIMEOUT:
            break;
            
        case CY_BLE_EVT_HARDWARE_ERROR:    /* This event indicates that some internal HW error has occurred */
            DBG_PRINTF("Hardware Error \r\n");
            ShowError();
            break;
            
        /* This event will be triggered by host stack if BLE stack is busy or not busy.
         *  Parameter corresponding to this event will be the state of BLE stack.
         *  BLE stack busy = CY_BLE_STACK_STATE_BUSY,
         *  BLE stack not busy = CY_BLE_STACK_STATE_FREE 
         */
        case CY_BLE_EVT_STACK_BUSY_STATUS:
            DBG_PRINTF("CY_BLE_EVT_STACK_BUSY_STATUS: %x\r\n", *(uint8_t *)eventParam);
            break;
            
        case CY_BLE_EVT_SET_TX_PWR_COMPLETE:
            DBG_PRINTF("CY_BLE_EVT_SET_TX_PWR_COMPLETE \r\n");
            break;
            
        case CY_BLE_EVT_LE_SET_EVENT_MASK_COMPLETE:
            DBG_PRINTF("CY_BLE_EVT_LE_SET_EVENT_MASK_COMPLETE \r\n");
            break;
            
        case CY_BLE_EVT_SET_DEVICE_ADDR_COMPLETE:
            DBG_PRINTF("CY_BLE_EVT_SET_DEVICE_ADDR_COMPLETE \r\n");
                        
            /* Reads the BD device address from BLE Controller's memory */
            apiResult = Cy_BLE_GAP_GetBdAddress();
            if(apiResult != CY_BLE_SUCCESS)
            {   
                DBG_PRINTF("Cy_BLE_GAP_GetBdAddress API Error: 0x%x \r\n", apiResult);
            }
            break;
            
        case CY_BLE_EVT_GET_DEVICE_ADDR_COMPLETE:
            DBG_PRINTF("CY_BLE_EVT_GET_DEVICE_ADDR_COMPLETE: ");
            for(i = CY_BLE_GAP_BD_ADDR_SIZE; i > 0u; i--)
            {
                DBG_PRINTF("%2.2x", ((cy_stc_ble_bd_addrs_t *)
                                    ((cy_stc_ble_events_param_generic_t *)eventParam)->eventParams)->publicBdAddr[i-1]);
            }
            DBG_PRINTF("\r\n");
            
            break;
            
        case CY_BLE_EVT_STACK_SHUTDOWN_COMPLETE:
            DBG_PRINTF("CY_BLE_EVT_STACK_SHUTDOWN_COMPLETE \r\n");
            DBG_PRINTF("Hibernate\r\n");
            UpdateLedState();
            UART_DEB_WAIT_TX_COMPLETE();
            /* Hibernate */
            Cy_SysPm_Hibernate();
            break;   
            
        /**********************************************************
        *                       GAP Events
        ***********************************************************/
        case CY_BLE_EVT_GAP_AUTH_REQ:
            DBG_PRINTF("CY_BLE_EVT_GAP_AUTH_REQ: bdHandle=%x, security=%x, bonding=%x, ekeySize=%x, err=%x \r\n", 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).bdHandle, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).security, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).bonding, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).ekeySize, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).authErr);
            
            if(cy_ble_configPtr->authInfo[CY_BLE_SECURITY_CONFIGURATION_0_INDEX].security == 
                (CY_BLE_GAP_SEC_MODE_1 | CY_BLE_GAP_SEC_LEVEL_1))
            {
                cy_ble_configPtr->authInfo[CY_BLE_SECURITY_CONFIGURATION_0_INDEX].authErr = 
                  CY_BLE_GAP_AUTH_ERROR_PAIRING_NOT_SUPPORTED;
            }    
            
            cy_ble_configPtr->authInfo[CY_BLE_SECURITY_CONFIGURATION_0_INDEX].bdHandle = 
              ((cy_stc_ble_gap_auth_info_t *)eventParam)->bdHandle;

            apiResult = Cy_BLE_GAPP_AuthReqReply(&cy_ble_configPtr->authInfo[CY_BLE_SECURITY_CONFIGURATION_0_INDEX]);            
            if(apiResult != CY_BLE_SUCCESS)
            {
                DBG_PRINTF("Cy_BLE_GAPP_AuthReqReply API Error: 0x%x," 
                           " call Cy_BLE_GAP_RemoveOldestDeviceFromBondedList\r\n", apiResult);
                apiResult = Cy_BLE_GAP_RemoveOldestDeviceFromBondedList();
                if(apiResult != CY_BLE_SUCCESS)
                {
                    DBG_PRINTF("Cy_BLE_GAP_RemoveOldestDeviceFromBondedList API Error: 0x%x \r\n", apiResult);
                }
                else
                {
                    apiResult = Cy_BLE_GAPP_AuthReqReply(&cy_ble_configPtr->authInfo[CY_BLE_SECURITY_CONFIGURATION_0_INDEX]);            
                    if(apiResult != CY_BLE_SUCCESS)
                    {
                        DBG_PRINTF("Cy_BLE_GAPP_AuthReqReply API Error: 0x%x \r\n", apiResult);
                    }
                }
            }
            break;
            
        case CY_BLE_EVT_GAPP_ADVERTISEMENT_START_STOP:
            DBG_PRINTF("CY_BLE_EVT_GAPP_ADVERTISEMENT_START_STOP, state: %d \r\n", Cy_BLE_GetAdvertisementState());
            if(Cy_BLE_GetAdvertisementState() == CY_BLE_ADV_STATE_STOPPED)
            {   
                /* Fast and slow advertising period complete, go to low power  
                 * mode (Hibernate) and wait for an external
                 * user event to wake up the device again */
                UpdateLedState();
                apiResult = Cy_BLE_Stop(); 
                if(apiResult != CY_BLE_SUCCESS)
                {
                    DBG_PRINTF("Cy_BLE_Stop API Error: 0x%x \r\n", apiResult);
                }                
            }
            break;
                   
        case CY_BLE_EVT_GAPP_UPDATE_ADV_SCAN_DATA_COMPLETE:
            DBG_PRINTF("CY_BLE_EVT_GAPP_UPDATE_ADV_SCAN_DATA_COMPLETE: %d \r\n", *(uint8_t *)eventParam);
            break;
            
        case CY_BLE_EVT_GAP_PASSKEY_ENTRY_REQUEST:
            DBG_PRINTF("CY_BLE_EVT_GAP_PASSKEY_ENTRY_REQUEST press 'p' to enter passkey \r\n");
            break;
            
        case CY_BLE_EVT_GAP_PASSKEY_DISPLAY_REQUEST:
            DBG_PRINTF("CY_BLE_EVT_GAP_PASSKEY_DISPLAY_REQUEST %6.6ld \r\n", *(uint32_t *)eventParam);
            break;
            
        case CY_BLE_EVT_GAP_KEYINFO_EXCHNGE_CMPLT:
            DBG_PRINTF("CY_BLE_EVT_GAP_KEYINFO_EXCHNGE_CMPLT \r\n");
            break;
            
        case CY_BLE_EVT_GAP_DEVICE_CONNECTED:
            DBG_PRINTF("CY_BLE_EVT_GAP_DEVICE_CONNECTED: connIntv = %d ms \r\n",          /* in milliseconds / 1.25ms */
                        ((cy_stc_ble_gap_connected_param_t *)eventParam)->connIntv * CY_BLE_CONN_INTRV_TO_MS);
             
            /* Set security keys for new device which is not already bonded */
            if(App_IsDeviceInBondList((*(cy_stc_ble_gap_connected_param_t *)eventParam).bdHandle) == 0u)
            {
                keyInfo.SecKeyParam.bdHandle = (*(cy_stc_ble_gap_connected_param_t *)eventParam).bdHandle;
                apiResult = Cy_BLE_GAP_SetSecurityKeys(&keyInfo);
                if(apiResult != CY_BLE_SUCCESS)
                {
                    DBG_PRINTF("Cy_BLE_GAP_SetSecurityKeys API Error: 0x%x \r\n", apiResult);
                }
            }
            break;
            
        case CY_BLE_EVT_GAP_KEYS_GEN_COMPLETE:
            DBG_PRINTF("CY_BLE_EVT_GAP_KEYS_GEN_COMPLETE \r\n");
            keyInfo.SecKeyParam = (*(cy_stc_ble_gap_sec_key_param_t *)eventParam);
            Cy_BLE_GAP_SetIdAddress(&cy_ble_deviceAddress);
            break;
            
        case CY_BLE_EVT_GAP_SMP_NEGOTIATED_AUTH_INFO:
            DBG_PRINTF("CY_BLE_EVT_GAP_SMP_NEGOTIATED_AUTH_INFO:" 
                       " bdHandle=%x, security=%x, bonding=%x, ekeySize=%x, err=%x \r\n", 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).bdHandle, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).security, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).bonding, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).ekeySize, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).authErr);
            break;
            
        case CY_BLE_EVT_GAP_AUTH_COMPLETE:
            DBG_PRINTF("CY_BLE_EVT_GAP_AUTH_COMPLETE: bdHandle=%x, security=%x, bonding=%x, ekeySize=%x, err=%x \r\n", 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).bdHandle, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).security, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).bonding, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).ekeySize, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).authErr);
            break;
            
        case CY_BLE_EVT_GAP_AUTH_FAILED:
            DBG_PRINTF("CY_BLE_EVT_GAP_AUTH_FAILED: bdHandle=%x, authErr=%x\r\n", 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).bdHandle, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).authErr);
            break;
            
        case CY_BLE_EVT_GAP_CONNECTION_UPDATE_COMPLETE:
            DBG_PRINTF("CY_BLE_EVT_GAP_CONNECTION_UPDATE_COMPLETE: connIntv = %d ms \r\n", /* in milliseconds / 1.25ms */
                        ((cy_stc_ble_gap_conn_param_updated_in_controller_t *)eventParam)->connIntv * CY_BLE_CONN_INTRV_TO_MS);
            break;

        case CY_BLE_EVT_GAP_DEVICE_DISCONNECTED:
            DBG_PRINTF("CY_BLE_EVT_GAP_DEVICE_DISCONNECTED: bdHandle=%x, reason=%x, status=%x\r\n",
                (*(cy_stc_ble_gap_disconnect_param_t *)eventParam).bdHandle, 
                (*(cy_stc_ble_gap_disconnect_param_t *)eventParam).reason, 
                (*(cy_stc_ble_gap_disconnect_param_t *)eventParam).status);
                        
            /* Put the device into discoverable mode so that a remote can search it */
            apiResult = Cy_BLE_GAPP_StartAdvertisement(CY_BLE_ADVERTISING_FAST, CY_BLE_PERIPHERAL_CONFIGURATION_0_INDEX);
            if(apiResult != CY_BLE_SUCCESS)
            {
                DBG_PRINTF("StartAdvertisement API Error: 0x%x \r\n", apiResult);
                ShowError();
            }
            break;
            
        case CY_BLE_EVT_GAP_ENCRYPT_CHANGE:
            DBG_PRINTF("CY_BLE_EVT_GAP_ENCRYPT_CHANGE: %x \r\n", *(uint8_t *)eventParam);
            break;            
            
        /**********************************************************
        *                       GATT Events
        ***********************************************************/
        case CY_BLE_EVT_GATT_CONNECT_IND:
            appConnHandle = *(cy_stc_ble_conn_handle_t *)eventParam;
            DBG_PRINTF("CY_BLE_EVT_GATT_CONNECT_IND: %x, %x \r\n", 
                (*(cy_stc_ble_conn_handle_t *)eventParam).attId, 
                (*(cy_stc_ble_conn_handle_t *)eventParam).bdHandle);
            break;
            
        case CY_BLE_EVT_GATT_DISCONNECT_IND:
            DBG_PRINTF("CY_BLE_EVT_GATT_DISCONNECT_IND: %x, %x \r\n", 
                (*(cy_stc_ble_conn_handle_t *)eventParam).attId, 
                (*(cy_stc_ble_conn_handle_t *)eventParam).bdHandle);
            break;
            
        case CY_BLE_EVT_GATTS_XCNHG_MTU_REQ:
            { 
                cy_stc_ble_gatt_xchg_mtu_param_t mtu = 
                {
                    .connHandle = ((cy_stc_ble_gatt_xchg_mtu_param_t *)eventParam)->connHandle
                };
                Cy_BLE_GATT_GetMtuSize(&mtu);
                DBG_PRINTF("CY_BLE_EVT_GATTS_XCNHG_MTU_REQ %x, %x, final mtu= %d \r\n", mtu.connHandle.attId,
                            mtu.connHandle.bdHandle, mtu.mtu);
            }
            break;
            
        case CY_BLE_EVT_GATTS_READ_CHAR_VAL_ACCESS_REQ:
            /* Triggered on server side when client sends read request and when
            * characteristic has CY_BLE_GATT_DB_ATTR_CHAR_VAL_RD_EVENT property set.
            * This event could be ignored by application unless it need to response
            * by error response which needs to be set in gattErrorCode field of
            * event parameter. */
            DBG_PRINTF("CY_BLE_EVT_GATTS_READ_CHAR_VAL_ACCESS_REQ: handle: %x \r\n", 
                ((cy_stc_ble_gatts_char_val_read_req_t *)eventParam)->attrHandle);
            break;
            
        case CY_BLE_EVT_GATTS_INDICATION_DISABLED:
            DBG_PRINTF("CY_BLE_EVT_GATTS_INDICATION_DISABLED");
            break;
            
        case CY_BLE_EVT_GATTS_INDICATION_ENABLED:
            DBG_PRINTF("CY_BLE_EVT_GATTS_INDICATION_ENABLED");
            break;
            
        /**********************************************************
        *                       Other Events
        ***********************************************************/
        case CY_BLE_EVT_PENDING_FLASH_WRITE:
            /* Inform application that flash write is pending. Stack internal data 
            * structures are modified and require to be stored in Flash using 
            * Cy_BLE_StoreBondingData() */
            DBG_PRINTF("CY_BLE_EVT_PENDING_FLASH_WRITE\r\n");
            break;

        default:
            DBG_PRINTF("Other event: 0x%lx \r\n", event);
            break;
    }
}

/*******************************************************************************
* Function Name: LowPowerImplementation
********************************************************************************
*
* Summary:
*   Implements low power in the project.
*
* Theory:
*   The function tries to enter deep sleep as much as possible - whenever the 
*   BLE is idle and the UART transmission/reception is not happening. 
*
*******************************************************************************/
static void LowPowerImplementation(void)
{
    if(UART_DEB_IS_TX_COMPLETE() == true)
    {            
        /* Entering into the Deep Sleep */
        Cy_SysPm_DeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
    }
}

/*******************************************************************************
* Function Name: Timer_Interrupt
********************************************************************************
*
* Summary:
*   Handles the Interrupt Service Routine for the MCWDT timer.
*
*******************************************************************************/
void Timer_Interrupt(void)
{
    /* Blink LED to indicate that device advertises */
    UpdateLedState();
    
    /* Indicate that timer is raised to main loop */
    mainTimer++;
    
    if(windSpeed[CHARACTERISTIC_INSTANCE_1].isMeasurementPeriodElapsed == true)
    {
        windSpeed[CHARACTERISTIC_INSTANCE_1].updateIntervalTimer--;
    }
    
    if(windSpeed[CHARACTERISTIC_INSTANCE_2].isMeasurementPeriodElapsed == true)
    {
        windSpeed[CHARACTERISTIC_INSTANCE_2].updateIntervalTimer--;
    }
    
    if(humidity.isMeasurementPeriodElapsed == true)
    {
        humidity.updateIntervalTimer--;
    }

    if(windSpeed[CHARACTERISTIC_INSTANCE_1].ntfTimer != 0u)
    {
        windSpeed[CHARACTERISTIC_INSTANCE_1].ntfTimer--;
    }
    
    if(windSpeed[CHARACTERISTIC_INSTANCE_2].ntfTimer != 0u)
    {   
        windSpeed[CHARACTERISTIC_INSTANCE_2].ntfTimer--;
        
    }
    
    if(humidity.ntfTimer != 0u)
    {   
        humidity.ntfTimer--;
        
    }
    
    /* Press and hold the mechanical button (SW2) during 4 seconds to clear the bond list */
    App_RemoveDevicesFromBondListBySW2Press(SW2_PRESS_TIME_DEL_BOND_LIST);
                
    MCWDT_ClearInterrupt(CY_MCWDT_CTR0);
}

/*******************************************************************************
* Function Name: SW2_Interrupt
********************************************************************************
*
* Summary:
*   Handles the mechanical button press (SW2).
*
*******************************************************************************/
void SW2_Interrupt(void)
{
    /* In connected state - indicate that button was pressed. Check timer
     * value to cut false button presses.
     */
    if (Cy_GPIO_GetInterruptStatusMasked(SW2_0_PORT, SW2_0_NUM) == 1u)
    { 
        if(Cy_BLE_GetConnectionState(appConnHandle) >= CY_BLE_CONN_STATE_CONNECTED)
        {
            isButtonPressed = true;
        }
    
        SW2_CleanInt();
    } 
}


/*******************************************************************************
* Function Name: HostMain
********************************************************************************
*
* Summary:
*  Main function for the project.
*
* Theory:
*  The function starts BLE and UART components.
*  This function processes all BLE events and also implements the low power 
*  functionality.
*
*******************************************************************************/
int HostMain(void)
{
    cy_en_ble_api_result_t apiResult;
    
    /* Initialization the user interface: LEDs, SW2, etc.  */
    InitUserInterface();
    
    /* Initialize Debug UART */
    UART_START();
    DBG_PRINTF("BLE Environmental Sensing Profile Example\r\n");

    /* Start BLE component and register generic event handler */
    apiResult = Cy_BLE_Start(AppCallBack);
    if(apiResult != CY_BLE_SUCCESS)
    {
        DBG_PRINTF("Cy_BLE_Start API Error: 0x%x \r\n", apiResult);
    }

    /* Print stack version */
    PrintStackVersion();
    
    /* Initialize BLE Services */
    EssInit();
       
    /* Initialize Timer */
    Cy_SysInt_Init(&Timer_Int_cfg, Timer_Interrupt);
    NVIC_EnableIRQ(Timer_Int_cfg.intrSrc);   
    MCWDT_SetInterruptMask(CY_MCWDT_CTR0);
    MCWDT_Start();
    
    /***************************************************************************
    * Main polling loop
    ***************************************************************************/
    while(1)
    {
        /* Cy_BLE_ProcessEvents() allows BLE stack to process pending events */
        Cy_BLE_ProcessEvents();
        
        /* To achieve low power */
        LowPowerImplementation();

        /* Check if there is updated data */
        EssSimulateProfile(&windSpeed[0], mainTimer);
        EssSimulateProfile(&windSpeed[1], mainTimer);
        EssSimulateProfile(&humidity, mainTimer);
          
        /***********************************************************************
        * Wait for connection established with Central device
        ***********************************************************************/
        if(Cy_BLE_GetConnectionState(appConnHandle) >= CY_BLE_CONN_STATE_CONNECTED)
        {
            if(prevMainTimer != mainTimer)
            {
                if(isButtonPressed == true)
                {
                    /* Change ES Configuration descriptor value and indicate it to Client */
                    HandleButtonPress(&windSpeed[0]);  
                    HandleButtonPress(&windSpeed[1]);  
                    HandleButtonPress(&humidity);  
                    
                    SetIndicationPendingFlag(true);
                    EssSetIndicationValue(CY_BLE_ESS_VALUE_CHANGE_ES_CONFIG);
                    isButtonPressed     = false;
                }

                /* Check if there are notifications for True Wind Speed #1 and send them */
                EssChkNtfAndSendData(appConnHandle, &windSpeed[0]);

                /* Check if there are notifications for True Wind Speed #2 and send them */
                EssChkNtfAndSendData(appConnHandle, &windSpeed[1]);                
                
                /* Check if there are notifications for Humidity and send them */
                EssChkNtfAndSendData(appConnHandle, &humidity);                

                /* Check if there are indications need to send to remote Client
                * and send them.
                */
                if(isIndicationPending == true)
                {
                    EssHandleIndication(appConnHandle);
                    SetIndicationPendingFlag(false);
                    Cy_BLE_ProcessEvents();
                }
                
            }
            prevMainTimer = mainTimer;
     }
     
       
    #if(CY_BLE_BONDING_REQUIREMENT == CY_BLE_BONDING_YES)
        /* Store bonding data to flash only when all debug information has been sent */    
        if(cy_ble_pendingFlashWrite != 0u) 
        {   
            apiResult = Cy_BLE_StoreBondingData();    
            DBG_PRINTF("Store bonding data, status: %x, pending: %x \r\n", apiResult, cy_ble_pendingFlashWrite);
        }
    #endif /* CY_BLE_BONDING_REQUIREMENT == CY_BLE_BONDING_YES */
    }
}

/*******************************************************************************
* Function Name: SetIndicationPendingFlag()
********************************************************************************
* Summary:
*  Set flag for рending іndication
*
*******************************************************************************/
void SetIndicationPendingFlag(bool flagValue)
{
    isIndicationPending = flagValue;
}
    
/* [] END OF FILE */