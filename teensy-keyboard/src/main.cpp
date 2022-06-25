#include <Arduino.h>

const int NUM_LED = 20;
uint8_t leds[3 * NUM_LED] = {0};

uint32_t led_time = 0;               // last update time, in micros()
const uint32_t led_interval = 16667; // 60 hz?

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

// HID Usage Table for Keyboard (and keylayouts.h)
#define K_A 4
#define K_B 5
#define K_C 6
#define K_D 7
#define K_E 8
#define K_F 9
#define K_G 10
#define K_H 11
#define K_I 12
#define K_J 13
#define K_K 14
#define K_L 15
#define K_M 16
#define K_N 17
#define K_O 18
#define K_P 19
#define K_Q 20
#define K_R 21
#define K_S 22
#define K_T 23
#define K_U 24
#define K_V 25
#define K_W 26
#define K_X 27
#define K_Y 28
#define K_Z 29

#define K_1 30
#define K_2 31
#define K_3 32
#define K_4 33
#define K_5 34
#define K_6 35
#define K_7 36
#define K_8 37
#define K_9 38
#define K_0 39
#define K_ENTER 40
#define K_ESC 41
#define K_BACKSPACE 42
#define K_TAB 43
#define K_SPACE 44
#define K_MINUS 45
#define K_PLUS 46
#define K_LEFT_BRACE 47
#define K_RIGHT_BRACE 48
#define K_BACKSLASH 49
#define K_SEMICOLON 51
#define K_QUOTE 52
#define K_TILDE 53
#define K_COMMA 54
#define K_PERIOD 55
#define K_SLASH 56
#define K_CAPS_LOCK 57
#define K_F1 58
#define K_F2 59
#define K_F3 60
#define K_F4 61
#define K_F5 62
#define K_F6 63
#define K_F7 64
#define K_F8 65
#define K_F9 66
#define K_F10 67
#define K_F11 68
#define K_F12 69
#define K_PRINT_SCREEN 70
#define K_SCROLL_LOCK 71
#define K_PAUSE 72
#define K_INSERT 73
#define K_HOME 74
#define K_PAGE_UP 75
#define K_DELETE 76
#define K_END 77
#define K_PAGE_DOWN 78
#define K_RIGHT 79
#define K_LEFT 80
#define K_DOWN 81
#define K_UP 82

#define K_MENU 101

// It turns out I was not debouncing the modifier keys, but only the keys with keycodes (A,S,D,F, ..).
// It would be nice to keep it in one loop:
//   if the key at this row and col was pressed, get its 'id'
//   set the debounce count (or decrement it) in some array
//   but modifier keys do not have keycodes!!
// Since all the other keycodes are less than 128, let's use this bit to indicate modifier keys.

#define K_LEFT_CTRL (128 | 0)
#define K_LEFT_SHIFT (128 | 1)
#define K_LEFT_ALT (128 | 2)
#define K_LEFT_GUI (128 | 3)
#define K_RIGHT_CTRL (128 | 4)
#define K_RIGHT_SHIFT (128 | 5)
#define K_RIGHT_ALT (128 | 6)
#define K_RIGHT_GUI (128 | 7)

#define MOD_MASK 128

// #define S0PIN 2
// #define S1PIN 3
// #define S2PIN 4
// #define S3PIN 5
// #define COL16PIN 6
// #define ROW0PIN 7
// #define ROW1PIN 8
// #define ROW2PIN 9
// #define ROW3PIN 10
// #define ROW4PIN 11
// #define ROW5PIN 12

#define S0PIN 12
#define S1PIN 11
#define S2PIN 10
#define S3PIN 9
#define COL16PIN 8
#define ROW0PIN 2
#define ROW1PIN 3
#define ROW2PIN 4
#define ROW3PIN 5
#define ROW4PIN 6
#define ROW5PIN 7

// don't use pin13 because there's an led there...
#define CAPS_LOCK_LED 14   // bit 1
#define SCROLL_LOCK_LED 15 // bit 2

// how many consecutive HIGH samples to determine a rising edge
// 17 columns, 17*turn_on_delay*count = response time?
#define TURN_ON_COLUMN_DELAY_US 48 // 24
#define DEBOUNCE_COUNT 5 // 7 

const int rowPins[] = {2, 3, 4, 5, 6, 7};

