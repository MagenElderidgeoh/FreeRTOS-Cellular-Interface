# MQTT Implementation for Quectel EC600Z/EC800Z/EG800Z Series

## Overview

This implementation provides MQTT (Message Queuing Telemetry Transport) functionality for Quectel EC600Z/EC800Z/EG800Z series cellular modules using the FreeRTOS Cellular Interface framework.

The implementation is based on the Quectel MQTT Application Guide (Version 1.4.0, dated 2025-09-25) and follows the AT command specifications defined in the official documentation.

## Features

- MQTT protocol versions 3.1 and 3.1.1 support
- QoS levels 0, 1, and 2
- SSL/TLS encrypted connections
- Will message configuration
- Clean session and persistent session support
- Topic subscription and unsubscription
- Message publishing with retain flag
- Asynchronous event notifications via callbacks
- Support for up to 6 concurrent MQTT clients (indices 0-5)

## Supported Modules

- EC600Z-CN
- EC800Z-CN
- EG800Z series

**Note:** For EC800Z-CN and EG800Z series, MQTT functionality is optional. Please contact Quectel technical support for details.

## Architecture

The MQTT implementation consists of the following components:

1. **cellular_mqtt.h** - Header file with MQTT API definitions and data structures
2. **cellular_mqtt.c** - Implementation of MQTT functionality using Quectel AT commands
3. **cellular_mqtt_example.c** - Example code demonstrating MQTT usage

## AT Commands Used

The implementation uses the following Quectel AT commands:

- `AT+QMTCFG` - Configure MQTT optional parameters (version, PDN, keepalive, session, SSL, will message, etc.)
- `AT+QMTOPEN` - Open MQTT client network connection
- `AT+QMTCLOSE` - Close MQTT client network connection
- `AT+QMTCONN` - Connect to MQTT broker
- `AT+QMTDISC` - Disconnect from MQTT broker
- `AT+QMTSUB` - Subscribe to MQTT topic
- `AT+QMTUNS` - Unsubscribe from MQTT topic
- `AT+QMTPUBEX` - Publish MQTT message

## URC (Unsolicited Result Code) Handlers

The following URCs are handled by the implementation:

- `+QMTSTAT` - MQTT link layer status change
- `+QMTRECV` - MQTT message received from broker
- `+QMTPING` - MQTT keep-alive ping status

## API Functions

### Configuration and Connection

```c
CellularError_t Cellular_MqttConfigure( CellularHandle_t cellularHandle,
                                        uint8_t clientIdx,
                                        const CellularMqttConnectInfo_t * pConnectInfo,
                                        const CellularMqttWillInfo_t * pWillInfo,
                                        const CellularMqttSslConfig_t * pSslConfig );
```

Configure MQTT client parameters including protocol version, PDN context, keep-alive time, session type, SSL settings, and Will message.

```c
CellularError_t Cellular_MqttOpen( CellularHandle_t cellularHandle,
                                   uint8_t clientIdx,
                                   const char * pHostName,
                                   uint16_t port );
```

Open a network connection to the MQTT broker at the TCP/IP level.

```c
CellularError_t Cellular_MqttConnect( CellularHandle_t cellularHandle,
                                      const CellularMqttConnectInfo_t * pConnectInfo );
```

Connect to the MQTT broker at the application level using MQTT CONNECT packet.

```c
CellularError_t Cellular_MqttDisconnect( CellularHandle_t cellularHandle,
                                         uint8_t clientIdx );
```

Disconnect from the MQTT broker gracefully using MQTT DISCONNECT packet.

```c
CellularError_t Cellular_MqttClose( CellularHandle_t cellularHandle,
                                    uint8_t clientIdx );
```

Close the network connection to the MQTT broker.

### Publish and Subscribe

```c
CellularError_t Cellular_MqttSubscribe( CellularHandle_t cellularHandle,
                                        uint8_t clientIdx,
                                        const CellularMqttSubscribeInfo_t * pSubscribeInfo );
```

Subscribe to an MQTT topic with specified QoS level.

```c
CellularError_t Cellular_MqttUnsubscribe( CellularHandle_t cellularHandle,
                                          uint8_t clientIdx,
                                          uint16_t msgId,
                                          const char * pTopic );
```

Unsubscribe from an MQTT topic.

