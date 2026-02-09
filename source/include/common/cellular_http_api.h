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
 * @file cellular_http_api.h
 * @brief Common HTTP APIs for cellular modules.
 */

#ifndef __CELLULAR_HTTP_API_H__
#define __CELLULAR_HTTP_API_H__

/* *INDENT-OFF* */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON* */

#include "cellular_types.h"

/*-----------------------------------------------------------*/

/**
 * @brief Initialize HTTP client context.
 *
 * This is the common implementation of HTTP initialization that can be used
 * by cellular modules following the Quectel HTTP AT command standard.
 *
 * @param[in] cellularHandle The cellular context pointer.
 * @param[in] pHttpConfig HTTP configuration parameters.
 * @param[out] pHttpHandle Out parameter to receive the HTTP handle.
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code.
 */
CellularError_t Cellular_CommonHttpInit( CellularHandle_t cellularHandle,
                                         const CellularHttpConfig_t * pHttpConfig,
                                         CellularHttpHandle_t * pHttpHandle );

/**
 * @brief Cleanup HTTP client context.
 *
 * @param[in] cellularHandle The cellular context pointer.
 * @param[in] httpHandle HTTP handle to cleanup.
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code.
 */
CellularError_t Cellular_CommonHttpCleanup( CellularHandle_t cellularHandle,
                                            CellularHttpHandle_t httpHandle );

/**
 * @brief Set custom HTTP header.
 *
 * @param[in] cellularHandle The cellular context pointer.
 * @param[in] httpHandle HTTP handle.
 * @param[in] pHeaderName The name of the HTTP header.
 * @param[in] pHeaderValue The value of the HTTP header.
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code.
 */
CellularError_t Cellular_CommonHttpSetHeader( CellularHandle_t cellularHandle,
                                               CellularHttpHandle_t httpHandle,
                                               const char * pHeaderName,
                                               const char * pHeaderValue );

/**
 * @brief Perform an HTTP request.
 *
 * Supports HTTP GET, POST, HEAD, PUT, DELETE methods.
 * For large file downloads (MB-level), use Cellular_CommonHttpReadData
 * to read response data in chunks.
 *
 * @param[in] cellularHandle The cellular context pointer.
 * @param[in] httpHandle HTTP handle.
 * @param[in] pRequest HTTP request parameters.
 * @param[out] pResponse HTTP response information.
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code.
 */
CellularError_t Cellular_CommonHttpRequest( CellularHandle_t cellularHandle,
                                            CellularHttpHandle_t httpHandle,
                                            const CellularHttpRequest_t * pRequest,
                                            CellularHttpResponse_t * pResponse );

/**
 * @brief Read HTTP response data in chunks.
 *
 * This function is designed to handle large file downloads by reading
 * data in manageable chunks. It should be called in a loop after
 * Cellular_CommonHttpRequest until all data is received.
 *
 * @param[in] cellularHandle The cellular context pointer.
 * @param[in] httpHandle HTTP handle.
 * @param[out] pBuffer Buffer to receive the HTTP response data.
 * @param[in] bufferLength Length of the buffer.
 * @param[out] pReceivedDataLength Actual length of data received.
 *
 * @return CELLULAR_SUCCESS if successful, CELLULAR_SUCCESS with
 * pReceivedDataLength=0 when all data read, otherwise an error code.
 */
CellularError_t Cellular_CommonHttpReadData( CellularHandle_t cellularHandle,
                                             CellularHttpHandle_t httpHandle,
                                             uint8_t * pBuffer,
                                             uint32_t bufferLength,
                                             uint32_t * pReceivedDataLength );

/**
 * @brief Register callback for HTTP data ready events.
 *
 * @param[in] cellularHandle The cellular context pointer.
 * @param[in] httpHandle HTTP handle.
 * @param[in] dataReadyCallback The callback to register.
 * @param[in] pCallbackContext The context to pass to callback.
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code.
 */
CellularError_t Cellular_CommonHttpRegisterDataReadyCallback( CellularHandle_t cellularHandle,
                                                              CellularHttpHandle_t httpHandle,
                                                              CellularHttpDataReadyCallback_t dataReadyCallback,
                                                              void * pCallbackContext );

/**
 * @brief Register callback for HTTP request completion.
 *
 * @param[in] cellularHandle The cellular context pointer.
 * @param[in] httpHandle HTTP handle.
 * @param[in] requestCompleteCallback The callback to register.
 * @param[in] pCallbackContext The context to pass to callback.
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code.
 */
CellularError_t Cellular_CommonHttpRegisterRequestCompleteCallback( CellularHandle_t cellularHandle,
                                                                    CellularHttpHandle_t httpHandle,
                                                                    CellularHttpRequestCompleteCallback_t requestCompleteCallback,
                                                                    void * pCallbackContext );

/**
 * @brief Query the remaining data length to be read.
 *
 * @param[in] cellularHandle The cellular context pointer.
 * @param[in] httpHandle HTTP handle.
 * @param[out] pRemainingLength Remaining data length in bytes.
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code.
 */
CellularError_t Cellular_CommonHttpQueryDataLength( CellularHandle_t cellularHandle,
                                                    CellularHttpHandle_t httpHandle,
                                                    uint32_t * pRemainingLength );

/* *INDENT-OFF* */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON* */

#endif /* __CELLULAR_HTTP_API_H__ */
