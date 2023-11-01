#include "i2c_stick_arduino.h"

#ifdef HAS_EEPROM_H
#include <EEPROM.h>
#endif

#ifdef ENABLE_USB_MSC // Mass Storage device Class
  #include <SPI.h>
  #include <SdFat.h>
  #include <Adafruit_SPIFlash.h>
  #include <Adafruit_TinyUSB.h>
#endif // ENABLE_USB_MSC // Mass Storage device Class

#include "i2c_stick.h"
#include "i2c_stick_task.h"
#include "i2c_stick_cmd.h"
#include "i2c_stick_dispatcher.h"
#include "i2c_stick_hal.h"


#include <stdlib.h>

#ifdef ENABLE_USB_MSC // Mass Storage device Class
  // Un-comment to run example with custom SPI and SS e.g with FRAM breakout
  // #define CUSTOM_CS   A5
  // #define CUSTOM_SPI  SPI

  #if defined(CUSTOM_CS) && defined(CUSTOM_SPI)
    Adafruit_FlashTransport_SPI flashTransport(CUSTOM_CS, CUSTOM_SPI);

  #elif defined(ARDUINO_ARCH_ESP32)
    // ESP32 use same flash device that store code.
    // Therefore there is no need to specify the SPI and SS
    Adafruit_FlashTransport_ESP32 flashTransport;

  #elif defined(ARDUINO_ARCH_RP2040)
    // RP2040 use same flash device that store code.
    // Therefore there is no need to specify the SPI and SS
    // Use default (no-args) constructor to be compatible with CircuitPython partition scheme
    Adafruit_FlashTransport_RP2040 flashTransport;

    // For generic usage:
    //    Adafruit_FlashTransport_RP2040 flashTransport(start_address, size)
    // If start_address and size are both 0, value that match filesystem setting in
    // 'Tools->Flash Size' menu selection will be used

  #else
    // On-board external flash (QSPI or SPI) macros should already
    // defined in your board variant if supported
    // - EXTERNAL_FLASH_USE_QSPI
    // - EXTERNAL_FLASH_USE_CS/EXTERNAL_FLASH_USE_SPI
    #if defined(EXTERNAL_FLASH_USE_QSPI)
      Adafruit_FlashTransport_QSPI flashTransport;

    #elif defined(EXTERNAL_FLASH_USE_SPI)
      Adafruit_FlashTransport_SPI flashTransport(EXTERNAL_FLASH_USE_CS, EXTERNAL_FLASH_USE_SPI);

    #else
      #error No QSPI/SPI flash are defined on your board variant.h !
    #endif
  #endif


  Adafruit_SPIFlash flash(&flashTransport);

  // file system object from SdFat
  FatFileSystem fatfs;

  FatFile root;
  FatFile file;

  // USB Mass Storage object
  Adafruit_USBD_MSC usb_msc;

  // Check if flash is formatted
  bool fs_formatted;

  // Set to true when PC write to flash
  bool fs_changed;

#endif // ENABLE_USB_MSC // Mass Storage device Class



// global variables
int8_t g_mode;
int8_t g_active_slave;
uint16_t g_config_host;



int8_t g_app_id = APP_NONE;

int8_t g_state = 0;
uint8_t g_channel_mask =
    (1U<<CHANNEL_UART)
  | (1U<<CHANNEL_WIFI)
  | (1U<<CHANNEL_ETHERNET)
  | (1U<<CHANNEL_BLUETOOTH)
#ifdef BUFFER_COMMAND_ENABLE
  | (1U<<CHANNEL_BUFFER)
#endif
  ;

#ifdef BUFFER_COMMAND_ENABLE
  uint32_t g_channel_buffer_pos;
  char g_channel_buffer[BUFFER_CHANNEL_SIZE];
#endif

void uart_welcome()
{
  Serial.println("melexis-i2c-stick booted: '?' for help");
  g_channel_mask |= (1U<<CHANNEL_UART);
  g_state = 1;
}


