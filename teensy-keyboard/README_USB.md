How to make the Teensy act as a USB keyboard?

I first tried reading the USB section of the datasheet. My takeaway was that the host and peripheral communicate through packets. The microcontroller has a USB module that handles the physical side (reading and generating signals), and probably runs independently from the processor. The module accesses buffers in memory to send data or to store received data, and the processor cannot access the buffer at the same time. Interrupts are triggered after every USB transaction...

But it was not mentioned what data the packets contained. Hence I thought I should learn about USB first, and then come back to the datasheet.

I also want to be able to debug and print out USB packets that are received on the Teesny. How to do this? Is it possible to implement both functionality?

### Side note: It's common to have an Arduino open a serial port for communication. When it is plugged into a computer via USB, does it act as a USB device or what...?
* I know Arduino UNO's 328p uses UART hardware, but what is it connected to?
* https://electronics.stackexchange.com/questions/403374/arduino-and-usb-how-it-works
  * A second chip (16u2) implements the "USB bridge". I'm assuming it acts as a USB device and converts between USB packets and serial (UART on the 328p).
* https://superuser.com/questions/1207025/why-are-usb-ports-sometimes-referred-to-as-serial-ports-and-called-com
  * A COM port is like an endpoint, the bridge inbetween two endpoints is ignored.
    * example: CPU - PCIe - USB - keyboard
  * USB also uses the idea of endpoints.
  * Virtual COM port is a serial port on a USB device as an endpoint.
    * COM port can be accessed using the Windows API for COM ports (?)
    * Usually, manufacturers just tell Windows that the device is a USB COM port and avoid writing USB device drivers.
* USB CDC (Communication device class)

Bootloader:
* https://www.baldengineer.com/arduino-bootloader.html
* A bootloader runs before the main program runs.
  * If the bootloader receives a particular sequence of bytes, it will act as a programmer and change the flash (PROGMEM). Outside of the bootloader program, it's not possible to reprogram the memory without a dedicated programmer (?)
    * This is to prevent (buggy) code from changing the flash on its own.
    * To re-program, do something to reset the chip. Then, while it's in the bootloader stage, send the program over serial (UART).
  * Downsides:
    * The bootloader code results in a delay on startup.
    * The bootloader code takes up memory.
    * The initial state of registers might be altered.