// PROGMEM?
const int NROWS = 6;
const int NCOLS = 17;
const uint8_t key_map[NROWS][NCOLS] = {
    {K_ESC, 0, K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, K_F7, K_F8, K_F9, K_F10, K_F11, K_F12, K_PRINT_SCREEN, K_SCROLL_LOCK, K_PAUSE},
    {K_TILDE, K_1, K_2, K_3, K_4, K_5, K_6, K_7, K_8, K_9, K_0, K_MINUS, K_PLUS, K_BACKSPACE, K_INSERT, K_HOME, K_PAGE_UP},
    {K_TAB, K_Q, K_W, K_E, K_R, K_T, K_Y, K_U, K_I, K_O, K_P, K_LEFT_BRACE, K_RIGHT_BRACE, K_BACKSLASH, K_DELETE, K_END, K_PAGE_DOWN},
    {K_CAPS_LOCK, K_A, K_S, K_D, K_F, K_G, K_H, K_J, K_K, K_L, K_SEMICOLON, K_QUOTE, 0, K_ENTER, 0, 0, 0},
    {K_LEFT_SHIFT, 0, K_Z, K_X, K_C, K_V, K_B, K_N, K_M, K_COMMA, K_PERIOD, K_SLASH, 0, K_RIGHT_SHIFT, 0, K_UP, 0},
    {K_LEFT_CTRL, K_LEFT_GUI, K_LEFT_ALT, 0, 0, 0, K_SPACE, 0, 0, 0, K_RIGHT_ALT, K_RIGHT_GUI, K_MENU, K_RIGHT_CTRL, K_LEFT, K_DOWN, K_RIGHT},
};
// const uint8_t modifiers_row_col[8][2] = {
//     {5, 0},  // Left Ctrl
//     {4, 0},  // Left Shift
//     {5, 2},  // Left Alt
//     {5, 1},  // Left GUI
//     {5, 13}, // Right Ctrl
//     {4, 13}, // Right Shift
//     {5, 10}, // Right Alt
//     {5, 11}, // Right GUI
// };

void set_column(int col)
{
    if (col < 16)
    {
        digitalWrite(COL16PIN, LOW); // this is also the Enable (active low) pin of the MUX
        col = 15 - col;              // rewire and remove this later
        digitalWrite(S0PIN, col & 1);
        digitalWrite(S1PIN, (col >> 1) & 1);
        digitalWrite(S2PIN, (col >> 2) & 1);
        digitalWrite(S3PIN, (col >> 3) & 1);
    }
    else
    {
        digitalWrite(COL16PIN, HIGH);
    }
}

class PressedKeys
{
public:
    uint8_t a[6] = {0};
    uint8_t cnt = 0;

    bool contains(uint8_t key);
    void remove(uint8_t key);
    void append(uint8_t key);
};
bool PressedKeys::contains(uint8_t key)
{
    for (auto k : this->a)
    {
        if (k == key)
        {
            return true;
        }
    }
    return false;
}
void PressedKeys::remove(uint8_t key)
{
    for (int j = 0; j < cnt; j++)
    {
        if (a[j] == key)
        {
            for (; j + 1 < cnt; j++)
            {
                a[j] = a[j + 1]; // shift the array contents forward by 1
            }
            cnt--;
            a[cnt] = 0;
            return;
        }
    }
}
void PressedKeys::append(uint8_t key)
{
    if (cnt == 6)
    {
        this->remove(a[0]);
    }
    a[cnt] = key;
    cnt++;
}

int which_kb[136] = {0};
uint8_t modifiers[2] = {0};
uint8_t six_keys[2][6] = {0};
uint8_t key_state[2][136] = {0};
bool state_changed[2] = {false};

void check_keys()
{
    static PressedKeys keys[2];

    for (int col = 0; col < NCOLS; col++)
    {
        set_column(col);
        delayMicroseconds(TURN_ON_COLUMN_DELAY_US);
        for (int row = 0; row < NROWS; row++)
        {
            uint8_t key = key_map[row][col];
            int kb = which_kb[key];

            // if there is a key at this intersection (looked up a nonzero keycode),
            // use the state from digitalRead to reset or decrement the counter
            if (key)
            {
                uint8_t state = digitalRead(rowPins[row]);
                if (state)
                {
                    if (key_state[kb][key] == 1) // debounce_count about to reach 0, so it's considered pressed
                    {
                        // Handle it differently if it's a modifier key or a regular key
                        if (key & MOD_MASK)
                        {
                            modifiers[kb] |= 1 << (key & 7);
                        }
                        else
                        {
                            keys[kb].append(key);
                        }
                        state_changed[kb] = true;
                    }
                    if (key_state[kb][key]) // decrement while pressed, but don't underflow
                    {
                        key_state[kb][key]--;
                    }
                }
                else
                {
                    state_changed[kb] |= key_state[kb][key] == 0; // released key
                    key_state[kb][key] = DEBOUNCE_COUNT;

                    if (key & MOD_MASK)
                    {
                        modifiers[kb] &= ~(1 << (key & 7));
                    }
                    else
                    {
                        keys[kb].remove(key);
                    }
                }
            }
        }
    }
    // if (state_changed)
    // {
    //     Serial.print("cnt=");
    //     Serial.print(keys.cnt);
    //     Serial.print(' ');
    // }

    memcpy(six_keys[0], keys[0].a, 6);
    memcpy(six_keys[1], keys[1].a, 6);
}

