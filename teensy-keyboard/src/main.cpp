// Undefine it
#ifdef USB_SERIAL
#undef USB_SERIAL
#endif

// Don't use it because I want to write my own
// #define USBkeyboard_h_
// #define KEYBOARD_INTERFACE
#define USB_KEYBOARDONLY
// #define KEYBOARD_INTERFACE 0


#include <stdint.h>
#include <stddef.h>


// #include <usb_keyboard.h>
#include <Arduino.h>

// usb_keyboard_class Keyboard;


int main()
{
    // Serial.begin(115200);
    Serial2.begin(38400); // RX 9, TX 10

    Keyboard.print('a');

    while (1)
    {
        // Serial.println("hello com4");
        Serial2.println("hello from Teensy2");
        delay(1000);
    }
}