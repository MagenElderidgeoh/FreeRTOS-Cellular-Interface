/*
 * FreeRTOS-Cellular-Interface
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 * @file cellular_mqtt.c
 * @brief MQTT implementation for Quectel EC600Z/EC800Z/EG800Z series modules
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "cellular_config.h"
#include "cellular_config_defaults.h"
#include "cellular_types.h"
#include "cellular_mqtt.h"
#include "cellular_api.h"
#include "cellular_common_internal.h"

/*-----------------------------------------------------------*/

/**
 * @brief MQTT AT command timeout in milliseconds
 */
#define MQTT_AT_CMD_TIMEOUT_MS           ( 120000UL )

/**
 * @brief MQTT connect timeout in milliseconds
 */
#define MQTT_CONNECT_TIMEOUT_MS          ( 120000UL )

/**
 * @brief MQTT publish timeout in milliseconds
 */
#define MQTT_PUBLISH_TIMEOUT_MS          ( 60000UL )

/**
 * @brief Maximum AT command buffer size
 */
#define MQTT_AT_CMD_MAX_SIZE             ( 512U )

/**
 * @brief MQTT callback context
 */
typedef struct CellularMqttCallbackContext
{
    CellularMqttConnectionCallback_t connectionCallback;
    void * pConnectionCallbackContext;
    CellularMqttMessageReceivedCallback_t messageCallback;
    void * pMessageCallbackContext;
} CellularMqttCallbackContext_t;

static CellularMqttCallbackContext_t mqttCallbackContext = { 0 };

/*-----------------------------------------------------------*/

static CellularPktStatus_t _Cellular_RecvFuncGetMqttResult( CellularContext_t * pContext,
                                                            const CellularATCommandResponse_t * pAtResp,
                                                            void * pData,
                                                            uint16_t dataLen );

static CellularATCommandType_t _getMqttCmdType( const char * pCmd );

/*-----------------------------------------------------------*/

static CellularATCommandType_t _getMqttCmdType( const char * pCmd )
{
    CellularATCommandType_t cmdType = CELLULAR_AT_NO_RESULT;

    if( strstr( pCmd, "AT+QMTCFG" ) != NULL )
    {
        cmdType = CELLULAR_AT_WITH_PREFIX;
    }
    else if( strstr( pCmd, "AT+QMTOPEN" ) != NULL )
    {
        cmdType = CELLULAR_AT_NO_RESULT;
    }
    else if( strstr( pCmd, "AT+QMTCLOSE" ) != NULL )
    {
        cmdType = CELLULAR_AT_NO_RESULT;
    }
    else if( strstr( pCmd, "AT+QMTCONN" ) != NULL )
    {
        cmdType = CELLULAR_AT_NO_RESULT;
    }
    else if( strstr( pCmd, "AT+QMTDISC" ) != NULL )
    {
        cmdType = CELLULAR_AT_NO_RESULT;
    }
    else if( strstr( pCmd, "AT+QMTSUB" ) != NULL )
    {
        cmdType = CELLULAR_AT_NO_RESULT;
    }
    else if( strstr( pCmd, "AT+QMTUNS" ) != NULL )
    {
        cmdType = CELLULAR_AT_NO_RESULT;
    }
    else if( strstr( pCmd, "AT+QMTPUBEX" ) != NULL )
    {
        cmdType = CELLULAR_AT_NO_RESULT;
    }

    return cmdType;
}

/*-----------------------------------------------------------*/

