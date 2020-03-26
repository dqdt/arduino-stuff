const int NROW = 2;
const int NCOL = 2;
const int NKEY = NROW * NCOL;

const int RowPins[] = {8, 9};
const int ColPins[] = {6, 7};

int pressed[NKEY] = {0};

void setup() {
    Serial.begin(115200);

    for (int i = 0; i < NROW; i++) {
        pinMode(RowPins[i], INPUT);
    }
    for (int i = 0; i < NCOL; i++) {
        pinMode(ColPins[i], OUTPUT);
    }
}

void loop() {
    bool changed = false;

    for (int c = 0; c < NCOL; c++) {
        
        digitalWrite(ColPins[c], HIGH);
        
        for (int r = 0; r < NROW; r++) {
            int i = r * NCOL + c;
            int val = digitalRead(RowPins[r]);
            changed |= val != pressed[i];
            pressed[i] = val;
        }
        
        digitalWrite(ColPins[c], LOW);
    }

    if (changed) {
        for (int i = 0; i < NKEY; i++) {
            Serial.print(pressed[i]);
        }
        Serial.println();
    }
}
