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
    * This state indicates that the hardware is able to transmit 0,1,2 packets.
      * because it has EVEN and ODD buffers and the USB module uses at most one at a time
      * you can queue two packets, set both OWN bits?
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

* `usb_init()`
  * `usb_init_serialnumber()` (check this later)
  * clear bdt memory
  * usb module initialization sequence
    * clock gating, set bdt page registers, clear isr flags, enable usb module, enable usb reset interrupt, enable interrupt bits in NVIC, enable D+ pullup

* `usb_isr()`
  * look here first?
  * status = `USB0_ISTAT`  interrupt status 
  * if received a SOF token,
    * if usb was configured: (configuration happens in SETUP)
      * decrement reboot_timer? then reboot teensy ??
    * clear the SOFTOKEN flag
  * if USBRST
    * reset ODD bit
    * initialize BDT endpoint 0 with buffer descriptor and pointer to memory
    * activate USB0_ENDPT0 (RX,TX,HSHK shake) enable
    * clear current interrupts
    * reset USB0_ADDR to 0 (for the USB spec)
    * enable interrupts
    * USB0_USBCTL = USB_CTL_USBENSOFEN;  // enable again? redundant?
  * if STALL:
    * re-enable endpoint (RX,TX,HSHK) and clear ISTAT_STALL bit
  * if ERROR:
    * don't handle error. clear ISTAT_ERROR bit
  * if SLEEP:
    * ?? clear ISTAT_SLEEP bit
  * if TOKDNE:
    * stat = `USB0_STAT`
    * endpoint = stat >> 4
    * if endpoint == 0, move to `usb_control(stat)`
      * else it's a endpoint for some interface
      * packet = stat2bufferdescriptor(STAT) -> addr in BDT
      * check if the most recent transaction was a TX or RX operation
        * transmit
          * usb_free(packet)  because we're done with this packet
          * packet = tx_first[endpoint] ??
          * tx_first[endpoint] = packet->next  move to next packet in the list?
          * tx_state[endpoint] = use the even or odd buffer next?
            * if no packet, both even and odd buffers become free
            * b->desc gets updated if there is a packet, otherwise don't update
        * receive
          * initialize packet struct
            * packet->len = b->desc >> 16   extract buffer length
            * packet->index = 0, packet->next = NULL
            * rx_first[endpoint] = packet or rx_last[endpoint]->next = packet
              * rx_last[endpoint] = packet
              * OK it looks like the packets form a linked list
            * usb_rx_byte_count_data[endpoint] += packet->len
          * allocate a new packet for the next receive
            * packet = usb_malloc()
            * buffer descriptor {addr = packet->buf, desc = ...}
            * if unable to get a new packet, b_desc = 0 ...
          * if received packet with zero-length data, just reset the buffer descriptor
    * clear ISTAT_TOKDNE bit

  * the code checks the status bits in a different order...

* `usb_control()`
  * types of packet IDs (PID)
    * https://beyondlogic.org/usbnutshell/usb3.shtml
    * TOKEN packets: In (0x09), Out (0x1), Setup (0x0D)
    * Data packets: Data0 (0x03), Data1 (0x0B)
    * Handshake packets: ACK (0x02), NAK (0x0A), STALL (0x0E)
    * Start of frame packets: SOF (0x05)
  
  * convert USB0_STAT to a buffer descriptor in the BDT
  * get the current buffer from BDT
  * get packet id `PID` (token) from buffer descriptor.

    * if PID is SETUP:
      * setup comes with a data packet that is 8 bytes??
      * release the current buffer, end any pending TX transfers on endpoint0, clear TX BDT entries for endpoint0
        * ep0_tx_data_toggle = 1  (first IN after SETUP is always DATA1 ??)
      * then jump to `usb_setup()`
      * USB0_CTL = USB_CTL_USBENSOFEN   re-enable the USB (when did it get disabled?)
        * there's a TXSUSPENDTOKENBUSY bit

    * if PID is OUT
      * it means data was received from host.
        * for keyboards, it's could be the LED states
      * status stage, send back a data packet of 0 length?
        * endpoint0_transmit(NULL, 0)
      * then give the buffer back (?)

    * if PID is IN
      * send remaining data, if any. but can't send more than EP0_SIZE bytes.
        * guessing that it will send the rest next time?
      * data = ep0_tx_ptr
      * if (data)
        * size = min(ep0_tx_ten, EP0_SIZE)
        * endpoint0_transmit(data,size)
        * data += size
        * ep0_tx_len -= size
        * ep0_tx_ptr = (ep0_tx_len > 0 || size == EP0_SIZE) ? data : NULL;

      * if bRequest == 0x05 (SET ADDRESS) then set USB0_ADDR = wValue.
        * the device should not set its address until after the completion of the status stage
        * but why does setting the ADDR happen after the IN transaction?
      

