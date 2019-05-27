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