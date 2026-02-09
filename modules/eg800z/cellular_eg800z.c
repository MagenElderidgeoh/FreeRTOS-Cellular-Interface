/*
 * FreeRTOS-Cellular-Interface
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 */

/**
 * @file cellular_eg800z.c
 * @brief Implementation of Quectel EG800Z LTE module support
 */

/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Cellular includes. */
#ifndef CELLULAR_DO_NOT_USE_CUSTOM_CONFIG
    #include "cellular_config.h"
#endif
#include "cellular_config_defaults.h"
#include "cellular_types.h"
#include "cellular_common.h"
#include "cellular_common_api.h"
#include "cellular_common_portable.h"
#include "cellular_at_core.h"

/* EG800Z module includes. */
#include "cellular_eg800z.h"

/*-----------------------------------------------------------*/

/**
 * @brief Module context for EG800Z.
 */
typedef struct CellularEG800Z_ModuleContext
{
    uint32_t moduleInitialized;
    char mqttClientId[ CELLULAR_EG800Z_MAX_MQTT_CLIENTS ][ 64 ];  /* Store client identifiers */
} CellularEG800Z_ModuleContext_t;

/*-----------------------------------------------------------*/

/* Forward declarations of static functions */

/*-----------------------------------------------------------*/

/**
 * @brief Initialize the EG800Z module.
 */
CellularError_t Cellular_ModuleInit( const CellularContext_t * pContext,
                                    void ** ppModuleContext )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularEG800Z_ModuleContext_t * pModuleContext = NULL;

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( ppModuleContext == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        pModuleContext = ( CellularEG800Z_ModuleContext_t * ) Platform_Malloc(
            sizeof( CellularEG800Z_ModuleContext_t ) );

        if( pModuleContext != NULL )
        {
            ( void ) memset( pModuleContext, 0, sizeof( CellularEG800Z_ModuleContext_t ) );
            pModuleContext->moduleInitialized = 1;
            *ppModuleContext = pModuleContext;
        }
        else
        {
            cellularStatus = CELLULAR_NO_MEMORY;
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief Clean up the EG800Z module.
 */
CellularError_t Cellular_ModuleCleanUp( const CellularContext_t * pContext )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularEG800Z_ModuleContext_t * pModuleContext = NULL;

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        pModuleContext = ( CellularEG800Z_ModuleContext_t * ) pContext->pModuleContext;

        if( pModuleContext != NULL )
        {
            Platform_Free( pModuleContext );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief Enable the EG800Z user equipment.
 */
CellularError_t Cellular_ModuleEnableUE( CellularContext_t * pContext )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularAtReq_t atReqGetNoResult = { 0 };
    CellularAtReq_t atReqGetWithResult = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Disable echo. */
        atReqGetNoResult.pAtCmd = "ATE0";
        atReqGetNoResult.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqGetNoResult.pAtRspPrefix = NULL;
        atReqGetNoResult.respCallback = NULL;
        atReqGetNoResult.pData = NULL;
        atReqGetNoResult.dataLen = 0;
        cellularStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqGetNoResult );

        if( cellularStatus == CELLULAR_SUCCESS )
        {
            /* Enable verbose error messages. */
            atReqGetNoResult.pAtCmd = "AT+CMEE=2";
            cellularStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqGetNoResult );
        }

        if( cellularStatus == CELLULAR_SUCCESS )
        {
            /* Check module firmware version - AT+CGMR */
            atReqGetWithResult.pAtCmd = "AT+CGMR";
            atReqGetWithResult.atCmdType = CELLULAR_AT_WITH_PREFIX;
            atReqGetWithResult.pAtRspPrefix = NULL;
            atReqGetWithResult.respCallback = NULL;
            atReqGetWithResult.pData = NULL;
            atReqGetWithResult.dataLen = 0;
            cellularStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqGetWithResult );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief Enable URC (Unsolicited Result Code) for EG800Z.
 */
CellularError_t Cellular_ModuleEnableUrc( CellularContext_t * pContext )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Enable network registration URC. */
        /* Enable other module-specific URCs as needed. */
        cellularStatus = CELLULAR_SUCCESS;
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief MQTT Open - Configure and open connection to MQTT broker.
 */
CellularError_t Cellular_EG800Z_MqttOpen( CellularHandle_t cellularHandle,
                                          const CellularEG800Z_MqttConfig_t * pMqttConfig )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { 0 };
    CellularAtReq_t atReqOpenMqtt = { 0 };

    /* Validate parameters. */
    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( pMqttConfig == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( pMqttConfig->clientId >= CELLULAR_EG800Z_MAX_MQTT_CLIENTS )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( ( pMqttConfig->pHostName == NULL ) || ( pMqttConfig->pClientIdentifier == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        CellularEG800Z_ModuleContext_t * pModuleContext = ( CellularEG800Z_ModuleContext_t * ) pContext->pModuleContext;

        /* Store client identifier for later use */
        if( pModuleContext != NULL )
        {
            ( void ) strncpy( pModuleContext->mqttClientId[ pMqttConfig->clientId ],
                            pMqttConfig->pClientIdentifier,
                            sizeof( pModuleContext->mqttClientId[ pMqttConfig->clientId ] ) - 1 );
            pModuleContext->mqttClientId[ pMqttConfig->clientId ][ sizeof( pModuleContext->mqttClientId[ pMqttConfig->clientId ] ) - 1 ] = '\0';
        }

        /* Configure MQTT parameters - AT+QMTCFG */
        /* Configure SSL context if SSL is enabled */
        if( pMqttConfig->sslContextId != 0xFF )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                             "AT+QMTCFG=\"ssl\",%u,%u,0",
                             pMqttConfig->clientId,
                             pMqttConfig->sslContextId );

            atReqOpenMqtt.pAtCmd = cmdBuf;
            atReqOpenMqtt.atCmdType = CELLULAR_AT_NO_RESULT;
            atReqOpenMqtt.pAtRspPrefix = NULL;
            atReqOpenMqtt.respCallback = NULL;
            atReqOpenMqtt.pData = NULL;
            atReqOpenMqtt.dataLen = 0;

            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqOpenMqtt );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }

        /* Open MQTT connection - AT+QMTOPEN */
        if( cellularStatus == CELLULAR_SUCCESS )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                             "AT+QMTOPEN=%u,\"%s\",%u",
                             pMqttConfig->clientId,
                             pMqttConfig->pHostName,
                             pMqttConfig->port );

            atReqOpenMqtt.pAtCmd = cmdBuf;
            atReqOpenMqtt.atCmdType = CELLULAR_AT_NO_RESULT;
            atReqOpenMqtt.pAtRspPrefix = NULL;
            atReqOpenMqtt.respCallback = NULL;
            atReqOpenMqtt.pData = NULL;
            atReqOpenMqtt.dataLen = 0;

            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqOpenMqtt );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief MQTT Connect - Connect to MQTT broker.
 */
