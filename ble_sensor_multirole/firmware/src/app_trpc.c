/*******************************************************************************
  Application Transparent Service  (client) Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_trpc.c

  Summary:
    This file contains the Application Transparent Client Role functions for this project.

  Description:
    This file contains the Application Transparent Client Role functions for this project.
    The implementation of demo mode is included in this file.
 *******************************************************************************/

//DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2020 Microchip Technology Inc. and its subsidiaries.
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
//DOM-IGNORE-END

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include <string.h>
#include <stdint.h>
#include "stack_mgr.h"
#include "ble_trspc/ble_trspc.h"
#include "app_trpc.h"
#include "mba_error_defs.h"
#include "app_error_defs.h"
#include "app_ble_conn_handler.h"
#include "system/console/sys_console.h"

// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************

static APP_MultiLink_SensorData_T multiLinkSensorData[BLE_GAP_MAX_LINK_NBR];


// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************

void APP_mLink_SensorData_Init(void)
{
    memset(&multiLinkSensorData, 0, sizeof(multiLinkSensorData));
}

void APP_mLink_Clear_SensData_ByConnHandle(uint16_t connHandle)
{
    for (uint8_t i = 0; i < BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (multiLinkSensorData[i].connHandle == connHandle)
        {
            memset((uint8_t *)(&multiLinkSensorData[i]), 0, sizeof(APP_MultiLink_SensorData_T));

            multiLinkSensorData[i].connHandle = 0;
        }
    }
}

static APP_MultiLink_SensorData_T *APP_mLink_GetFreeSensData_Item(void)
{
    for (uint8_t i = 0; i < BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (multiLinkSensorData[i].connHandle == 0)
        {
            return (&multiLinkSensorData[i]);
        }
    }
    return NULL;
}

uint16_t APP_mLink_GetConnHandleByIndex(uint8_t index)
{
    if (index < BLE_GAP_MAX_LINK_NBR)
    {
        if (multiLinkSensorData[index].connHandle != 0)
        {
            return multiLinkSensorData[index].connHandle;
        }
    }

    return 0xFFFF;
}

APP_MultiLink_SensorData_T *APP_mLink_GetSensDataByConnHandle(uint16_t connHandle)
{
    for (uint8_t i = 0; i < BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (multiLinkSensorData[i].connHandle == connHandle)
        {
            return (&multiLinkSensorData[i]);
        }
    }
    return NULL;
}

void APP_mLink_Temperature_Update(uint16_t connHandle, uint16_t temperature) 
{
    APP_MultiLink_SensorData_T *sensDataItem;
    
    sensDataItem = APP_mLink_GetSensDataByConnHandle(connHandle);
    if( sensDataItem == NULL )
    {
        sensDataItem = APP_mLink_GetFreeSensData_Item();
    }
    
    if( sensDataItem != NULL)
    {
        sensDataItem->connHandle = connHandle;
        sensDataItem->mLinkSensorData.tempSens.msb = (uint8_t)(temperature & 0xFF);
        sensDataItem->mLinkSensorData.tempSens.lsb = (uint8_t)(temperature>>8 & 0xFF);
    }
}

void APP_TRPC_OnOffGet_cmd(void)
{
    uint8_t payload_cmd[2] = {0x01, 0x11};
    APP_BLE_ConnList_T    *cBleLink;
    uint16_t connHandle;
    uint16_t result = MBA_RES_SUCCESS;
    uint8_t retryCount = 0;
    
    for(uint8_t i = 0; i<BLE_GAP_MAX_LINK_NBR; i++)
    {
        connHandle = APP_GetConnHandleByIndex(i);
        if(connHandle != 0xFFFF )
        {
            cBleLink  = APP_GetConnInfoByConnHandle(connHandle);
            if( (cBleLink != NULL) && (cBleLink->linkState == APP_BLE_STATE_CONNECTED) && (cBleLink->connData.role == 0) )
            {
                retryCount = 0;
                do
                {
                    result = BLE_TRSPC_SendVendorCommand(connHandle, APP_TRP_VENDOR_OPCODE_BLE_SENSOR, sizeof(payload_cmd), payload_cmd);
                    retryCount++;
                    if(retryCount > 5)
                        break;
                    if(result != MBA_RES_SUCCESS)
                        vTaskDelay(100  /portTICK_PERIOD_MS);
                }
                while(result != MBA_RES_SUCCESS);
                SYS_CONSOLE_PRINT("%d -> Get onOff[0x%X]: 0x%X\r\n", retryCount, connHandle, result);
            }
        }
    }
}

