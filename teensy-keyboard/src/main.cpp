#include <Arduino.h>

// Looking at how QMK stores the layout:
// https://beta.docs.qmk.fm/using-qmk/guides/keymap
//
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
#define K_APP 118 // (Windows key)

#define S0PIN 2
#define S1PIN 3
#define S2PIN 4
#define S3PIN 5
#define COL16PIN 6
#define ROW0PIN 7
#define ROW1PIN 8
#define ROW2PIN 9
#define ROW3PIN 10
#define ROW4PIN 11
#define ROW5PIN 12

const int selectPins[] = {2, 3, 4, 5};
const int rowPins[] = {7, 8, 9, 10, 11, 12};

// PROGMEM?
const int NROWS = 6;
const int NCOLS = 17;
const uint8_t key_map[NROWS][NCOLS] = {
    {K_ESC, 0, K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, K_F7, K_F8, K_F9, K_F10, K_F11, K_F12, K_PRINT_SCREEN, K_SCROLL_LOCK, K_PAUSE},
    {K_TILDE, K_1, K_2, K_3, K_4, K_5, K_6, K_7, K_8, K_9, K_0, K_MINUS, K_PLUS, K_BACKSPACE, K_INSERT, K_HOME, K_PAGE_UP},
    {K_TAB, K_Q, K_W, K_E, K_R, K_T, K_Y, K_U, K_I, K_O, K_P, K_LEFT_BRACE, K_RIGHT_BRACE, K_BACKSLASH, K_DELETE, K_END, K_PAGE_DOWN},
    {K_CAPS_LOCK, K_A, K_S, K_D, K_F, K_G, K_H, K_J, K_K, K_L, K_SEMICOLON, K_QUOTE, 0, K_ENTER, 0, 0, 0},
    {/*left shift*/ 0, 0, K_Z, K_X, K_C, K_V, K_B, K_N, K_M, K_COMMA, K_PERIOD, K_SLASH, 0, /*right shift*/ 0, 0, K_UP, 0},
    {/*left ctrl*/ 0, /*left gui*/ 0, /*left alt*/ 0, 0, 0, 0, K_SPACE, 0, 0, 0, /*right alt*/ 0, /*right gui*/ 0, K_MENU, /*right ctrl*/ 0, K_LEFT, K_DOWN, K_RIGHT},
};
const uint8_t modifiers_row_col[8][2] = {
    {5, 0},  // Left Ctrl
    {4, 0},  // Left Shift
    {5, 2},  // Left Alt
    {5, 1},  // Left GUI
    {5, 13}, // Right Ctrl
    {4, 13}, // Right Shift
    {5, 10}, // Right Alt
    {5, 11}, // Right GUI
};

uint8_t key_state[128] = {0};
bool state_changed = false;

void set_column(int col)
{
    if (col < 16)
    {
        digitalWrite(COL16PIN, LOW);
        col = 15 - col; // rewire and remove this later
        digitalWrite(selectPins[0], col & 1);
        digitalWrite(selectPins[1], (col >> 1) & 1);
        digitalWrite(selectPins[2], (col >> 2) & 1);
        digitalWrite(selectPins[3], (col >> 3) & 1);
    }
    else
    {
        digitalWrite(COL16PIN, HIGH);
    }
}

void check_keys()
{
    for (int col = 0; col < NCOLS; col++)
    {
        set_column(col);
        for (int row = 0; row < NROWS; row++)
        {
            uint8_t state = digitalRead(rowPins[row]);
            uint8_t key = key_map[row][col];

            // nonzero keycode, and state changed
            if (key && (state != key_state[key]))
            {
                state_changed = true;
                if (state)
                {
                    Keyboard.set_key1(key);
                }
                else
                {
                    Keyboard.set_key1(0);
                }
                key_state[key] = state;
            }
        }
    }
}

void check_modifiers()
{
    int mods = 0;
    int mask = 1;
    for (int i = 0; i < 8; i++)
    {
        int row = modifiers_row_col[i][0];
        int col = modifiers_row_col[i][1];

        set_column(col);
        if (digitalRead(rowPins[row]))
        {
            mods |= mask;
        }
        mask <<= 1;
    }
    state_changed |= mods != keyboard_modifier_keys;
    Keyboard.set_modifier(mods);
}

int main()
{
    Serial.begin(115200);

    pinMode(selectPins[0], OUTPUT);
    pinMode(selectPins[1], OUTPUT);
    pinMode(selectPins[2], OUTPUT);
    pinMode(selectPins[3], OUTPUT);

    pinMode(COL16PIN, OUTPUT); // also connected to ENable pin of MUX

    for (int i = 0; i < NROWS; i++)
    {
        pinMode(rowPins[i], INPUT);
    }

    int prev_val = 0;

    while (1)
    {
        state_changed = false;
        check_modifiers();
        check_keys();

        if (state_changed)
        {
            Keyboard.send_now();
        }
        // // delay(500);

        // set_column(0);
        // int val = 0;
        // for (int i = 0; i < NROWS; i++)
        // {
        //     if (digitalRead(rowPins[i]))
        //     {
        //         val |= 1 << i;
        //     }
        // }

        // if (val != prev_val)
        // {
        //     Serial.print((val >> 0) & 1);
        //     Serial.print((val >> 1) & 1);
        //     Serial.print((val >> 2) & 1);
        //     Serial.print((val >> 3) & 1);
        //     Serial.print((val >> 4) & 1);
        //     Serial.print((val >> 5) & 1);
        //     Serial.println();
        // }
        // prev_val = val;
        // // delay(500);
    }
}