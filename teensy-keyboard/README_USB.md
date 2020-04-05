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

* Maybe someday I'll start from a lower level, so I can feel like I'm making progress with each step
  * Currently it's just not approachable. I'm using libraries but don't understand what the underlying code does (and it's hard to find an explanation, because that probably requires explaining even more things)

# Notes on USB stuff

### USB in a Nutshell
* https://beyondlogic.org/usbnutshell/usb1.shtml
* http://www.usbmadesimple.co.uk/index.html

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
