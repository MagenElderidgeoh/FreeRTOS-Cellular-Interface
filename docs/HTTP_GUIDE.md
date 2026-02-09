# HTTP Functionality Guide

## Overview

This document describes the HTTP functionality integrated into the FreeRTOS Cellular Interface library, following the Quectel EC600Z/EC800Z/EG800Z HTTP(S) Application Guide V1.4.

## Features

- Support for HTTP methods: GET, POST, HEAD, PUT, DELETE
- HTTPS support through SSL context configuration
- Custom HTTP headers
- Chunked data reading for large file downloads (MB-level)
- Configurable timeouts
- Asynchronous callbacks for data ready and request completion events

## API Overview

### Initialization and Cleanup

```c
CellularError_t Cellular_HttpInit( CellularHandle_t cellularHandle,
                                   const CellularHttpConfig_t * pHttpConfig,
                                   CellularHttpHandle_t * pHttpHandle );

CellularError_t Cellular_HttpCleanup( CellularHandle_t cellularHandle,
                                      CellularHttpHandle_t httpHandle );
```

### HTTP Request

```c
CellularError_t Cellular_HttpRequest( CellularHandle_t cellularHandle,
                                      CellularHttpHandle_t httpHandle,
                                      const CellularHttpRequest_t * pRequest,
                                      CellularHttpResponse_t * pResponse );
```

### Reading Response Data

For large files (MB-level), use chunked reading:

```c
CellularError_t Cellular_HttpReadData( CellularHandle_t cellularHandle,
                                       CellularHttpHandle_t httpHandle,
                                       uint8_t * pBuffer,
                                       uint32_t bufferLength,
                                       uint32_t * pReceivedDataLength );
```

### Custom Headers

```c
CellularError_t Cellular_HttpSetHeader( CellularHandle_t cellularHandle,
                                        CellularHttpHandle_t httpHandle,
                                        const char * pHeaderName,
                                        const char * pHeaderValue );
```

## Usage Example: HTTP GET with Large File

```c
#include "cellular_api.h"

void httpGetLargeFile( CellularHandle_t cellularHandle )
{
    CellularError_t cellularStatus;
    CellularHttpHandle_t httpHandle;
    CellularHttpConfig_t httpConfig;
    CellularHttpRequest_t httpRequest;
    CellularHttpResponse_t httpResponse;
    uint8_t readBuffer[8192];  /* 8KB read buffer */
    uint32_t receivedLen;
    uint32_t totalReceived = 0;

    /* Step 1: Initialize HTTP context */
    httpConfig.contextId = 1;  /* Use PDN context 1 */
    httpConfig.sslCtxId = 0;   /* No SSL (use 1-6 for HTTPS) */
    httpConfig.requestTimeout = 60;
    httpConfig.responseTimeout = 60;
    httpConfig.enableCustomHeader = false;

    cellularStatus = Cellular_HttpInit( cellularHandle, &httpConfig, &httpHandle );
    if( cellularStatus != CELLULAR_SUCCESS )
    {
        /* Handle error */
        return;
    }

    /* Step 2: Prepare HTTP GET request */
    httpRequest.method = CELLULAR_HTTP_METHOD_GET;
    httpRequest.pUrl = "http://example.com/largefile.bin";
    httpRequest.urlLen = strlen( httpRequest.pUrl );
    httpRequest.pData = NULL;
    httpRequest.dataLen = 0;
    httpRequest.contentType = CELLULAR_HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM;
    httpRequest.pCustomHeader = NULL;

    /* Step 3: Send HTTP request */
    cellularStatus = Cellular_HttpRequest( cellularHandle, httpHandle,
                                          &httpRequest, &httpResponse );
    if( cellularStatus != CELLULAR_SUCCESS )
    {
        /* Handle error */
        Cellular_HttpCleanup( cellularHandle, httpHandle );
        return;
    }

    printf( "HTTP Status: %d\n", httpResponse.httpStatusCode );
    printf( "Content Length: %u bytes\n", httpResponse.contentLength );

    /* Step 4: Read response data in chunks for large files */
    while( totalReceived < httpResponse.contentLength )
    {
        cellularStatus = Cellular_HttpReadData( cellularHandle, httpHandle,
                                               readBuffer, sizeof( readBuffer ),
                                               &receivedLen );
        if( cellularStatus != CELLULAR_SUCCESS )
        {
            /* Handle error */
            break;
        }

        if( receivedLen == 0 )
        {
            /* All data received */
            break;
        }

        /* Process received data chunk */
        totalReceived += receivedLen;
        printf( "Received %u bytes, total: %u/%u\n",
                receivedLen, totalReceived, httpResponse.contentLength );

        /* Here you would typically:
         * - Write data to file
         * - Process data
         * - Send data to another component
         */
    }

    printf( "Download complete. Total bytes: %u\n", totalReceived );

    /* Step 5: Cleanup */
    Cellular_HttpCleanup( cellularHandle, httpHandle );
}
```

## Usage Example: HTTP POST with JSON

