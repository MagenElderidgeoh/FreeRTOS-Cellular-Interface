# EG800Z Module Integration Guide

## Overview

This directory contains the implementation of Quectel EG800Z LTE module support for the FreeRTOS Cellular Interface library. The module provides comprehensive support for:

- **MQTT** - Message queuing with SSL/TLS support
- **HTTP(S)** - Web requests with SSL/TLS support
- **FILE** - Module filesystem operations
- **SSL/TLS** - Secure communication configuration

## Implementation Details

### Files

1. **cellular_eg800z.h** (388 lines)
   - Public API definitions
   - Data structures for MQTT, HTTP, SSL, and FILE operations
   - Function prototypes

2. **cellular_eg800z.c** (1,132 lines)
   - Module portable interface implementation
   - AT command implementations for all features
   - Error handling and validation

3. **README.md** (321 lines)
   - API documentation
   - Usage examples
   - Integration instructions

4. **CMakeLists.txt**
   - Build system integration

### Architecture

The module follows the FreeRTOS Cellular Interface architecture:

```
Application
    │
    ├─→ FreeRTOS Cellular API (cellular_api.h)
    │       │
    │       ├─→ Common 3GPP Implementation (cellular_common_api.c)
    │       │
    │       └─→ EG800Z Module (cellular_eg800z.c)
    │               │
    │               ├─→ Module Portable Interface
    │               │   ├─→ Cellular_ModuleInit()
    │               │   ├─→ Cellular_ModuleCleanUp()
    │               │   ├─→ Cellular_ModuleEnableUE()
    │               │   └─→ Cellular_ModuleEnableUrc()
    │               │
    │               └─→ Vendor-Specific APIs
    │                   ├─→ MQTT Operations
    │                   ├─→ HTTP(S) Operations
    │                   ├─→ SSL/TLS Configuration
    │                   └─→ FILE Operations
    │
    └─→ Comm Interface (UART/SPI)
```

## AT Commands Implemented

### MQTT Commands
- `AT+QMTCFG` - Configure MQTT parameters (SSL, version, session, etc.)
- `AT+QMTOPEN` - Open network connection for MQTT client
- `AT+QMTCONN` - Connect to MQTT broker
- `AT+QMTDISC` - Disconnect from MQTT broker
- `AT+QMTCLOSE` - Close network connection
- `AT+QMTPUBEX` - Publish messages (extended mode with binary data)
- `AT+QMTSUB` - Subscribe to topics
- `AT+QMTUNS` - Unsubscribe from topics

### HTTP Commands
- `AT+QHTTPCFG` - Configure HTTP parameters (context, SSL, headers, timeout)
- `AT+QHTTPURL` - Set URL for HTTP request
- `AT+QHTTPGET` - Send HTTP GET request
- `AT+QHTTPPOST` - Send HTTP POST request
- `AT+QHTTPREAD` - Read HTTP response data

### SSL Commands
- `AT+QSSLCFG` - Configure SSL context
  - SSL version (SSL3.0, TLS1.0/1.1/1.2/1.3)
  - CA certificate path
  - Client certificate path
  - Client private key path
  - Security level
  - Cipher suite

### FILE Commands
- `AT+QFUPL` - Upload file to module filesystem
- `AT+QFDWL` - Download file from module filesystem
- `AT+QFDEL` - Delete file from module filesystem
- `AT+QFLST` - List files in module filesystem

## Integration Steps

### 1. Enable EG800Z Module in Build

Add to your CMake configuration:
```cmake
set( CELLULAR_MODULE_EG800Z ON )
include( path/to/cellularInterfaceFilePaths.cmake )
```

Or manually add the source files:
```cmake
add_library( cellular_eg800z
    modules/eg800z/cellular_eg800z.c
    # ... common cellular sources ...
)

target_include_directories( cellular_eg800z PUBLIC
    modules/eg800z
    source/include
    source/include/common
    source/include/private
)
```

### 2. Implement Platform Layer

Provide implementations for:
- `cellular_platform.h` - Platform-specific definitions
- `Platform_Malloc()` / `Platform_Free()` - Memory allocation
- Mutex and queue operations for your RTOS
- Comm interface for UART/SPI communication

### 3. Initialize Cellular Context

```c
#include "cellular_api.h"
#include "cellular_eg800z.h"

CellularHandle_t cellularHandle;
CellularCommInterface_t commInterface = {
    /* Your comm interface implementation */
};

/* Initialize cellular library */
CellularError_t status = Cellular_Init( &cellularHandle, &commInterface );
```

### 4. Use Module-Specific Features

See README.md for detailed examples of:
- MQTT publish/subscribe
- HTTP GET/POST requests
- SSL/TLS configuration
- File upload/download

## Testing Recommendations

1. **Basic Module Tests**
   - Initialize module and verify no errors
   - Send AT commands and verify responses
   - Test module enable/disable

2. **MQTT Tests**
   - Configure SSL context
   - Connect to MQTT broker
   - Publish test messages
   - Subscribe to topics
   - Verify message delivery

3. **HTTP Tests**
   - Configure HTTP with/without SSL
   - Perform GET requests
   - Perform POST requests
   - Verify response data

4. **FILE Tests**
   - Upload test files
   - List files
   - Download files
   - Verify data integrity
   - Delete files

5. **SSL Tests**
   - Configure SSL with different versions
   - Test certificate paths
   - Verify secure connections

## Reference Documentation

Implementation based on official Quectel documentation:

1. `Quectel_EC600Z&EC800Z&EG800Z系列_MQTT_应用指导_V1.4.0_Preliminary_20250925.pdf`
2. `Quectel_EC600Z&EC800Z&EG800Z系列_HTTP(S)_应用指导_V1.4.pdf`
3. `Quectel_EC600Z&EC800Z&EG800Z系列_FILE_应用指导_V1.2.pdf`
4. `Quectel_EC600Z&EC800Z&EG800Z系列_SSL_应用指导_V1.4.0_Preliminary_20250930.pdf`
5. `Quectel_LTE_Standard(A)系列_AT命令手册_V1.3.pdf`

## Limitations and Considerations

1. **Module Context**: The module stores MQTT client identifiers (up to 6 clients). Ensure proper initialization before use.

2. **Buffer Sizes**: 
   - AT commands limited to CELLULAR_AT_CMD_MAX_SIZE (200 bytes by default)
   - Adjust buffer sizes in cellular_config.h if needed

3. **Blocking Operations**: AT commands are synchronous and will block until response received or timeout

4. **Error Handling**: All functions return CellularError_t. Always check return values.

5. **Thread Safety**: The cellular library uses mutexes for thread safety. Ensure your platform layer implements them correctly.

6. **SSL Certificates**: Must be uploaded to module filesystem before configuring SSL contexts.

## Future Enhancements

Potential areas for extension:

1. **URC Handlers**: Implement unsolicited result code handlers for:
   - MQTT connection status changes
   - HTTP request completion
   - File operation completion

2. **Async Operations**: Add support for asynchronous operation completion callbacks

3. **Additional Features**: Implement other EG800Z features as needed:
   - FTP client
   - GNSS/GPS
   - Audio
   - Network scanning

4. **Enhanced Error Reporting**: Parse vendor-specific error codes

## Support

For issues or questions:
1. Check the README.md for usage examples
2. Review the Quectel documentation PDFs
3. Consult FreeRTOS Cellular Interface documentation
4. Open an issue on the repository

## License

Copyright (C) 2024 Amazon.com, Inc. or its affiliates.  All Rights Reserved.

SPDX-License-Identifier: MIT
