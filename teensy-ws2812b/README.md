Driving WS2812B rgb leds with a Teensy-LC. 

In the past I learned about driving WS2812Bs with an arduino:
* https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
* https://cpldcpu.wordpress.com/2014/01/14/light_ws2812-library-v2-0-part-i-understanding-the-ws2812/
* My understanding is that the WS2812B leds have a shift register (actually, it is the integrated constant-current driver IC, the WS2811?). Data is sent to the DIN pin, bit-by-bit. Bits are encoded as pulses:
  * 1 bit: high for 0.8us, low for 0.45us
  * 0 bit: high for 0.45us, low for 0.8us
  * Reset: low for &gt;50us  (actually, it's as short as 9us?)
  * Each of the high/low durations can vary by &pm;150ns
  * 1.25us per bit. 24 bits (1 byte for r,g,b) => 30us per led
  * How does it work? (cpldcpu)
    * The shift registers of WS2812Bs can be chained (DOUT to DIN of the next one). And it turns out the WS2812B reshapes the inputted signal into its true (?) format which we can see at DOUT.
    * The period (1.25us) seems to be divided into 6 parts:
      * 1 bit: high for 4/6, low for 2/3
      * 0 bit: high for 2/6, low for 4/6
        * but the pulse length can vary by 150ns, which is less than 1/6.
      * After receiving the rising edge of a pulse, how long does it take for it a pulse to be shifted out? 1/6.
      * I wonder how the timing works internally... (multivibrator?)
  * Some people say that the LOW duration of the pulse does not matter too much, as long as it's less than the reset duration.
    * I'm assuming the LEDs reshape and output data at a fixed rate. Then you can't input data faster than it's outputting, ...?
      * I didn't look into this

Ways to drive WS2812B:
* there's a rabbit hole of links...
  * https://hackaday.com/2014/09/10/driving-ws2812b-pixels-with-dma-based-spi/
  * https://www.instructables.com/id/My-response-to-the-WS2811-with-an-AVR-thing/
  * https://github.com/FastLED/FastLED/wiki/SPI-Hardware-or-Bit-banging
    * avoids the lower-level details...
  * https://hackaday.com/2016/10/06/driving-16-ws2812b-strips-with-gpios-and-dma/
  * http://www.martinhubacek.cz/arm/improved-stm32-ws2812b-library
    * writing ports in parallel, using three-DMA transfers:
      * 1: start of pulse, write all 1s
      * 2: write the next bit for each port. This event happens when it's time for a 0-bit pulse to go LOW. If the data bit is 0, GPIO (pulse) goes LOW. If 1, GPIO stays HIGH.
      * 3: write 0 to each port. This event happens when it's time for a 1-bit pulse to go LOW.
    * He seems to have trouble at lower clock speeds, when other interrupts are turned on (and fighting for bus control?).
* SPI:
  * fixed clock speed, send data bit-by-bit
  * Needs to be in slave mode, because in master mode, MOSI is pulled high after every byte.
  * I think the key is that because we're sending pulses, the data output must start and end at zero. The first rising edge signals the start of the first pulse (and the following stream of pulses).
* Timer/PWM: 
  * set on bottom, clear on compare match. Need to set the output compare register after each bit?
* I2C:
  * Like SPI, fixed clock speed and send bit-by-bit
  * Start condition: SDA low while SCL high
  * First byte is the address packet... can we ignore it? 
    * This byte (and the start condition?) will get shifted out of the last LED, anyway.
  * Slave ACKs after every byte by pulling SDA low (OK, ignore it)
    * Well now there's an extra bit for every 8 bits...
* Bit banging:
  * In a blocking loop, read rgb data from memory and set the GPIO output manually.

DMA (Direct Memory Access) on the Teensy:
* See the application note AN4631
  * One of the main points is to reduce power consumption. Power is consumed every time transistors switch, so it would be better for peripherals to operate asynchronous to the CPU clock (ADC, DAC, UART, I2C, SPI, TPM (Timer/Pulse Module), Low Power Timer (LPTMR))
* Configure the DMA
  * `DMA_DCRn[EADREQ] = 1` Enable Asynchronous DMA Requests bit
    * All other bits/registers must be configured as they would be for synchronous operation
* Configure the peripheral to use the DMA
  * These modules are capable of generating asynchronous DMA requests:
    * UART0 (TX, RX), TPM (Channel interrupt, Overflow), ADC, CMP, PORTA, PORTD
  * Guidelines:
    1) Enable the clock to be used. The clock must be able to source the module and operate in the lower power mode to be used...?
    2) Disable the DMA channel to be used
    3) Disable the module being configured
    4) Configure the module in the same manner as for interrupt operation
    5) Configure the module to use the clock that the user enabled for this application ...?
    6) Set the appropriate DMA enable bit in the module (Ctrl+F "DMAEN" in the datasheet)
    7) Enable the module
    8) Enable the DMA channel being used for this module
    9) Enter the desired low-power mode ...?