Writing my own USB drivers?
* Does it mean I have to write a bootloader too? (feels like I shouldn't have to...)
  * https://stackoverflow.com/questions/38386255/writing-my-own-arduino-driver
    * http://www.davidegrayson.com/signing/
* What's the smaller chip on the Teensy for?
  * https://forum.pjrc.com/threads/35399-Basic-question-about-creating-custom-Teensy?p=109751#post109751
    * It contains the bootloader code. The main chip is flashed with the bootloader, and then the main chip can receive your program through USB. Then it restarts and runs your program.

Onwards!
* http://kevincuzner.com/2014/12/12/teensy-3-1-bare-metal-writing-a-usb-driver/

# Actually, I can't follow along because Teensy already uses the usb_isr() interrupt and I can't re-define it
* unless I want to rip it out and replace it everywhere...?
* I'll just take the easier road then... (using/modifying teensy code)

* Maybe someday I'll start from something more basic, so I can feel like I'm making progress with each step

# Notes on USB stuff

### Datasheet `Chapter 35: USB OTG Controller`

* Low Speed = 1.5 Mbit/s, Full speed = 12 Mbit/s, (and High speed = 480 Mbit/s)
* Attach and detach peripherals to a host (Host Controller)
  * There can only be one host
  * Peripherals share the USB bus bandwidth
    * Host schedules transactions and manages bandwidth. Token-based protocol (?)
* On-The-Go (OTG) spec allows devices to switch roles between host and device.

* Buffer Descriptor (BD) and Buffer Descriptor Table (BDT)
  * 16 bi-directional endpoints (?)
    * Each endpoint direction requires two 8-byte buffer descriptors
  * Two buffer descriptors (ping-pong)
    * A BD contains a pointer to the buffer in memory, and other info.
    * The processor accesses one BD while the USB-FS works on the other.
    * 1-bit `OWN` semaphore indicates who owns the BD.

* RX vs TX
  * RX: transfers data from USB to memory
  * TX: transfers data from memory to USB
  * Device: receives `OUT` or `SETUP` tokens, transmits `IN` token
  * Host: vice versa

* Section `35.3.5 USB transaction`
  * still too vague

Other links:
* https://engineering.biu.ac.il/files/engineering/shared/PE_project_book_0.pdf

# USB in a Nutshell
* https://beyondlogic.org/usbnutshell/usb1.shtml

# Starting over (but with the knowledge above)
* https://forum.pjrc.com/threads/49045?p=164512&viewfull=1#post164512

Teensy has:
* `usb_keyboard.h` defines a usb_keyboard_class
* `usb_desc.h` defines for usb descriptors

USB spec has descriptors (Device, Configuration, Interface, Endpoint) and these form a tree structure?
* One device. A device usually has only one configuration, but can have more. A configuration defines a number of interfaces. Each interface represents a functionality (a mouse or keyboard would have its own interface). An interface has multiple endpoints where data is transmitted and received (in buffers).
* A single device can implement many interfaces, and thus act as different things.

For HID class devices, the interface descriptor defines the class, subclass, and protocol.
* * This triplet is also in the device descriptor, but they decided to move it into the interface descriptor for HID class devices.
* Under the Interface descriptor is a HID descriptor
  * Under the HID descriptor is a Report descriptor which describes the format of the data provided by the device.
  
HID ver. 1.11, Report descriptor for boot interface for a keyboard
* The keyboard input report is 8 bytes:
  * Modifier keys, reserved, Keycode 1-6
    * We can only report 6 keys pressed at a time?
* The keyboard output report is 1 byte:
  * LED state for num, caps, scroll
  * compose, kana (??)
* GetReport, SetReport requests from the host to get or send data

Report desriptor is confusing...
* https://eleccelerator.com/tutorial-about-usb-hid-report-descriptors/
  * USAGE_PAGE is like a namespace, changing it changes what USAGEs are available.
    * USAGE_PAGE(Generic Desktop) -> Usage(Keyboard), Usage(Mouse), Usage(Game Pad)
  * Each collection has its own report id/type.
* https://who-t.blogspot.com/2018/12/understanding-hid-report-descriptors.html
  * Things like Usage Page and Report Size are "global items" and apply to all following items until changed.
  * Input and Output tells to "process what you've seen so far" with a few flags:
    * Rel, Abs: the values are relative, absolute
    * Cnst: constant, don't need a Usage, it's ignored. Only used for padding?
  * Usage Minimum (a) / Usage Maximum (b): there are b-a+1 items, rather than listing all the items in the range individually
  * Logical Minimum / Maximum: allowable range
    * Physical Minimum / Maximum: map logical [min,max] to physical [min,max]
  * Report ID (i) 
    * An Application Collection describes a set of inputs that makes sense as a whole
    * A Report descriptor has at least one application collection
    * Put the report id at the start a collection?

##### WALL OF TEXT -- DONT READ
HID Chapter 5
* Report descriptors are composed of pieces of information called "items".
  * All items have a 1-byte prefix containing the item tag, type, size.
    * An item may contain optional data after the prefix.
* Item parser:
  * parses the report descriptor in a linear fashion. The parser collects the state of each known item as it walks through the descriptor, and stores them in an item state table. 
    * it makes a tree:
      * Application Collection -> Collection -> Report (ID?) -> {Main Item, Report Size, Report Count, Logical Min/Max } -> Usage
  * Main item: a new report (node) is put in the item state table. "Local" items are removed, but "global" items remain?
    * Global items set the default value for subsequent "main" items
  * Push item: item state table is copied and pushed on a stack
  * Pop item: restore item state table from stack
* Usage: 
  * 32 bit. Higher 16 bits are the Usage Page, lower 16 bits are the Usage ID.
* Report ID is a 1-byte identification prefix to reach report transfer.
  * If there are no Report ID tags, assume there is only one of each {Input,Output,Feature} report structures.
    * Input is sent via Interrupt In pipe. Output and Feature must be initiated by the host via Control pipe (or Interrupt Out).
  * If there are multiple report structures, use the ReportID to determine which structure applies.
* Main items:
  * Input: data from controls on the device
  * Output: data to controls (variable data) or LEDs (array data)
  * Feature: data not intended for the user (software feature or control panel toggle?)
    * {input, output, feature} also specify data/const, variable/array, abs/rel 
  * Collection tag: a grouping of Input, Output, Feature
  * End Collection tag
* Local items only describe fields for the next Main item. Global items persist.
  * Usage, Usage Minimum/Maximum are local items.
* Required items: Input/Output/Feature, Usage, UsagePage, Logical Min/Max, Report Size/Count

* Short items contain 1 prefix byte and up to bytes of data.
  * The prefix byte:
    * bTag  [7:4] specifies the function of the item
    * bType [3:2] 0,1,2,3: main, global, local, reserved
    * bSize [1:0] 0,1,2,3: 0,1,2,4 bytes of data

* Item prefixes in page 38.
* The value reurned by an array item is an index, not a value!
  * Recommended: Logical Minimum = 1. When invalid, let the index be 0 (out of range).
* Get_Report(Input) via the control pipe, or periodically sent through Interrupt IN pipe
* Set_Report(Output) via the control pipe, or optionally through Interrupt OUT pipe

### Examining Teensy usb code
* `usb_inst.cpp` declares global variables(classes) which are the APIs for usb device interfaces such as `usb_serial_class`, `usb_keyboard_class`
* `usb_keyboard.h, c` implements `usb_keyboard_class`
  * modifiers, keys[6], protocol, idle_config (report periodically even if idle), leds
  * there's `write_unicode`... whaa?
* I don't want to deal with the USB module, so maybe I can get away with just replacing `usb_desc` and `usb_keyboard`...

Defines:
* Somewhere, it defines `USB_SERIAL` and `LAYOUT_US_ENGLISH`. It is there in `c_cpp_properties.json`.
  * How to undefine them?
  * in `platform.ini` add a line `build_flags -U USB_SERIAL ...`
  * in `main.cpp` do `#ifdef X`, `#undef X` 
  * however these two conflict with each other! IDK what platformio does, but it screws up the `#undef`, so use only one of them

Another explanation of USB HID:
* https://wiki.osdev.org/USB_Human_Interface_Devices
  * Device descriptor: class/sub-class are 0
  * Interface descriptor: class = 3, sub-class = 1/0 (supports boot protocol or not), protocol = 1/2 (keyboard/mouse)
* A device's protocol can be set by the host through "SetProtocol":
  * Report protocol is more general
  * Boot protocol is simpler, for mouse and keyboard
* The host can send a "GetReport" request:
  * It goes through the control endpoint as a SETUP packet.
    * There is overhead, it is better to use an interrupt endpoint.
  * Interrupt endpoint:
    * The device's descriptors should have an "interrupt IN" endpoint, for interrupt transfers.
    * The device should report at regular intervals.
* "SetReport" request for sending the LED states (NUM, CAPS, SCROLL)

Let's do the descriptors first:
* Device descriptor:
  * `usb_desc.c` and https://beyondlogic.org/usbnutshell/usb5.shtml
  * For fields that are multiple bytes long, put the LSB first
  ```c
  static uint8_t device_descriptor[] = {
    18,                               // bLength (1)
    0x01,                             // bDescriptorType (1)  Device Descriptor (0x01)
    0x10, 0x01,                       // bcdUSB (2)           USB 1.1 or 01.10
    0,                                // bDeviceClass (1)     If 0, each interface has its own class
    0,                                // bDeviceSubClass (1)
    0,                                // bDeviceProtocol (1)
    ENDPOINT0_SIZE,                   // bMaxPacketSize (1)   Valid values: {8, 16, 32, 64}
    LSB(VENDOR_ID), MSB(VENDOR_ID),   // idVendor (2)
    LSB(PRODUCT_ID), MSB(PRODUCT_ID), // idProduct (2)        Using USB_SERIAL codes
    0x73, 0x02,                       // bcdDevice (2)        Device Release Number. Copying Teensy. Arbitrary?
    1,                                // iManufacturer (1)    Use index 0 if there are no string descriptors
    2,                                // iProduct (1)
    3,                                // iSerialNumber (1)
    1,                                // bNumConfigurations (1)  One configuration
  };
  ```
  * and others... there's an example of keyboard descriptors in HID 1.11 Appendix E

When the host sends Get_Descriptor(Configuration), the device should return:
* Configuration descriptor
  * Interface descriptor (specifying HID Class)
    * HID descriptor (for the above interface)
      * Endpoint descriptor (for HID Interrupt IN Endpoint)
      * Optional Endpoint descriptor (for HID Interrupt Out Endpoint)


Looking at `usbmem.c`:
* I recall reading that `usb_packet_t` are pooled... seems like it is done here.
  * pool is size `NUM_USB_BUFFERS`
    * max size is 32. use a bitmask to determine which in the pool are free.
    * CLZ to find the first free index
  * `usb_packet_t * usb_malloc()`  get one from the pool
  * `usb_free(usb_packet_t *p)`    release back into the pool
    * OR immediately give it to something that is requesting it
* packets have a uint8_t buffer of size 64, a buffer length, and index field (for iterating?)
  * and a pointer `*next` to chains together multi-byte packets?

Looking at `usb_dev.c`:
* if `KEYBOARD_INTERFACE` is defined:
  * keyboard_ {modifier_keys, keys[6], protocol, idle_config, idle_count, leds}
* usb_ {init, isr, rx, tx_byte_count, tx_packet_count, tx, }
* usb_configuration, rx_byte_count(endpoint),

##### Wall of text incoming
* buffer descriptor table
  * bdt_t {uint32_t desc; void *addr;}
  * bdt table[(NUM_ENDPOINTS+1)*4];   control endpoint is index 0, each endpoint has rx/tx/even/odd
  * usb_packet_t *rx_first[NUM_ENDPOINTS], *rx_last, *tx_first, *tx_last ??
  * tx_state[NUM_ENDPOINTS]
    * TX_STATE_[EVEN|ODD]_FREE
    * TX_STATE_BOTH_FREE_[EVEN|ODD]_FIRST
    * TX_STATE_NONE_FREE_[EVEN_ODD]_FIRST
  * BDT_{OWN, DATA1, DATA0, DTS, STALL, PID}
    * BDT_DESC(count, data) forms upper 16 bits of the descriptor
    * index(ep, tx, odd) (ep << 2 | tx << 1 | odd)    (forms an index into the bdt)
    * stat2bufferdescriptor(s) (table + (s >> 2))
* setup packet
  * bmRequestType, bRequest, wValue, wIndex, wLength
* endpoint0
  * ep0_ {rx0_buf, rx1_buf, tx_ptr, tx_len, tx_bdt_bank (ODD bit), tx_data_toggle}
  * usb_ {rx_memory_needed, configuration, reboot_timer}
  * endpoint0_ 
    * stall() { USB0_ENDPT0 |= EPSTALL | ... }  set flags in module
      * enable endpoint rx,tx, stall bit causes any access to this endpoint to return a stall handshake
    * transmit(*data, len) { set address in BDT and toggle OWN bit? so the USB module begins a transfer}

Now for the functions...
* `usb_setup()` handles a SETUP request from the control pipe

* `usb_control()`

* `usb_rx()`
* `usb_queue_byte_count()`
* `usb_tx_byte_count()`
* `usb_tx_packet_count()`
* `usb_rx_memory()`
* `usb_tx()`
* `usb_isr()`
  * 

* `usb_init()`
  * `usb_init_serialnumber()` (check this later)
  * clear bdt memory
  * usb module initialization sequence
    * clock gating, set bdt page registers, clear isr flags, enable usb module, enable usb reset interrupt, enable interrupt bits in NVIC, enable D+ pullup