```c
CellularError_t Cellular_MqttPublish( CellularHandle_t cellularHandle,
                                      uint8_t clientIdx,
                                      const CellularMqttPublishInfo_t * pPublishInfo );
```

Publish a message to an MQTT topic with specified QoS and retain flag.

### Callbacks

```c
CellularError_t Cellular_MqttRegisterConnectionCallback( CellularHandle_t cellularHandle,
                                                         CellularMqttConnectionCallback_t connectionCallback,
                                                         void * pCallbackContext );
```

Register a callback function to be notified of MQTT connection status changes.

```c
CellularError_t Cellular_MqttRegisterMessageCallback( CellularHandle_t cellularHandle,
                                                      CellularMqttMessageReceivedCallback_t messageCallback,
                                                      void * pCallbackContext );
```

Register a callback function to receive incoming MQTT messages.

## Usage Example

See `docs/cellular_mqtt_example.c` for a complete example demonstrating:

1. Registering callbacks for connection status and message reception
2. Configuring MQTT parameters
3. Opening network connection
4. Connecting to MQTT broker
5. Subscribing to a topic
6. Publishing a message
7. Receiving messages (via callback)
8. Unsubscribing from topic
9. Disconnecting and cleanup

## Data Flow

The MQTT data interaction follows this sequence:

1. **Configuration**: Configure MQTT parameters using `AT+QMTCFG`
2. **Open Network**: Establish TCP connection using `AT+QMTOPEN`
3. **Connect**: Send MQTT CONNECT packet using `AT+QMTCONN`
4. **Subscribe**: Subscribe to topics using `AT+QMTSUB`
5. **Publish**: Publish messages using `AT+QMTPUBEX`
6. **Receive**: Receive messages via `+QMTRECV` URC
7. **Disconnect**: Send MQTT DISCONNECT using `AT+QMTDISC`
8. **Close**: Close TCP connection using `AT+QMTCLOSE`

## Integration

To integrate this MQTT implementation into your project:

1. Include the MQTT header file:
   ```c
   #include "cellular_mqtt.h"
   ```

2. Ensure you have initialized the cellular handle using `Cellular_Init()`

3. Activate a PDN context before using MQTT:
   ```c
   Cellular_SetPdnConfig( cellularHandle, PDN_CONTEXT_ID, &pdnConfig );
   Cellular_ActivatePdn( cellularHandle, PDN_CONTEXT_ID );
   ```

4. Use the MQTT APIs as shown in the example

## Building

Add the following source files to your build system:

- `source/cellular_mqtt.c`
- Include `source/include/cellular_mqtt.h` in your include path

## Notes

1. **Flash Size Requirement**: This implementation is only applicable to modules with 4 MB Flash size.

2. **SSL/TLS Support**: For SSL/TLS encrypted MQTT connections, you need to configure the SSL context using the Quectel SSL AT commands before enabling SSL in MQTT configuration.

3. **Keep-Alive**: The module automatically sends PINGREQ packets during the keep-alive interval to maintain the connection.

4. **QoS Handling**: 
   - QoS 0: At most once delivery (fire and forget)
   - QoS 1: At least once delivery (acknowledged)
   - QoS 2: Exactly once delivery (assured)

5. **Error Handling**: Always check the return value of MQTT functions. In case of errors, appropriate error codes from `CellularError_t` enum are returned.

6. **Message ID**: Each publish, subscribe, and unsubscribe operation requires a unique message ID for tracking responses.

## Limitations

1. Maximum number of concurrent MQTT clients: 6 (indices 0-5)
2. Maximum topic length: 256 characters
3. Maximum message payload length: 1024 bytes (can be extended if needed)
4. The implementation relies on the underlying cellular module firmware supporting the Quectel MQTT AT commands

## Reference Documentation

- Quectel EC600Z&EC800Z&EG800Z Series MQTT Application Guide V1.4.0
- Quectel LTE Standard(A) Series AT Command Manual
- FreeRTOS Cellular Interface Documentation

## License

This code is provided under the MIT License. See the LICENSE file in the repository root for details.

## Support

For technical support or questions about this implementation:
- Open an issue in the GitHub repository
- Refer to the Quectel technical documentation in the `docs/` folder
- Contact Quectel technical support for module-specific questions
