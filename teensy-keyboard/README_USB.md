How to make the Teensy act as a USB keyboard?

I first tried reading the USB section of the datasheet. My takeaway was that the host and peripheral communicate through packets. The microcontroller has a USB module that handles the physical side (reading and generating signals), and works independently of the processor. The module accesses buffers in memory to send data or to store received data, and the processor cannot access the buffer at the same time. Interrupts are triggered after every USB transaction...

But it was not mentioned what data the packets contained. Hence I had to learn about USB first, and then come back to the datasheet.


### USB in a Nutshell
* https://beyondlogic.org/usbnutshell/usb1.shtml
  
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
  * Two buffer descriptors (because it is double buffered)
    * A BD contains a pointer to the buffer in memory, and other info.
    * The processor accesses one BD while the USB-FS works on the other.
    * 1-bit `OWN` semaphore indicates who owns the BD.

* RX vs TX
  * RX: transfers data from USB to memory
  * TX: transfers data from memory to USB
  * Device: receives `OUT` or `SETUP` tokens, transmits `IN` token
  * Host: vice versa

* Section `35.3.5 USB transaction`

Teensy has:
* `usb_keyboard.h`
* `usb_desc.h`
* https://forum.pjrc.com/threads/49045?p=164512&viewfull=1#post164512


In `kinetis.h`:
* There's an interrupt for USB-OTG `IRQ_USBOTG`
* System Integration Module (SIM)
  * Things that look interesting:
    * USB regulator enable
    * USB clock gate control
* USB OTG Controller (USBOTG)
  * `USB0_x` various registers
  * masks for these registers are also defined
  * IDK how the USB protocol works yet

* Don't think Teensy-LC has these
  * USB Device Charger Detection Module (USBDCD)
  * USB High Speed OTG Controller (USBHS)
  * USB 2.0 Integrated PHY (USB-PHY)

Other links:
* https://engineering.biu.ac.il/files/engineering/shared/PE_project_book_0.pdf