* `usb_setup()` handles a SETUP request from the control pipe
  * https://beyondlogic.org/usbnutshell/usb6.shtml
    * The SETUP packet contains bmRequestType, bRequest, wValue, wIndex, wLength
      * `bmRequestType`: data transfer direction, type (standard/class/vendor), recipient (device, interface, endpoint)
        * usually the parser branches to a number of handlers
      * `bRequest`: the request being made
      * `wValue`, `wIndex`: other parameters in the request
      * `wLength`: number of bytes if there is a data stage
    * Standard Device requests:
      * `0x00` GET_STATUS
      * `0x01` CLEAR_FEATURE
      * `0x02` SET_FEATURE
      * `0x05` SET_ADDRESS
      * `0x06` GET_DESCRIPTOR
      * `0x07` SET_DESCRIPTOR
      * `0x08` GET_CONFIGURATION
      * `0x09` SET_CONFIGURATION

  * it looks like there is a switch() statement and some of the branches fill a `reply_buffer` and set `data` and `datalen`
    * after the switch() is a `send:` label:
      * datalen = min(datalen, wLength)  // truncate
      * send at most two packets
        * size = min(size, EP0_SIZE)
        * endpoint0_transmit(data, size)
        * data += size  // advance the ptr
        * datalen -= size
        * if (datalen == 0 && size < EP0_SIZE) early return;
        * repeat once more for another packet
      * if there is still data to be sent:
        * ep0_tx_ptr = data
        * ep0_tx_len = datalen
  * switch (wRequestAndType)
    * SET_ADDRESS
      * do nothing, address will be set later, at the end of the status stage
    * SET_CONFIGURATION
      * `usb_configuration` = wValue
      * cfg = usb_endpoint_config_table
      * clear all BDT entries that the USB module owns 
        * free the usb_packet_t in use by that endpoint
      * free all queued packets
        * p = rx_first[ep]; while (p) {n = p->next; usb_free(p); p = n;}; rx_first[ep] = rx_last[ep] = null;
        * p = tx_first[ep]; ...
        * usb_rx_byte_count_data[ep] = 0
        * tx_state[ep] = BOTH_FREE_[EVEN|ODD]_FIRST, depending on what which one (even or odd) is currently free
      * then clear the bdt entries (other than endpoint 0)?
        * epconf = *cfg++
        * *(&USB_ENDPTx) = epconf  // update the low 8 bits with the default config (initialized in usb_endpoint_config_table)
        * if (epconf & USB_ENDPT_EPRXEN) // enabled for RX
          * re-initialize the BDT entries for this endpoint (EVEN and ODD). 
          * RX buffer entries:
            * need to allocate a buffer
            * BDT_DESC(64, DATA0 for EVEN, DATA1 for ODD)
          * TX buffer entries:
            * set both TX descriptors = 0

    * GET_CONFIGURATION
      * reply_buffer[0] = usb_configuration  // the configuration value was stored here
    * GET_STATUS (device)
      * reply_buffer = [0,0], datalen = 2
      * the GET Status request directed at the device will return two bytes during the data stage with the following format:
        * bit [1] remote wakeup
        * bit [0] self powered
          * the remote wakeup bit can be changed by the SetFeature and ClearFeature requests with a feature selector of DEVICE_REMOTE_WAKEUP (0x01)
    * GET_STATUS (endpoint)
      * the recipient is encoded in the lowest 5 bits of bmRequestType
      * endpoint index is i = wIndex?
        * if (USB0_ENDPT0 + 4*i) & 0x02 then reply_buffer[0] = 1
          * why are we checking if the endpoint is stalled?
    * CLEAR_FEATURE (endpoint)
      * (*(uint8_t *)(&USB0_ENDPT0 + i * 4)) &= ~0x02;
      * clear the stall bit of the specified endpoint
    * SET_FEATURE (endpoint)
      * set the stall bit of the specified endpoint
    * GET_DESCRIPTOR (to a device or interface recipient)
      * loop through usb_descriptor_list
        * look at it
        * usb_descriptor_list is in `usb_desc.c` and provides access to all the descriptors (device, configuration, interface, endpoint, ...)
        * the struct `usb_descriptor_list_t` has {uint16 wValue, uint16 wIndex, const uint8 addr, uint16 length}
      * if wValue == list->wValue && wIndex == list->wIndex
        * wValue is the descriptor type and index
        * wIndex is zero or language ID
        * data = list->addr
          * for string descriptors, use the descriptor's length field (datalen = *(list->addr)), else datalen = list->length
        * goto send label, since the descriptor is found
        * if descriptor not found, stall endpoint0.
    * if unsupported request and type, stall

