# Melexis I2C Stick -- Serial Interface: Tasks.

## Tasks

Tasks are a single character requests, no need for `LF`  
This is handy when working in a serial terminal.

### `;` - Go Continuous Task

Enable to continuous mode.

The firmware will continuously poll each sensor (which has an associated
driver) and display TA & TO as the data becomes available.

Send: `;`

Receive: `\\n;:continuous mode` + `LF`

### `!` - Stop Task

Stop the continuous mode, if it was started earlier.

Send: `!`

Receive: `\\n:interactive mode` + `LF`

Note: this is a single character command, no need for `LF`

### `>` -- Next active sensor Task

Select the next sensor on the bus as the active sensor.

Send: `>`

Receive: `\n>:5A:1,MLX90614` + `LF`

### `<` -- Previous active sensor Command

Select the previous sensor on the bus as the active sensor.

Send: `<`

Receive: `\n<:5A,1,MLX90614` + `LF`

### `?` -- help Task

Display help info about the tasks.

Send: `?`

Receive: `....` + `LF`

### `1` -- help Task

Alias for `?`-task.

### `5` -- refresh scan Task

Alias for `scan`-command. (see next chapter)
