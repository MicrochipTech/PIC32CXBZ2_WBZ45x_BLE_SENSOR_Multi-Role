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


/*******************************************************************************
  Application BLE Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_ble.c

  Summary:
    This file contains the Application BLE implementation for this project.

  Description:
    This file contains the Application BLE implementation for this project.
 *******************************************************************************/


#include "app.h"
#include "osal/osal_freertos_extend.h"
#include "app_ble.h"
#include "app_ble_handler.h"


#include "app_trsps_handler.h"
#include "app_trspc_handler.h"




#include "app_otaps_handler.h"






// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************

#define GAP_DEV_NAME_VALUE          "BLE_SENSOR_MULTIROLE"

// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************
BLE_DD_Config_T         ddConfig;

// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************

void APP_BleStackCb(STACK_Event_T *p_stack)
{
    STACK_Event_T stackEvent;
    APP_Msg_T   appMsg;
    APP_Msg_T   *p_appMsg;

    memcpy((uint8_t *)&stackEvent, (uint8_t *)p_stack, sizeof(STACK_Event_T));
    stackEvent.p_event=OSAL_Malloc(p_stack->evtLen);
    if(stackEvent.p_event==NULL)
    {
        return;
    }
    memcpy(stackEvent.p_event, p_stack->p_event, p_stack->evtLen);
    stackEvent.p_event=stackEvent.p_event;

    if (p_stack->groupId==STACK_GRP_GATT)
    {
        GATT_Event_T *p_evtGatt = (GATT_Event_T *)stackEvent.p_event;

        if (p_evtGatt->eventId == GATTS_EVT_CLIENT_CCCDLIST_CHANGE)
        {
            uint8_t *p_payload;

            p_payload = (uint8_t *)OSAL_Malloc((p_evtGatt->eventField.onClientCccdListChange.numOfCccd*4));
            if (p_payload != NULL)
            {
                memcpy(p_payload, (uint8_t *)p_evtGatt->eventField.onClientCccdListChange.p_cccdList, (p_evtGatt->eventField.onClientCccdListChange.numOfCccd*4));
                p_evtGatt->eventField.onClientCccdListChange.p_cccdList = (GATTS_CccdList_T *)p_payload;
            }
        }
    }

    appMsg.msgId=APP_MSG_BLE_STACK_EVT;

    ((STACK_Event_T *)appMsg.msgData)->groupId=p_stack->groupId;
    ((STACK_Event_T *)appMsg.msgData)->evtLen=p_stack->evtLen;
    ((STACK_Event_T *)appMsg.msgData)->p_event=stackEvent.p_event;

    p_appMsg = &appMsg;
    OSAL_QUEUE_Send(&appData.appQueue, p_appMsg, 0);
}

void APP_BleStackEvtHandler(STACK_Event_T *p_stackEvt)
{
    switch(p_stackEvt->groupId)
    {
        case STACK_GRP_BLE_GAP:
        {
            APP_BleGapEvtHandler((BLE_GAP_Event_T *)p_stackEvt->p_event);
        }
        break;
        
        case STACK_GRP_BLE_L2CAP:
        {
            APP_BleL2capEvtHandler((BLE_L2CAP_Event_T *)p_stackEvt->p_event);
         }
        break;
        case STACK_GRP_BLE_SMP:
        {
            APP_BleSmpEvtHandler((BLE_SMP_Event_T *)p_stackEvt->p_event);
         }
        break;

        case STACK_GRP_GATT:
        {
            APP_GattEvtHandler((GATT_Event_T *)p_stackEvt->p_event);
        }
        break;

        default:
        break;

    } 

    //Direct event to BLE middleware
    BLE_DM_BleEventHandler(p_stackEvt);

    BLE_DD_BleEventHandler(&ddConfig, p_stackEvt);

    //Direct event to BLE profiles
    /* Transparent Profile */
    BLE_TRSPS_BleEventHandler(p_stackEvt);

    BLE_TRSPC_BleEventHandler(p_stackEvt);




    /* Over-The-Air Profile */
    BLE_OTAPS_BleEventHandler(p_stackEvt);






    OSAL_Free(p_stackEvt->p_event);
}


void APP_BleStackLogHandler(BT_SYS_LogEvent_T *p_logEvt)
{
}

void APP_DdEvtHandler(BLE_DD_Event_T *p_event)
{

    BLE_TRSPC_BleDdEventHandler(p_event);




}



