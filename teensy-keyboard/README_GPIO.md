A port register has 32 bits, so you would think it supports 32 pins. However, not all bits are used, and adjacent pins on the Teensy-LC are not adjacent bits of the same port. It's kind of random (?)
* https://www.nxp.com/part/MKL26Z64VFT4#/
* (page 53) https://www.nxp.com/docs/en/data-sheet/KL26P64M48SF5.pdf

* These port pins are exposed on the 48-QFN package.
  * `Port A: 0,1,2,3,4,18,19,20`
  * `Port B: 0,1,2,3,16,17`
  * `Port C: 0,1,2,3,4,5,6,7`
  * `Port D: 0,1,2,3,4,5,6,7`
  * `Port E: 20,21,24,25,29,30`

|Teensy-LC Pin|Port A|Port B|Port C|Port D|Port E|
|-|-|-|-|-|-|
|0|-|16|-|-|-|
|1|-|17|-|-|-|
|2|-|-|-|0|-|
|3|1|-|-|-|-|
|4|2|-|-|-|-|
|5|-|-|-|7|-|
|6|-|-|-|4|-|
|7|-|-|-|2|-|
|8|-|-|-|3|-|
|9|-|-|3|-|-|
|10|-|-|4|-|-|
|11|-|-|6|-|-|
|12|-|-|7|-|-|
|13|-|-|5|-|-|
|14|-|-|-|1|-|
|15|-|-|0|-|-|
|16|-|0|-|-|-|
|17|-|1|-|-|-|
|18|-|3|-|-|-|
|19|-|2|-|-|-|
|20|-|-|-|5|-|
|21|-|-|-|6|-|
|22|-|-|1|-|-|
|23|-|-|2|-|-|
|24|-|-|-|-|20?|
|25|-|-|-|-|21?|
|26|-|-|-|-|30|
* `core_pins.h` (line 52) comments a pin mapping but it is slightly different...

The memory mapped register defines are in `kinetis.h`.

### GPIO
* `GPIOx_PDOR` Port Data Output Register
* `GPIOx_PSOR` Port Set Output Register (set, clear, or toggle PDOR, for efficient bit manipulation)
* `GPIOx_PCOR` Port Clear Output Register
* `GPIOx_PTOR` Port Toggle Output Register
* `GPIOx_PDIR` Port Data Input Register (Port Control and Interrupt module must be enabled first)
* `GPIOx_PDDR` Port Data Direction Register (0 = input, 1 = output)
* Fast GPIO (`FGPIOx_xxxx`) can access these registers in a single cycle (??)

To use GPIO, we first need to initialize the Port Control and Interrupt module for that port.
* Datasheet (page 176) shows the signals that can be connected to each external pin.
  * The Port Control module selects which signal is connected to the external pin.
    * (can be GPIO, UART, SPI, Timer/PWM, etc)
* `PORTx_PCRn` Pin Control Register n
  * Each pin has its own 32-bit register for configuration.
  * 3-bit MUX field selects what is connected to the external pin. `001` selects GPIO.
* `PORTx_GPCLR`, `PORTx_GPCHR` Global Pin Control Low/High Register
  * Allows writing to multiple Pin Control Registers with the same value.
  * `Low Register` writes to pins `15 to 0` and `High Register` writes to pins `31 to 16`.
  * For each `GPCxR` Register, bits `[31:16]` = enable (write data to this pin), and bits `[15:0]` is the data to write.
    * Assuming we only write to the lower 16 bits of `PCRn` (don't need to write to the upper 16 bits).
* Example: Pin 2 is Port D, pin 0.
  * ```
    PORTD_GPCLR = (1 << 16) | PORT_PCR_MUX(1); // MUX selects GPIO for the 0-th pin
    GPIOD_PDDR |= 1;                           // pinMode OUTPUT
    ```