#ifndef __I2C_STICK_FW_CONFIG_H__
#define __I2C_STICK_FW_CONFIG_H__

// FW Configuration
// ****************

#define FW_VERSION "V1.3.0"


// enable/disable modules
// ======================
#define DEVICE_PT100_ADS122C_ENABLE
//#undef DEVICE_PT100_ADS122C_ENABLE

//#define DEVICE_MLX90640_TEST_ENABLE
//#undef DEVICE_MLX90640_TEST_ENABLE

//#define BUFFER_COMMAND_ENABLE
#undef BUFFER_COMMAND_ENABLE


// memory sizing
// =============

#define BUFFER_CHANNEL_SIZE (2*1024)
#define MAX_SA_DRV_REGISTRATIONS 128

#endif // __I2C_STICK_FW_CONFIG_H__
