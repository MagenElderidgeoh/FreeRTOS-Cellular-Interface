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
 * @file cellular_mqtt_example.c
 * @brief Example demonstrating MQTT usage with Quectel EC600Z/EC800Z/EG800Z modules
 *
 * This example demonstrates:
 * 1. MQTT configuration
 * 2. Opening MQTT network connection
 * 3. Connecting to MQTT broker
 * 4. Subscribing to a topic
 * 5. Publishing a message
 * 6. Disconnecting and cleanup
 */

#include <stdio.h>
#include <string.h>

#include "cellular_config.h"
#include "cellular_types.h"
#include "cellular_api.h"
#include "cellular_mqtt.h"

/*-----------------------------------------------------------*/

/* MQTT broker settings - update these for your broker */
#define MQTT_BROKER_ENDPOINT        "broker.emqx.io"
#define MQTT_BROKER_PORT            1883
#define MQTT_CLIENT_IDENTIFIER      "FreeRTOS_Cellular_Client"

/* MQTT topics */
#define MQTT_EXAMPLE_TOPIC          "freertos/cellular/example"

/* MQTT QoS level */
#define MQTT_EXAMPLE_QOS            CELLULAR_MQTT_QOS_1

/* MQTT client index (0-5) */
#define MQTT_CLIENT_IDX             0

/* PDN context ID */
#define PDN_CONTEXT_ID              1

/*-----------------------------------------------------------*/

/**
 * @brief Callback function for MQTT connection status changes
 */
