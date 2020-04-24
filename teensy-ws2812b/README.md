Driving WS2812B rgb leds with a Teensy-LC. 

### In the past I learned about driving WS2812Bs with an arduino:
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

### Ways to drive WS2812B:
* there's a rabbit hole of links...
  * https://hackaday.com/2014/09/10/driving-ws2812b-pixels-with-dma-based-spi/
  * https://www.instructables.com/id/My-response-to-the-WS2811-with-an-AVR-thing/
  * https://github.com/FastLED/FastLED/wiki/SPI-Hardware-or-Bit-banging
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

### DMA (Direct Memory Access) on the Teensy:
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

### Datasheet Chaper 22: DMA Multiplexer
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
  
### Datasheet Chapter 23: DMA Controller Module
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


### Timer/PWM
* Teensy-LC Pin 17 is PWM. Let's try it, as I haven't used it yet.

### Datasheet Chapter 31: Timer/PWM Module (TPM)
* The TPM is built on the HCS08 TPM used on Freescale's 8-bit MCUs, and modified to run in low power modes by clocking the counter, compare and capture registers on an asynchronous clock.
* Modes of operation:
  * Debug mode: can be configured to pause counting and ignore trigger inputs / and input capture events.
  * Doze mode: can be configured to pause counting  ?????
  * Stop mode: TPM counter clock can still run, and TPM can generate an asynchronous interrupt to exit the MCU from stop mode
* Block diagram:
  * clock select -> prescalar -> module counter -> (timer overflow interrupt) and connected to (n) channels ??
  * each TPM channel can be configured as input or output
* Registers:
  * TPMx_SC: Status and Control
    * DMA: Enable DMA transfers for the overflow flag, TOF: Timer overflow flag, TOIE: Timer overflow interrupt enable, CPWMS: Center-aligned PWM select (operate in up-down counting mode), CMOD: Clock mode select, PS: Prescale factor select (2^[0-7])
  * TPMx_CNT: Counter value. Writing any value will clear the counter. Reading this register adds two wait states due to synchronization delays (??)
  * TPMx_MOD: Modulo: When the TPM counter reaches the modulo value and increments, the TOF flag is set and the next value depends on the counting method. Writing to MOD actually writes to a buffer, and the register is updated later. Write to CNT before writing to MOD, so to avoid confusion as to when overflow occurs.
  * TPMx_CnSC: Channel (n) Status and Control:
    * When switching from one channel mode to a different channel mode, disable the channel first.
    * Modes: Software compare, Input Capture (capture on rising edge,falling, or both), Output Compare (set/clear/toggle on match, pulse output low/high on mactch), Edge-aligned PWM (high-true pulses (clear output on match, set output on reload), low-true pulses (clear on reload, set on match)), Center-aligned PWM (high-true pulses(clear on match-up, set on match-down ???), low-true pulses)
    * CHF: Set by hardware when an event occurs on the channel. Clear by writing 1. CHIE: Channel interrupt enable. MSB, MSA: Channel Mode select. ELSB,ELSA: Edge or level select. DMA: DMA Enable.
  * TPMx_CnV: Channel (n) Value: contains the captured TPM counter value for input modes, or the match value for output modes. Writes to CnV will latch it to a buffer, and subsequent writes are ignored until the register has been updated.
  * TPMx_STATUS: Capture and compare status:
    * Contains a copy of the status flag CnSC[CHnF] for each TPM channel, and SC[TOF], for software convenience. All ChnF bits can be checked with one read.
  * TPMx_CONF: Configuration
    * this register selects the behavior in debug and wait modes and the use of an external global time base (??)
    * TRGSEL: Trigger select for starting the counter and/or reloading the counter, CROT: Counter reload on trigger, CSOO: Counter stop on overflow, CSOT: Counter start on trigger, GTBEEN: Global time base enable (use this instead of the internal TPM counter), DBGMODE: Debug mode, DOZEEN: Doze enable: configures for wait mode
    * ugh