void APP_BleConfigBasic()
{
    int8_t                          connTxPower;
    int8_t                          advTxPower;
    BLE_GAP_AdvParams_T             advParam;
    
    // Configure advertising parameters
    BLE_GAP_SetAdvTxPowerLevel(14,&advTxPower);      /* Advertising TX Power */

    BLE_GAP_Addr_T devAddr;
    if (!IB_GetBdAddr(&devAddr.addr[0]) )
    {
        devAddr.addrType = BLE_GAP_ADDR_TYPE_PUBLIC;
        devAddr.addr[0] = rand();       //0x18;
        devAddr.addr[1] = rand();       //0x32;
        devAddr.addr[2] = rand();       //0x78;
        devAddr.addr[3] = rand();
        devAddr.addr[4] = 0xB7;
        devAddr.addr[5] = 0xC8;
        // Configure device address
        BLE_GAP_SetDeviceAddr(&devAddr);
    }
   
    memset(&advParam, 0, sizeof(BLE_GAP_AdvParams_T));
    advParam.intervalMin = 512;     /* Advertising Interval Min */
    advParam.intervalMax = 512;     /* Advertising Interval Max */
    advParam.type = BLE_GAP_ADV_TYPE_ADV_IND;        /* Advertising Type */
    advParam.advChannelMap = BLE_GAP_ADV_CHANNEL_ALL;        /* Advertising Channel Map */
    advParam.filterPolicy = BLE_GAP_ADV_FILTER_DEFAULT;     /* Advertising Filter Policy */
    BLE_GAP_SetAdvParams(&advParam);



    BLE_GAP_SetConnTxPowerLevel(10, &connTxPower);      /* Connection TX Power */
}
void APP_BleConfigAdvance()
{
    uint8_t devName[]={GAP_DEV_NAME_VALUE};

    BLE_SMP_Config_T                smpParam;
    

    BLE_GAP_ScanningParams_T        scanParam;
    BLE_DM_Config_T                 dmConfig;
    BLE_GAP_ServiceOption_T         gapServiceOptions;
    

    // Configure Device Name
    BLE_GAP_SetDeviceName(sizeof(devName), devName);    /* Device Name */
    

    // GAP Service option
    gapServiceOptions.charDeviceName.enableWriteProperty = false;             /* Enable Device Name Write Property */
    gapServiceOptions.charAppearance.appearance = 0x0;                          /* Appearance */
    gapServiceOptions.charPeriPreferConnParam.enable = false;                    /* Enable Peripheral Preffered Connection Parameters */

    BLE_GAP_ConfigureBuildInService(&gapServiceOptions);


    // Configure scan parameters
    scanParam.type = BLE_GAP_SCAN_TYPE_PASSIVE_SCAN;      /* Scan Type */
    scanParam.interval = 160;      /* Scan Interval */
    scanParam.window = 32;      /* Scan Window */
    scanParam.filterPolicy = BLE_GAP_SCAN_FP_ACCEPT_ALL;       /* Scan Filter Policy */
    scanParam.disChannel = 0;      /* Disable specific channel during scanning */
    BLE_GAP_SetScanningParam(&scanParam);


    // Configure SMP parameters
    memset(&smpParam, 0, sizeof(BLE_SMP_Config_T));
    smpParam.ioCapability = BLE_SMP_IO_NOINPUTNOOUTPUT;                  /* IO Capability */
    smpParam.authReqFlag |= BLE_SMP_OPTION_BONDING;             /* Authentication Setting: Bonding */
    smpParam.authReqFlag |= BLE_SMP_OPTION_SECURE_CONNECTION;   /* Authentication Setting: Secure Connections */
    BLE_SMP_Config(&smpParam);

    // Configure BLE_DM middleware parameters
    dmConfig.secAutoAccept = true;                          /* Auto Accept Security Request */
    dmConfig.connConfig.autoReplyUpdateRequest = true;      /* Auto Accept Connection Parameter Update Request */
    dmConfig.connConfig.minAcceptConnInterval = 6;    /* Minimum Connection Interval */
    dmConfig.connConfig.maxAcceptConnInterval = 3200;    /* Maximum Connection Interval */
    dmConfig.connConfig.minAcceptPeripheralLatency = 0;    /* Minimum Connection Latency */
    dmConfig.connConfig.maxAcceptPeripheralLatency = 499;    /* Maximum Connection Latency */
    BLE_DM_Config(&dmConfig);


    // Configure BLE_DD middleware parameters
    ddConfig.waitForSecurity = false;
    ddConfig.initDiscInCentral = true;
    ddConfig.initDiscInPeripheral = false;
}

void APP_BleStackInitBasic()
{
    BLE_GAP_Init();

    BLE_GAP_AdvInit();  /* Advertising */

    BLE_GAP_ConnPeripheralInit();   /* Peripheral */
}

void APP_BleStackInitAdvance()
{
    uint16_t gattsInitParam=GATTS_CONFIG_NONE;
    
    uint16_t gattcInitParam=GATTC_CONFIG_NONE;

    STACK_EventRegister(APP_BleStackCb);
    


    BLE_GAP_ScanInit();     /* Scan */

    BLE_GAP_ConnCentralInit();  /* Central */
    
    BLE_L2CAP_Init();
    
    GATTS_Init(gattsInitParam);

    GATTC_Init(gattcInitParam);     /* Enable Client Role */

    BLE_SMP_Init();
    

    //Initialize BLE middleware
    BLE_DM_Init();
    BLE_DM_EventRegister(APP_DmEvtHandler);

    BLE_DD_Init();
    BLE_DD_EventRegister(APP_DdEvtHandler);



    //Initialize BLE services

    //Initialize BLE profiles
    /* Transparent Profile */
    BLE_TRSPS_Init();                                   /* Enable Server Role */
    BLE_TRSPS_EventRegister(APP_TrspsEvtHandler);   /* Enable Server Role */
    
    BLE_TRSPC_Init();                                   /* Enable Client Role */
    BLE_TRSPC_EventRegister(APP_TrspcEvtHandler);   /* Enable Client Role */
#ifdef APP_PAIRING_ENABLE    
    BLE_TRS_PermissionConfig(TRS_HDL_CCCD_CTRL, PERMISSION_WRITE_ENC);
    BLE_TRS_PermissionConfig(TRS_HDL_CCCD_TX, PERMISSION_WRITE_ENC);
#endif




    /* Over-The-Air Profile */
    BLE_OTAPS_Init();                                   /* Enable Server Role */
    BLE_OTAPS_EventRegister(APP_OtapsEvtHandler);   /* Enable Server Role */

    




    APP_BleConfigAdvance();
}

void APP_BleStackInit()
{
    APP_BleStackInitBasic();
    APP_BleConfigBasic();
    APP_BleStackInitAdvance();
}