* `usb_rx()`
  * receive a packet from this endpoint
  * disable interrupts
    * ret = rx_first[endpoint]
    * if (ret) { rx_first[endpoint] = ret->next; rx_byte_count_data[endpoint] -= ret->len }
      * we took the first node of this endpoint's RX linked list
  * re-enable interrupts

* `usb_queue_byte_count()`
  * counts the total number of data bytes for packets in the queue
  * for( ;p; p = p->next) count += p->len;

* `usb_tx_byte_count()`
  * return usb_queue_byte_count(tx_first[endpoint])
  * ok, same thing... linked lists

* `usb_tx_packet_count()`
  * same thing, traverses a linked list

* `usb_rx_memory()`
  * called from usb_free() when usb_rx_memory_needed > 0
  * loops 1..NUM_ENDPOINTS and gives a packet to the first endpoint that needs it
    * if the endpoint is configured for RX:
      * is the EVEN descriptor == 0? give it to EVEN slot, with DATA0
      * else is the ODD descriptor free? give it, with DATA1
  
* `usb_tx()`
  * check tx_state[endpoint] to determine if we should put the next packet in the EVEN or ODD TX buffer
  * BOTH_FREE_EVEN_FIRST: put into even, next state will be ODD_FREE
  * BOTH_FREE_ODD_FIRST: put into odd, next state is EVEN_FREE
  * EVEN_FREE: put into even, next state is NONE_FREE_ODD_FIRST
  * ODD_FREE: put into odd, next state is NONE_FREE_EVEN_FIRST
  * otherwise if none are free currently, queue up the next packet into this endpoint's tx linked list:
    * tx_first[ep] == null? tx_first = packet, else  tx_last[ep]->next = packet
    * then early return
  * if there is a buffer descriptor available, 
    * set b->desc and b->addr. DATA1 if b&8 else DATA0 ??
      * the 32-bit descriptor should match the buffer descriptor in the datasheet? (page 637)
      * DATASHEET:
        * [25:16] byte count
        * [7] own 
        * [6] data0/1
        * [5] keep
        * [4] ninc
        * [3] dts
        * [2] bdt_stall
      * `usb_dev.c`
        * BDT_OWN 0x80    same
        * BDT_DATA1 0x40  same
        * BDT_DATA0 0x00
        * BDT_DTS 0x08    same
        * BDT_STALL 0x04  same
        * BDT_PID(n) (n>>2) & 15  same [5:2] can also be TOK_PID[3:0]
      * so what does b&8 mean? b is cast to a uint32_t, which becomes the descriptor part of the struct? 
      * then whether it's DATA0/1 must be the same as DTS?
        * data toggle synchronization
        * https://wiki.osdev.org/Universal_Serial_Bus#Data_Toggle_Synchronization
          * the host maintains a data toggle bit for every endpoint with which it comminucates
          * the state of the data toggle bit is indicated by the DATA PID (ok, so DATA0 or DATA1)
          * the receiver toggles its data sequence bit when it is able to accept data and it receives an error-free packet with the expected DATA PID
          * the sender toggles its data sequence bit upon receiving a valid ACK handshake
          * the sender and receiver synchronize their data toggle bits at the start of a transaction
            * control transfers initialize endpoint toggle bits to 0 with a SETUP packet
            * interrupt endpoints intialize their data toggle bits to 0 upon any configuration event
            * 
    * set tx_state[ep] = next state