```c
void httpPostJson( CellularHandle_t cellularHandle )
{
    CellularError_t cellularStatus;
    CellularHttpHandle_t httpHandle;
    CellularHttpConfig_t httpConfig;
    CellularHttpRequest_t httpRequest;
    CellularHttpResponse_t httpResponse;
    const char * jsonData = "{\"temperature\":25.5,\"humidity\":60}";

    /* Initialize HTTP context */
    httpConfig.contextId = 1;
    httpConfig.sslCtxId = 1;  /* Use SSL context 1 for HTTPS */
    httpConfig.requestTimeout = 30;
    httpConfig.responseTimeout = 30;
    httpConfig.enableCustomHeader = true;

    cellularStatus = Cellular_HttpInit( cellularHandle, &httpConfig, &httpHandle );
    if( cellularStatus != CELLULAR_SUCCESS )
    {
        return;
    }

    /* Set custom header */
    Cellular_HttpSetHeader( cellularHandle, httpHandle,
                           "Authorization", "Bearer token123" );

    /* Prepare HTTP POST request */
    httpRequest.method = CELLULAR_HTTP_METHOD_POST;
    httpRequest.pUrl = "https://api.example.com/data";
    httpRequest.urlLen = strlen( httpRequest.pUrl );
    httpRequest.pData = jsonData;
    httpRequest.dataLen = strlen( jsonData );
    httpRequest.contentType = CELLULAR_HTTP_CONTENT_TYPE_APPLICATION_JSON;
    httpRequest.pCustomHeader = NULL;

    /* Send HTTP POST request */
    cellularStatus = Cellular_HttpRequest( cellularHandle, httpHandle,
                                          &httpRequest, &httpResponse );
    if( cellularStatus == CELLULAR_SUCCESS )
    {
        printf( "POST Status: %d\n", httpResponse.httpStatusCode );
    }

    /* Cleanup */
    Cellular_HttpCleanup( cellularHandle, httpHandle );
}
```

## Large File Download Best Practices

When downloading large files (MB-level):

1. **Use Chunked Reading**: Always use `Cellular_HttpReadData()` in a loop to read data in small chunks (e.g., 8KB-64KB per chunk).

2. **Buffer Management**: Use a reasonably sized buffer (8KB-64KB) to balance memory usage and performance.

3. **Error Handling**: Check return values and handle errors appropriately at each step.

4. **Progress Tracking**: Track `totalReceived` vs `contentLength` to monitor download progress.

5. **Timeout Configuration**: Set appropriate timeouts based on network conditions and file size.

6. **Memory Efficiency**: Process or save each chunk immediately rather than buffering the entire file.

## Configuration Options

Configure HTTP behavior in `cellular_config.h`:

```c
/* HTTP request timeout (1-65535 seconds) */
#define CELLULAR_CONFIG_HTTP_REQUEST_TIMEOUT_SECONDS    60

/* HTTP response timeout (1-65535 seconds) */
#define CELLULAR_CONFIG_HTTP_RESPONSE_TIMEOUT_SECONDS   60

/* HTTP read chunk size for large files (bytes) */
#define CELLULAR_CONFIG_HTTP_READ_CHUNK_SIZE            65536

/* Enable custom HTTP headers (0 or 1) */
#define CELLULAR_CONFIG_HTTP_ENABLE_CUSTOM_HEADERS      1

/* Maximum number of custom headers */
#define CELLULAR_CONFIG_HTTP_MAX_CUSTOM_HEADERS         10
```

## Quectel AT Commands Used

The implementation uses the following Quectel AT commands:

- `AT+QHTTPCFG` - Configure HTTP parameters
- `AT+QHTTPURL` - Set HTTP URL
- `AT+QHTTPGET` - Send HTTP GET request
- `AT+QHTTPPOST` - Send HTTP POST request
- `AT+QHTTPHEAD` - Send HTTP HEAD request
- `AT+QHTTPREAD` - Read HTTP response data

## Error Handling

All HTTP functions return `CellularError_t`:

- `CELLULAR_SUCCESS` - Operation successful
- `CELLULAR_BAD_PARAMETER` - Invalid parameters
- `CELLULAR_NO_MEMORY` - Memory allocation failure
- `CELLULAR_TIMEOUT` - Operation timed out
- `CELLULAR_UNSUPPORTED` - Operation not supported
- `CELLULAR_NOT_ALLOWED` - Operation not allowed in current state
- Other error codes as defined in `cellular_types.h`

## Thread Safety

The HTTP APIs use the same thread safety mechanisms as other cellular APIs. Ensure proper synchronization when calling HTTP functions from multiple threads.

## Limitations

- Maximum URL length: 1024 characters
- Maximum custom header length: 512 characters per header
- Maximum custom headers: Configurable (default 10)
- Supported methods: GET, POST, HEAD (PUT and DELETE marked as unsupported in current implementation)

## References

- Quectel EC600Z/EC800Z/EG800Z HTTP(S) Application Guide V1.4
- FreeRTOS Cellular Interface API Documentation
- cellular_api.h - Main API header
- cellular_http_api.h - HTTP common API header
