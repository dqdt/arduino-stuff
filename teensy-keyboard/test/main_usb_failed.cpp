// http://kevincuzner.com/2014/12/12/teensy-3-1-bare-metal-writing-a-usb-driver/
// The path of least effort is to follow along with the blog post...
//
// I can't redefine usb_isr() so I will give up on writing my own USB drivers.
// It's beyond my skill level...

#include <Arduino.h>

typedef struct
{
    uint32_t desc; // buffer descriptor
    void *addr;    // buffer address in memory
} bdt_t;

__attribute__((section(".usbdescriptortable"), used)) // copied from usb_dev.c
bdt_t table[16 * 4];                                  // BDT table location
// each entry corresponds to one of 4 buffers in one of the 16 endpoints.
// RX/TX, even/odd (ping-pong buffers)
// ep0 is always the control endpoint

void usb_init()
{
    // 1. Select Clock Source
    //
    // (page 135 and 211)
    // The USB-FS OTG controller is a bus master attached to the crossbar switch.
    //   So it's clock is connected to the system clock.
    // It also needs a 48 MHz clock. Select the 48MHz clock source through the
    //   System Integration Module (SIM) registers.
    //
    // Note: MCGFLLCLK does not meet the USB jitter specification.
    //   So choose MCGPLLCLK or an external clock...
    SIM_SOPT2 |= SIM_SOPT2_PLLFLLSEL; // PLL with fixed divide by 2
    SIM_SOPT2 |= SIM_SOPT2_USBSRC;    // Select the MCG clock (over USB_CLKIN)

    // 2. USB Clock Gating
    //
    // (page 131)
    // Prior to initializing a module, set the corresponding bit in the SCGCx
    //   register to enable the clock. Before turning off the clock, make sure
    //   to disable the module.
    SIM_SCGC4 |= SIM_SCGC4_USBOTG;

    // 3. Reset USB Module (Software)
    //
    // USBx_CTL has a RESET bit, but only for Host mode. Used to reset peripherals?
    // USBx_USBTCR0 has a USBRESET bit, generates a hard reset to the USB_OTG module.
    //   Wait two clock cycles after setting this bit.
    //
    // (...doesn't mention RESET anywhere else in this chapter)
    USB0_USBTRC0 |= USB_USBTRC_USBRESET;
    __asm__ __volatile__("nop\n\t");
    __asm__ __volatile__("nop\n\t"); // ...I think this will waste 2 clock cycles

    // (page 662)
    // Undocumented: (but mentioned in the blog)
    //   USBx_TRC0 bit 6: Software must set this bit to 1.
    //   The bit is not labeled, but it needs to be set it to 1? (It's reset to 0.)
    USB0_USBTRC0 |= 0x40;

    // 4. Set BDT base registers
    //
    // The BDT base address is aligned at 512-byte boundaries, so the lowest 9 bits
    //   are always zero. The BDTPAGEx registers contain the rest of the address.
    //
    // USB0_BDTPAGE1[7:1] [15:9]
    // USB0_BDTPAGE2[7:0] [23:16]
    // USB0_BDTPAGE3[7:0] [31:24]
    USB0_BDTPAGE1 = ((uint32_t)table >> 8);
    USB0_BDTPAGE2 = ((uint32_t)table >> 16);
    USB0_BDTPAGE3 = ((uint32_t)table >> 24);

    // 5. Clear all USB ISR flags, and enable weak pulldowns (?)
    //
    // Which registers contain ISR flags???
    //
    // Interrupt Enable/Control Register: Enables the corresponding status bits.
    // Interrupt Status Register: Records changes in signals. Software can read
    //   this register to determine the event that triggered the interrupt.
    //   Writing a one to a bit clears the associated interrupt. (page 645)
    //
    // Control registers are reset to 0, so they are disabled (?)
    //
    // OTGISTAT records changes in ID sense and VBUS signals.
    USB0_OTGISTAT |= 0xFF;
    // ISTAT contains fields for each of the interrupt sources in the USB module.
    USB0_ISTAT |= 0xFF;
    // ERRSTAT contains fields for error sources. It is OR'd and compressed into
    //   the ERROR bit in the ISTAT register.
    USB0_ERRSTAT |= 0xFF;
    // Enables weak pulldowns (page 661)
    USB0_USBCTRL |= USB_USBCTRL_PDE;

    // 6. Enable USB Reset Interrupt
    //
    // (page 655)
    // USB Enable
    // Setting this bit causes the SIE to reset all of its ODD bits to the BDTs.
    // Therefore, setting this bit resets much of the logic in the SIE.
    USB0_CTL |= USB_CTL_USBENSOFEN; // USB enable, SOF enable (in host mode) ?
    USB0_USBCTRL = 0;               // disables weak pulldowns...

    USB0_INTEN |= USB_INTEN_USBRSTEN; // USB RESET interrupt enable
    NVIC_ENABLE_IRQ(IRQ_USBOTG);      // 24

    // 7. Enable pullup resistor
    //
    // (page 662)
    // Provides control of the DP Pullup in the USB OTG module, if USB is
    //   configured in non-OTG device mode.
    USB0_CONTROL = USB_CONTROL_DPPULLUPNONOTG; // DP PULLUP NON-OTG

    // 8. Waiting for Host connection
}

void usb_reset()
{
    // (page 650)
    // USBx_ISTAT USBRST bit
    //     This bit is set when the USB Module has decoded a valid USB reset. This informs the processor that it
    // should write 0x00 into the address register and enable endpoint 0. USBRST is set after a USB reset has
    // been detected for 2.5 microseconds. It is not asserted again until the USB reset condition has been
    // removed and then reasserted.
}

void usb_isr()
{
    // digitalWrite(2, HIGH);
}

int main()
{
    Serial2.begin(38400);

    while (1)
    {
        Serial2.println("hello from Teensy");
        delay(1000);
    }
}