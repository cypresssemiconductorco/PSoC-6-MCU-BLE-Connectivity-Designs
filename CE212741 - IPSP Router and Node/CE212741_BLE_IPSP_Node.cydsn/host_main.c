/*******************************************************************************
* File Name: host_main.c
*
* Version: 1.0
*
* Description:
*  This example demonstrates how to setup an IPv6 communication infrastructure 
*  between two devices over a BLE transport using L2CAP channel. Creation 
*  and transmission of IPv6 packets over BLE is not part of this example.
*
*  Router sends generated packets with different content to Node in the loop 
*  and validate them with the afterwards received data packet. Node simply wraps
*  received data coming from Node, back to the Router.
*
* Note:
*
* Hardware Dependency:
*  CY8CKIT-062 PSoC6 BLE Pioneer Kit
*
******************************************************************************
* Copyright (2017), Cypress Semiconductor Corporation.
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

#include "common.h"
#include <stdbool.h>


uint16_t                            connIntv;   /* in milliseconds / 1.25ms */
bool                                l2capConnected[CY_BLE_CONN_COUNT] = {false};
bool                                l2capReadReceived[CY_BLE_CONN_COUNT] = {false};
uint8_t                             ipv6LoopbackBuffer[CY_BLE_CONN_COUNT][L2CAP_MAX_LEN];
uint16_t                            ipv6LoopbackLength[CY_BLE_CONN_COUNT]; 
volatile uint32_t                   mainTimer = 1u;
cy_stc_ble_timer_info_t             timerParam = { .timeout = ADV_TIMER_TIMEOUT };   

/* L2CAP Channel ID and parameters for the peer device */
cy_stc_ble_l2cap_cbfc_conn_ind_param_t   l2capParameters[CY_BLE_CONN_COUNT];


