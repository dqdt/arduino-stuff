How do interrupts work on the Teensy-LC?
* There is a Nested Vector Interrupt Controller (NVIC) 
  * (datasheet page 57)
  * See the ARM Cortex-M0+ Technical Reference Manual for this?
  * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0497a/Cihgjeed.html
* https://www.pjrc.com/teensy/interrupts.html
  * wait... this is for AVR, not ARM
* https://shawnhymel.com/661/learning-the-teensy-lc-interrupt-service-routines/

NVIC that features:
* 32 external interrupt inputs, each with four levels of priority
* non-maskable interrupt input (?)
* level-sensitive and pulse-sensitive interrupt lines
* wake-up interrupt controller
* relocation of the vector table (?)

Exception handling:
* To minimize interrupt latency, software abandons any instructions that take many cycles, and restarts them after the intterupt.
* To be jitter-free, it should take a fixed number of cycles to begin the interrupt.
  * interrupt late-arrival
    * while inside an interrupt, another interrupt with higher priority occurs
  * interrupt tail-chaining
    * process interrupts back-to-back, no need to save state to the stack

NVIC:
* interrupt signals (from modules) connect to the NVIC, and the NVIC prioritizes the interrupts
* Registers:
  * `NVIC_ISER` Interrupt Set-Enable Register
  * `NVIC_ICER` Interrupt Clear-Enable Register
    * (write 1 to the bit to enable it)
    * an interrupt becomes "pending" when the event is detected and the corresponding interrupt routine is not "active" (running)
  * `NVIC_ISPR` Interrupt Set-Pending Register
  * `NVIC_ICPR` Interrupt Clear-Pending Register
    * software must clear the "pending" bit (?)
      * we can disable interrupts but still poll the pending bit
  * `NVIC_IPR0-7` Interrupt Priority Registers
    * (the priority is stored in 1 byte for each interrupt. `IPR0` has the bytes for interrupts `0`,`1`,`2`,`3`)

Interrupt vector table:
* Interrupt vector table: table of interrupt vectors
* Interrupt vector: address of an interrupt handler
* (Datasheet page 58)
  * There are system core interrupt handlers, and non-core interrupts (these start at index 16 of the table)
* I see `extern void x_isr(void)` functions in `kinetis.h`. 
* `_VectorsFlash` is defined in `mk20dx128.c`.


NVIC usage:
* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0497a/Cihgjeed.html
* Software uses the `CPSIE i` and `CPSID i` instructions to enable and disable interrupts. There are also functions on the Teensy which are named similarly:
```c
#define __disable_irq() __asm__ volatile("CPSID i":::"memory");
#define __enable_irq()	__asm__ volatile("CPSIE i":::"memory");
// same as cli(), sei(), interrupts(), noInterrupts()
#define NVIC_ENABLE_IRQ(n)
#define NVIC_DISABLE_IRQ(n)
#define NVIC_SET_PENDING(n)
#define NVIC_CLEAR_PENDING(n)
#define NVIC_IS_ENABLED(n)
#define NVIC_IS_PENDING(n)
#define NVIC_IS_ACTIVE(n)
```

Ok, the PORT module allows triggering interrupts but you have to manually clear it from triggering again, and it is separate from the NVIC?
* Ex: Pin 15 is Port C, Pin 0. I want to generate an interrupt on pin change.
  ```c
  void portcd_isr(void)
  {
      PORTC_PCR0 |= 1 << 24; // Clear the Interrupt Status Flag (ISF) bit
  }
  int main()
  {
      // IRQC bits [19:16]: 1011 Interrupt on either edge.
      PORTC_PCR0 &= ~(0b1111 << 16);
      PORTC_PCR0 |= (0b1011 << 16);
  }
  ```
* I think I get it. The PORT control module generates the interrupt for the NVIC.

Arduino has `attachInterrupt` 

