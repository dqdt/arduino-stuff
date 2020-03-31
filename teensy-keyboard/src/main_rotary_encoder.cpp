// This is a copy of the rotary encoder code, but without pinMode and digitalWrite.
// For Teensy-LC.

#include <Arduino.h>

// this is kinda stupid...?
struct Pins
{
    struct PinRegs
    {
        const uint32_t mask; // which bit of the port

        volatile uint32_t *gpcxr; // global pin control (write)

        volatile uint32_t *pdor; // data output
        volatile uint32_t *psor; // set output (write)
        volatile uint32_t *pcor; // clear output (write)
        volatile uint32_t *ptor; // toggle output (write)

        volatile uint32_t *pdir; // data input (read)

        volatile uint32_t *pddr; // data direction
    };

    const PinRegs pin_regs[16] = {

        /* 0*/ PinRegs{},
        /* 1*/ PinRegs{},
        /* 2*/ PinRegs{1 << 0, &PORTD_GPCLR, &GPIOD_PDOR, &GPIOD_PSOR, &GPIOD_PCOR, &GPIOD_PTOR, &GPIOD_PDIR, &GPIOD_PDDR},
        /* 3*/ PinRegs{1 << 1, &PORTA_GPCLR, &GPIOA_PDOR, &GPIOA_PSOR, &GPIOA_PCOR, &GPIOA_PTOR, &GPIOA_PDIR, &GPIOA_PDDR},
        /* 4*/ PinRegs{1 << 2, &PORTA_GPCLR, &GPIOA_PDOR, &GPIOA_PSOR, &GPIOA_PCOR, &GPIOA_PTOR, &GPIOA_PDIR, &GPIOA_PDDR},
        /* 5*/ PinRegs{1 << 7, &PORTD_GPCLR, &GPIOD_PDOR, &GPIOD_PSOR, &GPIOD_PCOR, &GPIOD_PTOR, &GPIOD_PDIR, &GPIOD_PDDR},
        /* 6*/ PinRegs{},
        /* 7*/ PinRegs{},
        /* 8*/ PinRegs{},
        /* 9*/ PinRegs{},
        /*10*/ PinRegs{},
        /*11*/ PinRegs{},
        /*12*/ PinRegs{},
        /*13*/ PinRegs{},
        /*14*/ PinRegs{1 << 1, &PORTD_GPCLR, &GPIOD_PDOR, &GPIOD_PSOR, &GPIOD_PCOR, &GPIOD_PTOR, &GPIOD_PDIR, &GPIOD_PDDR},
        /*15*/ PinRegs{1 << 0, &PORTC_GPCLR, &GPIOC_PDOR, &GPIOC_PSOR, &GPIOC_PCOR, &GPIOC_PTOR, &GPIOC_PDIR, &GPIOC_PDDR},
    };

    void gpio_init(int pin)
    {
        PinRegs p = pin_regs[pin];
        *(p.gpcxr) = (p.mask << 16) | PORT_PCR_MUX(1);
    }
    void ddr_input(int pin)
    {
        PinRegs p = pin_regs[pin];
        *(p.pddr) &= ~p.mask;
    }
    void ddr_output(int pin)
    {
        PinRegs p = pin_regs[pin];
        *(p.pddr) |= p.mask;
    }
    int read(int pin)
    {
        PinRegs p = pin_regs[pin];
        return *(p.pdir) & p.mask ? 1 : 0;
    }
    void set(int pin)
    {
        PinRegs p = pin_regs[pin];
        *(p.psor) = p.mask;
    }
    void clear(int pin)
    {
        PinRegs p = pin_regs[pin];
        *(p.pcor) = p.mask;
    }
    void toggle(int pin)
    {
        PinRegs p = pin_regs[pin];
        *(p.ptor) = p.mask;
    }
};
// void pin2_init_gpio() { PORTD_GPCLR = ((1 << 0) << 16) | PORT_PCR_MUX(1); };
// void pin2_ddr_input() { GPIOD_PDDR &= ~1; }
// void pin2_ddr_output() { GPIOD_PDDR |= 1; }
// void pin2_set() { GPIOD_PSOR |= 1; }
// void pin2_clear() { GPIOD_PCOR |= 1; }
// void pin2_toggle() { GPIOD_PTOR |= 1; }

int main()
{
    Serial.begin(115200);

    Pins pins;

    const int ledPins[] = {2, 3, 4, 5};
    for (auto pin : ledPins)
    {
        pins.gpio_init(pin);
        pins.ddr_output(pin);
    }

    const int dtPin = 14;
    const int clkPin = 15;
    pins.gpio_init(dtPin);
    pins.gpio_init(clkPin);
    pins.ddr_input(dtPin);
    pins.ddr_input(clkPin);

    int val = 0;
    int clkPrev = pins.read(clkPin);

    // cli();
    while (1)
    {
        int clk = pins.read(clkPin);
        int dt = pins.read(dtPin);

        // Check rotary encoder and update counter value
        if (clk != clkPrev)
        {
            if (clk != dt)
            {
                val++;
            }
            else
            {
                val--;
            }
            Serial.println(val);
        }
        clkPrev = clk;

        // Set LEDs based on lowest 4 bits of counter value.
        for (int i = 0; i < 4; i++)
        {
            if (val & (1 << i))
            {
                pins.set(ledPins[i]);
            }
            else
            {
                pins.clear(ledPins[i]);
            }
        }
    }
}