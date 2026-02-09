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
 * @file cellular_http_api.c
 * @brief Implementation of common HTTP APIs for Quectel modules.
 *
 * This implementation follows the Quectel EC600Z/EC800Z/EG800Z HTTP(S)
 * Application Guide V1.4, with special attention to handling large files
 * (MB-level) in HTTP GET operations through chunked reading.
 */

/* The config header is always included first. */

#ifndef CELLULAR_DO_NOT_USE_CUSTOM_CONFIG
    /* Include custom config file before other headers. */
    #include "cellular_config.h"
#endif
#include "cellular_config_defaults.h"

/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cellular_platform.h"
#include "cellular_types.h"
#include "cellular_api.h"
#include "cellular_common.h"
#include "cellular_common_api.h"
#include "cellular_common_portable.h"
#include "cellular_common_internal.h"
#include "cellular_http_api.h"

/*-----------------------------------------------------------*/

/**
 * @brief Maximum URL length supported.
 */
#define HTTP_MAX_URL_LENGTH           ( 1024U )

/**
 * @brief Maximum header length.
 */
#define HTTP_MAX_HEADER_LENGTH        ( 512U )

/**
 * @brief Maximum number of custom headers.
 */
#define HTTP_MAX_CUSTOM_HEADERS       ( 10U )

/**
 * @brief Default timeout for HTTP operations in milliseconds.
 */
#define HTTP_DEFAULT_TIMEOUT_MS       ( 60000U )

/**
 * @brief Chunk size for reading large files (64KB).
 */
#define HTTP_READ_CHUNK_SIZE          ( 65536U )

/**
 * @brief Maximum retry count for reading chunks.
 */
#define HTTP_MAX_READ_RETRIES         ( 3U )

/*-----------------------------------------------------------*/

/**
 * @brief HTTP context structure.
 */
struct CellularHttpContext
{
    CellularHandle_t cellularHandle;                    /**< Cellular handle. */
    CellularHttpConfig_t config;                        /**< HTTP configuration. */
    CellularHttpResponse_t response;                    /**< HTTP response info. */
    CellularHttpDataReadyCallback_t dataReadyCallback;  /**< Data ready callback. */
    void * pDataReadyCallbackContext;                   /**< Data ready callback context. */
    CellularHttpRequestCompleteCallback_t completeCallback; /**< Request complete callback. */
    void * pCompleteCallbackContext;                    /**< Complete callback context. */
    uint32_t remainingDataLength;                       /**< Remaining data to read. */
    bool isRequestActive;                               /**< Is HTTP request active. */
    char customHeaders[ HTTP_MAX_CUSTOM_HEADERS ][ HTTP_MAX_HEADER_LENGTH ]; /**< Custom headers. */
    uint8_t customHeaderCount;                          /**< Number of custom headers. */
};

/*-----------------------------------------------------------*/

/**
 * @brief Convert HTTP method enum to string.
 */
static const char * _getHttpMethodString( CellularHttpMethod_t method )
{
    const char * pMethodStr = NULL;

    switch( method )
    {
        case CELLULAR_HTTP_METHOD_GET:
            pMethodStr = "GET";
            break;

        case CELLULAR_HTTP_METHOD_POST:
            pMethodStr = "POST";
            break;

        case CELLULAR_HTTP_METHOD_HEAD:
            pMethodStr = "HEAD";
            break;

        case CELLULAR_HTTP_METHOD_PUT:
            pMethodStr = "PUT";
            break;

        case CELLULAR_HTTP_METHOD_DELETE:
            pMethodStr = "DELETE";
            break;

        default:
            pMethodStr = "GET";
            break;
    }

    return pMethodStr;
}

/*-----------------------------------------------------------*/

/**
 * @brief Convert content type enum to string.
 */
static const char * _getContentTypeString( CellularHttpContentType_t contentType )
{
    const char * pTypeStr = NULL;

    switch( contentType )
    {
        case CELLULAR_HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED:
            pTypeStr = "application/x-www-form-urlencoded";
            break;

        case CELLULAR_HTTP_CONTENT_TYPE_TEXT_PLAIN:
            pTypeStr = "text/plain";
            break;

        case CELLULAR_HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM:
            pTypeStr = "application/octet-stream";
            break;

        case CELLULAR_HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA:
            pTypeStr = "multipart/form-data";
            break;

        case CELLULAR_HTTP_CONTENT_TYPE_APPLICATION_JSON:
            pTypeStr = "application/json";
            break;

        case CELLULAR_HTTP_CONTENT_TYPE_APPLICATION_XML:
            pTypeStr = "application/xml";
            break;

        case CELLULAR_HTTP_CONTENT_TYPE_TEXT_HTML:
            pTypeStr = "text/html";
            break;

        default:
            pTypeStr = "application/octet-stream";
            break;
    }

    return pTypeStr;
}