#ifdef HAS_CORE1
void main_core1()
{
  while(true)
  {}
}
#endif // HAS_CORE1


const char *
hal_get_board_info()
{
  return BOARD_INFO;
}

void
hal_write_pin(uint8_t pin, uint8_t state)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin, state ? 1 : 0);
}


uint8_t
hal_read_pin(uint8_t pin)
{
  pinMode(pin, INPUT);
  return digitalRead(pin);
}


uint64_t
hal_get_millis()
{
  return millis();
}


void
hal_delay(uint32_t ms)
{
  delay(ms);
}


int16_t
hal_i2c_slave_address_available(uint8_t sa)
{
  if (sa > 127) return 0; // invalid slave addresses are not available
  WIRE.endTransmission();
  delayMicroseconds(5);

  WIRE.beginTransmission(sa);
  byte result = WIRE.endTransmission();     // stop transmitting

  if (result != 2)
  {
    return 1;
  }
  return 0;
}


void
hal_i2c_set_clock_frequency(uint32_t frequency_in_hz)
{
  uint32_t frequency_in_hz_corrected = frequency_in_hz;
#ifdef ARDUINO_ARCH_RP2040
  if (frequency_in_hz ==  100000) frequency_in_hz_corrected = frequency_in_hz * 1.05;
  if (frequency_in_hz == 1000000) frequency_in_hz_corrected = frequency_in_hz * 1.25;
  if (frequency_in_hz ==  400000) frequency_in_hz_corrected = frequency_in_hz * 1.105;
#endif
  WIRE.end();
  WIRE.setClock(frequency_in_hz_corrected); // RP2040 requires first to set the clock
  WIRE.begin();
  WIRE.setClock(frequency_in_hz_corrected); // NRF52840 requires first begin, then set clock.
}


int16_t
hal_i2c_direct_read(uint8_t sa, uint8_t *read_buffer, uint16_t read_n_bytes)
{
  WIRE.endTransmission();
  delayMicroseconds(5);

  WIRE.beginTransmission((uint8_t)sa);
  WIRE.requestFrom((uint8_t)sa, uint8_t(read_n_bytes), uint8_t(true));
  for (uint16_t i=0; i<read_n_bytes; i++)
  {
    read_buffer[i] = (uint8_t)WIRE.read();
  }
  byte result = WIRE.endTransmission();     // stop transmitting
#ifdef ARDUINO_ARCH_RP2040
  if (result == 4) result = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif
  return result;
}


int16_t
hal_i2c_direct_write(uint8_t sa, uint8_t *write_buffer, uint16_t write_n_bytes)
{
  WIRE.endTransmission();
  delayMicroseconds(5);

  WIRE.beginTransmission(sa);
  for (uint16_t i=0; i<write_n_bytes; i++)
  {
    WIRE.write(write_buffer[i]);
  }
  byte result = WIRE.endTransmission();     // stop transmitting

  if (write_n_bytes == 0) // shorthand for hal_i2c_slave_address_available
  {
    if (result != 2)
    { // found SA; return OK
      return 0;
    }
    return 1; // not found return not-ok.
  }

#ifdef ARDUINO_ARCH_RP2040
  if (result == 4) result = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif
  return result;
}


int16_t
hal_i2c_indirect_read(uint8_t sa, uint8_t *write_buffer, uint16_t write_n_bytes, uint8_t *read_buffer, uint16_t read_n_bytes)
{
  WIRE.endTransmission();
  delayMicroseconds(5);

  WIRE.beginTransmission(sa);
  for (uint16_t i=0; i<write_n_bytes; i++)
  {
    WIRE.write(write_buffer[i]);
  }
  byte result = WIRE.endTransmission(false);     // repeated start
#ifdef ARDUINO_ARCH_RP2040
  if (result == 4) result = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif
  if (result != 0) return result;

  result = WIRE.requestFrom((uint8_t)sa, uint8_t(read_n_bytes), uint8_t(true));
  if (result != uint8_t(read_n_bytes))
  {
    return -1;
  }
  for (uint16_t i=0; i<read_n_bytes; i++)
  {
    read_buffer[i] = (uint8_t)WIRE.read();
  }

  result = WIRE.endTransmission();     // stop transmitting
#ifdef ARDUINO_ARCH_RP2040
  if (result == 4) result = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif
  return result;
}


