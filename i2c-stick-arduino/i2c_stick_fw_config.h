#ifndef __I2C_STICK_FW_CONFIG_H__
#define __I2C_STICK_FW_CONFIG_H__

// FW Configuration
// ****************

#define FW_VERSION "V1.3.11"


// enable/disable modules
// ======================
//#define BUFFER_COMMAND_ENABLE
#undef BUFFER_COMMAND_ENABLE


// memory sizing
// =============

#define BUFFER_CHANNEL_SIZE (2*1024)
#define MAX_SA_DRV_REGISTRATIONS 128

#endif // __I2C_STICK_FW_CONFIG_H__