### `usb_keyboard.c`
* uses usb_desc.h and keylayouts.h
* usb_keyboard_class implements Print... why?
* write(uint8), write_unicode(uint16), set_modifier(uint16), set_key[1-6](uint8), set_media(uint16)
* send_now(), press(), release(), releaseAll()

Global variables:
* uint8 keyboard_modifier_keys
  * bitmask for left/right ctrl/shift/alt/gui(?)
* uint8 keyboard_keys[6]
  * how do they know the keycodes? USB HID usage tables has a table for keys
* uint8 keyboard_protocol = 1
  * bInterfaceProtocol = 1 for keyboard 
    * iirc there's a boot protocol and report protocol
      * but it doesn't matter(?)
* uint8 keyboard_idle_config
  * ms*4  how often reports are periodically sent to the host
* uint8 keyboard_idle_count
  * count until idle timeout
* uint8 keyboard_leds
  * bitmask for keyboard leds

The functions...
* `usb_keyboard_write(uint c)`
  * if (c < 0x80) 
    * go straight to keyboard_write_unicode
  * if (c < 0xC0)
    * means it's a 2nd, 3rd, or 4th byte, 0x80 to 0x8F
  * WAIT A MINUTE it's assuming it's unicode
    * this function has static variables utf8_state, unicode_wchar
    * not sure how it is used yet. for international layouts, i can assume there are unicode chars.
      * but how does the computer differentiate it?? there is a country code in the HID descriptor...
    * https://forum.pjrc.com/threads/47473-Teensy-write-key-by-key-UTF-8
      * on non-US keyboard layouts, pressing shift+alt gives a third function (rather than just two: shift -> uppercase or lowercase)
      * maybe third or fourth functions (shift and/or alt)
      * dead-key sequences (for typing accents): depends on the key you previously pressed
        * like ctrl+k, ctrl+c  to auto-format in vscode...
    * https://forum.pjrc.com/threads/43569-Keyboard-Send-Any-Unicode-Character
      * they decided to make every keyboard identical and merely put different keycap labels
      * HID defines usage numbers for the various key positions, not their characters
        * Usage ID and Usage Name
        * Usage 224 = LeftCtrl, 225 LeftShift, ... 231 RightGUI
        * It's up to the Host to decode the usage numbers based on your layout (?)
* `unicode_to_keycode(uint16_t cpoint)`
  * converts unicode chars to a uint8 key and a modifier mask?
* `deadkey_to_keycode(KEYCODE_TYPE)`
  * keys substituting for other functions...
* `usb_keyboard_write_unicode(uint16_t cpoint)`
  * keycode = unicode_to_keycode(cpoint)
  * if (keycode)
    * deadkeycode = deadkey_to_keycode(keycode)
    * if (deadkeycode) write_key(deadkeycode)
    * write_key(keycode)
* `write_key(uint8 keycode)`
  * usb_keyboard_press(keycode_to_key(keycode), keycode_to_modifier(keycode))
