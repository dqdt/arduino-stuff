// Looked at this for help:
// https://shawnhymel.com/649/learning-the-teensy-lc-manual-pwm/
//
// TPMx_SC[CPWMS and PS] fields are write protected. It can be written only
//   when the counter is disabled.
// I guess that's a hint to disable and reset everything first...
//
// I think the TPM counter clock is 48MHz.
//   Set Prescale=128, MOD=62500-1, toggle on output compare.
//   48e6/128/62500 = 6 toggles/second. The LED should blick 3 times/sec.
//
//
//
//

#include <Arduino.h>

void setup()
{
    // Do we need to initialize MCG stuff?

    // MCGFLLCLK, or MCGPLLCLK/2
    SIM_SOPT2 |= SIM_SOPT2_TPMSRC(1) | SIM_SOPT2_PLLFLLSEL; // should be redundant

    // Enable clock gate for Port B
    SIM_SCGC5 |= SIM_SCGC5_PORTB; // should be redundant

    // Port B Pin 1 Alt 3:  Connect pin to TPM1_CH1
    PORTB_PCR1 &= ~PORT_PCR_MUX_MASK;
    PORTB_PCR1 |= PORT_PCR_MUX(3);

    // Disable the TPM module first.
    // Note: This does not affect CNT or MOD registers (pg.582)
    // For some reason, it doesn't update if _SC is set right after clearing it.
    TPM1_SC = 0;
    // TPM1_SC = (1 << 3) | 0b111; // set it after setting CNT and MOD

    // Counter is 16-bit
    // Write to CNT before writing to MOD.
    TPM1_CNT = 0;

    // Modulo: max value of counter
    TPM1_MOD = 62500 - 1;

    // Now enable the TPM.
    // CMOD = 1, increment on TPM Counter clock
    // Prescale = 128 (pg.573)
    TPM1_SC = (1 << 3) | 0b111;

    // Channel (n)

    // Output compare value
    TPM1_C1V = TPM1_MOD;

    // Channel must be disabled first and this must be acknowledged by the
    //   TPM counter clock domain (write 0 to the register) (pg.575)
    // Toggle output on match
    TPM1_C1SC = 0;
    TPM1_C1SC = 0b0101 << 2;
}

void loop()
{
    // delay(1000);
    Serial.print(TPM1_SC);
    Serial.print(' ');
    Serial.print(TPM1_CNT);
    Serial.print(' ');
    Serial.print(TPM1_MOD);
    Serial.println();
    // TPM1_CNT = 0;
    delay(1000);
}