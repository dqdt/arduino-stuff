What if you want your keyboard to register as two keyboards, such that some keys are sent through Keyboard #1 and others through Keyboard #2?

This requires further modifying the Teensy library files... how do you do this? More build flags?

### Modify
* `platform.ini`
  * `build_flags = -DUSB_SERIAL_HID -DKEYBOARD_TWO_INTERFACES`
  * 

* `usbdesc.h`
  * Inside `#elif defined(USB_SERIAL_HID)`
    ```
    #if defined(KEYBOARD_TWO_INTERFACES)  // One more interface and one more endpoint
      #define NUM_ENDPOINTS		8
    #else
      #define NUM_ENDPOINTS		7
    #endif 

    #if defined(KEYBOARD_TWO_INTERFACES)
      #define NUM_INTERFACE		7
    #else
      #define NUM_INTERFACE		6
    #endif 

    ...

    #if defined(KEYBOARD_TWO_INTERFACES)
      #define KEYBOARD1_INTERFACE 6       // Second keyboard
      #define KEYBOARD1_ENDPOINT  8
      #define KEYBOARD1_SIZE      32
      #define KEYBOARD1_INTERVAL  1
      #define ENDPOINT8_CONFIG	  ENDPOINT_TRANSMIT_ONLY
    #endif
    ```
* `usbdesc.c`
  * Under USB Descriptor Sizes, insert the new keyboard interface size defines right under KEYMEDIA, and then make AUDIO_INTERFACE use KEYBOARD1 sizes
    ```
    #define KEYMEDIA_INTERFACE_DESC_POS	MTP_INTERFACE_DESC_POS+MTP_INTERFACE_DESC_SIZE
    #ifdef  KEYMEDIA_INTERFACE
    #define KEYMEDIA_INTERFACE_DESC_SIZE	9+9+7
    #define KEYMEDIA_HID_DESC_OFFSET	KEYMEDIA_INTERFACE_DESC_POS+9
    #else
    #define KEYMEDIA_INTERFACE_DESC_SIZE	0
    #endif

    #define KEYBOARD1_INTERFACE_DESC_POS	KEYMEDIA_INTERFACE_DESC_POS+KEYMEDIA_INTERFACE_DESC_SIZE
    #ifdef  KEYBOARD1_INTERFACE
    #define KEYBOARD1_INTERFACE_DESC_SIZE	9+9+7
    #define KEYBOARD1_HID_DESC_OFFSET	KEYBOARD1_INTERFACE_DESC_POS+9
    #else
    #define KEYBOARD1_INTERFACE_DESC_SIZE	0
    #endif

    #define AUDIO_INTERFACE_DESC_POS	KEYBOARD1_INTERFACE_DESC_POS+KEYBOARD1_INTERFACE_DESC_SIZE
    #ifdef  AUDIO_INTERFACE
    #define AUDIO_INTERFACE_DESC_SIZE	8 + 9+10+12+9+12+10+9 + 9+9+7+11+9+7 + 9+9+7+11+9+7+9
    #else
    #define AUDIO_INTERFACE_DESC_SIZE	0
    #endif
    ```

  * Inside `static uint8_t config_descriptor[CONFIG_DESC_SIZE] = {`, add the KEYBOARD1 (same format as KEYBOARD, but change variables) under KEYMEDIA 
    ```
    #ifdef KEYMEDIA_INTERFACE
    ...

    #ifdef KEYBOARD1_INTERFACE
          // interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
          9,                                      // bLength
          4,                                      // bDescriptorType
          KEYBOARD1_INTERFACE,                     // bInterfaceNumber
          0,                                      // bAlternateSetting
          1,                                      // bNumEndpoints
          0x03,                                   // bInterfaceClass (0x03 = HID)
          0x01,                                   // bInterfaceSubClass (0x01 = Boot)
          // 0x00, // bInterfaceSubClass (0 = No Subclass)
          0x01,                                   // bInterfaceProtocol (0x01 = Keyboard)
          0,                                      // iInterface
          // HID interface descriptor, HID 1.11 spec, section 6.2.1
          9,                                      // bLength
          0x21,                                   // bDescriptorType
          0x11, 0x01,                             // bcdHID
          0,                                      // bCountryCode
          1,                                      // bNumDescriptors
          0x22,                                   // bDescriptorType
          LSB(sizeof(keyboard_report_desc)),      // wDescriptorLength
          MSB(sizeof(keyboard_report_desc)),
          // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
          7,                                      // bLength
          5,                                      // bDescriptorType
          KEYBOARD1_ENDPOINT | 0x80,               // bEndpointAddress
          0x03,                                   // bmAttributes (0x03=intr)
          KEYBOARD1_SIZE, 0,                       // wMaxPacketSize
          KEYBOARD1_INTERVAL,                      // bInterval
    #endif // KEYBOARD_INTERFACE
    ```
  * Inside `const usb_descriptor_list_t usb_descriptor_list[] = {`, add right under KEYMEDIA again
  ```
  #ifdef KEYMEDIA_INTERFACE
          {0x2200, KEYMEDIA_INTERFACE, keymedia_report_desc, sizeof(keymedia_report_desc)},
          {0x2100, KEYMEDIA_INTERFACE, config_descriptor+KEYMEDIA_HID_DESC_OFFSET, 9},
  #endif

  #if defined(KEYBOARD_TWO_INTERFACES)
          {0x2200, KEYBOARD1_INTERFACE, keyboard_report_desc, sizeof(keyboard_report_desc)},
          {0x2100, KEYBOARD1_INTERFACE, config_descriptor+KEYBOARD1_HID_DESC_OFFSET, 9},
  #endif
  ```
  