* Clock domains:
  * bus clock domain is used by the register interface and for synchronizing interrupts and DMA requests
  * TPM counter clock domain is used to clock the counter and prescalar along with the output compare and input capture logic. The counter clock is considered asynchronous to the bus clock.
  * CMOD[1:0] either disables the TPM counter or selects a clock mode for the TPM counter.
  * An external clock input passes through a synchronizer clocked by the TPM counter clock...
  * The selected clock source passes through a prescalar that is a 7-bit counter.
* Up-counting is selected when SC[CPWMS] = 0
  * counts from 0 to MOD, reset back to 0...
    * MOD+1 counts in a period
    * TOF overflow flag is set at the same time the counter is reset to zero.
* Up-down counting is selected when SC[CPWMS] = 1
  * MOD cannot be less than 2.
  * counts from 0 to MOD to 0...
    * 2*MOD counts in a period
    * TOF flag is set at the same time MOD decrements to MOD-1.
* Reset: Any write to CNT resets the TPM counter and channel outputs to their initial values.
* Global time base (GTB):
  * allows multiple TPM modules to share the same timebase. The local TPM channels use the counter value, counter enable, and overflow indication from the TPM generating the global time base. 
* Counter trigger:
  * TPM counter can start,stop,or reset in response to a hardware trigger input. The trigger input is synchronized to the asynchronous counter clock, so there is a 3 counter clock delay between the trigger assertion and the counter responding.
* Input capture mode:
  * when a selected edge occurs on the channel input, the current value of the TPM counter is captured in the CnV register. At the same time, CHnF bit is set and an interrupt is generated if CHnIE.
* Output compare mode:
  * The TPM can generate timed pulses with programmable position, polarity, duration, and frequency.
  * When the counter matches the value in the CnV register, the channel (n) output can be set,cleared,toggled, or pulsed high or low for as long as the counter matches the value in the CnV register.
* Edge-aligned PWM
  * Count from 0 to MOD. Set on overflow, clear on CnV (or negated pulse). The start edge of the pulse is aligned with the beginning of the period (overflow), and is the same for all channels within a TPM.
* Center-aligned PWM
  * The pulses are centered for all channels within a TPM.
  * Counts from 0 to MOD to 0. Duty cycle is 2 * CnV and period is 2 * MOD. 
  * When counting down, set after seeing CNT==CnV+1. When counting up, clear after seeing CNT==CnV-1. So it's HIGH when the count is [CnV .. 0 .. CnV-1].
  * CHnF bit is set at both compare matches when counting up and down
* Updating MOD/CnV registers:
  * If clock is disabled, MOD is updated on write. If the clock is enabled, and if the mode is not CPWM, then MOD is updated after overflow (this prevents the counter from being above MOD). If the mode _is_ CPWM, it's updated when MOD goes to MOD-1 (begins decrementing).
  * Updates to CnV is delayed similarly.
* DMA:
  * The channel generates a DMA transfer according to DMA and CHnIE bits.
  * CHnF is cleared by the hardware when the DMA transfer is done, or if software writes a 1 to the CHnF bit.
* Output triggers:
  * Counter trigger asserts whenever the TOF is set and remains asserted until the next increment.
  * Each TPM channel generates a pre-trigger and trigger output.
    * Pre-trigger asserts when CHnF is set, the Trigger asserts on the counter increment after the pre-trigger asserts, and both are cleared on the next counter increment.
* Interrupts:
  * Timer overflow:  TOIE = 1 and TOF = 1
  * Channel (n); CHnIE = 1 and CHnF = 1

### Now what
* https://www.pjrc.com/non-blocking-ws2812-led-library/
* Let's try bit-banging the WS2812B data signal (disable interrupts). I don't know if this will interfere with the USB interrupts...
  * How can I bit-bang in assembly if I don't know the instruction set, or how it's done in ARM? Is it worth trying?
  * Can we bit-bang in software by checking the elapsed time / clock cycles?
  * The `micros()` function seems to rely on "systick" to increment a counter...