CellularError_t Cellular_EG800Z_MqttConnect( CellularHandle_t cellularHandle,
                                             uint8_t clientId )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { 0 };
    CellularAtReq_t atReqConnectMqtt = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( clientId >= CELLULAR_EG800Z_MAX_MQTT_CLIENTS )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        CellularEG800Z_ModuleContext_t * pModuleContext = ( CellularEG800Z_ModuleContext_t * ) pContext->pModuleContext;
        const char * pClientIdentifier = "client_default";

        /* Use stored client identifier if available */
        if( ( pModuleContext != NULL ) && ( pModuleContext->mqttClientId[ clientId ][ 0 ] != '\0' ) )
        {
            pClientIdentifier = pModuleContext->mqttClientId[ clientId ];
        }

        /* Connect to MQTT broker - AT+QMTCONN */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                         "AT+QMTCONN=%u,\"%s\"",
                         clientId, pClientIdentifier );

        atReqConnectMqtt.pAtCmd = cmdBuf;
        atReqConnectMqtt.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqConnectMqtt.pAtRspPrefix = NULL;
        atReqConnectMqtt.respCallback = NULL;
        atReqConnectMqtt.pData = NULL;
        atReqConnectMqtt.dataLen = 0;

        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqConnectMqtt );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief MQTT Disconnect - Disconnect from MQTT broker.
 */
CellularError_t Cellular_EG800Z_MqttDisconnect( CellularHandle_t cellularHandle,
                                                uint8_t clientId )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { 0 };
    CellularAtReq_t atReqDisconnectMqtt = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( clientId >= CELLULAR_EG800Z_MAX_MQTT_CLIENTS )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Disconnect from MQTT broker - AT+QMTDISC */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                         "AT+QMTDISC=%u",
                         clientId );

        atReqDisconnectMqtt.pAtCmd = cmdBuf;
        atReqDisconnectMqtt.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqDisconnectMqtt.pAtRspPrefix = NULL;
        atReqDisconnectMqtt.respCallback = NULL;
        atReqDisconnectMqtt.pData = NULL;
        atReqDisconnectMqtt.dataLen = 0;

        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqDisconnectMqtt );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief MQTT Close - Close MQTT client connection.
 */
