#ifndef __I2C_STICK_HAL_H__
#define __I2C_STICK_HAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

const char *hal_get_board_info();

uint64_t hal_get_millis();
int16_t hal_i2c_slave_address_available(uint8_t sa);
void hal_i2c_set_clock_frequency(uint32_t frequency_in_hz);

int16_t hal_i2c_direct_read(uint8_t sa, uint8_t *read_buffer, uint16_t read_n_bytes);
int16_t hal_i2c_direct_write(uint8_t sa, uint8_t *write_buffer, uint16_t write_n_bytes);
int16_t hal_i2c_indirect_read(uint8_t sa, uint8_t *write_buffer, uint16_t write_n_bytes, uint8_t *read_buffer, uint16_t read_n_bytes);

void hal_i2c_set_pwm(uint8_t pin_no, uint8_t pwm);

#ifdef __cplusplus
}
#endif

#endif // __I2C_STICK_HAL_H__
