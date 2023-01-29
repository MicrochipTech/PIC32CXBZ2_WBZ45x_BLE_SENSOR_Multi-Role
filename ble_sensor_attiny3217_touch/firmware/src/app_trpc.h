/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.h

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

#ifndef APP_TRPC_H
#define APP_TRPC_H


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include "ble_gap.h"
#include "ble_trspc/ble_trspc.h"
#include "app_ble_sensor.h"
#include "osal/osal_freertos_extend.h"

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint16_t connHandle;
    APP_TRPS_SensorData_T mLinkSensorData;
}APP_MultiLink_SensorData_T;


void APP_TRPC_OnOffGet_cmd(void);
void APP_TRPC_OnOffSet_cmd(uint8_t onOff_cmd);
void APP_TRPC_RGBSet_cmd(uint8_t *hue, uint8_t *saturation, uint8_t *level);
void APP_mLink_SensorData_Init(void);
void APP_mLink_Temperature_Update(uint16_t connHandle, uint16_t temperature);
uint16_t APP_mLink_GetConnHandleByIndex(uint8_t index);
APP_MultiLink_SensorData_T *APP_mLink_GetSensDataByConnHandle(uint16_t connHandle);
void APP_mLink_Clear_SensData_ByConnHandle(uint16_t connHandle);

    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* APP_TRPC_H */

/* *****************************************************************************
 End of File
 */