CellularError_t Cellular_EG800Z_MqttClose( CellularHandle_t cellularHandle,
                                           uint8_t clientId )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { 0 };
    CellularAtReq_t atReqCloseMqtt = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( clientId >= CELLULAR_EG800Z_MAX_MQTT_CLIENTS )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Close MQTT client - AT+QMTCLOSE */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                         "AT+QMTCLOSE=%u",
                         clientId );

        atReqCloseMqtt.pAtCmd = cmdBuf;
        atReqCloseMqtt.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqCloseMqtt.pAtRspPrefix = NULL;
        atReqCloseMqtt.respCallback = NULL;
        atReqCloseMqtt.pData = NULL;
        atReqCloseMqtt.dataLen = 0;

        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqCloseMqtt );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief MQTT Publish - Publish message to MQTT topic.
 */
CellularError_t Cellular_EG800Z_MqttPublish( CellularHandle_t cellularHandle,
                                             uint8_t clientId,
                                             const char * pTopic,
                                             CellularEG800Z_MqttQos_t qos,
                                             const uint8_t * pPayload,
                                             uint32_t payloadLength )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { 0 };
    CellularAtReq_t atReqPublishMqtt = { 0 };
    CellularAtDataReq_t atDataReq = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ( pTopic == NULL ) || ( pPayload == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( clientId >= CELLULAR_EG800Z_MAX_MQTT_CLIENTS )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Publish to MQTT topic - AT+QMTPUBEX */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                         "AT+QMTPUBEX=%u,0,%u,0,\"%s\",%u",
                         clientId,
                         qos,
                         pTopic,
                         payloadLength );

        atReqPublishMqtt.pAtCmd = cmdBuf;
        atReqPublishMqtt.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqPublishMqtt.pAtRspPrefix = NULL;
        atReqPublishMqtt.respCallback = NULL;
        atReqPublishMqtt.pData = NULL;
        atReqPublishMqtt.dataLen = 0;

        atDataReq.pData = pPayload;
        atDataReq.dataLen = payloadLength;
        atDataReq.pSentDataLength = NULL;
        atDataReq.pEndPattern = "\r\n";
        atDataReq.endPatternLen = 2;

        pktStatus = _Cellular_AtcmdDataSend( pContext, atReqPublishMqtt, atDataReq,
                                            NULL, NULL, 0U, 0, 0 );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief MQTT Subscribe - Subscribe to MQTT topic.
 */
CellularError_t Cellular_EG800Z_MqttSubscribe( CellularHandle_t cellularHandle,
                                               uint8_t clientId,
                                               const char * pTopic,
                                               CellularEG800Z_MqttQos_t qos )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { 0 };
    CellularAtReq_t atReqSubscribeMqtt = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( pTopic == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( clientId >= CELLULAR_EG800Z_MAX_MQTT_CLIENTS )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Subscribe to MQTT topic - AT+QMTSUB */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                         "AT+QMTSUB=%u,1,\"%s\",%u",
                         clientId,
                         pTopic,
                         qos );

        atReqSubscribeMqtt.pAtCmd = cmdBuf;
        atReqSubscribeMqtt.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqSubscribeMqtt.pAtRspPrefix = NULL;
        atReqSubscribeMqtt.respCallback = NULL;
        atReqSubscribeMqtt.pData = NULL;
        atReqSubscribeMqtt.dataLen = 0;

        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqSubscribeMqtt );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief MQTT Unsubscribe - Unsubscribe from MQTT topic.
 */
CellularError_t Cellular_EG800Z_MqttUnsubscribe( CellularHandle_t cellularHandle,
                                                 uint8_t clientId,
                                                 const char * pTopic )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { 0 };
    CellularAtReq_t atReqUnsubscribeMqtt = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( pTopic == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( clientId >= CELLULAR_EG800Z_MAX_MQTT_CLIENTS )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Unsubscribe from MQTT topic - AT+QMTUNS */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                         "AT+QMTUNS=%u,1,\"%s\"",
                         clientId,
                         pTopic );

        atReqUnsubscribeMqtt.pAtCmd = cmdBuf;
        atReqUnsubscribeMqtt.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqUnsubscribeMqtt.pAtRspPrefix = NULL;
        atReqUnsubscribeMqtt.respCallback = NULL;
        atReqUnsubscribeMqtt.pData = NULL;
        atReqUnsubscribeMqtt.dataLen = 0;

        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqUnsubscribeMqtt );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief HTTP Configure - Configure HTTP parameters.
 */
