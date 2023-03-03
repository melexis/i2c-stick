.PHONY: clean all dist arduino_trinkey_qt

DIST_DIR=dist


all: dist

arduino_trinkey_qt:
	rm -f i2c-stick-arduino/i2c_stick_dispatcher.cpp
	rm -f i2c-stick-arduino/i2c_stick_dispatcher.h
	make -C i2c-stick-arduino arduino_install_boards arduino_install_libs arduino_trinkey_qt
	echo "- [adafruit Trinkey RP2040 QT UF2-file](firmware/rp2040.rp2040.adafruit_trinkeyrp2040qt/melexis-i2c-stick-arduino.ino.uf2) -- [url](https://www.adafruit.com/product/5056)" >> firmware_list.md

dist: arduino_trinkey_qt
	@-cp -fv firmware_list.md web-interface
	make -C web-interface all
	@mkdir -p ${DIST_DIR}/firmware
	@cd i2c-stick-arduino/build/ && find -type f -name '*' -printf "mkdir -p ../../${DIST_DIR}/firmware/%h && cp -v '%h/%f' '../../${DIST_DIR}/firmware/%h/melexis-%f'\n" | sh && cd ../..

clean:
	@rm -rfv ${DIST_DIR}
	@rm -fv firmware.csv
