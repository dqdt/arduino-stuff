// This is an attempt to read a rotary encoder using interrupts.
// The interrupt triggers on an edge change on either CLK or DT pins.
// Sometimes it misses turns. At the moment it's unreliable and idk how to solve it.
// Better results when we just poll inside main().

#include <Arduino.h>

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
    void write(int pin, int val)
    {
        PinRegs p = pin_regs[pin];
        if (val)
        {
            *(p.psor) = p.mask;
        }
        else
        {
            *(p.pcor) = p.mask;
        }
    }
};

Pins pins;

const int ledPins[] = {2, 3, 4, 5};
const int dtPin = 14;
const int clkPin = 15;

uint32_t state = 0; // [c][b][a]
volatile int val = 0;

// incorrect readings...
// int clkPrev = 0;
// void portcd_isr(void)
// {
//     int clk = pins.read(clkPin);
//     int dt = pins.read(dtPin);
//     if (clk != clkPrev)
//     {
//         if (clk != dt)
//         {
//             val++;
//         }
//         else
//         {
//             val--;
//         }
//         clk = clkPrev;
//     }
//     PORTC_PCR0 |= (1 << 24); // Clear the Interrupt Status Flag (ISF) bit
//     PORTD_PCR1 |= (1 << 24);
// }

// misses some turns
void portcd_isr(void)
{
    int clk = pins.read(clkPin);
    int dt = pins.read(dtPin);

    int a = (clk << 1) | dt;
    int b = state & 0x3;
    if (a != b)
    {
        // state = ((state << 2) | a) & 0x3F;
        state = (state << 2) | a;
        switch (state & 0x3F)
        {
        case 0b001011:
            val++;
            break;
        case 0b110100:
            val++;
            break;
        case 0b000111:
            val--;
            break;
        case 0b111000:
            val--;
            break;
        }

        // for (int i = 31; i >= 0; i--)
        // {
        //     Serial.print(!!(state & (1 << i)));
        //     if (i && ((i & 1) == 0))
        //     {
        //         Serial.print('|');
        //     }
        // }
        // Serial.println();
    }

    PORTC_PCR0 |= (1 << 24); // Clear the Interrupt Status Flag (ISF) bit
    PORTD_PCR1 |= (1 << 24);
}

int main()
{
    Serial.begin(115200);

    for (auto pin : ledPins)
    {
        pins.gpio_init(pin);
        pins.ddr_output(pin);
    }

    pins.gpio_init(dtPin);
    pins.gpio_init(clkPin);
    pins.ddr_input(dtPin);
    pins.ddr_input(clkPin);

    // Enable pin change interrupt on clkPin(15) and dtPin(14).
    // bits [19:16]:  1011 Interrupt on either edge.
    PORTC_PCR0 &= ~(0b1111 << 16);
    PORTC_PCR0 |= (0b1011 << 16); // either edge

    PORTD_PCR1 &= ~(0b1111 << 16);
    PORTD_PCR1 |= (0b1011 << 16); // either edge

    // Loops can get optimized away (?)
    // * https://wiki.sei.cmu.edu/confluence/display/c/MSC06-C.+Beware+of+compiler+optimizations
    // * while-loops with constant expressions will not get optimized away.
    while (1)
    {
        // This for-loop could get optimized away (?)
        // When "val" was not volatile, none of the LEDs would light up
        //   (was it assuming val was always 0?)

        // Set LEDs based on lowest 4 bits of counter value.
        int x = val;
        for (int i = 0; i < 4; i++)
        {
            pins.write(ledPins[i], x & (1 << i));
        }
    }
}

// global
//
// const int buf_len = 256;
// int clk_buf[buf_len];
// int dt_buf[buf_len];
// int buf_ri = 0;
// int buf_wi = 0;

// isr
//
// int clk = pins.read(clkPin);
// int dt = pins.read(dtPin);
// clk_buf[buf_wi] = clk;
// dt_buf[buf_wi] = dt;
// buf_wi = (buf_wi + 1) % buf_len;

// main
//
// bits [19:16]:  1011 Interrupt on either edge.
// PORTC_PCR0 &= ~(0b1111 << 16);
// // PORTC_PCR0 |= (0b1011 << 16); // either edge
// PORTC_PCR0 |= (0b1001 << 16); // rising edge
//
// while (buf_ri != buf_wi)
// {
//     Serial.print(clk_buf[buf_ri]);
//     Serial.print(' ');
//     Serial.println(dt_buf[buf_ri]);
//     buf_ri = (buf_ri + 1) % buf_len;
// }
