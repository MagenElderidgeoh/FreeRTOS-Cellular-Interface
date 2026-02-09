# Quectel EG800Z LTE Module Integration

## Overview

This module provides integration for the Quectel EG800Z LTE standard module with the FreeRTOS Cellular Interface library. It implements support for MQTT, HTTP(S), FILE, and SSL AT commands based on Quectel's official documentation.

## Supported Features

### MQTT Support
- Multiple MQTT client connections (up to 6 clients)
- SSL/TLS support for secure MQTT connections
- Quality of Service (QoS) levels 0, 1, and 2
- Publish/Subscribe operations
- Keep-alive and clean session support

**Implemented Commands:**
- `AT+QMTCFG` - Configure MQTT parameters
- `AT+QMTOPEN` - Open network for MQTT client
- `AT+QMTCONN` - Connect a client to MQTT server
- `AT+QMTDISC` - Disconnect a client from MQTT server
- `AT+QMTCLOSE` - Close a network for MQTT client
- `AT+QMTPUBEX` - Publish messages
- `AT+QMTSUB` - Subscribe to topics
- `AT+QMTUNS` - Unsubscribe from topics

### HTTP(S) Support
- HTTP/HTTPS GET and POST requests
- Configurable request/response headers
- SSL/TLS support for HTTPS
- Timeout configuration

**Implemented Commands:**
- `AT+QHTTPCFG` - Configure HTTP(S) parameters
- `AT+QHTTPURL` - Set URL for HTTP(S) request
- `AT+QHTTPGET` - Send HTTP(S) GET request
- `AT+QHTTPPOST` - Send HTTP(S) POST request
- `AT+QHTTPREAD` - Read HTTP(S) response

### SSL/TLS Support
- Multiple SSL contexts (up to 6 contexts)
- Configurable SSL versions (SSL3.0, TLS1.0/1.1/1.2/1.3)
- Certificate management (CA, client cert, client key)
- Cipher suite configuration
- Security level configuration

**Implemented Commands:**
- `AT+QSSLCFG` - Configure SSL parameters (version, certificates, security level)

### FILE Operations
- File upload to module filesystem
- File download from module filesystem
- File deletion
- File listing with pattern matching

**Implemented Commands:**
- `AT+QFUPL` - Upload file
- `AT+QFDWL` - Download file
- `AT+QFDEL` - Delete file
- `AT+QFLST` - List files

## API Reference

### MQTT APIs

```c
/* Configure and open MQTT connection */
CellularError_t Cellular_EG800Z_MqttOpen( 
    CellularHandle_t cellularHandle,
    const CellularEG800Z_MqttConfig_t * pMqttConfig );

/* Connect to MQTT broker */
CellularError_t Cellular_EG800Z_MqttConnect( 
    CellularHandle_t cellularHandle,
    uint8_t clientId );

/* Publish message */
CellularError_t Cellular_EG800Z_MqttPublish( 
    CellularHandle_t cellularHandle,
    uint8_t clientId,
    const char * pTopic,
    CellularEG800Z_MqttQos_t qos,
    const uint8_t * pPayload,
    uint32_t payloadLength );

/* Subscribe to topic */
CellularError_t Cellular_EG800Z_MqttSubscribe( 
    CellularHandle_t cellularHandle,
    uint8_t clientId,
    const char * pTopic,
    CellularEG800Z_MqttQos_t qos );

/* Disconnect and close */
CellularError_t Cellular_EG800Z_MqttDisconnect( 
    CellularHandle_t cellularHandle,
    uint8_t clientId );

CellularError_t Cellular_EG800Z_MqttClose( 
    CellularHandle_t cellularHandle,
    uint8_t clientId );
```

### HTTP(S) APIs

```c
/* Configure HTTP parameters */
CellularError_t Cellular_EG800Z_HttpConfigure( 
    CellularHandle_t cellularHandle,
    const CellularEG800Z_HttpConfig_t * pHttpConfig );

/* Perform HTTP GET */
CellularError_t Cellular_EG800Z_HttpGet( 
    CellularHandle_t cellularHandle,
    const char * pUrl,
    uint8_t * pResponseBuffer,
    uint32_t responseBufferSize,
    uint32_t * pResponseSize );

/* Perform HTTP POST */
CellularError_t Cellular_EG800Z_HttpPost( 
    CellularHandle_t cellularHandle,
    const char * pUrl,
    const uint8_t * pPostData,
    uint32_t postDataSize,
    uint8_t * pResponseBuffer,
    uint32_t responseBufferSize,
    uint32_t * pResponseSize );
```

### SSL APIs

```c
/* Configure SSL context */
CellularError_t Cellular_EG800Z_SslConfigure( 
    CellularHandle_t cellularHandle,
    const CellularEG800Z_SslConfig_t * pSslConfig );
```

### FILE APIs

```c
/* Upload file */
CellularError_t Cellular_EG800Z_FileUpload( 
    CellularHandle_t cellularHandle,
    const char * pFileName,
    const uint8_t * pFileData,
    uint32_t fileSize );

/* Download file */
CellularError_t Cellular_EG800Z_FileDownload( 
    CellularHandle_t cellularHandle,
    const char * pFileName,
    uint8_t * pFileBuffer,
    uint32_t fileBufferSize,
    uint32_t * pFileSize );

/* Delete file */
CellularError_t Cellular_EG800Z_FileDelete( 
    CellularHandle_t cellularHandle,
    const char * pFileName );

/* List files */
CellularError_t Cellular_EG800Z_FileList( 
    CellularHandle_t cellularHandle,
    const char * pPattern,
    char * pFileList,
    uint32_t fileListSize );
```

