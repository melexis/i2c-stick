# Thermal image

This application uses the MLX90640, MLX90641, or MLX90642 thermal sensor to capture an array of temperature readings. Each pixel in the thermal image represents a measured temperature value, not a visual color. The color mapping is chosen to visually indicate temperature differences, with warmer colors for higher temperatures and cooler colors for lower temperatures.

The sensor communicates over I2C, and the maximum I2C clock frequency is 1 MHz. Feel free to increase the default value of 100kHz in this demo to accomodate the higher refresh rates.

<iframe width="560" height="315" src="https://www.youtube.com/embed/4r4jVoeVULo?si=wJh-io1n4CPaKi0G" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>

Supported by Firmware version 1.3 and above.


### Product page:  
- https://melexis.com/mlx90640
- https://melexis.com/mlx90641
- https://melexis.com/mlx90642
