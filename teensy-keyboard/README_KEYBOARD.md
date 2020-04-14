Using Teensy's Keyboard library:
* https://www.pjrc.com/teensy/td_keyboard.html
* See `usb_keyboard.h/c` for the keyboard-related function global variables
  * uint8_t modifiers,  set_modifier()
  * uint8_t keys[6],  set_key[1-6]()
  * send_now()  queues up a packet to be transmitted to the usb host
* See `usb_desc.h/c` for the keyboard report descriptor (boot protocol?)

It appears that the report descriptor for keyboard-boot-protocol only allows reporting up to 6 keys (not modifiers) pressed at once. There are four modifier keys (Ctrl, Shift, Alt, GUI) and they have left and right versions, so 8 bits packed into a byte.
Hmm
* https://deskthority.net/wiki/Rollover,_blocking_and_ghosting#Interface-limited_NKRO
* https://www.devever.net/~hl/usbnkro
  * boot-protocol ignores the report descriptor, and just looks for data at fixed positions?
    * the keyboard report descriptor could start with the boot protocol, but then include more keys...?

What happens when you hold down 6 keys and press a new key? Does it "make room" in the length-6 array for the new key? Or "un-press" a key that was previously pressed?
* On my Logitech K360 keyboard, after holding down 6 keys, the 7th key is ignored.
  * Pressing modifiers after pressing 6 keys has no effect.
  * Pressing modifiers before pressing 6 keys will keep the modifiers.
    * (weird...)
* On my WASD CODE keyboard, pressing a 7th key will shift out the oldest key that was held down.
  * circular buffer?
    * not really...
  * i think i managed to implement it...
    * keep track of the state of each key (state[101+1])
    * keep an array of length 6 (with list operations)
    * on a rising edge, append the key to the list. If there are already 6 elements, pop A[0] and then insert.
    * on a falling edge, pop the key from the list, if it exists (it could have already been "shifted" out)
    * O(n) list operations, but it's only N=6...

### N-key rollover
I want to try N-key rollover by modifying the keyboard report descriptor, and the send_now function...
* my idea is to add bitmasks for all keys (as with the modifiers) after the initial 8 bytes
  * 101/8 = 12.625 => 13 bits
  * 8+13=21 round up to 32
* `usbdesc.h`
  * change KEYBOARD_SIZE from 8 to 32?
* `usbdesc.c`
  * change keyboard_report_desc[]
    ```
    // Appended bits
    // packet size is 32 bytes, so 24 extra bytes
    // 24*8 = 192 = 101 + 91 => need 91 bits of padding
    0x05, 0x07,                     //   Usage Page (Key Codes),
    0x19, 0x00,                     //   Usage Minimum (0),
    0x29, 0x65,                     //   Usage Maximum (101),
    0x15, 0x00,                     //   Logical Minimum (0),
    0x25, 0x01,                     //   Logical Maximum (1),
    0x75, 0x01,                     //   Report Size (1),
    0x95, 0x65,                     //   Report Count (101),
    0x81, 0x02,                     //   Input (Data, Variable, Absolute), ;bits for all non-modifier keys

    0x75, 0x01,                     //   Report Size (1),
    0x95, 0x5B,                     //   Report Count (91),
    0x81, 0x03,                     //   Input (Constant),          ;padding
    ```
  * change config_descriptor[]
    * do we need to change this?
      * ~~change bInterfaceSubclass from (0x01 = Boot)  to (0x00 = No Subclass)~~
      * no, it works with 0x01 Boot 
* `usb_keyboard.h`
  * add `extern uint8_t key_state[102]`
* `usb_keyboard.c`
  * change usb_keyboard_send() to include the appended bytes
    ```
    *(tx_packet->buf) = keyboard_modifier_keys;
    *(tx_packet->buf + 1) = 0;
    memcpy(tx_packet->buf + 2, keyboard_keys, 6);
    // tx_packet->len = 8;

    // added bits
    memset(tx_packet->buf + 8, 0, 24);
    for(int i=0; i < 102; i++) {
      if (key_state[i]) {
        *(tx_packet->buf + 8 + i/8) |= 1 << (i & 7);
      }
    }
    tx_packet->len = 32;
    ```

### Debouncing
What about debouncing? (gateron yellows)
* it's actually pretty bad... but debouncing is expected, i guess
