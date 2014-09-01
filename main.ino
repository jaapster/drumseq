// define globals
uint32_t pattern;
byte patternLength;
byte pos;

// Returns a uint representing new button presses (high bits)
uint32_t readInputRegisters() {
    uint32_t value = 0;

    // prepare shift register (74HC165) for parallel load
    PORTB &= ~(1 << PIN_IN_PLOAD);
    PORTB |= (1 << PIN_IN_PLOAD);

    // make sure clock pin is low
    PORTB &= ~(1 << PIN_CLOCK);

    // read button states (in reverse)
    byte i = DATA_WIDTH;
    while (i--) {
        // read bit of data pin
        boolean bit = (PINB >> PIN_IN_DATA) & 1;  

        // shove the bit on the return byte
        value |= (bit << ((DATA_WIDTH - 1) - i));
        
        tick();
    }

    return value;
}

void writeOutputRegisters() {
    // led pulse width modulation
    static byte pw = LED_PWM_RATE;

    if (!pw--) pw = LED_PWM_RATE;

    // lower latch on shift register (74HC595)
    PORTB &= ~(1 << PIN_OUT_LATCH);

    // write LED states (in reverse)
    byte i = DATA_WIDTH;
    while (i--) {
        // get the bit to output
        boolean bit = pattern >> i;

        // data out pin low by default
        PORTB &= ~(1 << PIN_OUT_DATA);

        // raise data pin if bit is high
        if(((bit & 1) && !pw) || i == pos) {
            PORTB |= (1 << PIN_OUT_DATA);
        }
    
        tick();
    }

    // raise latch
    PORTB |= (1 << PIN_OUT_LATCH);
}

// toggle clock pins on all shift registers
void tick() {
    PORTB &= ~(1 << PIN_CLOCK);
    PORTB |= (1 << PIN_CLOCK);
}

void io() {
    static int ioCounter = 0;
    static uint32_t lastInputBits = 0;

    // avoid jitter by not sampling every cycle
    if (ioCounter++ == IO_SAMPLE_RATE) {
        ioCounter = 0;

        uint32_t inputBits = readInputRegisters();
        uint32_t changed = inputBits ^ lastInputBits;

        // check if the input state hase changed
        if (changed) {
            // toggle pattern bits for buttons that were pressed
            uint32_t justPressed = changed & inputBits;

            if (justPressed) {
                pattern ^= justPressed;
            }
        }

        lastInputBits = inputBits;
    }

    writeOutputRegisters();
}

void advancePosition() {
    static long rateCounter = 0;

    if (rateCounter++ > rate()) {
        rateCounter = 0;

        if (++pos == patternLength) {
            pos = 0;
        }
    }
}

long rate() {
    return 20000;
}

int main() {
    // initialize direction for shift register digital pins
    DDRB = B00001111;

    // initialize clock and latch pin states
    PORTB &= ~(1 << PIN_CLOCK);
    PORTB |= (1 << PIN_IN_PLOAD);

    // initialize globals
    pattern = 0;
    patternLength = 8;
    pos = 0;

    while (true) {
        advancePosition();
        io();
    }
}