* Now windows device manager detects two keyboards!

* I believe the only thing left to change the way keys are sent. Some should be sent through the KEYBOARD_ENDPOINT while others through KEYBOARD1_ENDPOINT.

* `usb_keyboard.h`
  * Add a new function
    ```
    // C language implementation
    #if defined(KEYBOARD_TWO_INTERFACES)
      int usb_keyboard_send1(uint8_t keyboard_id, uint8_t mods, uint8_t keys[6], uint8_t other_keys[128]);  // id is 0 or 1
    #endif

    #if defined(KEYBOARD_TWO_INTERFACES) 
      extern uint8_t key_state[2][128];
    #else 
      extern uint8_t key_state[102];  // added
    #endif


    // `usb_keyboard_class` public function
    #if defined(KEYBOARD_TWO_INTERFACES)
    void send_now1(uint8_t keyboard_id, uint8_t mods, uint8_t keys[6], uint8_t other_keys[128] )
    {
      usb_keyboard_send1(keyboard_id, mods, keys, other_keys);
    }
    #endif
    ```
* `usb_keyboard.c`
  * Add a modified version of `usb_keyboard_send()`. Bypass the variables and pass in ourselves the modifier mask, array of 6 pressed keys, and array of other_keys[128]. Send packet to the endpoint (either or) using the `keyboard_id` parameter.
  ```
  #if defined(KEYBOARD_TWO_INTERFACES)
  int usb_keyboard_send1(uint8_t keyboard_id, uint8_t mods, uint8_t keys[6], uint8_t other_keys[128])
  {
    uint32_t wait_count=0;
    usb_packet_t *tx_packet;

    uint32_t endpoint = keyboard_id ? KEYBOARD_ENDPOINT : KEYBOARD1_ENDPOINT;

    while (1) {
      if (!usb_configuration) {
        return -1;
      }
      if (usb_tx_packet_count(endpoint) < TX_PACKET_LIMIT) {
        tx_packet = usb_malloc();
        if (tx_packet) break;
      }
      if (++wait_count > TX_TIMEOUT || transmit_previous_timeout) {
        transmit_previous_timeout = 1;
        return -1;
      }
      yield();
    }
    *(tx_packet->buf) = mods;
      *(tx_packet->buf + 1) = 0;
      memcpy(tx_packet->buf + 2, keys, 6);
      // tx_packet->len = 8;

      // additional bits
      memset(tx_packet->buf + 8, 0, 24);
      for(int i=0; i < 102; i++) {
        if (!other_keys[i]) {  // counted down to zero
          *(tx_packet->buf + 8 + i/8) |= 1 << (i & 7);
        }
      }
      for(int k=0; k < 6; k++) {
        int i = keys[k]; // clear bits for keys that are already in the length-6 array
        *(tx_packet->buf + 8 + i/8) &= ~(1 << (i & 7));
      }
      tx_packet->len = 32;

    usb_tx(endpoint, tx_packet);


    return 0;
  }
  #endif
  ```
  
* ugly stuff in main.cpp that splits the keys into two sets of variables/arrays...

* why does 'keyboard_leds' take 5 secs to update??
  * `usb_dev.c`: add KEYBOARD1_INTERFACE below the first keyboard. adding this made it update immediately!? 
  ```
  #ifdef KEYBOARD_INTERFACE
      if (setup.word1 == 0x02000921 && setup.word2 == ((1<<16)|KEYBOARD_INTERFACE)) {
        keyboard_leds = buf[0];
        endpoint0_transmit(NULL, 0);
      }
  #endif

  #ifdef KEYBOARD1_INTERFACE
      if (setup.word1 == 0x02000921 && setup.word2 == ((1<<16)|KEYBOARD1_INTERFACE)) {
        keyboard_leds = buf[0];
        endpoint0_transmit(NULL, 0);
      }
  #endif
  ```
