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

What about debouncing? (gateron yellows)
* it's actually pretty bad... but debouncing is expected, i guess