* System timer -- SysTick ARMv7-M Architecture Reference Manual, B3.3
  * The ARMv7-M implementation must include a system timer, SysTick, that provides a 24-bit clear-on-write, decrementing, wrap-on-zero counter with a flexible control mechanism.
  * Registers:
    * Control and status: config, enable the counter, enable interrupt, counter status
    * Counter reload: `SYST_RVR` wrap value for the counter (?). Reloading is called wrapping.
    * Counter current value: `SYST_CVR` goes to zero, then to SYST_RVR, and decrements again
    * Calibration value: the preload value for a 10ms (100Hz) system clock
  * When the counter transitions to zero, `COUNTFLAG` status bit is set.
  * Software can use the calibration value TENMS to scale the counter to other desired clock rates within the dynamic range of the counter. (???)
* On blocking:*
  * https://github.com/FastLED/FastLED/wiki/Interrupt-problems
  * The signal for the WS2812B takes quite a long time. If you need to receive serial data but have interrupts disabled, then the serial buffer might overflow. What FastLED does for ARM mcus is to briefly re-enable interrupts between LEDs. As long as the interrupts take less than 5us (or whatever the reset time is for the LEDs) then it will be OK.

### Timers
* Three low-power TPM modules, all have basic TPM function, no quadrature decoder (?) and can be functional in Stop/VLPS mode (??).
  * TPM0 has 6 channels, while TPM1/2 have 2 channels.
* Teensy-LC Pin17 is `PTB1` (Datasheet pg.178) which is has TPM1_CH1 as the ALT3
* Configure it with the System Intergration Module (Chap.12) registers:
  * SIM_SOPT2
    * TPMSRC: TPM Clock Source Select
      * mk20dx128 already sets this register, and chooses: MCGFLLCLK clock, or MCGPLLCLK/2
    * PLLFLLSEL ??
  * SIM_SOPT4
    * TPM[0-2]CLKSEL: External Clock Pin Select: select TPM_CLKIN[0-1]
    * TPM[0-2]CH0SRC: Channel 0 Input Capture Source Select: select TPM1_CH0 signal, CMP0 output, USB start of frame pulse
  * SIM_SCGC5:
    * PORT[A-E] clock gate control
      * it's already set to enable all ports
  * SIM_SCGC6:
    * TPM[0-2] clock gate control
      * it's set to enable ADC0, TPM[0-2], FTF
      * not enabled: DAC, RTC, PIT, I2S, DMAMUX
  * SIM_SCGC7:
    * DMA clock gate control
  * DMA and DMAMUX are initialized in `DMAChannel.cpp`
  * SIM_CLKDIV1 ???
    * OUTDIV1  sets the divide for the core/system clock, and bus/flash clock
    * OUTDIV4  sets the divide for the bus/flash clock, in addition to the system clock divide ratio
* Pin Control Register n (PORTx_PCRn)
  * ISF: Interrupt status flag: if the pin is configured to generate a DMA request, then the corresponding flag will be cleared when the DMA completes.
  * IRQC: Interrupt configuration: can be DMA request on rising/falling/either edge 
  * MUX: Select which ALTx the pin is connected to

* Selecting the clock for TPM:
  * (pg.135)
  * SIM_SOPT2[PLLFLLSEL], SIM_SOPT2[TPMSRC]

Took note of how this guy initialized the TPM:
* https://shawnhymel.com/649/learning-the-teensy-lc-manual-pwm/

A better explanation of the DMA writing to GPIO port SET/CLEAR registers:
* https://mcuoneclipse.com/2015/08/05/tutorial-adafruit-ws2812b-neopixels-with-the-freescale-frdm-k64f-board-part-5-dma/
* I think you'd have to turn off the timers ASAP after the last falling edge, or else it will send another pulse...
* WAIT! If there is no more data remaining, nothing will be written to the port registers! We don't HAVE to turn off the timers (but do it to save power).
  
Counting cycles? `ARM_DWT_CYCCNT`
* aww. Doesn't look like Teensy LC (Cortex M0+) has CYCCNT.
* It's used in AudioStream.cpp, but on the PJRC page, the TeensyLC is not supported.
* https://forum.pjrc.com/threads/28407-Teensyduino-access-to-counting-cpu-cycles?p=71036&viewfull=1#post71036
