.PHONY: install build upload clean help superclean monitor

help:
	@echo "Please use 'make <target>' where <target> is one of"
	@echo "  install: 	Install Platformio via pip"
	@echo "  build: 	Compile arduino code!"
	@echo "  upload: 	Upload to esp8266"
	@echo "  monitor: Serial monitor [Default: port->ttyUSB0, baud->115200]"
	@echo "  clean: 	Do some cleaning up"
	@echo "  superclean: 	Delete all generated files including lib_deps"

install:
	@pip install --user -U platformio
	@platformio update
	@platformio lib -g install 1089 1092 1826 305 549 5509 551 64 89 798

build:
	@platformio run

upload:
	@platformio run --target upload

monitor:
	@platformio device monitor --port /dev/ttyUSB0 --baud 115200

clean:
	@platformio run -t clean

superclean: clean
	@rm -rf .pioenvs .piolibdeps
