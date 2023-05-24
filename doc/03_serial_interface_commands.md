# Melexis I2C Stick -- Commands.

## Commands

`LF`: Line Feed, `\n`, ascii character 10 decimal.

`sa`: Slave address

Note: A command cannot start with the same character as any task!

### `mlx` - Melexis Command

Check communication with MCU and return "Melexis Inspired Engineering".

Send: `mlx` + `LF`

Receive:
 - `mlx:MELEXIS I2C STICK`             + `LF`
 - `mlx:=================`             + `LF`
 - `mlx:`                              + `LF`
 - `mlx:Melexis Inspired Engineering`  + `LF`
 - `mlx:`                              + `LF`
 - `mlx:hit '?' for help`              + `LF`

### `fv` - Firmware Version Command

Get the firmware of the demonstrator. (we do not have a firmware
revision of the drivers!)

Send: `fv` + `LF`

Receive: `fv:<firmware version>` + `LF`

Receive example: `fv:V1.1.0\n`

-   The firmware version is `V1.1.0`.

Note: we use the Semantic Versioning (https://semver.org/) system.

### `bi` - Board Information Command

Send: `bi` + `LF`

Receive: `bi:<board-type>` + `LF`

Receive example: `bi:Adafruit|Trinkey RP2040 QT` + `LF`

The goal is only informative, it will return a text describing the board.

### `i2c` - I2C Command

Universal I2C master commands.  
The goal is to read and write anything from/to the slave from the master of the interface.
Here 3 different formats possible:

1. Send: READ: `i2c:` + `sa` + `:R` + `<#bytes to read>` + `LF`
1. Send: WRITE: `i2c:` + `sa` + `:W` + `Byte#0 ...` + `LF`
1. Send: Addressed READ: `i2c:` + `sa` + `:W` + `Byte#0 ...` + `R` + `<# bytes to read>` + `LF`

Notes:
- `<# bytes to read>` is in decimal format and most arduino MCU's can read max 32 bytes at a time.
- `Byte#0 ...` are coded in hexadecimal format without any character between the bytes.

Examples of receives:
- Receive `i2c:` + `sa` + `:R:` + `data[0]data[1]..` + `:OK`
- Receive `i2c:` + `sa` + `:R:FAIL:<error code>`

For `<error code>` see:
<https://www.arduino.cc/en/Reference/WireEndTransmission>

- 1:data too long to fit in transmit buffer
- 2:received NACK on transmit of address
- 3:received NACK on transmit of data
- 4:other error


#### READ:

Send: READ: `i2c:` + `sa` + `:R:` + `<#bytes to read>:<STATUS>` + `LF`

**Read example: `i2c:3A:R1` + `LF`**

Receive example: `i2c:3A:R:FF:OK` + `LF`

Meaning:  
Send: read from slave address 3A(hex, 7 bit), 1 byte.  
Receive: First byte is FF hex.

**Read example: `i2c:3A:R` + `LF`**

Receive example: `i2c:3A:R::OK` + `LF`

Meaning: read from slave address 3A(hex, 7 bit), 0 bytes, and ACK received from slave

Receive example: `i2c:3A:R::FAIL:02` + `LF`

Meaning: read from slave address 3A(hex, 7 bit), 0 bytes, and **no** ACK received from slave (slave not responding)

#### WRITE:

Send: WRITE: `i2c:` + `sa` + `:W` + `Byte#0 ...` + `LF`

**Write example: `i2c:3A:W30045A69` + `LF`**

Receive example: `i2c:3A:W30045A69:OK` + `LF`

Meaning: 4 bytes have been written; all ACK are successful.

#### Addressed Read:

Send: Addressed READ: `i2c:` + `sa` + `:W` + `Byte#0 ...` + `R` + `<# bytes to read>` + `LF`

**Addressed read example: `i2c:3A:W3004R2` + `LF`**

Receive example: `i2c:3A:W3004:R:5A69:OK` + `LF`

Meaning: First is 3004hex written to slave address 3A(7-bit), then a repeated start followed by the slave address to read 2 bytes.

Meaning: read at register address 3004h from slave address 3A(hex, 7 bit), 2 bytes.

The first byte is 0x5A, the 2nd is 0x69.

### `ch` - get Configure Host command

Get the configuration of the host:  
- the format of the communication
- the I2C frequency
- which drivers there are provided by the firmware
- The slave address assosiations with the drivers

Send: `ch` + `LF`

Receive: `ch:<configuration>=<value>[(<description>)]` + `LF`

Receive example:
```
ch:FORMAT=0(DEC)
ch:I2C_FREQ=0(100kHz)
ch:SA_DRV=5A,01,MLX90614
ch:SA_DRV=3E,01,MLX90614
ch:SA_DRV=33,02,MLX90640
ch:SA_DRV=33,03,MLX90641
ch:SA_DRV=3A,04,MLX90632
ch:DRV=01,MLX90614
ch:DRV=02,MLX90640
ch:DRV=03,MLX90641
ch:DRV=04,MLX90632
```

### `+ch` - set Configure Host command

Set the configuration of the host:  
- the format of the communication
- the I2C frequency
- which drivers there are provided by the firmware
- The slave address assosiations with the drivers

Send: `+ch:<configuration>=<value>|<description>` + `LF`

Receive: `+ch:<status> [<where the configuration is stored>]` + `LF`

Send example:  
```
+ch:FORMAT=DEC
```

Receive example:  
```
+ch:OK [host-register]
```

### `scan` - Scan I2C bus command

Scan the I2C bus and look at which slave address returns an
acknowledgement.

It also will display the driver info associated with the slave address.

Send: `scan` + `LF`

Receive: `scan:` + `sa` + `:<driver-id>,<raw>,<disabled>,<driver-name>` + `LF`

Receive example:
```
scan:5A:01,00,00,MLX90614\n
scan:3A:01,00,00,MLX90632\n
```

Meaning:
2 devices found:
    -   address 5A MLX90614; not in raw data mode and not disabled for continious mode
    -   address 3A MLX90632; not in raw data mode and not disabled for continious mode

### `ls` - List Slave command

This command returns the same as the `scan` command, however it does not actively do something on the I2C bus.

Send: `ls` + `LF`

Receive: `ls:` + `sa` + `:<driver-id>,<raw>,<disabled>,<driver-name>` + `LF`

Receive example:
```
ls:5A:01,00,00,MLX90614\n
ls:3A:01,00,00,MLX90632\n
```

Meaning:
2 devices found during previous `scan` command:
    -   address 5A MLX90614; not in raw data mode and not disabled for continious mode
    -   address 3A MLX90632; not in raw data mode and not disabled for continious mode

### `dis` - DIsable Slave Command

Disable slave for emitting data in continous mode

Send: `dis:<sa>:<disable-flag>` + `LF`

Receive: `dis:` + `sa` + `:<disable-flag>:<result>` + `LF`

Example to disable:
 - Sent: `dis:3A`
 - Receive: `dis:3A:01:OK`

Example to enable:
 - Sent: `dis:3A:0`
 - Receive: `dis:3A:01:OK`


### `as` - Active Sensor Command

Shows the active sensor (active slave address).

Send: `as` + `LF`

Receive: `as:` + `sa`+ `:<driver-id>,<driver-name>` + `LF`

Receive example: `as:3A:5,MLX90632` + `LF`

Meaning: The current active sensor is at slave address 3A, using the MLX90632 driver.


### `mv` - Measure Values Command

Wait until new measurement data is available and report its measured values.


Format: `mv` + `LF`  (for active slave)
or
Format: `mv:` + `sa` + `LF`

Receive: `mv:` + `sa` + `:<timestamp>:<value1>,<value2>` + `LF`

Receive example: `mv:3A:06930451:22.25,22.88` + `LF`

Meaning: The sensor at slave address 3A reports 2 measurement values of 22.25 and 22.88 at 6930.451 seconds after bootup.  
For the unit and value-name see `cs` command.


### `sn` - Serial Number Command

Get the serial number of the active sensor, also referred as 'chipid' in Melexis.

A serial number is a unique number for each part scoped within a product.

Format: `sn` + `LF`  (for active slave)
or
Format: `sn:` + `sa` + `LF`

Receive: `sn:` + `sa` + `:<sn-3>-<sn-2>-<sn-1>-<sn-0>` + `LF`

Receive example: `sn:3A:2127-0447-0987-798E` + `LF`

Meaning: The serial number of the sensor at slave address 3A is "2127 0447 0987 798E".

Receive example: `sn:33:0A1E-B6F3-0189` + `LF`

Meaning: The serial number of the sensor at slave address 33 is "0A1E B6F3 0189".

### `cs` - Config Slave Command

Get configuration of a slave (address).

List temperature unit or which driver is connected to a slave address.

Send: `cs:` + `sa` + `LF`

Receive:
`cs:` + `sa` + `:<key>=<value>` + `LF`

Example:
```
cs:33:DRV=03
cs:33:RAW=00
cs:33:SA=33
cs:33:EM=0.950
cs:33:TR=25.0
cs:33:RR=03
cs:33:RES=02
cs:33:MODE=01
cs:33:FLAGS=8F(CORRECT_BROKEN_PIXELS,CORRECT_OUTLIER_PIXELS,DEINTERLACE_FILTER,IIR_FILTER)
cs:33:RO:MV_HEADER=TA,TO_[768]
cs:33:RO:MV_UNIT=DegC,DegC[768]
cs:33:RO:MV_RES=32,32[768]
```

**Writing the configuration of a sensor.**

Format: `+cs:` + `sa` + `:<key>=<new-value>` + `LF`

Sent: `+cs:3A:SA=3F` + `LF`
Receive: `+cs:3A:SA=OK; [mlx-EE] new SA[3F] use 'scan' to discover it.` + `LF`  
Note: this message is sensor specific.

Therefore make sure that `scan` yields the proper driver.  
If not one can change the driver linked to a slave address.

Format: `+cs:` + `sa` + `:DRV=<driver-name>` + `LF`  
or
Format: `+cs:` + `sa` + `:DRV=<driver-id>` + `LF`  

Sent: `+cs:3A:DRV=mlx90632`  
Receive: `+cs:3A:OK: DRV=5` + `LF`

Note: Each sensor has it's own specific properties. See the product doc for configuring those.

### `nd` -- New Data Command

Poll the selected sensor for new temperature data available and tell if available or not.

Format: `nd` + `LF`  
or
Format: `nd:` + `sa` + `LF`

Receive: `nd:` + `sa` + `:<new-data>` + `LF`

Receive example: `nd:3A:0` + `LF`

Meaning: The sensor at slave address 3A has NO new data.

Receive example: `nd:3A:1` + `LF`

Meaning: The sensor at slave address 3A has new data.


### `mr` -- Memory Read Command

Read the memory content of the selected sensor.

Format: `mr::<address>` + `LF`  
or  
Format: `mr:` + `sa` + `:<address>` + `LF`  
or  
Format: `mr:` + `sa` + `:<address>,<address_count>` + `LF`  

Note: `<address>` and `<address_count>` are in hex format with 4 characters (always)

Response: `mr:3A:<address>,<bits per address>,<address increment>,<# of addresses>,DATA,<data#0>,<data#1>, ...` + `LF`  
All values are in hex format.

Example:
sent: `mr:3A:2400,0002` + `LF`
receive: `mr:3A:2400,10,01,0003,DATA,EF4A,3480` + `LF`

### `mw` -- Memory Write Command

Write the memory content of the selected sensor.

Format: `mw::<address>,<data>` + `LF`  
or  
Format: `mr:` + `sa` + `:<address>,<data>` + `LF`  
or  
Format: `mr:` + `sa` + `:<address>,<data>,<data+1>,..` + `LF`  

Note: `<address>` and `<data>` are in hex format with 4 characters (always)

Response: `mr:3A:OK` + `LF`  
All values are in hex format (4 nibbles).

Example:
sent: `mw:3A:3002,1234` + `LF`
receive: `mw:3A:OK` + `LF`

### `raw` -- RAW measurement data dump Command

Dump the raw data of the selected sensor.

Format: `raw` + `LF`  
or
Format: `raw:` + `sa` + `LF`


Response: `raw:3A:<data#0>,<data#1>,...` + `LF`  
All values are in hex format (4 nibbels).

Example response: `raw:3A:FFDB,FFDB,55F4,FFDE,FFDE,5CA3` + `LF`

### `help` -- HELP Command

Show help about the different tasks and commands.

```
Usage: Melexis I2C STICK
========================
Tasks: single character tasks; no new line required
- ;  ==>  enter continuous mode
- !  ==>  quit continuous mode
- >  ==>  next active slave
- <  ==>  previous active slave
- ?  ==>  this help
- 1  ==>  this help
- 5  ==>  scan command alias

Commands: 2+ character commands with new line required
- mlx  ==>  test uplink communication
- help ==>  this help!
- sos  ==>  more detailed help!
- fv   ==>  Firmware Version
- bi   ==>  Board Info
- scan ==>  SCAN I2C bus for slaves
- ls   ==>  List Slaves (already discovered with 'scan')
- dis  ==>  DIsable Slave (for continuous dump mode)
- i2c  ==>  low level I2C
- ch   ==>  Configuration of Host (I2C freq, output format)

- as   ==>  Active Slave
- mv   ==>  Measure Value of the sensor
- sn   ==>  Serial Number of slave
- cs   ==>  Configuration of Slave
- nd   ==>  New Data available
- ee   ==>  EEprom dump
- raw  ==>  RAW sensor data dump

- app  ==>  APPlication id
- ca   ==>  Configuration of Application

more at https://github.com/melexis/i2c-stick
```

### `sos` -- SOS help Command

Show detailed help on a specific command.

Currently there is only SOS information for the commands `i2c`, `dis` and `ch`.

Example:
```
 <- sos:i2c
 -> I2C command format:
 -> 1] WRITE         : 'i2c::W...' bytes in hex format
 -> 2] READ          : 'i2c::R' amount in decimal format
 -> 3] ADDRESSED READ: 'i2c::WR>...' bytes in hex format, amount in decimal
 ->  Slave Address in hex format (7-bit only)
```
