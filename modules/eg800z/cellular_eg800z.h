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
 * @file cellular_eg800z.h
 * @brief Definitions for Quectel EG800Z LTE module support with MQTT, HTTP, FILE, and SSL AT commands
 */

#ifndef __CELLULAR_EG800Z_H__
#define __CELLULAR_EG800Z_H__

/* *INDENT-OFF* */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON* */

#include "cellular_config.h"
#include "cellular_config_defaults.h"
#include "cellular_types.h"

/*-----------------------------------------------------------*/

/**
 * @brief Maximum number of MQTT clients supported.
 */
#define CELLULAR_EG800Z_MAX_MQTT_CLIENTS        ( 6U )

/**
 * @brief Maximum HTTP session ID.
 */
#define CELLULAR_EG800Z_MAX_HTTP_SESSIONS       ( 1U )

/**
 * @brief Maximum SSL context ID.
 */
#define CELLULAR_EG800Z_MAX_SSL_CONTEXTS        ( 6U )

/**
 * @brief Maximum file name length.
 */
#define CELLULAR_EG800Z_MAX_FILE_NAME_LENGTH    ( 80U )

/*-----------------------------------------------------------*/

/**
 * @brief MQTT QoS levels.
 */
typedef enum CellularEG800Z_MqttQos
{
    CELLULAR_EG800Z_MQTT_QOS_0 = 0,  /**< At most once delivery */
    CELLULAR_EG800Z_MQTT_QOS_1 = 1,  /**< At least once delivery */
    CELLULAR_EG800Z_MQTT_QOS_2 = 2   /**< Exactly once delivery */
} CellularEG800Z_MqttQos_t;

/**
 * @brief MQTT connection state.
 */
typedef enum CellularEG800Z_MqttState
{
    CELLULAR_EG800Z_MQTT_DISCONNECTED = 0,
    CELLULAR_EG800Z_MQTT_CONNECTING,
    CELLULAR_EG800Z_MQTT_CONNECTED,
    CELLULAR_EG800Z_MQTT_DISCONNECTING
} CellularEG800Z_MqttState_t;

/**
 * @brief HTTP method types.
 */
typedef enum CellularEG800Z_HttpMethod
{
    CELLULAR_EG800Z_HTTP_GET = 0,
    CELLULAR_EG800Z_HTTP_POST,
    CELLULAR_EG800Z_HTTP_PUT,
    CELLULAR_EG800Z_HTTP_HEAD
} CellularEG800Z_HttpMethod_t;

/**
 * @brief SSL version types.
 */
typedef enum CellularEG800Z_SslVersion
{
    CELLULAR_EG800Z_SSL_VERSION_ALL = 0,     /**< SSL3.0, TLS1.0, TLS1.1, TLS1.2 */
    CELLULAR_EG800Z_SSL_VERSION_TLSV10 = 1,  /**< TLS1.0 */
    CELLULAR_EG800Z_SSL_VERSION_TLSV11 = 2,  /**< TLS1.1 */
    CELLULAR_EG800Z_SSL_VERSION_TLSV12 = 3,  /**< TLS1.2 */
    CELLULAR_EG800Z_SSL_VERSION_TLSV13 = 4   /**< TLS1.3 */
} CellularEG800Z_SslVersion_t;

/*-----------------------------------------------------------*/

/**
 * @brief MQTT configuration parameters.
 */
typedef struct CellularEG800Z_MqttConfig
{
    uint8_t clientId;                        /**< MQTT client identifier (0-5) */
    uint8_t contextId;                       /**< PDP context ID */
    char * pHostName;                        /**< MQTT broker hostname or IP */
    uint16_t port;                           /**< MQTT broker port */
    char * pClientIdentifier;                /**< MQTT client identifier string */
    char * pUserName;                        /**< Username for authentication */
    char * pPassword;                        /**< Password for authentication */
    uint16_t keepAliveTime;                  /**< Keep alive time in seconds */
    uint8_t cleanSession;                    /**< Clean session flag (0 or 1) */
    uint8_t sslContextId;                    /**< SSL context ID (0-5, or 0xFF for no SSL) */
} CellularEG800Z_MqttConfig_t;

/**
 * @brief HTTP configuration parameters.
 */
typedef struct CellularEG800Z_HttpConfig
{
    uint8_t contextId;                       /**< PDP context ID */
    uint8_t requestHeader;                   /**< Request header mode (0 or 1) */
    uint8_t responseHeader;                  /**< Response header mode (0 or 1) */
    uint8_t sslContextId;                    /**< SSL context ID (0-5, or 0xFF for no SSL) */
    uint16_t timeout;                        /**< Request timeout in seconds */
    char * pContentType;                     /**< Content-Type header value */
} CellularEG800Z_HttpConfig_t;

/**
 * @brief SSL configuration parameters.
 */