void
hal_i2c_set_pwm(uint8_t pin_no, uint8_t pwm)
{
  analogWrite(pin_no, pwm);
}


void
setup()
{
#ifdef ENABLE_USB_MSC

  flash.begin();

  // Set disk vendor id, product id and revision with string up to 8, 16, 4 characters respectively
  usb_msc.setID("Melexis", "I2C STICK", "1.0");

  // Set callback
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);

  // Set disk size, block size should be 512 regardless of spi flash page size
  usb_msc.setCapacity(flash.size()/512, 512);

  // MSC is ready for read/write
  usb_msc.setUnitReady(true);

  usb_msc.begin();

  // Init file system on the flash
  fs_formatted = fatfs.begin(&flash);
#endif // ENABLE_USB_MSC


#ifdef HAS_CORE1
  multicore_launch_core1(main_core1);
  if (rp2040.fifo.available())
  {}
#endif // HAS_CORE1

  Serial.begin(1000000);
  g_channel_mask &= ~(1U<<CHANNEL_UART);

  hal_i2c_set_clock_frequency(100000);

  // default link slave address vs device driver!
  memset(g_sa_list, 0, sizeof(g_sa_list));

  i2c_stick_register_all_drivers();

  uint8_t channel_mask = 0; // run quitely...
  handle_cmd(channel_mask, "scan"); // scan at startup!
  if (Serial) uart_welcome();
}


void
loop()
{
  if (g_state == 0)
  {
    if (Serial)
    {
      uart_welcome();
    }
  } else
  {
    int_uart();
  }
  //int_ethernet();
  //int_ble();
  //int_clue_button_display();
  if (g_mode == MODE_CONTINUOUS)
  {
    handle_continuous_mode();
  }
  handle_applications();
}


