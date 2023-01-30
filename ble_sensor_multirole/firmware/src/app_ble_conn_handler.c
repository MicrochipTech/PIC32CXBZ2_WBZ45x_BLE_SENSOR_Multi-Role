/*******************************************************************************
  Application BLE Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_ble_conn_handler.c

  Summary:
    This file contains the Application BLE functions for this project.

  Description:
    This file contains the Application BLE functions for this project.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2022 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <string.h>
#include <stdint.h>
#include "app_ble_conn_handler.h"
#include "ble_dm/ble_dm.h"
#include "app_timer/app_timer.h"
#include "app_adv.h"
#include "app_trps.h"
#include "app_trpc.h"
#include "app_ble_sensor.h"
#include "peripheral/gpio/plib_gpio.h"
#include "system/console/sys_console.h"

// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************

#define AD_TYPE_SERVICE_ID                      0x16
#define AD_TYPE_SERVICE_UUID                    0xFEDA        
#define AD_TYPE_SERVICE_DATA_BLE_SENSOR         0xFF02
#define AD_TYPE_SERVICE_DATA_COMBO_LIGHT        0xFF03
#define AD_TYPE_SERVICE_DATA_INTERMEDIATE       0xFFAA
// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************
static APP_BLE_ConnList_T                   s_bleConnList[BLE_GAP_MAX_LINK_NBR];
static APP_BLE_ConnList_T                   *sp_currentBleLink = NULL;
static uint8_t peripheralDevCount = 0;
// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************
static void APP_ClearConnListByConnHandle(uint16_t connHandle)
{
    uint8_t i;

    for (i = 0; i < BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (s_bleConnList[i].connData.handle == connHandle)
        {
            memset((uint8_t *)(&s_bleConnList[i]), 0, sizeof(APP_BLE_ConnList_T));

            s_bleConnList[i].linkState= APP_BLE_STATE_STANDBY;
        }
    }
}

static APP_BLE_ConnList_T *APP_GetFreeConnList(void)
{
    uint8_t i;

    for (i = 1; i < BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (s_bleConnList[i].connData.handle == 0)
        {
            return (&s_bleConnList[i]);
        }
    }
    return NULL;
}

uint8_t APP_GetConnectedDevice_Count(void)
{
    return peripheralDevCount;
}

APP_BLE_ConnList_T *APP_GetConnInfoByConnHandle(uint16_t connHandle)
{
    uint8_t i;

    for (i = 0; i < BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (s_bleConnList[i].connData.handle == connHandle)
        {
            return (&s_bleConnList[i]);
        }
    }
    return NULL;
}

uint16_t APP_GetConnHandleByIndex(uint8_t index)
{
    if (index < BLE_GAP_MAX_LINK_NBR)
    {
        if ((s_bleConnList[index].connData.handle != 0) && (s_bleConnList[index].linkState != APP_BLE_STATE_STANDBY))
            return s_bleConnList[index].connData.handle;
    }

    return 0xFFFF;
}

bool APP_CheckForServiceItemInAdvertisingString(uint8_t *adv, uint8_t advLength)
{
	uint8_t currentPos = 0;
	uint8_t sectionLength;
	uint16_t service_uuid, service_data;
	
	while(currentPos < advLength)
	{
		sectionLength = adv[currentPos];
		
		if( adv[currentPos+1] == AD_TYPE_SERVICE_ID)
		{
			service_uuid = ( adv[currentPos+2+1]<<8 | adv[currentPos+2]   );
			service_data = ( adv[currentPos+2+2]<<8 | adv[currentPos+2+3] );  

			if((service_uuid == AD_TYPE_SERVICE_UUID) && ((service_data == AD_TYPE_SERVICE_DATA_BLE_SENSOR) || (service_data == AD_TYPE_SERVICE_DATA_COMBO_LIGHT)) )
//            if((service_uuid == AD_TYPE_SERVICE_UUID) && (service_data == AD_TYPE_SERVICE_DATA_INTERMEDIATE)  )
			{
				return true; 
			}
			else
			{
				currentPos += (sectionLength + 1);
			}
		}
		else
		{
			currentPos += (sectionLength + 1);
		}
			
		
	}
	return false;
}

bool APP_Check_DeviceAddressByMAC(BLE_GAP_Addr_T *addr)
{
    for (uint8_t i = 0; i < BLE_GAP_MAX_LINK_NBR; i++)
    {
        if(memcmp(addr->addr, s_bleConnList[i].connData.remoteAddr.addr, GAP_MAX_BD_ADDRESS_LEN) == 0)
        {
            return true;
        }
    }
    return false;
}

void APP_BleGapConnEvtHandler(BLE_GAP_Event_T *p_event)
{
    APP_BLE_ConnList_T *p_bleConn = NULL;    
    switch(p_event->eventId)
    {
        case BLE_GAP_EVT_CONNECTED:
        {
//            SYS_CONSOLE_MESSAGE("BLE_GAP_EVT_CONNECTED\r\n");
            if( (p_event->eventField.evtConnect.role == 0x01) /*&& (APP_GetBleState() == APP_BLE_STATE_ADVERTISING)*/ )
            {
                if( s_bleConnList[0].connData.handle == 0)
                {
                    p_bleConn = &s_bleConnList[0];
                    SYS_CONSOLE_MESSAGE("[BLE] Mobile Device Detected\r\n");
                }
                else
                {
                    SYS_CONSOLE_MESSAGE("[BLE] Mobile Device Already Connected\r\n");
                }
            }
            else
            {
                peripheralDevCount++;
                p_bleConn = APP_GetFreeConnList();
            }

            if (p_bleConn)
            {
                GATTS_UpdateBondingInfo(p_event->eventField.evtConnect.connHandle, NULL, 0, NULL); 

                /* Update the connection parameter */
                p_bleConn->linkState                        = APP_BLE_STATE_CONNECTED;
                p_bleConn->connData.role                    = p_event->eventField.evtConnect.role;        // 0x00: WBZ45x Central, 0x01: WBZ45x Peripheral
                p_bleConn->connData.handle                  = p_event->eventField.evtConnect.connHandle;
                p_bleConn->connData.connInterval            = p_event->eventField.evtConnect.interval; 
                p_bleConn->connData.connLatency             = p_event->eventField.evtConnect.latency;    
                p_bleConn->connData.supervisionTimeout      = p_event->eventField.evtConnect.supervisionTimeout;

                /* Save Remote Device Address */
                p_bleConn->connData.remoteAddr.addrType = p_event->eventField.evtConnect.remoteAddr.addrType;                
                memcpy((uint8_t *)p_bleConn->connData.remoteAddr.addr, (uint8_t *)p_event->eventField.evtConnect.remoteAddr.addr, GAP_MAX_BD_ADDRESS_LEN);

                SYS_CONSOLE_MESSAGE("[BLE] Connected to Peer Device: 0x");
                int8_t idx;
                for(idx=(GAP_MAX_BD_ADDRESS_LEN-1); idx>=0; idx--)
                    SYS_CONSOLE_PRINT("%02X", p_bleConn->connData.remoteAddr.addr[idx]);
                SYS_CONSOLE_PRINT("\n\r[BLE] Connection Handle: 0x%X\n\rDevice Count: %d\r\n",p_bleConn->connData.handle,peripheralDevCount);

//                APP_TIMER_StopTimer(APP_TIMER_ADV_CTRL);
//                USER_LED_Clear();                             
            }
            else
            {
                SYS_CONSOLE_PRINT("[Error] Conn Memory Full: %d\r\n", peripheralDevCount);
            }
        }
        break;

        case BLE_GAP_EVT_DISCONNECTED:
        {
            extern bool scanStart;
            SYS_CONSOLE_PRINT("[BLE] Disconnected Handle - 0x%X: Reason: 0x%X\r\n",p_event->eventField.evtDisconnect.connHandle,
                                        p_event->eventField.evtDisconnect.reason);
            APP_TRPS_DiscEvtProc(p_event->eventField.evtDisconnect.connHandle);
            APP_TRPS_Sensor_DiscEvtProc();
            
            /* Restart advertising */
            if(p_event->eventField.evtDisconnect.connHandle == s_bleConnList[0].connData.handle)
            {
                scanStart = false;
                APP_SetBleState(APP_BLE_STATE_STANDBY);
            }
            else
            {
                
                scanStart = true;
                peripheralDevCount--;
            }
            APP_TIMER_SetTimer(APP_TIMER_SCAN_DELAY, APP_TIMER_2S, false);
            //Clear connection list
            APP_ClearConnListByConnHandle(p_event->eventField.evtDisconnect.connHandle);
            APP_mLink_Clear_SensData_ByConnHandle(p_event->eventField.evtDisconnect.connHandle);
        }
        break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE:
        {
            /* Update the connection parameter */
            if (p_event->eventField.evtConnParamUpdate.status == 0)
            {

                p_bleConn = APP_GetConnInfoByConnHandle(p_event->eventField.evtConnParamUpdate.connHandle);

                if (p_bleConn)
                {
                    p_bleConn->connData.handle                  = p_event->eventField.evtConnParamUpdate.connHandle;
                    p_bleConn->connData.connInterval            = p_event->eventField.evtConnParamUpdate.connParam.intervalMin;
                    p_bleConn->connData.connLatency             = p_event->eventField.evtConnParamUpdate.connParam.latency;
                    p_bleConn->connData.supervisionTimeout      = p_event->eventField.evtConnParamUpdate.connParam.supervisionTimeout;
                }
            }
        }
        break;

        case BLE_GAP_EVT_ENCRYPT_STATUS:
        {
            p_bleConn = APP_GetConnInfoByConnHandle(p_event->eventField.evtEncryptStatus.connHandle);

            if (p_event->eventField.evtEncryptStatus.status != BLE_GAP_ENCRYPT_SUCCESS)
            {
                BLE_GAP_Disconnect(p_event->eventField.evtEncryptStatus.connHandle, GAP_DISC_REASON_REMOTE_TERMINATE);
            }
        }
        break;

        case BLE_GAP_EVT_ADV_REPORT:
        {
            if(APP_CheckForServiceItemInAdvertisingString(p_event->eventField.evtAdvReport.advData,
                p_event->eventField.evtAdvReport.length) )
            {
                APP_Msg_T appMsg;
                appMsg.msgId = APP_MSG_BLE_SCAN_EVT;
                memcpy(appMsg.msgData, &p_event->eventField.evtAdvReport, sizeof(BLE_GAP_EvtAdvReport_T));
                OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
            }
        }
        break;

        case BLE_GAP_EVT_ENC_INFO_REQUEST:
        {
            /* TODO: implement your application code.*/
        }
        break;

        case BLE_GAP_EVT_REMOTE_CONN_PARAM_REQUEST:
        {
            /* TODO: implement your application code.*/
        }
        break;

        case BLE_GAP_EVT_EXT_ADV_REPORT:
        {
            /* TODO: implement your application code.*/
        }
        break;

        case BLE_GAP_EVT_ADV_TIMEOUT:
        {
            /* TODO: implement your application code.*/
        }
        break;

        case BLE_GAP_EVT_TX_BUF_AVAILABLE:
        {
            /* TODO: implement your application code.*/
        }
        break;

        case BLE_GAP_EVT_DEVICE_NAME_CHANGED:
        {
            /* TODO: implement your application code.*/
        }
        break;

        case BLE_GAP_EVT_AUTH_PAYLOAD_TIMEOUT:
        {
            /* TODO: implement your application code.*/
        }
        break;

        case BLE_GAP_EVT_PHY_UPDATE:
        {
            /* TODO: implement your application code.*/
        }
        break;

        case BLE_GAP_EVT_SCAN_REQ_RECEIVED:
        {
            /* TODO: implement your application code.*/
        }
        break;

        case BLE_GAP_EVT_DIRECT_ADV_REPORT:
        {
            /* TODO: implement your application code.*/
        }
        break;

        case BLE_GAP_EVT_PERI_ADV_SYNC_EST:
        {
            /* TODO: implement your application code.*/
        }
        break;

        case BLE_GAP_EVT_PERI_ADV_REPORT:
        {
            /* TODO: implement your application code.*/
        }
        break;

        case BLE_GAP_EVT_PERI_ADV_SYNC_LOST:
        {
            /* TODO: implement your application code.*/
        }
        break;

        case BLE_GAP_EVT_ADV_SET_TERMINATED:
        {
            /* TODO: implement your application code.*/
        }
        break;

        case BLE_GAP_EVT_SCAN_TIMEOUT:
        {
            extern bool scanStart;
            scanStart = true;
            APP_TIMER_StopTimer(APP_TIMER_ADV_CTRL);
            USER_LED_Set(); 
            SYS_CONSOLE_PRINT("Scanning Time out\r\n");
        }
        break;

        default:
        break;
    }
}

APP_BLE_LinkState_T APP_GetBleState(void)
{
    return (s_bleConnList[0].linkState);
}

void APP_SetBleState(APP_BLE_LinkState_T state)
{
    s_bleConnList[0].linkState = state;
}

void APP_InitConnList(void)
{
    uint8_t i;

    sp_currentBleLink = &s_bleConnList[0];       
    
    for (i = 0; i < BLE_GAP_MAX_LINK_NBR; i++)
    {
        memset((uint8_t *)(&s_bleConnList[i]), 0, sizeof(APP_BLE_ConnList_T));

        s_bleConnList[i].linkState= APP_BLE_STATE_STANDBY;
    }
}