* DMA Interrupts: DMA channel 0-3 transfer complete and error
  * Something has to trigger the DMA transfer first...
    * e.g. ADC0_DMA_Request for DMA_Channel0 to copy the ADC value to a location
  * After a DMA transfer on a channel completes, we can trigger another DMA transfer
    * e.g. copy the copied ADC value to a second location
  * "cycle steal": only one tranfser per DMA request (how?)
  * DMA writing to a static array (circular buffer)?

Datasheet Chaper 22: DMA Multiplexer
* Routes DMA sources (slots) to DMA channels.
  * up to 63 peripheral slots, up to 4 always-on slots (?), routed to 4 channels
* Modes of operation:
  * Disabled, Normal (a source is routed to the channel), Periodic Trigger (a DMA request is triggered periodically by the PIT (periodic interrupt timer)?)
* DMAMUX0_CHCFG[0-3]  Channel Configuration register
  * [7] ENBL: DMA Channel Enable
  * [6] TRIG: DMA Channel Trigger Enable (enable periodic trigger mode)
  * [5:0] SOURCE: DMA Channel Source (slot)
* Triggering:
  * A peripheral may set a bit indicating a DMA request, but it will not be serviced until a trigger is seen. Once the request has been completed, the peripheral will set the request signal LOW.
  * Ex. SPI module is assigned to a DMA channel with a trigger. If the SPI module's transmit buffer is empty, it requests DMA transfers from memory. The SPI module can also DMA received data into memory.
  * Ex. Driving GPIO ports periodically, or sampling periodically.
* 4 Always-enabled DMA sources:
  * DMA to/from GPIO pins, as fast as possible
  * DMA from memory <-> memory
  * DMA from memory <-> external bus
  * any DMA transfer that should be started by software
    * unlike peripheral DMA sources, where they control when to signal the DMA request
* Initialization:
  * (datasheet, page 365)
  * Clear the DMA channel config register, Configure the DMA and enable it, Configure the corresponding timer (if periodic triggering), Set the channel config register
    * (channel config is not the same as DMA configuration? Well this *is* only the MUX chapter...)
  
Datasheet Chapter 23: DMA Controller Module
* DMA controller module allows fast transfer of data without processor interaction
* 4 channels, n=0,1,2,3
  * 8, 16, or 32-bit transfers
  * Source Address register (SARn)
    * Only write 32-bit values to this register.
    * Bits 31-20 must be one of four values: 0x[000,1FF,200,400]x_xxxx, corresponding to a valid region of the device's memory map
  * Destination Address register (DARn)
  * Status register (DSRn)
  * Byte Count reigster (BCRn)
    * DSR and BCR are combined into one 32-bit register.
    * [31:24] DSR
      * CE: Configuration error, BES: Bus error on source, BED: Bus error on destination, REQ: Request is pending and the channel is not currently selected. BSY: Busy, set when the channel is enabled after a transfer is initiated. DONE: set when transactions complete (transfer count = 0) or error condition. Also used to abort a transfer. Writing a 1 will clear the error bits in DSRn.
    * [23:0] BCR
    * The DMA controller writes to the DSR bits. DSR[DONE] is set when the block transfer is complete.
    * BCR is decremented by 1,2,4 for 8,16,32-bit accesses. BCR is cleared if a 1 is written to DSR[DONE].
    * If a transfer sequence is initiated and BCRn[BCR] is not a multiple of 4 or 2 (for 32 or 16-bit transfers) then DSRn[CE] is set and no transfer occurs.
  * Control register (DCRn)
    * EINT: Enable interrupt on completion of transfer
    * ERQ: Enable peripheral request
    * CS: Cycle Steal: Forces a single read/write transfer per request. Without this, DMA keeps running until byte-count is decremented to 0.
    * AA: Auto-align ??? If SSIZE >= DSIZE, source accesses are auto-aligned, otherwise, destination accesses are auto-aligned.
      * if AA is enabled, the appropriate address register increments, regardless of DINC or SINC ???
      * In what cases would SSIZE != DSIZE?
        * e.g. we want to write 8-bit data to 32-bit locations?
    * EADREQ: Enable asynchronous DMA requests:  when the MCU is in Stop mode ???
    * SINC: Source Increment: controls whether the source address increments after each successful transfer
    * SSIZE: Source Size: 32,16,8-bit
    * DINC: Destination Increment
    * DSIZE: Destination Size
    * START: Start Transfer, begin the transfer using the transfer control descriptor. It is cleared after one module clock and always reads 0. ??
    * SMOD: Source Address Modulo
      * Size of the circular buffer used by the DMA controller. if SMOD != 0, the buffer is located on a boundary of the buffer size. The location is based on upper bits of the initial source address register (SARn)??? The base address should be aligned to a 0-modulo boundary.
    * DMOD: Destination Address Modulo
    * D_REQ: Disable Request: The DMA module clears the DCRn[ERQ] bit when the byte-count reaches 0.
    * LINKCC: Link Channel Control: The current DMA channel triggers a DMA request to the linked channels LCH[1-2]
    * LCH1, LCH2: linked channel [0-3]
  * Collectively, these define a TCD (transfer control descriptor) with the module acting as a 32-bit bus master connected to the system bus.
  * 32-bit connection with the slave peripheral bus.
  * DREQ (DMA request from a peripheral or package pins) and DACK (acknowledge) or done indicator back to the peripheral.