typedef struct CellularEG800Z_SslConfig
{
    uint8_t sslContextId;                    /**< SSL context ID (0-5) */
    CellularEG800Z_SslVersion_t sslVersion;  /**< SSL version */
    uint8_t cipherSuite;                     /**< Cipher suite */
    uint8_t secLevel;                        /**< Security level (0-2) */
    uint8_t ignoreLtime;                     /**< Ignore validity check */
    char * pCaCertPath;                      /**< CA certificate file path */
    char * pClientCertPath;                  /**< Client certificate file path */
    char * pClientKeyPath;                   /**< Client private key file path */
} CellularEG800Z_SslConfig_t;

/**
 * @brief File operation types.
 */
typedef enum CellularEG800Z_FileOperation
{
    CELLULAR_EG800Z_FILE_UPLOAD = 0,
    CELLULAR_EG800Z_FILE_DOWNLOAD,
    CELLULAR_EG800Z_FILE_DELETE,
    CELLULAR_EG800Z_FILE_LIST,
    CELLULAR_EG800Z_FILE_GET_SIZE
} CellularEG800Z_FileOperation_t;

/*-----------------------------------------------------------*/

/**
 * @brief Configure and open MQTT client connection.
 *
 * @param[in] cellularHandle The opaque cellular context pointer.
 * @param[in] pMqttConfig MQTT configuration parameters.
 *
 * @return CELLULAR_SUCCESS if the operation is successful, otherwise an error code.
 */
CellularError_t Cellular_EG800Z_MqttOpen( CellularHandle_t cellularHandle,
                                          const CellularEG800Z_MqttConfig_t * pMqttConfig );

/**
 * @brief Connect to MQTT broker.
 *
 * @param[in] cellularHandle The opaque cellular context pointer.
 * @param[in] clientId MQTT client identifier (0-5).
 *
 * @return CELLULAR_SUCCESS if the operation is successful, otherwise an error code.
 */
CellularError_t Cellular_EG800Z_MqttConnect( CellularHandle_t cellularHandle,
                                             uint8_t clientId );

/**
 * @brief Disconnect from MQTT broker.
 *
 * @param[in] cellularHandle The opaque cellular context pointer.
 * @param[in] clientId MQTT client identifier (0-5).
 *
 * @return CELLULAR_SUCCESS if the operation is successful, otherwise an error code.
 */
CellularError_t Cellular_EG800Z_MqttDisconnect( CellularHandle_t cellularHandle,
                                                uint8_t clientId );

/**
 * @brief Close MQTT client connection.
 *
 * @param[in] cellularHandle The opaque cellular context pointer.
 * @param[in] clientId MQTT client identifier (0-5).
 *
 * @return CELLULAR_SUCCESS if the operation is successful, otherwise an error code.
 */
CellularError_t Cellular_EG800Z_MqttClose( CellularHandle_t cellularHandle,
                                           uint8_t clientId );

/**
 * @brief Publish message to MQTT topic.
 *
 * @param[in] cellularHandle The opaque cellular context pointer.
 * @param[in] clientId MQTT client identifier (0-5).
 * @param[in] pTopic Topic name to publish to.
 * @param[in] qos Quality of Service level.
 * @param[in] pPayload Message payload.
 * @param[in] payloadLength Length of the payload.
 *
 * @return CELLULAR_SUCCESS if the operation is successful, otherwise an error code.
 */
CellularError_t Cellular_EG800Z_MqttPublish( CellularHandle_t cellularHandle,
                                             uint8_t clientId,
                                             const char * pTopic,
                                             CellularEG800Z_MqttQos_t qos,
                                             const uint8_t * pPayload,
                                             uint32_t payloadLength );

/**
 * @brief Subscribe to MQTT topic.
 *
 * @param[in] cellularHandle The opaque cellular context pointer.
 * @param[in] clientId MQTT client identifier (0-5).
 * @param[in] pTopic Topic name to subscribe to.
 * @param[in] qos Quality of Service level.
 *
 * @return CELLULAR_SUCCESS if the operation is successful, otherwise an error code.
 */
CellularError_t Cellular_EG800Z_MqttSubscribe( CellularHandle_t cellularHandle,
                                               uint8_t clientId,
                                               const char * pTopic,
                                               CellularEG800Z_MqttQos_t qos );

/**
 * @brief Unsubscribe from MQTT topic.
 *
 * @param[in] cellularHandle The opaque cellular context pointer.
 * @param[in] clientId MQTT client identifier (0-5).
 * @param[in] pTopic Topic name to unsubscribe from.
 *
 * @return CELLULAR_SUCCESS if the operation is successful, otherwise an error code.
 */
CellularError_t Cellular_EG800Z_MqttUnsubscribe( CellularHandle_t cellularHandle,
                                                 uint8_t clientId,
                                                 const char * pTopic );