void APP_TRPC_OnOffSet_cmd(uint8_t onOff_cmd)
{
    uint8_t payload_cmd[3] = {0x02, 0x10, 0x00};
    APP_BLE_ConnList_T    *cBleLink;
    uint16_t connHandle;
    uint16_t result = MBA_RES_SUCCESS;
    uint8_t retryCount = 0;
  
    payload_cmd[2] = onOff_cmd;
    for(uint8_t i = 0; i<BLE_GAP_MAX_LINK_NBR; i++)
    {
        connHandle = APP_GetConnHandleByIndex(i);
        if(connHandle != 0xFFFF )
        {
            cBleLink  = APP_GetConnInfoByConnHandle(connHandle);
            if( (cBleLink != NULL) && (cBleLink->linkState == APP_BLE_STATE_CONNECTED) && (cBleLink->connData.role == 0) )
            {
                retryCount = 0;
                do
                {
                    result = BLE_TRSPC_SendVendorCommand(connHandle, APP_TRP_VENDOR_OPCODE_BLE_SENSOR, sizeof(payload_cmd), payload_cmd);
                    retryCount++;
                    if(retryCount > 5)
                        break;
                    if(result != MBA_RES_SUCCESS)
                        vTaskDelay(100  /portTICK_PERIOD_MS);
                }
                while(result != MBA_RES_SUCCESS);
//                SYS_CONSOLE_PRINT("%d -> onOff[0x%X]: 0x%X\r\n", retryCount, connHandle, result);
            }
        }
    }
}

void APP_TRPC_RGBSet_cmd(uint8_t *hue, uint8_t *saturation, uint8_t *level)
{
    uint8_t payload_cmd[5] = {0x03, 0x12, 0x00, 0x00, 0x00};
    APP_BLE_ConnList_T    *cBleLink;
    uint16_t connHandle;
    uint16_t result = MBA_RES_SUCCESS;
    uint8_t retryCount = 0;
  
    payload_cmd[2] = *hue;
    payload_cmd[3] = *saturation;
    payload_cmd[4] = *level;
    for(uint8_t i = 0; i<BLE_GAP_MAX_LINK_NBR; i++)
    {
        connHandle = APP_GetConnHandleByIndex(i);
        if(connHandle != 0xFFFF )
        {
            retryCount = 0;
            cBleLink  = APP_GetConnInfoByConnHandle(connHandle);
            if( (cBleLink != NULL) && (cBleLink->linkState == APP_BLE_STATE_CONNECTED) && (cBleLink->connData.role == 0) )
            {
                do
                {
                    result = BLE_TRSPC_SendVendorCommand(connHandle, APP_TRP_VENDOR_OPCODE_BLE_SENSOR, sizeof(payload_cmd), payload_cmd);
                    retryCount++;
                    if(retryCount > 5)
                        break;
                    if(result != MBA_RES_SUCCESS)
                        vTaskDelay(100  /portTICK_PERIOD_MS);
                }
                while(result != MBA_RES_SUCCESS);
//                SYS_CONSOLE_PRINT("%d -> RGB_Color[0x%X]: 0x%X\r\n", retryCount, connHandle, result);
            }
        }
    }
}

/* *****************************************************************************
 End of File
 */
