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
 * @file cellular_mqtt.h
 * @brief MQTT API for Quectel EC600Z/EC800Z/EG800Z series modules
 */

#ifndef __CELLULAR_MQTT_H__
#define __CELLULAR_MQTT_H__

/* *INDENT-OFF* */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON* */

#include "cellular_types.h"

/**
 * @ingroup cellular_datatypes_paramstructs
 * @brief MQTT client index range (0-5)
 */
#define CELLULAR_MQTT_CLIENT_ID_MIN    ( 0U )
#define CELLULAR_MQTT_CLIENT_ID_MAX    ( 5U )

/**
 * @ingroup cellular_datatypes_paramstructs
 * @brief MQTT topic maximum length
 */
#define CELLULAR_MQTT_TOPIC_MAX_LENGTH    ( 256U )

/**
 * @ingroup cellular_datatypes_paramstructs
 * @brief MQTT message maximum length
 */
#define CELLULAR_MQTT_MESSAGE_MAX_LENGTH    ( 1024U )

/**
 * @ingroup cellular_datatypes_enums
 * @brief MQTT protocol version
 */
typedef enum CellularMqttVersion
{
    CELLULAR_MQTT_VERSION_3_1 = 3,      /**< MQTT version 3.1 */
    CELLULAR_MQTT_VERSION_3_1_1 = 4     /**< MQTT version 3.1.1 */
} CellularMqttVersion_t;

/**
 * @ingroup cellular_datatypes_enums
 * @brief MQTT QoS levels
 */
typedef enum CellularMqttQoS
{
    CELLULAR_MQTT_QOS_0 = 0,    /**< At most once delivery */
    CELLULAR_MQTT_QOS_1 = 1,    /**< At least once delivery */
    CELLULAR_MQTT_QOS_2 = 2     /**< Exactly once delivery */
} CellularMqttQoS_t;

/**
 * @ingroup cellular_datatypes_enums
 * @brief MQTT connection status
 */
typedef enum CellularMqttConnectionStatus
{
    CELLULAR_MQTT_DISCONNECTED = 0,     /**< Disconnected */
    CELLULAR_MQTT_CONNECTED = 1,        /**< Connected */
    CELLULAR_MQTT_CONNECTING = 2        /**< Connecting */
} CellularMqttConnectionStatus_t;

/**
 * @ingroup cellular_datatypes_enums
 * @brief MQTT result codes
 */
typedef enum CellularMqttResult
{
    CELLULAR_MQTT_RESULT_SUCCESS = 0,                   /**< Success */
    CELLULAR_MQTT_RESULT_PACKET_RETRANSMIT = 1,         /**< Packet retransmission */
    CELLULAR_MQTT_RESULT_PACKET_SEND_FAIL = 2,          /**< Failed to send packet */
    CELLULAR_MQTT_RESULT_UNKNOWN_ERROR = 0xFFFF         /**< Unknown error */
} CellularMqttResult_t;

/**
 * @ingroup cellular_datatypes_enums
 * @brief MQTT CONNACK return codes
 */
typedef enum CellularMqttConnackReturnCode
{
    CELLULAR_MQTT_CONNACK_ACCEPTED = 0,                         /**< Connection accepted */
    CELLULAR_MQTT_CONNACK_UNACCEPTABLE_PROTOCOL = 1,            /**< Unacceptable protocol version */
    CELLULAR_MQTT_CONNACK_IDENTIFIER_REJECTED = 2,              /**< Identifier rejected */
    CELLULAR_MQTT_CONNACK_SERVER_UNAVAILABLE = 3,               /**< Server unavailable */
    CELLULAR_MQTT_CONNACK_BAD_USERNAME_PASSWORD = 4,            /**< Bad username or password */
    CELLULAR_MQTT_CONNACK_NOT_AUTHORIZED = 5                    /**< Not authorized */
} CellularMqttConnackReturnCode_t;

/**
 * @ingroup cellular_datatypes_paramstructs
 * @brief MQTT connection parameters
 */
