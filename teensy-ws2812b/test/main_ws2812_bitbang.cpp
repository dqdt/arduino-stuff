#include <Arduino.h>

const int NUM_LED = 20;
uint8_t leds[3 * NUM_LED] = {0};

// const int max_brightness = 100;
// const int max_brightness = 1;

// pin 17 = GPIO B, pin 1
void bitbang_leds()
{
    __disable_irq();
    for (int i = 0; i < 3 * NUM_LED; i++)
    {
        uint8_t val = leds[i];
        for (int j = 0; j < 8; j++)
        {
            GPIOB_PSOR = 0b10; // set the pin
            if (val & 0x80)
            {
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 5
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 10
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 15
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 20
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 25
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 30
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 35
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 40
                GPIOB_PCOR = 0b10;   // clear the pin
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 5
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 10
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 15
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 20
            }
            else
            {
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 5
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 10
                GPIOB_PCOR = 0b10;   // clear the pin
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 5
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 10
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 15
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 20
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 25
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 30
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 35
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 40
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 45
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop"); // 50
            }
            val <<= 1;
        }
    }
    __enable_irq();
}

void setup()
{
    // for (int i = 0; i < 3 * NUM_LED; i += 9)
    // {
    //     leds[i] = max_brightness;
    // }
    // for (int i = 4; i < 3 * NUM_LED; i += 9)
    // {
    //     leds[i] = max_brightness;
    // }
    // for (int i = 8; i < 3 * NUM_LED; i += 9)
    // {
    //     leds[i] = max_brightness;
    // }

    pinMode(17, OUTPUT);
    Serial.begin(115200);
}

double phase_r = 0;
double phase_g = TWO_PI / 3;
double phase_b = TWO_PI * 2 / 3;

void loop()
{
    for (int i = 0; i < NUM_LED; i++)
    {
        double r = cos(PI * i / NUM_LED + phase_r);
        double g = cos(PI * i / NUM_LED + phase_g);
        double b = cos(PI * i / NUM_LED + phase_b);

        leds[3 * i + 0] = 200 * r * r;
        leds[3 * i + 1] = 200 * g * g;
        leds[3 * i + 2] = 200 * b * b;
    }
    phase_r += 0.03;
    phase_g += 0.03;
    phase_b += 0.03;
    // for (int i = 0; i < 3 * NUM_LED; i++)
    // {
    //     Serial.print(leds[i]);
    //     Serial.print(' ');
    // }
    // Serial.println();
    bitbang_leds();
    delayMicroseconds(50);
}