* `keycode_to_modifier(uint8 keycode)`
  * if keycode & SHIFT_MASK?  modifier |= MODIFIER_KEY_SHIFT
  * if keycode & ALTGR_MASK? RCTRL_MASK? 
  * keycodes have modifier mask bits in fixed positions?
* `keycode_to_key(uint8 keycode)`
  * keycode & 0x3F  (up to 63)
    * SHIFT_MASK is 0x40, raising it to 127
    * ALTGR_MASK is 0x80
    * RCTRL_MASK is 0x0800
    * if key == KEY_NON_US_100 (63) then key=100
* `usb_keyboard_press_keycode(uint16 n)`
  * msb = n >> 8
  * if msb >= 0xC2
    * if msb <= 0xDF
      * n = (n & 0x3F) | ((uint16_t)(msb & 0x1F) << 6)   // ???
      * keycode = unicode_to_keycode(n)  // woah
      * mod, key = keycode_to_modifier(keycode), keycode_to_key(keycode)
      * usb_keyboard_press_key(key, mod | modrestore)
        * modrestore is for deadkeys
        * un-press all modifier keys, do a press, then press them again (OR them back in)
    * if msb == 0xF0
      * usb_keyboard_press_key(n, 0)
    * if msb == 0xE0                   what does it mean?!?!
      * usb_keyboard_press_key(0, n)
* `usb_keyboard_release_keycode(uint16 n)`
  * same thing...
    * usb_keyboard_release_key(key, mod)  // no modrestore needed
* `usb_keyboard_press_key(uint8 key, uint8 modifier)`
  * at the end:  if (send_required) usb_keyboard_send();
  * send only if something changed (a bit in keyboard_modifier_keys, or keys[])
* `usb_keyboard_release_key(uint8 key, uint8 modifier)`
  * similar. only send if something changed
* `usb_keyboard_release_all()`
  * only send if something changed. OR modifiers and keys[i]
* `usb_keyboard_press(uint8 key, uint8 modifier)`
  * press one key only, with these modifiers. send.
  * then release all keys and modifiers. send.
* `#define TX_PACKET_LIMIT 4`
  * static uint8 transmit_previous_timeout
  * max number of packets in the queue
  * `#define TX_TIMEOUT_MSEC 50`
  * `#define TX_TIMEOUT (TX_TIMEOUT_MSEC * 428)`
* `usb_keyboard_send()`
  * wait until something...
    * while (1)
      * if (!usb_configuration)  return -1
        * not configured yet (need setup packet SET_CONFIGURATION)
      * if (usb_tx_packet_count(KEYBOARD_ENDPOINT) < TX_PACKET_LIMIT) tx_packet = usb_malloc() and continue if success
      * if (++wait_count > TX_TIMEOUT || transmit_previous_timeout) transmit_previous_timeout = 1; return -1  
        * give up on transmitting after timeout. something else has to clear the timeout flag
      * yield() ??  don't hog the processor (this is a while(1), after all)
  * send the contents of keyboard_keys and keyboard_modifier_keys
    * *(tx_packet->buf) = keyboard_modifier_keys
    * *(tx_packet->buf+1) = 0   // reserved byte
    * memcpy(tx_packet->buf+2, keyboard_keys, 6);
    * tx_packet->len = 8
    * usb_tx(KEYBOARD_ENDPOINT, tx_packet)
* keymedia keys
  * consumer keys, system keys
    * press, release, release_all
  * `usb_keymedia_send`
    * the packet data length is still 8 bytes, but it's packed differently?
    * usb_tx(KEYMEDIA_ENDPOINT, tx_packet)

that's all for the "extern C" functions.
usb_keyboard_class derives from the Print class
* begin() and end() are from print
  * need to know 
* write(c) calls usb_keyboard_write(c)
* send_now() calls usb_keyboard_send(), which is the thing calling usb_tx()

Where is `usb_init()` called?
* pins_teensy.c