There is already an arduino and teensy keyboard library, but I want to learn and build my own...
* https://www.arduino.cc/reference/en/language/functions/usb/keyboard/
* https://www.pjrc.com/teensy/td_keyboard.html

Datasheet for the Teensy-LC microcontroller MKL26Z64VFT4
* https://www.pjrc.com/teensy/datasheets.html

Teensy-LC schematic to see the wiring of the pins
* https://www.pjrc.com/teensy/schematic.html

I took notes in README_{GPIO, INTERRUPT, KEYBOARD, KEYBOARD_CODES}.

Current issues:
* How does Teensy know when the computer went to sleep?
* How to get the Teensy keyboard to wake up the computer?
  * https://forum.arduino.cc/index.php?topic=150157.0