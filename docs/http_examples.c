/*
 * Example: HTTP GET request to download a large file
 * 
 * This example demonstrates how to use the HTTP APIs to download
 * a large file (MB-level) in chunks.
 */

#include "cellular_config.h"
#include "cellular_api.h"

/* Example: Download a large file using HTTP GET with chunked reading */
void example_http_download_large_file( CellularHandle_t cellularHandle )
{
    CellularError_t cellularStatus;
    CellularHttpHandle_t httpHandle = NULL;
    CellularHttpConfig_t httpConfig;
    CellularHttpRequest_t httpRequest;
    CellularHttpResponse_t httpResponse;
    uint8_t * pReadBuffer = NULL;
    uint32_t receivedLen;
    uint32_t totalReceived = 0;
    const uint32_t READ_BUFFER_SIZE = 8192; /* 8KB buffer for reading chunks */

    /* Step 1: Configure HTTP client */
    httpConfig.contextId = 1;  /* Use PDN context 1 (must be activated first) */
    httpConfig.sslCtxId = 0;   /* 0 for HTTP, 1-6 for HTTPS */
    httpConfig.requestTimeout = 60;  /* Request timeout in seconds */
    httpConfig.responseTimeout = 60; /* Response timeout in seconds */
    httpConfig.enableCustomHeader = false; /* No custom headers needed */

    /* Initialize HTTP context */
    cellularStatus = Cellular_HttpInit( cellularHandle, &httpConfig, &httpHandle );
    
    if( cellularStatus != CELLULAR_SUCCESS )
    {
        LogError( ( "Failed to initialize HTTP: %d", cellularStatus ) );
        return;
    }

    /* Step 2: Prepare HTTP GET request */
    httpRequest.method = CELLULAR_HTTP_METHOD_GET;
    httpRequest.pUrl = "http://example.com/large_file.bin"; /* URL to download */
    httpRequest.urlLen = strlen( httpRequest.pUrl );
    httpRequest.pData = NULL;  /* No request body for GET */
    httpRequest.dataLen = 0;
    httpRequest.contentType = CELLULAR_HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM;
    httpRequest.pCustomHeader = NULL;

    /* Send HTTP GET request */
    cellularStatus = Cellular_HttpRequest( cellularHandle, httpHandle,
                                          &httpRequest, &httpResponse );
    
    if( cellularStatus != CELLULAR_SUCCESS )
    {
        LogError( ( "HTTP GET request failed: %d", cellularStatus ) );
        Cellular_HttpCleanup( cellularHandle, httpHandle );
        return;
    }

    LogInfo( ( "HTTP Status Code: %d", httpResponse.httpStatusCode ) );
    LogInfo( ( "Content Length: %u bytes (%.2f MB)",
               httpResponse.contentLength,
               ( float ) httpResponse.contentLength / ( 1024.0f * 1024.0f ) ) );

    /* Check if response is successful */
    if( httpResponse.httpStatusCode != 200 )
    {
        LogError( ( "HTTP request failed with status code: %d",
                    httpResponse.httpStatusCode ) );
        Cellular_HttpCleanup( cellularHandle, httpHandle );
        return;
    }

    /* Step 3: Allocate buffer for reading data chunks */
    pReadBuffer = ( uint8_t * ) Platform_Malloc( READ_BUFFER_SIZE );
    
    if( pReadBuffer == NULL )
    {
        LogError( ( "Failed to allocate read buffer" ) );
        Cellular_HttpCleanup( cellularHandle, httpHandle );
        return;
    }

    /* Step 4: Read response data in chunks
     * This is critical for large files to avoid memory overflow */
    LogInfo( ( "Starting chunked download..." ) );
    
    while( totalReceived < httpResponse.contentLength )
    {
        /* Read one chunk of data */
        cellularStatus = Cellular_HttpReadData( cellularHandle, httpHandle,
                                               pReadBuffer, READ_BUFFER_SIZE,
                                               &receivedLen );
        
        if( cellularStatus != CELLULAR_SUCCESS )
        {
            LogError( ( "Failed to read HTTP data: %d", cellularStatus ) );
            break;
        }

        if( receivedLen == 0 )
        {
            /* All data has been received */
            LogInfo( ( "All data received" ) );
            break;
        }

        /* Update total received count */
        totalReceived += receivedLen;

        /* Process the received chunk
         * In a real application, you would:
         * - Write to file
         * - Process the data
         * - Send to another component
         * etc.
         */
        LogInfo( ( "Received chunk: %u bytes, Total: %u/%u (%.1f%%)",
                   receivedLen,
                   totalReceived,
                   httpResponse.contentLength,
                   ( ( float ) totalReceived / httpResponse.contentLength ) * 100.0f ) );

        /* Example: Write to file (pseudo-code)
         * fwrite( pReadBuffer, 1, receivedLen, file );
         */
    }

    /* Step 5: Verify download completed successfully */
    if( totalReceived == httpResponse.contentLength )
    {
        LogInfo( ( "Download completed successfully! Total bytes: %u", totalReceived ) );
    }
    else
    {
        LogWarn( ( "Download incomplete. Received %u of %u bytes",
                   totalReceived, httpResponse.contentLength ) );
    }

    /* Step 6: Cleanup */
    Platform_Free( pReadBuffer );
    Cellular_HttpCleanup( cellularHandle, httpHandle );
}