## Usage Example

### MQTT Example

```c
#include "cellular_api.h"
#include "cellular_eg800z.h"

/* Configure MQTT connection */
CellularEG800Z_MqttConfig_t mqttConfig = {
    .clientId = 0,
    .contextId = 1,
    .pHostName = "mqtt.example.com",
    .port = 8883,
    .pClientIdentifier = "my-iot-device",
    .pUserName = "username",
    .pPassword = "password",
    .keepAliveTime = 120,
    .cleanSession = 1,
    .sslContextId = 0  /* Use SSL context 0 */
};

/* Open MQTT connection */
CellularError_t status = Cellular_EG800Z_MqttOpen( cellularHandle, &mqttConfig );

if( status == CELLULAR_SUCCESS )
{
    /* Connect to broker */
    status = Cellular_EG800Z_MqttConnect( cellularHandle, 0 );
    
    if( status == CELLULAR_SUCCESS )
    {
        /* Publish message */
        const char* topic = "sensor/temperature";
        const char* payload = "{\"temp\":25.5}";
        
        status = Cellular_EG800Z_MqttPublish( 
            cellularHandle, 
            0, 
            topic, 
            CELLULAR_EG800Z_MQTT_QOS_1,
            (const uint8_t*)payload,
            strlen(payload) );
    }
}
```

### HTTP(S) Example

```c
/* Configure HTTPS with SSL */
CellularEG800Z_HttpConfig_t httpConfig = {
    .contextId = 1,
    .requestHeader = 0,
    .responseHeader = 0,
    .sslContextId = 0,  /* Use SSL context 0 for HTTPS */
    .timeout = 60,
    .pContentType = "application/json"
};

Cellular_EG800Z_HttpConfigure( cellularHandle, &httpConfig );

/* Perform HTTPS GET request */
uint8_t responseBuffer[2048];
uint32_t responseSize;

status = Cellular_EG800Z_HttpGet( 
    cellularHandle,
    "https://api.example.com/data",
    responseBuffer,
    sizeof(responseBuffer),
    &responseSize );
```

### SSL Configuration Example

```c
/* Configure SSL context for TLS 1.2 with certificates */
CellularEG800Z_SslConfig_t sslConfig = {
    .sslContextId = 0,
    .sslVersion = CELLULAR_EG800Z_SSL_VERSION_TLSV12,
    .cipherSuite = 0xFFFF,  /* All supported ciphers */
    .secLevel = 1,           /* Verify server certificate */
    .ignoreLtime = 0,
    .pCaCertPath = "cacert.pem",
    .pClientCertPath = "clientcert.pem",
    .pClientKeyPath = "clientkey.pem"
};

Cellular_EG800Z_SslConfigure( cellularHandle, &sslConfig );
```

### FILE Operations Example

```c
/* Upload certificate file */
const char* certData = "-----BEGIN CERTIFICATE-----\n...";
status = Cellular_EG800Z_FileUpload( 
    cellularHandle,
    "cacert.pem",
    (const uint8_t*)certData,
    strlen(certData) );

/* List files */
char fileList[512];
status = Cellular_EG800Z_FileList( 
    cellularHandle,
    "*.*",
    fileList,
    sizeof(fileList) );
```

## Integration with FreeRTOS Cellular Interface

This module implements the required portable interface defined in `cellular_common_portable.h`:

- `Cellular_ModuleInit()` - Initialize module context
- `Cellular_ModuleCleanUp()` - Clean up module resources
- `Cellular_ModuleEnableUE()` - Enable user equipment
- `Cellular_ModuleEnableUrc()` - Enable unsolicited result codes

## Documentation References

The implementation is based on the following Quectel documentation:

1. **MQTT**: `Quectel_EC600Z&EC800Z&EG800Z系列_MQTT_应用指导_V1.4.0_Preliminary_20250925.pdf`
2. **HTTP(S)**: `Quectel_EC600Z&EC800Z&EG800Z系列_HTTP(S)_应用指导_V1.4.pdf`
3. **FILE**: `Quectel_EC600Z&EC800Z&EG800Z系列_FILE_应用指导_V1.2.pdf`
4. **SSL**: `Quectel_EC600Z&EC800Z&EG800Z系列_SSL_应用指导_V1.4.0_Preliminary_20250930.pdf`
5. **AT Commands**: `Quectel_LTE_Standard(A)系列_AT命令手册_V1.3.pdf`

## Build Integration

To include this module in your project:

1. Add the module source files to your build:
   - `modules/eg800z/cellular_eg800z.c`
   - Common cellular library sources

2. Add the include directories:
   - `modules/eg800z/`
   - `source/include/`
   - `source/include/common/`

3. Link with the FreeRTOS Cellular Interface library

## Notes

- The module requires an active PDP context to use MQTT, HTTP, and FILE features
- SSL contexts must be configured before use with MQTT or HTTP
- File operations work with the module's internal filesystem
- Maximum file name length is 80 characters
- Up to 6 MQTT clients and 6 SSL contexts are supported simultaneously