typedef struct CellularMqttConnectInfo
{
    uint8_t clientIdx;                          /**< MQTT client index (0-5) */
    const char * pHostName;                     /**< MQTT broker host name or IP address */
    uint16_t port;                              /**< MQTT broker port */
    const char * pClientId;                     /**< MQTT client identifier */
    const char * pUserName;                     /**< MQTT username (optional) */
    const char * pPassword;                     /**< MQTT password (optional) */
    uint16_t keepAliveSeconds;                  /**< Keep alive time in seconds */
    bool cleanSession;                          /**< Clean session flag */
    CellularMqttVersion_t version;              /**< MQTT protocol version */
    uint8_t pdnContextId;                       /**< PDN context ID */
} CellularMqttConnectInfo_t;

/**
 * @ingroup cellular_datatypes_paramstructs
 * @brief MQTT Will message configuration
 */
typedef struct CellularMqttWillInfo
{
    bool willFlag;                              /**< Will flag */
    CellularMqttQoS_t willQos;                  /**< Will message QoS */
    bool willRetain;                            /**< Will retain flag */
    const char * pWillTopic;                    /**< Will topic */
    const char * pWillMessage;                  /**< Will message payload */
    uint16_t willMessageLength;                 /**< Will message length */
} CellularMqttWillInfo_t;

/**
 * @ingroup cellular_datatypes_paramstructs
 * @brief MQTT SSL/TLS configuration
 */
typedef struct CellularMqttSslConfig
{
    bool sslEnable;                             /**< Enable SSL/TLS */
    uint8_t sslContextIdx;                      /**< SSL context index */
} CellularMqttSslConfig_t;

/**
 * @ingroup cellular_datatypes_paramstructs
 * @brief MQTT publish message
 */
typedef struct CellularMqttPublishInfo
{
    uint16_t msgId;                             /**< Message ID */
    CellularMqttQoS_t qos;                      /**< Quality of Service */
    bool retain;                                /**< Retain flag */
    const char * pTopic;                        /**< Topic name */
    const uint8_t * pPayload;                   /**< Message payload */
    uint32_t payloadLength;                     /**< Payload length */
} CellularMqttPublishInfo_t;

/**
 * @ingroup cellular_datatypes_paramstructs
 * @brief MQTT subscription information
 */
typedef struct CellularMqttSubscribeInfo
{
    uint16_t msgId;                             /**< Message ID */
    const char * pTopic;                        /**< Topic filter */
    CellularMqttQoS_t qos;                      /**< Requested QoS */
} CellularMqttSubscribeInfo_t;

/**
 * @ingroup cellular_datatypes_paramstructs
 * @brief MQTT received message
 */
typedef struct CellularMqttMessageInfo
{
    uint8_t clientIdx;                          /**< MQTT client index */
    uint16_t msgId;                             /**< Message ID */
    const char * pTopic;                        /**< Topic name */
    const uint8_t * pPayload;                   /**< Message payload */
    uint32_t payloadLength;                     /**< Payload length */
} CellularMqttMessageInfo_t;

/**
 * @ingroup cellular_datatypes_functionpointers
 * @brief Callback for MQTT connection status changes
 *
 * @param[in] clientIdx MQTT client index
 * @param[in] status Connection status
 * @param[in] pCallbackContext User context
 */
typedef void ( * CellularMqttConnectionCallback_t )( uint8_t clientIdx,
                                                      CellularMqttConnectionStatus_t status,
                                                      void * pCallbackContext );

/**
 * @ingroup cellular_datatypes_functionpointers
 * @brief Callback for MQTT message received
 *
 * @param[in] pMessageInfo Received message information
 * @param[in] pCallbackContext User context
 */
typedef void ( * CellularMqttMessageReceivedCallback_t )( const CellularMqttMessageInfo_t * pMessageInfo,
                                                           void * pCallbackContext );

/**
 * @brief Configure MQTT optional parameters
 *
 * @param[in] cellularHandle The cellular context pointer
 * @param[in] clientIdx MQTT client index
 * @param[in] pConnectInfo Connection information
 * @param[in] pWillInfo Will message information (optional)
 * @param[in] pSslConfig SSL configuration (optional)
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code
 */