/*-----------------------------------------------------------*/

/**
 * @brief Callback for AT+QHTTPCFG response.
 */
static CellularPktStatus_t _httpConfigCallback( CellularContext_t * pContext,
                                                const CellularATCommandResponse_t * pAtResp,
                                                void * pData,
                                                uint16_t dataLen )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;

    ( void ) pContext;
    ( void ) pData;
    ( void ) dataLen;

    if( pAtResp == NULL )
    {
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        atCoreStatus = pAtResp->status;

        if( atCoreStatus != CELLULAR_AT_SUCCESS )
        {
            pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
        }
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief Callback for AT+QHTTPURL response.
 */
static CellularPktStatus_t _httpUrlCallback( CellularContext_t * pContext,
                                             const CellularATCommandResponse_t * pAtResp,
                                             void * pData,
                                             uint16_t dataLen )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;

    ( void ) pContext;
    ( void ) pData;
    ( void ) dataLen;

    if( pAtResp == NULL )
    {
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        atCoreStatus = pAtResp->status;

        if( atCoreStatus != CELLULAR_AT_SUCCESS )
        {
            pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
        }
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief Callback for AT+QHTTPGET/POST/HEAD response.
 */
static CellularPktStatus_t _httpRequestCallback( CellularContext_t * pContext,
                                                 const CellularATCommandResponse_t * pAtResp,
                                                 void * pData,
                                                 uint16_t dataLen )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;
    char * pInputLine = NULL;
    CellularHttpResponse_t * pResponse = ( CellularHttpResponse_t * ) pData;

    ( void ) pContext;
    ( void ) dataLen;

    if( ( pAtResp == NULL ) || ( pResponse == NULL ) )
    {
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        atCoreStatus = pAtResp->status;

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            pInputLine = pAtResp->pItm->pLine;

            /* Parse response: +QHTTPGET: <err>,<http_code>,<content_length> */
            if( pInputLine != NULL )
            {
                int32_t err = 0;
                int32_t httpCode = 0;
                int32_t contentLen = 0;

                if( sscanf( pInputLine, "+QHTTPGET: %d,%d,%d", &err, &httpCode, &contentLen ) == 3 )
                {
                    if( err == 0 )
                    {
                        pResponse->httpStatusCode = ( uint16_t ) httpCode;
                        pResponse->contentLength = ( uint32_t ) contentLen;
                        pResponse->receivedLength = 0;
                    }
                    else
                    {
                        pktStatus = CELLULAR_PKT_STATUS_FAILURE;
                    }
                }
                else
                {
                    pktStatus = CELLULAR_PKT_STATUS_FAILURE;
                }
            }
        }
        else
        {
            pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
        }
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief Callback for AT+QHTTPREAD response for reading data chunks.
 */
static CellularPktStatus_t _httpReadDataCallback( CellularContext_t * pContext,
                                                  const CellularATCommandResponse_t * pAtResp,
                                                  void * pData,
                                                  uint16_t dataLen )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;

    ( void ) pContext;
    ( void ) pData;
    ( void ) dataLen;

    if( pAtResp == NULL )
    {
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        atCoreStatus = pAtResp->status;

        if( atCoreStatus != CELLULAR_AT_SUCCESS )
        {
            pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
        }
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_CommonHttpInit( CellularHandle_t cellularHandle,
                                         const CellularHttpConfig_t * pHttpConfig,
                                         CellularHttpHandle_t * pHttpHandle )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularHttpContext_t * pHttpContext = NULL;
    char cmdBuf[ 128 ];
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularAtReq_t atReqHttpCfg = { 0 };

    /* Validate parameters. */
    if( ( pContext == NULL ) || ( pHttpConfig == NULL ) || ( pHttpHandle == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( ( pHttpConfig->contextId < 1U ) || ( pHttpConfig->contextId > 16U ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Allocate HTTP context. */
        pHttpContext = ( CellularHttpContext_t * ) Platform_Malloc( sizeof( CellularHttpContext_t ) );

        if( pHttpContext == NULL )
        {
            cellularStatus = CELLULAR_NO_MEMORY;
        }
        else
        {
            ( void ) memset( pHttpContext, 0, sizeof( CellularHttpContext_t ) );
            pHttpContext->cellularHandle = cellularHandle;
            pHttpContext->config = *pHttpConfig;
            pHttpContext->isRequestActive = false;
            pHttpContext->remainingDataLength = 0;
            pHttpContext->customHeaderCount = 0;

            /* Configure HTTP context ID: AT+QHTTPCFG="contextid",<context_id> */
            ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                              "AT+QHTTPCFG=\"contextid\",%u", pHttpConfig->contextId );

            atReqHttpCfg.pAtCmd = cmdBuf;
            atReqHttpCfg.atCmdType = CELLULAR_AT_NO_RESULT;
            atReqHttpCfg.pAtRspPrefix = NULL;
            atReqHttpCfg.respCallback = _httpConfigCallback;
            atReqHttpCfg.pData = NULL;
            atReqHttpCfg.dataLen = 0;

            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqHttpCfg );

            if( pktStatus != CELLULAR_PKT_STATUS_OK )
            {
                Platform_Free( pHttpContext );
                cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
            }
            else
            {
                /* Configure SSL context if SSL is enabled. */
                if( pHttpConfig->sslCtxId > 0U )
                {
                    ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                                      "AT+QHTTPCFG=\"sslctxid\",%u", pHttpConfig->sslCtxId );

                    atReqHttpCfg.pAtCmd = cmdBuf;
                    pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqHttpCfg );

                    if( pktStatus != CELLULAR_PKT_STATUS_OK )
                    {
                        Platform_Free( pHttpContext );
                        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
                    }
                }

                if( cellularStatus == CELLULAR_SUCCESS )
                {
                    /* Configure request timeout: AT+QHTTPCFG="requestheader",<0|1> */
                    ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                                      "AT+QHTTPCFG=\"requestheader\",%u",
                                      pHttpConfig->enableCustomHeader ? 1U : 0U );

                    atReqHttpCfg.pAtCmd = cmdBuf;
                    pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqHttpCfg );

                    if( pktStatus != CELLULAR_PKT_STATUS_OK )
                    {
                        Platform_Free( pHttpContext );
                        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
                    }
                    else
                    {
                        *pHttpHandle = pHttpContext;
                    }
                }
            }
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_CommonHttpCleanup( CellularHandle_t cellularHandle,
                                            CellularHttpHandle_t httpHandle )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularHttpContext_t * pHttpContext = ( CellularHttpContext_t * ) httpHandle;

    /* Validate parameters. */
    if( ( pContext == NULL ) || ( pHttpContext == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Free the HTTP context. */
        Platform_Free( pHttpContext );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_CommonHttpSetHeader( CellularHandle_t cellularHandle,
                                               CellularHttpHandle_t httpHandle,
                                               const char * pHeaderName,
                                               const char * pHeaderValue )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularHttpContext_t * pHttpContext = ( CellularHttpContext_t * ) httpHandle;

    /* Validate parameters. */
    if( ( pContext == NULL ) || ( pHttpContext == NULL ) ||
        ( pHeaderName == NULL ) || ( pHeaderValue == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( pHttpContext->customHeaderCount >= HTTP_MAX_CUSTOM_HEADERS )
    {
        cellularStatus = CELLULAR_NO_MEMORY;
    }
    else
    {
        /* Store the custom header. */
        ( void ) snprintf( pHttpContext->customHeaders[ pHttpContext->customHeaderCount ],
                          HTTP_MAX_HEADER_LENGTH,
                          "%s: %s\r\n",
                          pHeaderName,
                          pHeaderValue );

        pHttpContext->customHeaderCount++;
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_CommonHttpRequest( CellularHandle_t cellularHandle,
                                            CellularHttpHandle_t httpHandle,
                                            const CellularHttpRequest_t * pRequest,
                                            CellularHttpResponse_t * pResponse )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularHttpContext_t * pHttpContext = ( CellularHttpContext_t * ) httpHandle;
    char cmdBuf[ HTTP_MAX_URL_LENGTH + 64 ];
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularAtReq_t atReqHttpUrl = { 0 };
    CellularAtReq_t atReqHttpReq = { 0 };
    const char * pMethodStr = NULL;

    /* Validate parameters. */
    if( ( pContext == NULL ) || ( pHttpContext == NULL ) ||
        ( pRequest == NULL ) || ( pResponse == NULL ) ||
        ( pRequest->pUrl == NULL ) || ( pRequest->urlLen == 0U ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Step 1: Set URL using AT+QHTTPURL */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                          "AT+QHTTPURL=%u,%u",
                          pRequest->urlLen,
                          pHttpContext->config.requestTimeout );

        atReqHttpUrl.pAtCmd = cmdBuf;
        atReqHttpUrl.atCmdType = CELLULAR_AT_WITH_PREFIX;
        atReqHttpUrl.pAtRspPrefix = "CONNECT";
        atReqHttpUrl.respCallback = _httpUrlCallback;
        atReqHttpUrl.pData = NULL;
        atReqHttpUrl.dataLen = 0;

        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqHttpUrl );

        if( pktStatus == CELLULAR_PKT_STATUS_OK )
        {
            /* Send URL data. */
            pktStatus = _Cellular_AtcmdDataSend( pContext,
                                                atReqHttpUrl,
                                                ( const uint8_t * ) pRequest->pUrl,
                                                pRequest->urlLen,
                                                NULL,
                                                0 );
        }

        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
        else
        {
            /* Step 2: Send custom headers if any. */
            if( pHttpContext->customHeaderCount > 0U )
            {
                uint8_t i;

                for( i = 0; i < pHttpContext->customHeaderCount; i++ )
                {
                    ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                                      "AT+QHTTPPOST=%u,%u,%u",
                                      ( uint32_t ) strlen( pHttpContext->customHeaders[ i ] ),
                                      pHttpContext->config.requestTimeout,
                                      pHttpContext->config.responseTimeout );

                    /* This is simplified - actual implementation would need proper header handling. */
                }
            }

            /* Step 3: Perform HTTP request based on method. */
            pMethodStr = _getHttpMethodString( pRequest->method );

            if( pRequest->method == CELLULAR_HTTP_METHOD_GET )
            {
                /* AT+QHTTPGET=<timeout> */
                ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                                  "AT+QHTTPGET=%u", pHttpContext->config.responseTimeout );

                atReqHttpReq.pAtCmd = cmdBuf;
                atReqHttpReq.atCmdType = CELLULAR_AT_WITH_PREFIX;
                atReqHttpReq.pAtRspPrefix = "+QHTTPGET";
                atReqHttpReq.respCallback = _httpRequestCallback;
                atReqHttpReq.pData = pResponse;
                atReqHttpReq.dataLen = sizeof( CellularHttpResponse_t );

                pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqHttpReq );

                if( pktStatus == CELLULAR_PKT_STATUS_OK )
                {
                    /* Store response info in HTTP context for chunked reading. */
                    pHttpContext->response = *pResponse;
                    pHttpContext->remainingDataLength = pResponse->contentLength;
                    pHttpContext->isRequestActive = true;
                }
                else
                {
                    cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
                }
            }
            else if( pRequest->method == CELLULAR_HTTP_METHOD_POST )
            {
                /* AT+QHTTPPOST=<data_len>,<input_time>,<timeout> */
                if( pRequest->pData != NULL )
                {
                    ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                                      "AT+QHTTPPOST=%u,%u,%u",
                                      pRequest->dataLen,
                                      pHttpContext->config.requestTimeout,
                                      pHttpContext->config.responseTimeout );

                    atReqHttpReq.pAtCmd = cmdBuf;
                    atReqHttpReq.atCmdType = CELLULAR_AT_WITH_PREFIX;
                    atReqHttpReq.pAtRspPrefix = "CONNECT";
                    atReqHttpReq.respCallback = _httpRequestCallback;
                    atReqHttpReq.pData = pResponse;
                    atReqHttpReq.dataLen = sizeof( CellularHttpResponse_t );

                    pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqHttpReq );

                    if( pktStatus == CELLULAR_PKT_STATUS_OK )
                    {
                        /* Send POST data. */
                        pktStatus = _Cellular_AtcmdDataSend( pContext,
                                                            atReqHttpReq,
                                                            ( const uint8_t * ) pRequest->pData,
                                                            pRequest->dataLen,
                                                            NULL,
                                                            0 );
                    }

                    if( pktStatus != CELLULAR_PKT_STATUS_OK )
                    {
                        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
                    }
                }
                else
                {
                    cellularStatus = CELLULAR_BAD_PARAMETER;
                }
            }
            else if( pRequest->method == CELLULAR_HTTP_METHOD_HEAD )
            {
                /* AT+QHTTPHEAD=<timeout> */
                ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                                  "AT+QHTTPHEAD=%u", pHttpContext->config.responseTimeout );

                atReqHttpReq.pAtCmd = cmdBuf;
                atReqHttpReq.atCmdType = CELLULAR_AT_NO_RESULT;
                atReqHttpReq.pAtRspPrefix = NULL;
                atReqHttpReq.respCallback = _httpRequestCallback;
                atReqHttpReq.pData = pResponse;
                atReqHttpReq.dataLen = sizeof( CellularHttpResponse_t );

                pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqHttpReq );
                cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
            }
            else
            {
                /* Other methods not implemented yet. */
                cellularStatus = CELLULAR_UNSUPPORTED;
            }
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_CommonHttpReadData( CellularHandle_t cellularHandle,
                                             CellularHttpHandle_t httpHandle,
                                             uint8_t * pBuffer,
                                             uint32_t bufferLength,
                                             uint32_t * pReceivedDataLength )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularHttpContext_t * pHttpContext = ( CellularHttpContext_t * ) httpHandle;
    char cmdBuf[ 64 ];
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularAtReq_t atReqHttpRead = { 0 };
    uint32_t readLen = 0;

    /* Validate parameters. */
    if( ( pContext == NULL ) || ( pHttpContext == NULL ) ||
        ( pBuffer == NULL ) || ( bufferLength == 0U ) ||
        ( pReceivedDataLength == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( !pHttpContext->isRequestActive )
    {
        cellularStatus = CELLULAR_NOT_ALLOWED;
    }
    else if( pHttpContext->remainingDataLength == 0U )
    {
        /* No more data to read. */
        *pReceivedDataLength = 0;
        pHttpContext->isRequestActive = false;
    }
    else
    {
        /* Calculate how much data to read in this chunk.
         * For large files (MB-level), we read in chunks to avoid memory issues. */
        readLen = ( pHttpContext->remainingDataLength < bufferLength ) ?
                  pHttpContext->remainingDataLength : bufferLength;

        /* Limit chunk size for efficiency. */
        if( readLen > HTTP_READ_CHUNK_SIZE )
        {
            readLen = HTTP_READ_CHUNK_SIZE;
        }

        /* AT+QHTTPREAD=<timeout> - Read HTTP response data */
        ( void ) snprintf( cmdBuf, sizeof( cmdBuf ),
                          "AT+QHTTPREAD=%u", pHttpContext->config.responseTimeout );

        atReqHttpRead.pAtCmd = cmdBuf;
        atReqHttpRead.atCmdType = CELLULAR_AT_WITH_PREFIX;
        atReqHttpRead.pAtRspPrefix = "+QHTTPREAD";
        atReqHttpRead.respCallback = _httpReadDataCallback;
        atReqHttpRead.pData = pBuffer;
        atReqHttpRead.dataLen = ( uint16_t ) readLen;

        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqHttpRead );

        if( pktStatus == CELLULAR_PKT_STATUS_OK )
        {
            *pReceivedDataLength = readLen;
            pHttpContext->remainingDataLength -= readLen;
            pHttpContext->response.receivedLength += readLen;

            /* Check if all data has been read. */
            if( pHttpContext->remainingDataLength == 0U )
            {
                pHttpContext->isRequestActive = false;
            }
        }
        else
        {
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
            *pReceivedDataLength = 0;
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_CommonHttpRegisterDataReadyCallback( CellularHandle_t cellularHandle,
                                                              CellularHttpHandle_t httpHandle,
                                                              CellularHttpDataReadyCallback_t dataReadyCallback,
                                                              void * pCallbackContext )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularHttpContext_t * pHttpContext = ( CellularHttpContext_t * ) httpHandle;

    /* Validate parameters. */
    if( ( pContext == NULL ) || ( pHttpContext == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        pHttpContext->dataReadyCallback = dataReadyCallback;
        pHttpContext->pDataReadyCallbackContext = pCallbackContext;
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_CommonHttpRegisterRequestCompleteCallback( CellularHandle_t cellularHandle,
                                                                    CellularHttpHandle_t httpHandle,
                                                                    CellularHttpRequestCompleteCallback_t requestCompleteCallback,
                                                                    void * pCallbackContext )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularHttpContext_t * pHttpContext = ( CellularHttpContext_t * ) httpHandle;

    /* Validate parameters. */
    if( ( pContext == NULL ) || ( pHttpContext == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        pHttpContext->completeCallback = requestCompleteCallback;
        pHttpContext->pCompleteCallbackContext = pCallbackContext;
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

CellularError_t Cellular_CommonHttpQueryDataLength( CellularHandle_t cellularHandle,
                                                    CellularHttpHandle_t httpHandle,
                                                    uint32_t * pRemainingLength )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularHttpContext_t * pHttpContext = ( CellularHttpContext_t * ) httpHandle;

    /* Validate parameters. */
    if( ( pContext == NULL ) || ( pHttpContext == NULL ) ||
        ( pRemainingLength == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        *pRemainingLength = pHttpContext->remainingDataLength;
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/