static void mqttConnectionCallback( uint8_t clientIdx,
                                   CellularMqttConnectionStatus_t status,
                                   void * pCallbackContext )
{
    ( void ) pCallbackContext;

    printf( "MQTT Connection Status Changed - Client: %u, Status: %u\n",
            clientIdx, ( unsigned int ) status );

    if( status == CELLULAR_MQTT_CONNECTED )
    {
        printf( "MQTT Connected successfully\n" );
    }
    else if( status == CELLULAR_MQTT_DISCONNECTED )
    {
        printf( "MQTT Disconnected\n" );
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Callback function for received MQTT messages
 */
static void mqttMessageCallback( const CellularMqttMessageInfo_t * pMessageInfo,
                                void * pCallbackContext )
{
    ( void ) pCallbackContext;

    if( pMessageInfo != NULL )
    {
        printf( "MQTT Message Received:\n" );
        printf( "  Client Index: %u\n", pMessageInfo->clientIdx );
        printf( "  Message ID: %u\n", pMessageInfo->msgId );
        printf( "  Topic: %s\n", pMessageInfo->pTopic );
        printf( "  Payload Length: %lu\n", ( unsigned long ) pMessageInfo->payloadLength );
        printf( "  Payload: %.*s\n",
                ( int ) pMessageInfo->payloadLength,
                ( const char * ) pMessageInfo->pPayload );
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Example function demonstrating MQTT operations
 */
CellularError_t runMqttExample( CellularHandle_t cellularHandle )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularMqttConnectInfo_t connectInfo = { 0 };
    CellularMqttWillInfo_t willInfo = { 0 };
    CellularMqttSubscribeInfo_t subscribeInfo = { 0 };
    CellularMqttPublishInfo_t publishInfo = { 0 };
    const char * pPublishPayload = "Hello from FreeRTOS Cellular!";

    /* Step 1: Register callbacks */
    printf( "Step 1: Registering MQTT callbacks\n" );
    cellularStatus = Cellular_MqttRegisterConnectionCallback( cellularHandle,
                                                              mqttConnectionCallback,
                                                              NULL );

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        cellularStatus = Cellular_MqttRegisterMessageCallback( cellularHandle,
                                                               mqttMessageCallback,
                                                               NULL );
    }

    /* Step 2: Configure MQTT parameters */
    if( cellularStatus == CELLULAR_SUCCESS )
    {
        printf( "Step 2: Configuring MQTT parameters\n" );

        /* Set connection parameters */
        connectInfo.clientIdx = MQTT_CLIENT_IDX;
        connectInfo.pHostName = MQTT_BROKER_ENDPOINT;
        connectInfo.port = MQTT_BROKER_PORT;
        connectInfo.pClientId = MQTT_CLIENT_IDENTIFIER;
        connectInfo.pUserName = NULL;  /* No authentication for public broker */
        connectInfo.pPassword = NULL;
        connectInfo.keepAliveSeconds = 60;
        connectInfo.cleanSession = true;
        connectInfo.version = CELLULAR_MQTT_VERSION_3_1_1;
        connectInfo.pdnContextId = PDN_CONTEXT_ID;

        /* Configure optional Will message */
        willInfo.willFlag = true;
        willInfo.willQos = CELLULAR_MQTT_QOS_0;
        willInfo.willRetain = false;
        willInfo.pWillTopic = MQTT_EXAMPLE_TOPIC "/will";
        willInfo.pWillMessage = "Client disconnected unexpectedly";
        willInfo.willMessageLength = strlen( willInfo.pWillMessage );

        cellularStatus = Cellular_MqttConfigure( cellularHandle,
                                                 MQTT_CLIENT_IDX,
                                                 &connectInfo,
                                                 &willInfo,
                                                 NULL );  /* No SSL for this example */
    }

    /* Step 3: Open MQTT network connection */
    if( cellularStatus == CELLULAR_SUCCESS )
    {
        printf( "Step 3: Opening MQTT network connection\n" );
        cellularStatus = Cellular_MqttOpen( cellularHandle,
                                           MQTT_CLIENT_IDX,
                                           MQTT_BROKER_ENDPOINT,
                                           MQTT_BROKER_PORT );
    }

    /* Step 4: Connect to MQTT broker */
    if( cellularStatus == CELLULAR_SUCCESS )
    {
        printf( "Step 4: Connecting to MQTT broker\n" );
        cellularStatus = Cellular_MqttConnect( cellularHandle, &connectInfo );
    }

    /* Step 5: Subscribe to a topic */
    if( cellularStatus == CELLULAR_SUCCESS )
    {
        printf( "Step 5: Subscribing to topic: %s\n", MQTT_EXAMPLE_TOPIC );

        subscribeInfo.msgId = 1;
        subscribeInfo.pTopic = MQTT_EXAMPLE_TOPIC;
        subscribeInfo.qos = MQTT_EXAMPLE_QOS;

        cellularStatus = Cellular_MqttSubscribe( cellularHandle,
                                                 MQTT_CLIENT_IDX,
                                                 &subscribeInfo );
    }

    /* Step 6: Publish a message */
    if( cellularStatus == CELLULAR_SUCCESS )
    {
        printf( "Step 6: Publishing message to topic: %s\n", MQTT_EXAMPLE_TOPIC );

        publishInfo.msgId = 2;
        publishInfo.qos = MQTT_EXAMPLE_QOS;
        publishInfo.retain = false;
        publishInfo.pTopic = MQTT_EXAMPLE_TOPIC;
        publishInfo.pPayload = ( const uint8_t * ) pPublishPayload;
        publishInfo.payloadLength = strlen( pPublishPayload );

        cellularStatus = Cellular_MqttPublish( cellularHandle,
                                               MQTT_CLIENT_IDX,
                                               &publishInfo );
    }

    /* Step 7: Wait for messages (in real application, this would be event-driven) */
    if( cellularStatus == CELLULAR_SUCCESS )
    {
        printf( "Step 7: Waiting for messages...\n" );
        /* In a real application, messages would be received via callbacks */
        /* For demonstration purposes, we'll just add a comment here */
        printf( "  (Messages will be received via mqttMessageCallback)\n" );
    }

    /* Step 8: Unsubscribe from topic */
    if( cellularStatus == CELLULAR_SUCCESS )
    {
        printf( "Step 8: Unsubscribing from topic\n" );
        cellularStatus = Cellular_MqttUnsubscribe( cellularHandle,
                                                   MQTT_CLIENT_IDX,
                                                   3,
                                                   MQTT_EXAMPLE_TOPIC );
    }

    /* Step 9: Disconnect from MQTT broker */
    if( cellularStatus == CELLULAR_SUCCESS )
    {
        printf( "Step 9: Disconnecting from MQTT broker\n" );
        cellularStatus = Cellular_MqttDisconnect( cellularHandle, MQTT_CLIENT_IDX );
    }

    /* Step 10: Close MQTT network connection */
    if( cellularStatus == CELLULAR_SUCCESS )
    {
        printf( "Step 10: Closing MQTT network connection\n" );
        cellularStatus = Cellular_MqttClose( cellularHandle, MQTT_CLIENT_IDX );
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        printf( "MQTT Example completed successfully!\n" );
    }
    else
    {
        printf( "MQTT Example failed with error code: %d\n", cellularStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/
