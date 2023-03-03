.PHONY: clean all source arduino_trinkey_qt i2c_stick_dispatcher.cpp i2c_stick_dispatcher.h

ARDUINO_ADD_URL= \
  --additional-urls https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

all: source arduino_install_boards arduino_install_libs arduino_trinkey_qt

arduino_install_boards:
	arduino-cli ${ARDUINO_ADD_URL} core update-index
	arduino-cli ${ARDUINO_ADD_URL} core install rp2040:rp2040

arduino_install_libs:

i2c_stick_dispatcher.cpp: i2c_stick_dispatcher.cpp.jinja2 context.yaml
	python mailmerge.py -t $<

i2c_stick_dispatcher.h: i2c_stick_dispatcher.h.jinja2 context.yaml
	python mailmerge.py -t $<

source: i2c_stick_dispatcher.cpp i2c_stick_dispatcher.h

arduino_trinkey_qt: source
	arduino-cli compile --fqbn rp2040:rp2040:adafruit_trinkeyrp2040qt i2c-stick-arduino.ino -e --clean