#ifdef ENABLE_USB_MSC
// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and
// return number of copied bytes (must be multiple of block size)
int32_t msc_read_cb (uint32_t lba, void* buffer, uint32_t bufsize)
{
  // Note: SPIFLash Block API: readBlocks/writeBlocks/syncBlocks
  // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
  return flash.readBlocks(lba, (uint8_t*) buffer, bufsize/512) ? bufsize : -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb (uint32_t lba, uint8_t* buffer, uint32_t bufsize)
{
  // digitalWrite(LED_BUILTIN, HIGH);

  // Note: SPIFLash Block API: readBlocks/writeBlocks/syncBlocks
  // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
  return flash.writeBlocks(lba, buffer, bufsize/512) ? bufsize : -1;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void msc_flush_cb (void)
{
  // sync with flash
  flash.syncBlocks();

  // clear file system's cache to force refresh
  fatfs.cacheClear();

  fs_changed = true;

  // digitalWrite(LED_BUILTIN, LOW);
}

#endif // ENABLE_USB_MSC




void
handle_applications()
{
  uint8_t channel_mask = 0xFF;
}

uint16_t
int_uart()
{
  uint8_t channel_mask = 1U<<CHANNEL_UART;
  static char cmd[32];
  static char *p_cmd = cmd;
  if (!Serial.available()) return 1; // no new data => return
  while (Serial.available())
  {
    int8_t ch = Serial.read();

    if (p_cmd == cmd)
    { // first char might be a single char task!
      const char *p_answer = handle_task(channel_mask, ch);
      if (p_answer)
      {
        return 0;
      }
    }

    if ((ch == '\n') || (ch == '\r') || // end of cmd character.
        ((p_cmd - cmd + 1) >= (int)(sizeof(cmd)))) // cmd buffer full! lets try to handle the command; some command will read more bytes later if needed....
    {
      if (p_cmd == cmd) return 0; // empty command; likely only <LF> was sent!
      const char *p_answer = handle_cmd(channel_mask, cmd);
      if (p_answer) Serial.println(p_answer);

      memset(cmd, 0, sizeof(cmd));
      p_cmd = cmd;
      return 0;
    }
    *p_cmd = ch;
    p_cmd++;
  }
  return 0; // 0 => success!
}


void
send_answer_chunk(uint8_t channel_mask, const char *answer, uint8_t terminate)
{ // we currently have only UART/Serial,
  if (channel_mask & (1U<<CHANNEL_UART))
  {
    while (Serial.availableForWrite() < 200)
    {
      // wait!!
    }

    if (terminate)
    {
      Serial.println(answer);
      Serial.flush();
    } else
    {
      Serial.print(answer);
    }
  }
  if (channel_mask & (1U<<CHANNEL_ETHERNET))
  { // todo
  }
  if (channel_mask & (1U<<CHANNEL_WIFI))
  { // todo
  }
  if (channel_mask & (1U<<CHANNEL_BLUETOOTH))
  { // todo
  }
#ifdef BUFFER_COMMAND_ENABLE
  if (channel_mask & (1U<<CHANNEL_BUFFER))
  {
    uint32_t answer_len = strlen(answer);
    if ((g_channel_buffer_pos + answer_len) < (BUFFER_CHANNEL_SIZE - 3))
    {
      strcpy(g_channel_buffer+g_channel_buffer_pos, answer);
      g_channel_buffer_pos += answer_len;
      if (terminate)
      {
        strcpy(g_channel_buffer+g_channel_buffer_pos, "\r\n");
        g_channel_buffer_pos += 2;
      }
    }
  }
#endif // BUFFER_COMMAND_ENABLE
}


void
send_answer_chunk_binary(uint8_t channel_mask, const char *blob, uint16_t length, uint8_t terminate)
{ // we currently have only UART/Serial,
  if (channel_mask & (1U<<CHANNEL_UART))
  {
    Serial.write(blob, length);
    if (terminate)
    {
      Serial.flush();
    }
  }
  if (channel_mask & (1U<<CHANNEL_ETHERNET))
  { // todo
  }
  if (channel_mask & (1U<<CHANNEL_WIFI))
  { // todo
  }
  if (channel_mask & (1U<<CHANNEL_BLUETOOTH))
  { // todo
  }
}


void
send_broadcast_message(const char *msg)
{// currently we do only UART
  Serial.println(msg);
}


void
handle_continuous_mode()
{
  uint8_t old_active_slave = g_active_slave;
  const char *error_message = NULL;
  for (uint8_t sa=0; sa<128; sa++)
  {
    int16_t spot = g_sa_list[sa].spot_;
    if (g_sa_list[sa].found_ && (!(g_sa_drv_register[spot].disabled_)) && (g_sa_drv_register[spot].drv_ > 0))
    {
      g_active_slave = sa;
      uint8_t nd = 0; // new data
      cmd_nd(sa, &nd, &error_message);
      if (nd > 0)
      {
        char buf[32];
        memset(buf, 0, sizeof(buf));
        char *p = buf;
        *p = '@'; p++;
        uint8_to_hex(p, sa); p += 2;
        *p = ':'; p++;
        uint8_to_hex(p, g_sa_drv_register[spot].drv_); p += 2;
        *p = ':'; p++;
        // Serial.println(Serial.availableForWrite());
        send_answer_chunk(g_channel_mask, buf, 0);
        handle_cmd_mv(sa, g_channel_mask);
        if (g_sa_drv_register[spot].raw_)
        {
          send_answer_chunk(g_channel_mask, buf, 0);
          handle_cmd_raw(sa, g_channel_mask);
        }
      }
    }
  }

  g_active_slave = old_active_slave;
}