/*******************************************************************************
* Function Name: AppCallBack()
********************************************************************************
*
* Summary:
*   This is an event callback function to receive events from the BLE Component.
*
* Parameters:
*  event - the event code
*  eventParam - the event parameters
*
* Theory:
* The function is responsible for handling the events generated by the stack.
* It first starts advertising once the stack is initialized. 
* Upon advertising timeout this function enters Hibernate mode.
* 
*******************************************************************************/
void AppCallBack(uint32_t event, void* eventParam)
{
    cy_en_ble_api_result_t apiResult;
    uint32_t i;
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
        case CY_BLE_EVT_STACK_ON: /* This event received when component is Started */
            {
                DBG_PRINTF("CY_BLE_EVT_STACK_ON, StartAdvertisement \r\n");
                
                /* Register IPSP protocol multiplexer to L2CAP and 
                *  set the initial Receive Credit Low Mark for Based Flow Control mode 
                */
                cy_stc_ble_l2cap_cbfc_psm_info_t l2capCbfcPsmParam =
                {
                    .creditLwm = LE_WATER_MARK_IPSP,
                    .l2capPsm  = CY_BLE_L2CAP_PSM_LE_PSM_IPSP,
                };
                apiResult = Cy_BLE_L2CAP_CbfcRegisterPsm(&l2capCbfcPsmParam);
                if(apiResult != CY_BLE_SUCCESS)
                {
                    DBG_PRINTF("Cy_BLE_L2CAP_CbfcRegisterPsm API Error: 0x%x \r\n", apiResult);
                }

                /* Enter into discoverable mode so that remote can find it. */
                apiResult = Cy_BLE_GAPP_StartAdvertisement(CY_BLE_ADVERTISING_FAST, CY_BLE_PERIPHERAL_CONFIGURATION_0_INDEX);
                if(apiResult != CY_BLE_SUCCESS)
                {
                    DBG_PRINTF("StartAdvertisement API Error: 0x%x \r\n", apiResult);
                }
                
                /* Generates the security keys */
                apiResult = Cy_BLE_GAP_GenerateKeys(&keyInfo);
                if(apiResult != CY_BLE_SUCCESS)
                {
                    DBG_PRINTF("Cy_BLE_GAP_GenerateKeys API Error: 0x%x \r\n", apiResult);
                }
                
            }
            break;
            
        case CY_BLE_EVT_TIMEOUT: 
            if((((cy_stc_ble_timeout_param_t *)eventParam)->reasonCode == CY_BLE_GENERIC_APP_TO) && 
               (((cy_stc_ble_timeout_param_t *)eventParam)->timerHandle == timerParam.timerHandle))
            {
                /* Update Led State */
                UpdateLedState();
                
                /* Indicate that timer is raised to the main loop */
                mainTimer++;
            }
            else
            {
               DBG_PRINTF("CY_BLE_EVT_TIMEOUT: %x \r\n", *(cy_en_ble_to_reason_code_t *)eventParam); 
            } 
            break;
            
        case CY_BLE_EVT_HARDWARE_ERROR:    /* This event indicates that some internal HW error has occurred. */
            DBG_PRINTF("Hardware Error: 0x%x \r\n", *(uint8_t *)eventParam);
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
            DBG_PRINTF("Hibernate \r\n");
            UART_DEB_WAIT_TX_COMPLETE();
            /* Hibernate */
            Cy_SysPm_Hibernate();
            break;
            
        /**********************************************************
        *                       GAP Events
        ***********************************************************/
        case CY_BLE_EVT_GAP_AUTH_REQ:
            DBG_PRINTF("CY_BLE_EVT_AUTH_REQ: security=%x, bonding=%x, ekeySize=%x, err=%x \r\n", 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).security, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).bonding, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).ekeySize, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).authErr);
            break;
            
        case CY_BLE_EVT_GAP_PASSKEY_ENTRY_REQUEST:
            DBG_PRINTF("CY_BLE_EVT_PASSKEY_ENTRY_REQUEST press 'p' to enter passkey \r\n");
            break;
            
        case CY_BLE_EVT_GAP_PASSKEY_DISPLAY_REQUEST:
            DBG_PRINTF("CY_BLE_EVT_PASSKEY_DISPLAY_REQUEST %6.6ld \r\n", *(uint32_t *)eventParam);
            break;
            
        case CY_BLE_EVT_GAP_KEYINFO_EXCHNGE_CMPLT:
            DBG_PRINTF("CY_BLE_EVT_GAP_KEYINFO_EXCHNGE_CMPLT \r\n");
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
            
        case CY_BLE_EVT_GAPP_ADVERTISEMENT_START_STOP:
            {
                DBG_PRINTF("CY_BLE_EVT_GAPP_ADVERTISEMENT_START_STOP, state: %d \r\n", Cy_BLE_GetAdvertisementState()); 
                UpdateLedState();
                if((Cy_BLE_GetAdvertisementState() == CY_BLE_ADV_STATE_STOPPED) && (Cy_BLE_GetNumOfActiveConn() == 0u))
                {   
                    /* Fast and slow advertising period complete, go to low power  
                     * mode (Hibernate) and wait for an external
                     * user event to wake up the device again */
                    UpdateLedState();
                    Cy_BLE_Stop();   
                }
            }
            break;
            
        case CY_BLE_EVT_GAP_DEVICE_CONNECTED:
            UpdateLedState();
            connIntv = ((cy_stc_ble_gap_connected_param_t *)eventParam)->connIntv * 5u /4u;
            DBG_PRINTF("CY_BLE_EVT_GAP_DEVICE_CONNECTED: connIntv = %d ms \r\n", connIntv);
            keyInfo.SecKeyParam.bdHandle = (*(cy_stc_ble_gap_connected_param_t *)eventParam).bdHandle;
            apiResult = Cy_BLE_GAP_SetSecurityKeys(&keyInfo);
            if(apiResult != CY_BLE_SUCCESS)
            {
                DBG_PRINTF("Cy_BLE_GAP_SetSecurityKeys API Error: 0x%x \r\n", apiResult);
            }
            break;
            
        case CY_BLE_EVT_GAP_KEYS_GEN_COMPLETE:
            DBG_PRINTF("CY_BLE_EVT_GAP_KEYS_GEN_COMPLETE \r\n");
            keyInfo.SecKeyParam = (*(cy_stc_ble_gap_sec_key_param_t *)eventParam);
            Cy_BLE_GAP_SetIdAddress(&cy_ble_deviceAddress);
            break;
            
        case CY_BLE_EVT_GAP_CONNECTION_UPDATE_COMPLETE:
            connIntv = ((cy_stc_ble_gap_conn_param_updated_in_controller_t *)eventParam)->connIntv * 5u /4u;
            DBG_PRINTF("CY_BLE_EVT_GAP_CONNECTION_UPDATE_COMPLETE: connIntv = %d ms \r\n", connIntv);
            break;
            
        case CY_BLE_EVT_GAP_DEVICE_DISCONNECTED:
            DBG_PRINTF("CY_BLE_EVT_GAP_DEVICE_DISCONNECTED: bdHandle=%x, reason=%x, status=%x\r\n",
                (*(cy_stc_ble_gap_disconnect_param_t *)eventParam).bdHandle, 
                (*(cy_stc_ble_gap_disconnect_param_t *)eventParam).reason, 
                (*(cy_stc_ble_gap_disconnect_param_t *)eventParam).status);

            if(Cy_BLE_GetNumOfActiveConn() == (CONN_COUNT - 1u))
            {
                apiResult =
                    Cy_BLE_GAPP_StartAdvertisement(CY_BLE_ADVERTISING_FAST, CY_BLE_PERIPHERAL_CONFIGURATION_0_INDEX);
                if(apiResult != CY_BLE_SUCCESS)
                {
                    DBG_PRINTF("StartAdvertisement API Error: 0x%x \r\n", apiResult);
                }
            }
            break;
            
        case CY_BLE_EVT_GAP_ENCRYPT_CHANGE:
            DBG_PRINTF("CY_BLE_EVT_GAP_ENCRYPT_CHANGE: %x \r\n", *(uint8_t *)eventParam);
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

        /**********************************************************
        *                       GATT Events
        ***********************************************************/
        case CY_BLE_EVT_GATT_CONNECT_IND:
            DBG_PRINTF("CY_BLE_EVT_GATT_CONNECT_IND: %x, %x \r\n", 
                (*(cy_stc_ble_conn_handle_t *)eventParam).attId, 
                (*(cy_stc_ble_conn_handle_t *)eventParam).bdHandle);
            break;
            
        case CY_BLE_EVT_GATT_DISCONNECT_IND:
            DBG_PRINTF("CY_BLE_EVT_GATT_DISCONNECT_IND: %x, %x \r\n", 
                (*(cy_stc_ble_conn_handle_t *)eventParam).attId, 
                (*(cy_stc_ble_conn_handle_t *)eventParam).bdHandle);
            break;

        /**********************************************************
        *                       L2CAP Events
        ***********************************************************/
        case CY_BLE_EVT_L2CAP_CBFC_CONN_IND:
            l2capParameters[Cy_BLE_GetConnHandleByBdHandle((*(cy_stc_ble_l2cap_cbfc_conn_ind_param_t *)eventParam).bdHandle).attId] = 
                *((cy_stc_ble_l2cap_cbfc_conn_ind_param_t *)eventParam);
            DBG_PRINTF("CY_BLE_EVT_L2CAP_CBFC_CONN_IND: bdHandle=%d, lCid=%d, psm=%d,connParam mtu=%d, mps=%d, credit=%d ", 
                (*(cy_stc_ble_l2cap_cbfc_conn_ind_param_t *)eventParam).bdHandle,
                (*(cy_stc_ble_l2cap_cbfc_conn_ind_param_t *)eventParam).lCid,
                (*(cy_stc_ble_l2cap_cbfc_conn_ind_param_t *)eventParam).psm,
                (*(cy_stc_ble_l2cap_cbfc_conn_ind_param_t *)eventParam).connParam.mtu,
                (*(cy_stc_ble_l2cap_cbfc_conn_ind_param_t *)eventParam).connParam.mps,
                (*(cy_stc_ble_l2cap_cbfc_conn_ind_param_t *)eventParam).connParam.credit);
            if((*(cy_stc_ble_l2cap_cbfc_conn_ind_param_t *)eventParam).psm == CY_BLE_L2CAP_PSM_LE_PSM_IPSP)
            {
                cy_stc_ble_l2cap_cbfc_connection_info_t connParam =
                {
                   .mtu    = CY_BLE_L2CAP_MTU,
                   .mps    = CY_BLE_L2CAP_MPS,
                   .credit = LE_DATA_CREDITS_IPSP
                };
                
                cy_stc_ble_l2cap_cbfc_conn_resp_info_t l2capCbfcParam =
                {
                    .connParam = connParam,
                    .localCid  = (*(cy_stc_ble_l2cap_cbfc_conn_ind_param_t *)eventParam).lCid,
                    .response  = CY_BLE_L2CAP_CONNECTION_SUCCESSFUL
                };
                
                apiResult = Cy_BLE_L2CAP_CbfcConnectRsp(&l2capCbfcParam);
                DBG_PRINTF("SUCCESSFUL \r\n"); 
                l2capConnected[Cy_BLE_GetConnHandleByBdHandle((*(cy_stc_ble_l2cap_cbfc_conn_ind_param_t *)eventParam).bdHandle).attId] = true;
            }
            else
            {
                cy_stc_ble_l2cap_cbfc_conn_resp_info_t l2capCbfcParam =
                {
                    .connParam = (*(cy_stc_ble_l2cap_cbfc_conn_ind_param_t *)eventParam).connParam,
                    .localCid  = (*(cy_stc_ble_l2cap_cbfc_conn_ind_param_t *)eventParam).lCid,
                    .response  = CY_BLE_L2CAP_CONNECTION_REFUSED_PSM_UNSUPPORTED
                };
                apiResult = Cy_BLE_L2CAP_CbfcConnectRsp(&l2capCbfcParam);
                DBG_PRINTF("UNSUPPORTED \r\n"); 
            }
            if(apiResult != CY_BLE_SUCCESS)
            {
                DBG_PRINTF("Cy_BLE_L2CAP_CbfcConnectRsp API Error: 0x%x \r\n", apiResult);
            }
            break;

        case CY_BLE_EVT_L2CAP_CBFC_DISCONN_IND:
            {
                uint8_t i;
                DBG_PRINTF("CY_BLE_EVT_L2CAP_CBFC_DISCONN_IND: lCid=%d \r\n", *(uint16_t *)eventParam);
                for(i = 0u; i < CY_BLE_CONN_COUNT; i++)
                {
                    if(l2capParameters[i].lCid == *(uint16_t *)eventParam)
                    {
                        l2capConnected[i] = false;
                        break;
                    }
                }
                break;
            }
           

        /* Following two events are required to receive data */        
        case CY_BLE_EVT_L2CAP_CBFC_DATA_READ:
            {
                cy_stc_ble_l2cap_cbfc_rx_param_t *rxDataParam = (cy_stc_ble_l2cap_cbfc_rx_param_t *)eventParam;
                uint8_t i;
                DBG_PRINTF("<- EVT_L2CAP_CBFC_DATA_READ: lCid=%d, result=%d, len=%d", 
                    rxDataParam->lCid,
                    rxDataParam->result,
                    rxDataParam->rxDataLength);
            #if(DEBUG_UART_FULL)  
                DBG_PRINTF(", data:");
                for(i = 0; i < rxDataParam->rxDataLength; i++)
                {
                    DBG_PRINTF("%2.2x", rxDataParam->rxData[i]);
                }
            #endif /* DEBUG_UART_FULL */
                DBG_PRINTF("\r\n");
                for(i = 0u; i < CY_BLE_CONN_COUNT; i++)
                {
                    if(l2capParameters[i].lCid == rxDataParam->lCid)
                    {
                        /* Data is received from Router. Copy the data to a memory buffer */
                        if(rxDataParam->rxDataLength <= L2CAP_MAX_LEN)
                        {
                            ipv6LoopbackLength[i] = rxDataParam->rxDataLength;
                        }
                        else
                        {
                            ipv6LoopbackLength[i] = L2CAP_MAX_LEN;
                        }
                        memcpy(ipv6LoopbackBuffer[i], rxDataParam->rxData, ipv6LoopbackLength[i]);
                        l2capReadReceived[i] = true;
                        break;
                    }
                }
                break;
            }
            

        case CY_BLE_EVT_L2CAP_CBFC_RX_CREDIT_IND:
            {
                cy_stc_ble_l2cap_cbfc_low_rx_credit_param_t *rxCreditParam = (cy_stc_ble_l2cap_cbfc_low_rx_credit_param_t *)eventParam;
                cy_stc_ble_l2cap_cbfc_credit_info_t l2capCbfcCreditParam =
                {
                    .credit   = LE_DATA_CREDITS_IPSP,
                    .localCid = rxCreditParam->lCid
                };
                
                DBG_PRINTF("CY_BLE_EVT_L2CAP_CBFC_RX_CREDIT_IND: lCid=%d, credit=%d \r\n", 
                    rxCreditParam->lCid,
                    rxCreditParam->credit);

                /* This event informs that receive credits reached the low mark. 
                 * If the device expects more data to receive, send more credits back to the peer device.
                 */
                apiResult = Cy_BLE_L2CAP_CbfcSendFlowControlCredit(&l2capCbfcCreditParam);
                if(apiResult != CY_BLE_SUCCESS)
                {
                    DBG_PRINTF("Cy_BLE_L2CAP_CbfcSendFlowControlCredit API Error: 0x%x \r\n", apiResult);
                }
                break;
            }
            
        /* Following events are required to send data */
        case CY_BLE_EVT_L2CAP_CBFC_TX_CREDIT_IND:
            DBG_PRINTF("CY_BLE_EVT_L2CAP_CBFC_TX_CREDIT_IND: lCid=%d, result=%d, credit=%d \r\n", 
                ((cy_stc_ble_l2cap_cbfc_low_tx_credit_param_t *)eventParam)->lCid,
                ((cy_stc_ble_l2cap_cbfc_low_tx_credit_param_t *)eventParam)->result,
                ((cy_stc_ble_l2cap_cbfc_low_tx_credit_param_t *)eventParam)->credit);
            break;
        
        case CY_BLE_EVT_L2CAP_CBFC_DATA_WRITE_IND:
            #if(DEBUG_UART_FULL)
            {
                cy_ble_l2cap_cbfc_data_write_param_t *writeDataParam = (cy_ble_l2cap_cbfc_data_write_param_t*)eventParam;
                DBG_PRINTF("CY_BLE_EVT_L2CAP_CBFC_DATA_WRITE_IND: lCid=%d, result=%d \r\n", 
                    writeDataParam->lCid,
                    writeDataParam->result);
            }
            #endif /* DEBUG_UART_FULL */
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
        Disconnect_LED_Write(LED_OFF);
        Simulation_LED_Write(LED_OFF);
        Advertising_LED_INV();
    }
    else if(Cy_BLE_GetState() == CY_BLE_STATE_INITIALIZING || Cy_BLE_GetState() == CY_BLE_STATE_STOPPED)
    {   
        Advertising_LED_Write(LED_OFF);
        Disconnect_LED_Write(LED_OFF);
        Simulation_LED_Write(LED_OFF);
    }
    else if(Cy_BLE_GetNumOfActiveConn() == 0u)
    {   
        Advertising_LED_Write(LED_OFF);
        Disconnect_LED_Write(LED_ON);
        Simulation_LED_Write(LED_OFF);
    }
    else /* CY_BLE_CONN_STATE_CONNECTED) */
    {
        uint8_t i;
        uint8_t led = LED_OFF;
        Advertising_LED_Write(LED_OFF);
        Disconnect_LED_Write(LED_OFF);
        for(i = 0u; i < CY_BLE_CONN_COUNT; i++)
        {
            if(l2capReadReceived[i] == true)
            {
                led = LED_ON;
                break;
            }
        }
        Simulation_LED_Write(led);
    }
