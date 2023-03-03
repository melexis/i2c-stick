#ifndef __I2C_STICK_CMD_H__
#define __I2C_STICK_CMD_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// supporting functions 
char nibble_to_hex(uint8_t nibble);
void uint8_to_hex(char *hex, uint8_t dec);
void uint16_to_hex(char *hex, uint16_t dec);
void uint32_to_dec(char *str, uint32_t dec, int8_t digits);
int16_t atohex8(const char *in);
int32_t atohex16(const char *in);
const char *bytetohex(uint8_t dec);
const char *bytetostr(uint8_t dec);
char *my_dtostrf(float val,  int8_t char_num, uint8_t precision, char *chr_buffer);

// command functions.


const char *handle_cmd(uint8_t channel_mask, const char *cmd);
void handle_cmd_mv(uint8_t sa, uint8_t channel_mask);
void handle_cmd_raw(uint8_t sa, uint8_t channel_mask);
void handle_cmd_nd(uint8_t sa, uint8_t channel_mask);
void handle_cmd_sn(uint8_t sa, uint8_t channel_mask);
void handle_cmd_ch(uint8_t channel_mask, const char *input);
void handle_cmd_ch_write(uint8_t channel_mask, const char *input);
void handle_cmd_cs(uint8_t sa, uint8_t channel_mask, const char *input);
void handle_cmd_cs_write(uint8_t sa, uint8_t channel_mask, const char *input);
void handle_cmd_mr(uint8_t sa, uint8_t channel_mask, const char *input);
void handle_cmd_mw(uint8_t sa, uint8_t channel_mask, const char *input);
void handle_cmd_is(uint8_t sa, uint8_t channel_mask);
void handle_cmd_sos(uint8_t channel_mask, const char *input);

uint8_t cmd_ch(uint8_t channel_mask, const char *input);
uint8_t cmd_ch_write(uint8_t channel_mask, const char *input);

uint8_t cmd_ca(uint8_t app_id, uint8_t channel_mask, const char *input);
uint8_t cmd_ca_write(uint8_t app_id, uint8_t channel_mask, const char *input);


void i2c_stick_set_i2c_clock_frequency(uint16_t enum_freq);
uint16_t i2c_stick_get_i2c_clock_frequency();

#ifdef __cplusplus
}
#endif

#endif // __I2C_STICK_CMD_H__