static CellularPktStatus_t _Cellular_RecvFuncGetMqttResult( CellularContext_t * pContext,
                                                            const CellularATCommandResponse_t * pAtResp,
                                                            void * pData,
                                                            uint16_t dataLen )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;

    if( pContext == NULL )
    {
        pktStatus = CELLULAR_PKT_STATUS_INVALID_HANDLE;
    }
    else if( ( pAtResp == NULL ) || ( pAtResp->pItm == NULL ) ||
             ( pAtResp->pItm->pLine == NULL ) )
    {
        LogError( ( "GetMqttResult: Input Line passed is NULL" ) );
        pktStatus = CELLULAR_PKT_STATUS_FAILURE;
    }
    else if( ( pData == NULL ) || ( dataLen == 0U ) )
    {
        LogError( ( "GetMqttResult: Invalid param" ) );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        /* Parse the response to get result code */
        /* Response format varies based on command */
        const char * pInputLine = pAtResp->pItm->pLine;
        
        /* For most MQTT commands, just check if OK was received */
        if( pAtResp->status == true )
        {
            *( ( bool * ) pData ) = true;
        }
        else
        {
            *( ( bool * ) pData ) = false;
        }
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_MqttConfigure( CellularHandle_t cellularHandle,
                                        uint8_t clientIdx,
                                        const CellularMqttConnectInfo_t * pConnectInfo,
                                        const CellularMqttWillInfo_t * pWillInfo,
                                        const CellularMqttSslConfig_t * pSslConfig )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularAtReq_t atReqCfg = { 0 };
    char cmdBuf[ MQTT_AT_CMD_MAX_SIZE ] = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ( clientIdx > CELLULAR_MQTT_CLIENT_ID_MAX ) || ( pConnectInfo == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Configure MQTT version */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                          "AT+QMTCFG=\"version\",%u,%u",
                          clientIdx,
                          ( uint32_t ) pConnectInfo->version );
        atReqCfg.pAtCmd = cmdBuf;
        atReqCfg.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqCfg.pAtRspPrefix = NULL;
        atReqCfg.respCallback = NULL;
        atReqCfg.pData = NULL;
        atReqCfg.dataLen = 0;
        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqCfg );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );

        /* Configure PDN context ID */
        if( cellularStatus == CELLULAR_SUCCESS )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                              "AT+QMTCFG=\"pdpcid\",%u,%u",
                              clientIdx,
                              pConnectInfo->pdnContextId );
            atReqCfg.pAtCmd = cmdBuf;
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqCfg );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }

        /* Configure keep alive time */
        if( cellularStatus == CELLULAR_SUCCESS )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                              "AT+QMTCFG=\"keepalive\",%u,%u",
                              clientIdx,
                              pConnectInfo->keepAliveSeconds );
            atReqCfg.pAtCmd = cmdBuf;
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqCfg );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }

        /* Configure clean session */
        if( cellularStatus == CELLULAR_SUCCESS )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                              "AT+QMTCFG=\"session\",%u,%u",
                              clientIdx,
                              pConnectInfo->cleanSession ? 1 : 0 );
            atReqCfg.pAtCmd = cmdBuf;
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqCfg );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }

        /* Configure SSL if provided */
        if( ( cellularStatus == CELLULAR_SUCCESS ) && ( pSslConfig != NULL ) )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                              "AT+QMTCFG=\"ssl\",%u,%u,%u",
                              clientIdx,
                              pSslConfig->sslEnable ? 1 : 0,
                              pSslConfig->sslContextIdx );
            atReqCfg.pAtCmd = cmdBuf;
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqCfg );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }

        /* Configure Will message if provided */
        if( ( cellularStatus == CELLULAR_SUCCESS ) && ( pWillInfo != NULL ) && ( pWillInfo->willFlag ) )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                              "AT+QMTCFG=\"will\",%u,%u,%u,%u,\"%s\",\"%s\"",
                              clientIdx,
                              pWillInfo->willFlag ? 1 : 0,
                              ( uint32_t ) pWillInfo->willQos,
                              pWillInfo->willRetain ? 1 : 0,
                              pWillInfo->pWillTopic,
                              pWillInfo->pWillMessage );
            atReqCfg.pAtCmd = cmdBuf;
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqCfg );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_MqttOpen( CellularHandle_t cellularHandle,
                                   uint8_t clientIdx,
                                   const char * pHostName,
                                   uint16_t port )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularAtReq_t atReqOpen = { 0 };
    char cmdBuf[ MQTT_AT_CMD_MAX_SIZE ] = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ( clientIdx > CELLULAR_MQTT_CLIENT_ID_MAX ) || ( pHostName == NULL ) || ( port == 0 ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* AT+QMTOPEN=<client_idx>,"<host_name>",<port> */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                          "AT+QMTOPEN=%u,\"%s\",%u",
                          clientIdx,
                          pHostName,
                          port );

        atReqOpen.pAtCmd = cmdBuf;
        atReqOpen.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqOpen.pAtRspPrefix = NULL;
        atReqOpen.respCallback = NULL;
        atReqOpen.pData = NULL;
        atReqOpen.dataLen = 0;

        pktStatus = _Cellular_TimeoutAtcmdRequestWithCallback( pContext, atReqOpen,
                                                               MQTT_AT_CMD_TIMEOUT_MS );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_MqttClose( CellularHandle_t cellularHandle,
                                    uint8_t clientIdx )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularAtReq_t atReqClose = { 0 };
    char cmdBuf[ MQTT_AT_CMD_MAX_SIZE ] = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( clientIdx > CELLULAR_MQTT_CLIENT_ID_MAX )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* AT+QMTCLOSE=<client_idx> */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                          "AT+QMTCLOSE=%u",
                          clientIdx );

        atReqClose.pAtCmd = cmdBuf;
        atReqClose.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqClose.pAtRspPrefix = NULL;
        atReqClose.respCallback = NULL;
        atReqClose.pData = NULL;
        atReqClose.dataLen = 0;

        pktStatus = _Cellular_TimeoutAtcmdRequestWithCallback( pContext, atReqClose,
                                                               MQTT_AT_CMD_TIMEOUT_MS );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_MqttConnect( CellularHandle_t cellularHandle,
                                      const CellularMqttConnectInfo_t * pConnectInfo )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularAtReq_t atReqConn = { 0 };
    char cmdBuf[ MQTT_AT_CMD_MAX_SIZE ] = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ( pConnectInfo == NULL ) || ( pConnectInfo->pClientId == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( pConnectInfo->clientIdx > CELLULAR_MQTT_CLIENT_ID_MAX )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* AT+QMTCONN=<client_idx>,"<clientID>"[,"<username>","<password>"] */
        if( ( pConnectInfo->pUserName != NULL ) && ( pConnectInfo->pPassword != NULL ) )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                              "AT+QMTCONN=%u,\"%s\",\"%s\",\"%s\"",
                              pConnectInfo->clientIdx,
                              pConnectInfo->pClientId,
                              pConnectInfo->pUserName,
                              pConnectInfo->pPassword );
        }
        else if( pConnectInfo->pUserName != NULL )
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                              "AT+QMTCONN=%u,\"%s\",\"%s\"",
                              pConnectInfo->clientIdx,
                              pConnectInfo->pClientId,
                              pConnectInfo->pUserName );
        }
        else
        {
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                              "AT+QMTCONN=%u,\"%s\"",
                              pConnectInfo->clientIdx,
                              pConnectInfo->pClientId );
        }

        atReqConn.pAtCmd = cmdBuf;
        atReqConn.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqConn.pAtRspPrefix = NULL;
        atReqConn.respCallback = NULL;
        atReqConn.pData = NULL;
        atReqConn.dataLen = 0;

        pktStatus = _Cellular_TimeoutAtcmdRequestWithCallback( pContext, atReqConn,
                                                               MQTT_CONNECT_TIMEOUT_MS );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_MqttDisconnect( CellularHandle_t cellularHandle,
                                         uint8_t clientIdx )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularAtReq_t atReqDisc = { 0 };
    char cmdBuf[ MQTT_AT_CMD_MAX_SIZE ] = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( clientIdx > CELLULAR_MQTT_CLIENT_ID_MAX )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* AT+QMTDISC=<client_idx> */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                          "AT+QMTDISC=%u",
                          clientIdx );

        atReqDisc.pAtCmd = cmdBuf;
        atReqDisc.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqDisc.pAtRspPrefix = NULL;
        atReqDisc.respCallback = NULL;
        atReqDisc.pData = NULL;
        atReqDisc.dataLen = 0;

        pktStatus = _Cellular_TimeoutAtcmdRequestWithCallback( pContext, atReqDisc,
                                                               MQTT_AT_CMD_TIMEOUT_MS );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_MqttSubscribe( CellularHandle_t cellularHandle,
                                        uint8_t clientIdx,
                                        const CellularMqttSubscribeInfo_t * pSubscribeInfo )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularAtReq_t atReqSub = { 0 };
    char cmdBuf[ MQTT_AT_CMD_MAX_SIZE ] = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ( pSubscribeInfo == NULL ) || ( pSubscribeInfo->pTopic == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( clientIdx > CELLULAR_MQTT_CLIENT_ID_MAX )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* AT+QMTSUB=<client_idx>,<msgID>,"<topic>",<QoS> */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                          "AT+QMTSUB=%u,%u,\"%s\",%u",
                          clientIdx,
                          pSubscribeInfo->msgId,
                          pSubscribeInfo->pTopic,
                          ( uint32_t ) pSubscribeInfo->qos );

        atReqSub.pAtCmd = cmdBuf;
        atReqSub.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqSub.pAtRspPrefix = NULL;
        atReqSub.respCallback = NULL;
        atReqSub.pData = NULL;
        atReqSub.dataLen = 0;

        pktStatus = _Cellular_TimeoutAtcmdRequestWithCallback( pContext, atReqSub,
                                                               MQTT_AT_CMD_TIMEOUT_MS );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_MqttUnsubscribe( CellularHandle_t cellularHandle,
                                          uint8_t clientIdx,
                                          uint16_t msgId,
                                          const char * pTopic )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularAtReq_t atReqUns = { 0 };
    char cmdBuf[ MQTT_AT_CMD_MAX_SIZE ] = { 0 };

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( pTopic == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( clientIdx > CELLULAR_MQTT_CLIENT_ID_MAX )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* AT+QMTUNS=<client_idx>,<msgID>,"<topic>" */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                          "AT+QMTUNS=%u,%u,\"%s\"",
                          clientIdx,
                          msgId,
                          pTopic );

        atReqUns.pAtCmd = cmdBuf;
        atReqUns.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqUns.pAtRspPrefix = NULL;
        atReqUns.respCallback = NULL;
        atReqUns.pData = NULL;
        atReqUns.dataLen = 0;

        pktStatus = _Cellular_TimeoutAtcmdRequestWithCallback( pContext, atReqUns,
                                                               MQTT_AT_CMD_TIMEOUT_MS );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_MqttPublish( CellularHandle_t cellularHandle,
                                      uint8_t clientIdx,
                                      const CellularMqttPublishInfo_t * pPublishInfo )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularAtReq_t atReqPub = { 0 };
    CellularAtDataReq_t atDataReq = { 0 };
    char cmdBuf[ MQTT_AT_CMD_MAX_SIZE ] = { 0 };
    char * pDataBuf = NULL;

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ( pPublishInfo == NULL ) || ( pPublishInfo->pTopic == NULL ) ||
             ( pPublishInfo->pPayload == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( clientIdx > CELLULAR_MQTT_CLIENT_ID_MAX )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* AT+QMTPUBEX=<client_idx>,<msgID>,<QoS>,<retain>,"<topic>",<length> */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                          "AT+QMTPUBEX=%u,%u,%u,%u,\"%s\",%lu",
                          clientIdx,
                          pPublishInfo->msgId,
                          ( uint32_t ) pPublishInfo->qos,
                          pPublishInfo->retain ? 1 : 0,
                          pPublishInfo->pTopic,
                          ( unsigned long ) pPublishInfo->payloadLength );

        atReqPub.pAtCmd = cmdBuf;
        atReqPub.atCmdType = CELLULAR_AT_NO_RESULT;
        atReqPub.pAtRspPrefix = ">";
        atReqPub.respCallback = NULL;
        atReqPub.pData = NULL;
        atReqPub.dataLen = 0;

        /* Send the command and wait for prompt */
        pktStatus = _Cellular_TimeoutAtcmdRequestWithCallback( pContext, atReqPub,
                                                               MQTT_PUBLISH_TIMEOUT_MS );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );

        /* Send the payload data */
        if( cellularStatus == CELLULAR_SUCCESS )
        {
            atDataReq.pData = pPublishInfo->pPayload;
            atDataReq.dataLen = pPublishInfo->payloadLength;
            atDataReq.pSentDataLength = NULL;
            atDataReq.pEndPattern = NULL;
            atDataReq.endPatternLen = 0;

            cellularStatus = _Cellular_AtcmdDataSend( pContext, atDataReq,
                                                     NULL, NULL,
                                                     0, 0, 0 );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_MqttRegisterConnectionCallback( CellularHandle_t cellularHandle,
                                                         CellularMqttConnectionCallback_t connectionCallback,
                                                         void * pCallbackContext )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    if( cellularHandle == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else
    {
        mqttCallbackContext.connectionCallback = connectionCallback;
        mqttCallbackContext.pConnectionCallbackContext = pCallbackContext;
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_MqttRegisterMessageCallback( CellularHandle_t cellularHandle,
                                                      CellularMqttMessageReceivedCallback_t messageCallback,
                                                      void * pCallbackContext )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    if( cellularHandle == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else
    {
        mqttCallbackContext.messageCallback = messageCallback;
        mqttCallbackContext.pMessageCallbackContext = pCallbackContext;
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/