CellularError_t Cellular_MqttConfigure( CellularHandle_t cellularHandle,
                                        uint8_t clientIdx,
                                        const CellularMqttConnectInfo_t * pConnectInfo,
                                        const CellularMqttWillInfo_t * pWillInfo,
                                        const CellularMqttSslConfig_t * pSslConfig );

/**
 * @brief Open MQTT client network connection
 *
 * @param[in] cellularHandle The cellular context pointer
 * @param[in] clientIdx MQTT client index
 * @param[in] pHostName MQTT broker host name or IP address
 * @param[in] port MQTT broker port
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code
 */
CellularError_t Cellular_MqttOpen( CellularHandle_t cellularHandle,
                                   uint8_t clientIdx,
                                   const char * pHostName,
                                   uint16_t port );

/**
 * @brief Close MQTT client network connection
 *
 * @param[in] cellularHandle The cellular context pointer
 * @param[in] clientIdx MQTT client index
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code
 */
CellularError_t Cellular_MqttClose( CellularHandle_t cellularHandle,
                                    uint8_t clientIdx );

/**
 * @brief Connect to MQTT broker
 *
 * @param[in] cellularHandle The cellular context pointer
 * @param[in] pConnectInfo Connection information
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code
 */
CellularError_t Cellular_MqttConnect( CellularHandle_t cellularHandle,
                                      const CellularMqttConnectInfo_t * pConnectInfo );

/**
 * @brief Disconnect from MQTT broker
 *
 * @param[in] cellularHandle The cellular context pointer
 * @param[in] clientIdx MQTT client index
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code
 */
CellularError_t Cellular_MqttDisconnect( CellularHandle_t cellularHandle,
                                         uint8_t clientIdx );

/**
 * @brief Subscribe to MQTT topic
 *
 * @param[in] cellularHandle The cellular context pointer
 * @param[in] clientIdx MQTT client index
 * @param[in] pSubscribeInfo Subscription information
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code
 */
CellularError_t Cellular_MqttSubscribe( CellularHandle_t cellularHandle,
                                        uint8_t clientIdx,
                                        const CellularMqttSubscribeInfo_t * pSubscribeInfo );

/**
 * @brief Unsubscribe from MQTT topic
 *
 * @param[in] cellularHandle The cellular context pointer
 * @param[in] clientIdx MQTT client index
 * @param[in] msgId Message ID
 * @param[in] pTopic Topic filter
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code
 */
CellularError_t Cellular_MqttUnsubscribe( CellularHandle_t cellularHandle,
                                          uint8_t clientIdx,
                                          uint16_t msgId,
                                          const char * pTopic );

/**
 * @brief Publish MQTT message
 *
 * @param[in] cellularHandle The cellular context pointer
 * @param[in] clientIdx MQTT client index
 * @param[in] pPublishInfo Publish information
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code
 */
CellularError_t Cellular_MqttPublish( CellularHandle_t cellularHandle,
                                      uint8_t clientIdx,
                                      const CellularMqttPublishInfo_t * pPublishInfo );

/**
 * @brief Register callback for MQTT connection status changes
 *
 * @param[in] cellularHandle The cellular context pointer
 * @param[in] connectionCallback Callback function
 * @param[in] pCallbackContext User context
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code
 */
CellularError_t Cellular_MqttRegisterConnectionCallback( CellularHandle_t cellularHandle,
                                                         CellularMqttConnectionCallback_t connectionCallback,
                                                         void * pCallbackContext );

/**
 * @brief Register callback for MQTT message received
 *
 * @param[in] cellularHandle The cellular context pointer
 * @param[in] messageCallback Callback function
 * @param[in] pCallbackContext User context
 *
 * @return CELLULAR_SUCCESS if successful, otherwise an error code
 */
CellularError_t Cellular_MqttRegisterMessageCallback( CellularHandle_t cellularHandle,
                                                      CellularMqttMessageReceivedCallback_t messageCallback,
                                                      void * pCallbackContext );

/* *INDENT-OFF* */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON* */

#endif /* __CELLULAR_MQTT_H__ */