/* Example: HTTP POST with JSON data */
void example_http_post_json( CellularHandle_t cellularHandle )
{
    CellularError_t cellularStatus;
    CellularHttpHandle_t httpHandle = NULL;
    CellularHttpConfig_t httpConfig;
    CellularHttpRequest_t httpRequest;
    CellularHttpResponse_t httpResponse;
    const char * pJsonData = "{\"sensor\":\"temperature\",\"value\":25.5}";

    /* Configure HTTP with HTTPS */
    httpConfig.contextId = 1;
    httpConfig.sslCtxId = 1;  /* Use SSL context 1 for HTTPS */
    httpConfig.requestTimeout = 30;
    httpConfig.responseTimeout = 30;
    httpConfig.enableCustomHeader = true; /* Enable custom headers */

    cellularStatus = Cellular_HttpInit( cellularHandle, &httpConfig, &httpHandle );
    
    if( cellularStatus != CELLULAR_SUCCESS )
    {
        return;
    }

    /* Add custom headers */
    Cellular_HttpSetHeader( cellularHandle, httpHandle,
                           "Authorization", "Bearer your-token-here" );
    Cellular_HttpSetHeader( cellularHandle, httpHandle,
                           "X-API-Key", "your-api-key" );

    /* Prepare POST request */
    httpRequest.method = CELLULAR_HTTP_METHOD_POST;
    httpRequest.pUrl = "https://api.example.com/sensor-data";
    httpRequest.urlLen = strlen( httpRequest.pUrl );
    httpRequest.pData = pJsonData;
    httpRequest.dataLen = strlen( pJsonData );
    httpRequest.contentType = CELLULAR_HTTP_CONTENT_TYPE_APPLICATION_JSON;
    httpRequest.pCustomHeader = NULL;

    /* Send POST request */
    cellularStatus = Cellular_HttpRequest( cellularHandle, httpHandle,
                                          &httpRequest, &httpResponse );
    
    if( cellularStatus == CELLULAR_SUCCESS )
    {
        LogInfo( ( "POST successful! Status: %d", httpResponse.httpStatusCode ) );
    }
    else
    {
        LogError( ( "POST failed: %d", cellularStatus ) );
    }

    Cellular_HttpCleanup( cellularHandle, httpHandle );
}

/* Example: HTTP GET with query parameters */
void example_http_get_with_params( CellularHandle_t cellularHandle )
{
    CellularError_t cellularStatus;
    CellularHttpHandle_t httpHandle = NULL;
    CellularHttpConfig_t httpConfig;
    CellularHttpRequest_t httpRequest;
    CellularHttpResponse_t httpResponse;
    char urlBuffer[ 256 ];

    /* Build URL with query parameters */
    snprintf( urlBuffer, sizeof( urlBuffer ),
              "http://api.example.com/data?device_id=%s&timestamp=%lu",
              "device123", ( unsigned long ) time( NULL ) );

    httpConfig.contextId = 1;
    httpConfig.sslCtxId = 0;
    httpConfig.requestTimeout = 30;
    httpConfig.responseTimeout = 30;
    httpConfig.enableCustomHeader = false;

    cellularStatus = Cellular_HttpInit( cellularHandle, &httpConfig, &httpHandle );
    
    if( cellularStatus != CELLULAR_SUCCESS )
    {
        return;
    }

    httpRequest.method = CELLULAR_HTTP_METHOD_GET;
    httpRequest.pUrl = urlBuffer;
    httpRequest.urlLen = strlen( urlBuffer );
    httpRequest.pData = NULL;
    httpRequest.dataLen = 0;
    httpRequest.contentType = CELLULAR_HTTP_CONTENT_TYPE_APPLICATION_JSON;
    httpRequest.pCustomHeader = NULL;

    cellularStatus = Cellular_HttpRequest( cellularHandle, httpHandle,
                                          &httpRequest, &httpResponse );
    
    if( cellularStatus == CELLULAR_SUCCESS )
    {
        LogInfo( ( "GET successful! Status: %d, Length: %u",
                   httpResponse.httpStatusCode,
                   httpResponse.contentLength ) );
        
        /* Read response if needed */
        if( httpResponse.contentLength > 0 )
        {
            uint8_t buffer[ 1024 ];
            uint32_t receivedLen;
            
            cellularStatus = Cellular_HttpReadData( cellularHandle, httpHandle,
                                                   buffer, sizeof( buffer ),
                                                   &receivedLen );
            if( cellularStatus == CELLULAR_SUCCESS && receivedLen > 0 )
            {
                /* Process response data */
                LogInfo( ( "Received %u bytes of response data", receivedLen ) );
            }
        }
    }

    Cellular_HttpCleanup( cellularHandle, httpHandle );
}
