// Polling the two pins of the rotary encoder.
//
// Polling in the main loop is very fast and seems to catch almost all edges,
//   so if we increment/decrement the counter based on edges then value will
//   be accurate. But without debouncing, we can't "do things" on edge events.
//   (Need to wait until the signal is stable first.)
//
// When rotating from one detent to the next, there are two edge events:
//   0 0 0 0 1 1 1 1 1 1 1
//   0 0 0 0 0 0 1 1 1 1 1
//           ^   ^
//  first edge   second edge
//
// I want the counter to update on the second edge (upon entering the next detent)
//   but due to bouncing, the second edge could register multiple times. To be safe,
//   consider all edge events. A simple solution is to increment/decrement on all
//   edges, and when querying the counter value, divide by 2.
//
//
// Let's have three registers:
//   a : current state
//   b : previous state
//   c : the state before b, but can't be the same as b
//
// Look for sequences [c][b][a]:
//   cw:  [00][01][11] or [11][10][00]
//   ccw: [00][10][11] or [11][01][00]
//
// It may bounce when the first leg leaves the pad: [00][01][00][01]...
// Between detents, it should stop bouncing and stick to [XX][00][01]
// When the second leg leaves the pad, we will see [00][01][11] and increment.
//   It may bounce after we see this sequence.

const int clkPin = 15;
const int dtPin = 14;

int s;    // state [c][b][a]
int val;  // counter value

void setup() {
    Serial.begin(115200);

    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);

    pinMode(clkPin, INPUT);
    pinMode(dtPin, INPUT);

    s = 0;
    val = 0;
}

void loop() {
    int clk = digitalRead(clkPin);
    int dt = digitalRead(dtPin);

    int a = (dt << 1) | clk;

    if ((s & 0x3) != a) {
        s = ((s << 2 ) | a) & 0x3F;
        switch (s) {
            case 0b000111: val++; break;
            case 0b111000: val++; break;
            case 0b001011: val--; break;
            case 0b110100: val--; break;
            default: break;
        }
        
        Serial.print(s & (1 << 5) ? 1 : 0);
        Serial.print(s & (1 << 4) ? 1 : 0);
        Serial.print(s & (1 << 3) ? 1 : 0);
        Serial.print(s & (1 << 2) ? 1 : 0);
        Serial.print(s & (1 << 1) ? 1 : 0);
        Serial.print(s & (1 << 0) ? 1 : 0);
        Serial.print(" ");
        Serial.println(val);
    }

    digitalWrite(2, val & (1 << 0));
    digitalWrite(3, val & (1 << 1));
    digitalWrite(4, val & (1 << 2));
    digitalWrite(5, val & (1 << 3));
}




// Lookup table
// The state is (dt,clk). When rotating clockwise, clk changes before dt.
//
//          cur
// prev|00|01|10|11|
//  00 |  |+1|-1|  |
//  01 |-1|  |  |+1|
//  10 |+1|  |  |-1|
//  11 |  |-1|+1|  |
//
// int table[] = {0, 1, -1, 0, -1, 0, 0, 1, 1, 0, 0, -1, 0, -1, 1, 0};
// int state;
//
// setup:
//   state = (dt << 1) | clk;
// loop:
//   state = ((state << 2) | (dt << 1) | clk) & 0xF;
//   val += table[state];




// Digital or software debouncing:
// * increment a counter while the value remains the same
// * restart (reject the signal) when it's "bouncing"
// * doesn't work because it's polled too fast.
//   * we have to count for longer (or poll slower)
//
// For efficiency, we COULD map the two rotary encoder pins to be
//   two adjacent bits of a pin register...
//
// bits: [dt[t-2], clk[t-2], dt[t-1], clk[t-1], dt[t], clk[t]]
// Look for:
//   cw:  [01][01][11] or [10][10][00]
//   ccw: [10][10][11] or [01][01][00]
//
// setup:
//     int s = (dt << 1) | clk;
//     s = (s << 4) | (s << 2) | s;
// loop:
//     s = ((s << 2) | (dt << 1) | clk) & 0x3F;
//     if (s == 0b010111 || s == 0b101000) { val++; } ...