#else
    if(Cy_BLE_GetAdvertisementState() == CY_BLE_ADV_STATE_ADVERTISING)
    {
        /* Blink advertising indication LED. */
        LED5_INV();
    }
    else if(Cy_BLE_GetNumOfActiveConn() == 0u)
    {
        /* If in disconnected state, turn on disconnect indication LED */
        LED5_Write(LED_ON);
    }
    else 
    {
        /* In connected state */
        LED5_Write(LED_OFF);
    }
#endif /* #if(CYDEV_VDDD_MV >= RGB_LED_MIN_VOLTAGE_MV) */    
}


/*******************************************************************************
* Function Name: LowPowerImplementation
********************************************************************************
*
* Summary:
*  Implements low power in the project.
*
* Theory:
*  The function tries to enter deep sleep as much as possible - whenever the 
*  BLE is idle and the UART transmission/reception is not happening. 
*  At all other times, the function tries to enter CPU sleep.
*
*******************************************************************************/
static void LowPowerImplementation(void)
{
    if(UART_DEB_IS_TX_COMPLETE() != 0u)
    {
        /* Entering into the Deep Sleep */
        Cy_SysPm_DeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
    }
}

/*******************************************************************************
* Function Name:  HostMain
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
        
    /* Initialize wakeup pin for Hibernate */
    Cy_SysPm_SetHibWakeupSource(CY_SYSPM_HIBPIN1_LOW);
    
    /* Initialize LEDs */
    DisableAllLeds();
    
    /* Initialize Debug UART */
    UART_START();
    DBG_PRINTF("BLE Node Example Project \r\n");
    
    /* Start BLE component and register generic event handler */
    apiResult = Cy_BLE_Start(AppCallBack);   
    if(apiResult != CY_BLE_SUCCESS)
    {
        DBG_PRINTF("Cy_BLE_Start API Error: 0x%x \r\n", apiResult);
    }
    
    /* Print stack version */
    PrintStackVersion();
   
    /***************************************************************************
    * Main polling loop
    ***************************************************************************/
    while(1u)
    {          
        
        /* Cy_BLE_ProcessEvents() allows BLE stack to process pending events */
        Cy_BLE_ProcessEvents();

        /* Restart timer */
        if(mainTimer != 0u)
        {
            mainTimer = 0u;
            Cy_BLE_StartTimer(&timerParam); 
        }    
        
        /* To achieve low power in the device */
        LowPowerImplementation();

        if((Cy_BLE_GetState() == CY_BLE_STATE_ON) && (Cy_BLE_GetAdvertisementState() == CY_BLE_ADV_STATE_STOPPED) && 
           (Cy_BLE_GetNumOfActiveConn() < CONN_COUNT))
        {
            apiResult = Cy_BLE_GAPP_StartAdvertisement(CY_BLE_ADVERTISING_FAST, CY_BLE_PERIPHERAL_CONFIGURATION_0_INDEX);
            if(apiResult != CY_BLE_SUCCESS)
            {
                DBG_PRINTF("StartAdvertisement API Error: 0x%x \r\n", apiResult);
            }
        }

        if(Cy_BLE_GetNumOfActiveConn() > 0u) 
        {
            static uint8_t l2capIndex = 0u;
            
            l2capIndex++;
            if(l2capIndex >= CY_BLE_CONN_COUNT)
            {
                l2capIndex = 0u;
            }
            
            /* Keep sending the data to the router, until TX credits are over */
            if((cy_ble_busyStatus[l2capIndex] == 0u) && (l2capConnected[l2capIndex] == true) && 
               (l2capReadReceived[l2capIndex] == true))
            {
                cy_stc_ble_l2cap_cbfc_tx_data_info_t l2capDataParam =
                {
                    .buffer = ipv6LoopbackBuffer[l2capIndex],
                    .bufferLength = ipv6LoopbackLength[l2capIndex],
                    .localCid = l2capParameters[l2capIndex].lCid,
                };
                
                UpdateLedState();
                l2capReadReceived[l2capIndex] = false;
                
                apiResult = Cy_BLE_L2CAP_ChannelDataWrite(&l2capDataParam);
                DBG_PRINTF("-> Cy_BLE_L2CAP_ChannelDataWrite API result: %d \r\n", apiResult);
            }
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

    
/* [] END OF FILE */