CellularError_t Cellular_EG800Z_HttpConfigure( CellularHandle_t cellularHandle,
                                               const CellularEG800Z_HttpConfig_t * pHttpConfig )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { 0 };
    CellularAtReq_t atReqConfigHttp = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( pHttpConfig == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Configure HTTP context - AT+QHTTPCFG="contextid" */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                         "AT+QHTTPCFG=\"contextid\",%u",
                         pHttpConfig->contextId );

        atReqConfigHttp.pAtCmd = cmdBuf;
        atReqConfigHttp.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqConfigHttp.pAtRspPrefix = NULL;
        atReqConfigHttp.respCallback = NULL;
        atReqConfigHttp.pData = NULL;
        atReqConfigHttp.dataLen = 0;

        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqConfigHttp );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );

        /* Configure SSL context if enabled */
        if( ( cellularStatus == CELLULAR_SUCCESS ) && ( pHttpConfig->sslContextId != 0xFF ) )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                             "AT+QHTTPCFG=\"sslctxid\",%u",
                             pHttpConfig->sslContextId );

            atReqConfigHttp.pAtCmd = cmdBuf;
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqConfigHttp );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }

        /* Configure request header if needed */
        if( cellularStatus == CELLULAR_SUCCESS )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                             "AT+QHTTPCFG=\"requestheader\",%u",
                             pHttpConfig->requestHeader );

            atReqConfigHttp.pAtCmd = cmdBuf;
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqConfigHttp );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }

        /* Configure response header if needed */
        if( cellularStatus == CELLULAR_SUCCESS )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                             "AT+QHTTPCFG=\"responseheader\",%u",
                             pHttpConfig->responseHeader );

            atReqConfigHttp.pAtCmd = cmdBuf;
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqConfigHttp );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief HTTP GET - Perform HTTP GET request.
 */
CellularError_t Cellular_EG800Z_HttpGet( CellularHandle_t cellularHandle,
                                         const char * pUrl,
                                         uint8_t * pResponseBuffer,
                                         uint32_t responseBufferSize,
                                         uint32_t * pResponseSize )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { 0 };
    CellularAtReq_t atReqHttpGet = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ( pUrl == NULL ) || ( pResponseBuffer == NULL ) || ( pResponseSize == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Set HTTP URL - AT+QHTTPURL */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                         "AT+QHTTPURL=%u,80",
                         ( uint32_t ) strlen( pUrl ) );

        atReqHttpGet.pAtCmd = cmdBuf;
        atReqHttpGet.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqHttpGet.pAtRspPrefix = NULL;
        atReqHttpGet.respCallback = NULL;
        atReqHttpGet.pData = NULL;
        atReqHttpGet.dataLen = 0;

        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqHttpGet );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );

        /* Send HTTP GET request - AT+QHTTPGET */
        if( cellularStatus == CELLULAR_SUCCESS )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                             "AT+QHTTPGET=80" );

            atReqHttpGet.pAtCmd = cmdBuf;
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqHttpGet );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }

        /* Read HTTP response - AT+QHTTPREAD */
        if( cellularStatus == CELLULAR_SUCCESS )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                             "AT+QHTTPREAD=80" );

            atReqHttpGet.pAtCmd = cmdBuf;
            atReqHttpGet.atCmdType = CELLULAR_AT_MULTI_DATA_WO_PREFIX;
            atReqHttpGet.pData = pResponseBuffer;
            atReqHttpGet.dataLen = ( uint16_t ) responseBufferSize;

            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqHttpGet );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );

            if( cellularStatus == CELLULAR_SUCCESS )
            {
                *pResponseSize = atReqHttpGet.dataLen;
            }
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief HTTP POST - Perform HTTP POST request.
 */