* Continuous-mode or cycle-steal transfers (?)
* Three steps:
  1) Channel inialization: set transfer control descriptor with address pointers, a byte-transfer count, and control info.
  2) Data transfer: the DMA accepts requests for data transfers, and performs one or more read/write transfers.
  3) Channel termination: transfer is done, or error. Check DSRn (DMA status register)
* Channel priority: 0 > 1 > 2 > 3. Simultaneous peripheral requests activate the channels in this order.
* Initializing the DMA controller module:
  * Initialize the transfer control descriptor (TCDn)
    * Set SARn with the peripheral data register, or the starting memory location.
    * Set DARn.
      * SARn and DARn change after each data transfer depending on DCRn[SSIZE, DSIZE, SINC, DINC, SMOD, DMOD]
    * Set BCRn.
      * DSRn[DONE] must be cleared for channel startup.
  * After initialization, the channel can be started by setting DCRn[START] or setting DCRn[ERQ] and receiving a peripheral DMA request.
  * The hardware can clear DCRn[ERQ] once the byte-count reaches 0, by setting DCRn[D_REQ] (disable request).
* Dual-address tranfers:
  * If no error condition exists, DSRn[REQ] is set.
  * The DMA controller drives the SARn value onto the system address bus. If SINC is set, SARn is incremented upon a successful read. When the number of read cycles is complete (multiple reads if the destination size is larger than the source), the DMA initiates the write portion of the transfer.
    * If a termination error occurs, DSRn[BES, DONE] are set and the DMA stops.
  * The DMA controller drives the DARn value onto the system address bus. When the number of write cycles completes (multiple writes if the source size is larger than the destination). BCRn (byte-count) decrements. DSRn[DONE] is set when BCRn reaches zero. If BCRn is not zero, another read/write transfer is initiated, if continuous mode is enabled.
  * Auto-alignment: applies for transfers of large blocks of data. Does not apply for peripheral-initiated cycle-steal transfers.
    * if AA is set, overrides SINC,DINC.
    * if byte-count > 16, the address determines the transfer size. Else, the number of remaining bytes determines transfer size.
    * ex. if we wanted to transfer 240 bytes from 0x2000_0001, the transfer sizes are: 1, 2, 4*59, 1. (The read locations are aligned with 1/2/4-byte boundaries?)
  * the read data is temporarily held in the DMA channel hardware until the write operation..
    * only mentioned once in this chapter...
* Termination:
  * error conditions: DSRn[BES or BED]: error on the bus
  * interrupts: if DCRn[EINT] is set, the DMA drives the corresponding interrupt request signal. The processor reads DSRn to see if the transfer was successful or an error. Write a 1 to DSRn[DONE] to clear the interrupt, DONE bit, and error status bits.