// void check_modifiers()
// {
//     uint8_t mods[2] = {0};
//     int mask = 1;
//     for (int i = 0; i < 8; i++)
//     {
//         int row = modifiers_row_col[i][0];
//         int col = modifiers_row_col[i][1];

//         uint8_t key = key_map[row][col];
//         int id = which_kb[key];

//         set_column(col);
//         delayMicroseconds(TURN_ON_COLUMN_DELAY_US);
//         if (digitalRead(rowPins[row]))
//         {
//             mods[id] |= mask;
//         }
//         mask <<= 1;
//     }
//     state_changed[0] |= mods[0] != modifiers[0];
//     state_changed[1] |= mods[1] != modifiers[1];
//     modifiers[0] = mods[0];
//     modifiers[1] = mods[1];
// }


bool suspended = false;

// causes a delay...
void wakeup() {
    uint8_t tmp = USB0_CTL;
    USB0_CTL |= USB_CTL_RESUME;
    delay(11);
    USB0_CTL = tmp;
}

int main()
{
    // Specify which keys are sent through second endpoint. Default is to the first keyboard.
    which_kb[(uint8_t)K_A] = 1;
    which_kb[(uint8_t)K_LEFT] = 1;

    // Serial.begin(115200);

    pinMode(17, OUTPUT);

    pinMode(S0PIN, OUTPUT);
    pinMode(S1PIN, OUTPUT);
    pinMode(S2PIN, OUTPUT);
    pinMode(S3PIN, OUTPUT);

    pinMode(COL16PIN, OUTPUT); // also connected to ENable pin of MUX

    for (int i = 0; i < NROWS; i++)
    {
        pinMode(rowPins[i], INPUT_PULLDOWN);
        // pinMode(rowPins[i], INPUT);
    }

    pinMode(CAPS_LOCK_LED, OUTPUT);
    pinMode(SCROLL_LOCK_LED, OUTPUT);

    // Initialize all keys to unpressed
    memset(key_state[0], DEBOUNCE_COUNT, 136);
    memset(key_state[1], DEBOUNCE_COUNT, 136);

    while (1)
    {
        // uint32_t start_time = micros();

        if (USB0_ISTAT & USB_ISTAT_SLEEP)
        {
            suspended = true;
            USB0_ISTAT = USB_ISTAT_SLEEP;
            continue;
        }

        if (USB0_ISTAT & USB_ISTAT_RESUME)
        {
            suspended = false;
            USB0_ISTAT = USB_ISTAT_RESUME;
            continue;
        }

        state_changed[0] = false;
        state_changed[1] = false;
        check_keys();
        
        if (suspended)
        {
            if (state_changed[0] || state_changed[1])
            {
                wakeup();
                continue;
            }
        }

        if (state_changed[0])
        {
            Keyboard.send_now1(0, modifiers[0], six_keys[0], key_state[0]);
        }
        if (state_changed[1])
        {
            Keyboard.send_now1(1, modifiers[1], six_keys[1], key_state[1]);
        }

        
        // digitalWrite(CAPS_LOCK_LED, keyboard_leds & 0b10);
        // digitalWrite(SCROLL_LOCK_LED, keyboard_leds & 0b100);

        uint32_t now = micros();
        if (now - led_time > led_interval)
        {
            // f(k*x + om*t)

            // Do LED update stuff
            // for (int i = 0; i < NUM_LED; i++)
            // {
            //     double x = PI * i / NUM_LED; // space
            //     double t = now / 1000000.0;  // time
            //     // double t = now;

            //     double r = cos(x + t);
            //     double g = cos(x + PI / 3.0 + t);
            //     double b = cos(x + PI * 2.0 / 3.0 + t);

            //     leds[3 * i + 0] = 237 * r * r;
            //     leds[3 * i + 1] = 237 * g * g;
            //     leds[3 * i + 2] = 237 * b * b;
            // }

            uint8_t r = 0;
            uint8_t g = 0;
            uint8_t b = 0;
            if (keyboard_leds & 0b10)
            {
                r = 237;
            }
            if (keyboard_leds & 0b100)
            {
                g = 237;
                b = 237;
            }
            for (int i = 0; i < NUM_LED; i++)
            {
                leds[3 * i + 0] = g;
                leds[3 * i + 1] = r;
                leds[3 * i + 2] = b;
            }

            // for (int i = 0; i < 3 * NUM_LED; i++)
            // {
            //     Serial.print(leds[i]);
            //     Serial.print(' ');
            // }
            // Serial.println();
            bitbang_leds();

            led_time = now;
        }
        // Serial.println(micros() - start_time);
        // delay(100);
        // delay(1);
    }
}