// Now for WS2812B:
//   Want a period of 1.25e-6 secs, or 800kHz.
//   48e6 / Prescale / (MOD+1) = 800e3
//   48e6 / 1 / 60 = 800e3
// Prescale=1, MOD=59. 60 clock cycles per 1.25e-6 secs.

#include <Arduino.h>

void setup()
{
    // MCGFLLCLK, or MCGPLLCLK/2
    SIM_SOPT2 |= SIM_SOPT2_TPMSRC(1) | SIM_SOPT2_PLLFLLSEL; // should be redundant

    // Enable clock gate for Port B
    SIM_SCGC5 |= SIM_SCGC5_PORTB; // should be redundant

    // Port B Pin 1 Alt 3:  Connect pin to TPM1_CH1
    PORTB_PCR1 &= ~PORT_PCR_MUX_MASK;
    PORTB_PCR1 |= PORT_PCR_MUX(3);

    // Disable the TPM module first.
    TPM1_SC = 0;

    // Counter is 16-bit
    // Write to CNT before writing to MOD.
    TPM1_CNT = 0;

    // Modulo: max value of counter
    TPM1_MOD = 60 - 1;

    // Prescale = 1
    TPM1_SC = (1 << 3) | 0b1;

    // Output compare value
    // TPM1_C1V = 40;
    // TPM1_C1V = 13; // 13 seems to be mistaken for ON
    TPM1_C1V = 12; // anything higher seems to be ON

    // Channel must be disabled first.
    // Edge-aligned PWM: High-true pulses (set on reload, clear on match)
    TPM1_C1SC = 0;
    TPM1_C1SC = 0b1010 << 2;
}

IntervalTimer a;

void loop()
{
    Serial.print(TPM1_SC);
    Serial.print(' ');
    Serial.print(TPM1_CNT);
    Serial.print(' ');
    Serial.print(TPM1_MOD);
    Serial.println();
    delay(1000);
}