CellularError_t Cellular_EG800Z_HttpPost( CellularHandle_t cellularHandle,
                                          const char * pUrl,
                                          const uint8_t * pPostData,
                                          uint32_t postDataSize,
                                          uint8_t * pResponseBuffer,
                                          uint32_t responseBufferSize,
                                          uint32_t * pResponseSize )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { 0 };
    CellularAtReq_t atReqHttpPost = { 0 };
    CellularAtDataReq_t atDataReq = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ( pUrl == NULL ) || ( pPostData == NULL ) || ( pResponseBuffer == NULL ) || ( pResponseSize == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Set HTTP URL - AT+QHTTPURL */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                         "AT+QHTTPURL=%u,80",
                         ( uint32_t ) strlen( pUrl ) );

        atReqHttpPost.pAtCmd = cmdBuf;
        atReqHttpPost.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqHttpPost.pAtRspPrefix = NULL;
        atReqHttpPost.respCallback = NULL;
        atReqHttpPost.pData = NULL;
        atReqHttpPost.dataLen = 0;

        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqHttpPost );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );

        /* Send HTTP POST request - AT+QHTTPPOST */
        if( cellularStatus == CELLULAR_SUCCESS )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                             "AT+QHTTPPOST=%u,80,80",
                             postDataSize );

            atReqHttpPost.pAtCmd = cmdBuf;
            atReqHttpPost.atCmdType = CELLULAR_AT_NO_RESULT;

            atDataReq.pData = pPostData;
            atDataReq.dataLen = postDataSize;
            atDataReq.pSentDataLength = NULL;
            atDataReq.pEndPattern = "\r\n";
            atDataReq.endPatternLen = 2;

            pktStatus = _Cellular_AtcmdDataSend( pContext, atReqHttpPost, atDataReq,
                                                NULL, NULL, 0U, 0, 0 );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }

        /* Read HTTP response - AT+QHTTPREAD */
        if( cellularStatus == CELLULAR_SUCCESS )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                             "AT+QHTTPREAD=80" );

            atReqHttpPost.pAtCmd = cmdBuf;
            atReqHttpPost.atCmdType = CELLULAR_AT_MULTI_DATA_WO_PREFIX;
            atReqHttpPost.pData = pResponseBuffer;
            atReqHttpPost.dataLen = ( uint16_t ) responseBufferSize;

            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqHttpPost );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );

            if( cellularStatus == CELLULAR_SUCCESS )
            {
                *pResponseSize = atReqHttpPost.dataLen;
            }
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief SSL Configure - Configure SSL parameters.
 */
CellularError_t Cellular_EG800Z_SslConfigure( CellularHandle_t cellularHandle,
                                              const CellularEG800Z_SslConfig_t * pSslConfig )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { 0 };
    CellularAtReq_t atReqConfigSsl = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( pSslConfig == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( pSslConfig->sslContextId >= CELLULAR_EG800Z_MAX_SSL_CONTEXTS )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Configure SSL version - AT+QSSLCFG="sslversion" */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                         "AT+QSSLCFG=\"sslversion\",%u,%u",
                         pSslConfig->sslContextId,
                         pSslConfig->sslVersion );

        atReqConfigSsl.pAtCmd = cmdBuf;
        atReqConfigSsl.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqConfigSsl.pAtRspPrefix = NULL;
        atReqConfigSsl.respCallback = NULL;
        atReqConfigSsl.pData = NULL;
        atReqConfigSsl.dataLen = 0;

        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqConfigSsl );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );

        /* Configure CA certificate if provided */
        if( ( cellularStatus == CELLULAR_SUCCESS ) && ( pSslConfig->pCaCertPath != NULL ) )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                             "AT+QSSLCFG=\"cacert\",%u,\"%s\"",
                             pSslConfig->sslContextId,
                             pSslConfig->pCaCertPath );

            atReqConfigSsl.pAtCmd = cmdBuf;
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqConfigSsl );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }

        /* Configure client certificate if provided */
        if( ( cellularStatus == CELLULAR_SUCCESS ) && ( pSslConfig->pClientCertPath != NULL ) )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                             "AT+QSSLCFG=\"clientcert\",%u,\"%s\"",
                             pSslConfig->sslContextId,
                             pSslConfig->pClientCertPath );

            atReqConfigSsl.pAtCmd = cmdBuf;
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqConfigSsl );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }

        /* Configure client key if provided */
        if( ( cellularStatus == CELLULAR_SUCCESS ) && ( pSslConfig->pClientKeyPath != NULL ) )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                             "AT+QSSLCFG=\"clientkey\",%u,\"%s\"",
                             pSslConfig->sslContextId,
                             pSslConfig->pClientKeyPath );

            atReqConfigSsl.pAtCmd = cmdBuf;
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqConfigSsl );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }

        /* Configure security level */
        if( cellularStatus == CELLULAR_SUCCESS )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                             "AT+QSSLCFG=\"seclevel\",%u,%u",
                             pSslConfig->sslContextId,
                             pSslConfig->secLevel );

            atReqConfigSsl.pAtCmd = cmdBuf;
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqConfigSsl );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief File Upload - Upload file to module filesystem.
 */