/**
 * @brief Configure HTTP parameters.
 *
 * @param[in] cellularHandle The opaque cellular context pointer.
 * @param[in] pHttpConfig HTTP configuration parameters.
 *
 * @return CELLULAR_SUCCESS if the operation is successful, otherwise an error code.
 */
CellularError_t Cellular_EG800Z_HttpConfigure( CellularHandle_t cellularHandle,
                                               const CellularEG800Z_HttpConfig_t * pHttpConfig );

/**
 * @brief Perform HTTP GET request.
 *
 * @param[in] cellularHandle The opaque cellular context pointer.
 * @param[in] pUrl URL to request.
 * @param[out] pResponseBuffer Buffer to store response.
 * @param[in] responseBufferSize Size of response buffer.
 * @param[out] pResponseSize Actual size of response received.
 *
 * @return CELLULAR_SUCCESS if the operation is successful, otherwise an error code.
 */
CellularError_t Cellular_EG800Z_HttpGet( CellularHandle_t cellularHandle,
                                         const char * pUrl,
                                         uint8_t * pResponseBuffer,
                                         uint32_t responseBufferSize,
                                         uint32_t * pResponseSize );

/**
 * @brief Perform HTTP POST request.
 *
 * @param[in] cellularHandle The opaque cellular context pointer.
 * @param[in] pUrl URL to post to.
 * @param[in] pPostData Data to post.
 * @param[in] postDataSize Size of post data.
 * @param[out] pResponseBuffer Buffer to store response.
 * @param[in] responseBufferSize Size of response buffer.
 * @param[out] pResponseSize Actual size of response received.
 *
 * @return CELLULAR_SUCCESS if the operation is successful, otherwise an error code.
 */
CellularError_t Cellular_EG800Z_HttpPost( CellularHandle_t cellularHandle,
                                          const char * pUrl,
                                          const uint8_t * pPostData,
                                          uint32_t postDataSize,
                                          uint8_t * pResponseBuffer,
                                          uint32_t responseBufferSize,
                                          uint32_t * pResponseSize );

/**
 * @brief Configure SSL parameters.
 *
 * @param[in] cellularHandle The opaque cellular context pointer.
 * @param[in] pSslConfig SSL configuration parameters.
 *
 * @return CELLULAR_SUCCESS if the operation is successful, otherwise an error code.
 */
CellularError_t Cellular_EG800Z_SslConfigure( CellularHandle_t cellularHandle,
                                              const CellularEG800Z_SslConfig_t * pSslConfig );

/**
 * @brief Upload file to module filesystem.
 *
 * @param[in] cellularHandle The opaque cellular context pointer.
 * @param[in] pFileName File name in module filesystem.
 * @param[in] pFileData File data to upload.
 * @param[in] fileSize Size of file data.
 *
 * @return CELLULAR_SUCCESS if the operation is successful, otherwise an error code.
 */
CellularError_t Cellular_EG800Z_FileUpload( CellularHandle_t cellularHandle,
                                            const char * pFileName,
                                            const uint8_t * pFileData,
                                            uint32_t fileSize );

/**
 * @brief Download file from module filesystem.
 *
 * @param[in] cellularHandle The opaque cellular context pointer.
 * @param[in] pFileName File name in module filesystem.
 * @param[out] pFileBuffer Buffer to store file data.
 * @param[in] fileBufferSize Size of file buffer.
 * @param[out] pFileSize Actual size of file downloaded.
 *
 * @return CELLULAR_SUCCESS if the operation is successful, otherwise an error code.
 */
CellularError_t Cellular_EG800Z_FileDownload( CellularHandle_t cellularHandle,
                                              const char * pFileName,
                                              uint8_t * pFileBuffer,
                                              uint32_t fileBufferSize,
                                              uint32_t * pFileSize );

/**
 * @brief Delete file from module filesystem.
 *
 * @param[in] cellularHandle The opaque cellular context pointer.
 * @param[in] pFileName File name in module filesystem.
 *
 * @return CELLULAR_SUCCESS if the operation is successful, otherwise an error code.
 */
CellularError_t Cellular_EG800Z_FileDelete( CellularHandle_t cellularHandle,
                                            const char * pFileName );

/**
 * @brief List files in module filesystem.
 *
 * @param[in] cellularHandle The opaque cellular context pointer.
 * @param[in] pPattern File pattern to match (e.g., "*.*").
 * @param[out] pFileList Buffer to store file list.
 * @param[in] fileListSize Size of file list buffer.
 *
 * @return CELLULAR_SUCCESS if the operation is successful, otherwise an error code.
 */
CellularError_t Cellular_EG800Z_FileList( CellularHandle_t cellularHandle,
                                          const char * pPattern,
                                          char * pFileList,
                                          uint32_t fileListSize );

/* *INDENT-OFF* */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON* */

#endif /* __CELLULAR_EG800Z_H__ */
