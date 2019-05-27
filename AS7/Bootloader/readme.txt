'breadboard' folder should be placed in:
C:\Program Files (x86)\Arduino\hardware

using AVRdude 6.3

To burn bootloader:
- connect USBasp to ATmega
- run 'NormalFuses'
- run 'BurnBootloader'

To upload program using bootloader:
- turn off supply voltage of ATmega
- draw PD7 to ground
- turn on supply voltage of ATmega
- run 'ArduinoBootloader'
- shortly after that set PD7 high



useful links:
- ATmega on breadboard: https://forum.mysensors.org/topic/3018/tutorial-how-to-burn-1mhz-8mhz-bootloader-using-arduino-ide-1-6-5-r5
- Verbose output on upload: https://forum.arduino.cc/index.php?topic=417659.0