CellularError_t Cellular_EG800Z_FileUpload( CellularHandle_t cellularHandle,
                                            const char * pFileName,
                                            const uint8_t * pFileData,
                                            uint32_t fileSize )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { 0 };
    CellularAtReq_t atReqFileUpload = { 0 };
    CellularAtDataReq_t atDataReq = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ( pFileName == NULL ) || ( pFileData == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Upload file - AT+QFUPL */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                         "AT+QFUPL=\"%s\",%u,100",
                         pFileName,
                         fileSize );

        atReqFileUpload.pAtCmd = cmdBuf;
        atReqFileUpload.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqFileUpload.pAtRspPrefix = NULL;
        atReqFileUpload.respCallback = NULL;
        atReqFileUpload.pData = NULL;
        atReqFileUpload.dataLen = 0;

        atDataReq.pData = pFileData;
        atDataReq.dataLen = fileSize;
        atDataReq.pSentDataLength = NULL;
        atDataReq.pEndPattern = "\r\n";
        atDataReq.endPatternLen = 2;

        pktStatus = _Cellular_AtcmdDataSend( pContext, atReqFileUpload, atDataReq,
                                            NULL, NULL, 0U, 0, 0 );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief File Download - Download file from module filesystem.
 */
CellularError_t Cellular_EG800Z_FileDownload( CellularHandle_t cellularHandle,
                                              const char * pFileName,
                                              uint8_t * pFileBuffer,
                                              uint32_t fileBufferSize,
                                              uint32_t * pFileSize )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { 0 };
    CellularAtReq_t atReqFileDownload = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ( pFileName == NULL ) || ( pFileBuffer == NULL ) || ( pFileSize == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Download file - AT+QFDWL */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                         "AT+QFDWL=\"%s\"",
                         pFileName );

        atReqFileDownload.pAtCmd = cmdBuf;
        atReqFileDownload.atCmdType = CELLULAR_AT_MULTI_DATA_WO_PREFIX;
        atReqFileDownload.pAtRspPrefix = NULL;
        atReqFileDownload.respCallback = NULL;
        atReqFileDownload.pData = pFileBuffer;
        atReqFileDownload.dataLen = ( uint16_t ) fileBufferSize;

        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqFileDownload );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );

        if( cellularStatus == CELLULAR_SUCCESS )
        {
            *pFileSize = atReqFileDownload.dataLen;
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief File Delete - Delete file from module filesystem.
 */
CellularError_t Cellular_EG800Z_FileDelete( CellularHandle_t cellularHandle,
                                            const char * pFileName )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { 0 };
    CellularAtReq_t atReqFileDelete = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( pFileName == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Delete file - AT+QFDEL */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                         "AT+QFDEL=\"%s\"",
                         pFileName );

        atReqFileDelete.pAtCmd = cmdBuf;
        atReqFileDelete.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqFileDelete.pAtRspPrefix = NULL;
        atReqFileDelete.respCallback = NULL;
        atReqFileDelete.pData = NULL;
        atReqFileDelete.dataLen = 0;

        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqFileDelete );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief File List - List files in module filesystem.
 */
CellularError_t Cellular_EG800Z_FileList( CellularHandle_t cellularHandle,
                                          const char * pPattern,
                                          char * pFileList,
                                          uint32_t fileListSize )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { 0 };
    CellularAtReq_t atReqFileList = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ( pPattern == NULL ) || ( pFileList == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* List files - AT+QFLST */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                         "AT+QFLST=\"%s\"",
                         pPattern );

        atReqFileList.pAtCmd = cmdBuf;
        atReqFileList.atCmdType = CELLULAR_AT_WITH_PREFIX;
        atReqFileList.pAtRspPrefix = "+QFLST";
        atReqFileList.respCallback = NULL;
        atReqFileList.pData = pFileList;
        atReqFileList.dataLen = ( uint16_t ) fileListSize;

        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqFileList );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/
