// https://www.avrfreaks.net/forum/tiny-fast-prng

#define MAX_DELAY (65535)
#define NUM_LEDS (20)

uint8_t buttonPressed = 0;
uint8_t dimDelay = 10;
const uint8_t LEDS[] = {
  // High pin - 1 | Low Pin - 1
  0x00 | 0xF, // Bottom left, outer
  0xF0 | 0x0, // Bottom left, inner
  0x00 | 0x7, // Left straight, 1/5
  0x70 | 0x0, // Left straight, 2/5
  0x00 | 0x3, // Left straight, 3/5
  0x30 | 0x0, // Left straight, 4/5
  0x10 | 0x0, // Top left
  0x00 | 0x1, // Left straight, 5/5
  0x10 | 0xF, // Top left slope
  0xF0 | 0x1, // Bottom left slope
  0x10 | 0x7, // Bottom right slope
  0x70 | 0x1, // Top right slope
  0x30 | 0x1, // Right straight 5/5
  0x10 | 0x3, // Top right
  0x30 | 0xF, // Right straight 4/5
  0xF0 | 0x3, // Right straight 3/5
  0x30 | 0x7, // Right straight 2/5
  0x70 | 0x3, // Right straight 1/5
  0x70 | 0xF, // Bottom right, inner
  0xF0 | 0x7, // Bottom right, outer
  0x00 | 0x0,
}; 

void _delayCycles(uint16_t i) {
  while(i--) {
    asm("nop");
  }
}

uint8_t pRNG(void) {
  static uint8_t s=0xaa,a=0;
  s^=s<<3;
  s^=s>>5;
  s^=a++>>2;
  return s;
}

void setLed(uint8_t led) {
  led = LEDS[led];
  uint8_t high = ((led & 0xF0) >> 4) + 1;
  uint8_t low = (led & 0x0F) + 1;
  charliePlex(high, low);
}

void drawPattern(uint32_t pattern) {
  for(uint8_t i = 0; i < NUM_LEDS; i++) {
    if(pattern & 1) {
      // If the LSb is 1, turn on the current LED
      setLed(i);
    }
    else {
      // Displays nothing, but takes the same amount of time
      // to keep the brightness equal regardless of how many LEDs are on
      setLed(NUM_LEDS);
    }
    // Shift right to get the next pixel
    pattern = pattern >> 1;
  }
}

inline void clearAll() {
  DDRB = 0;
  PORTB = 0;
}

void charliePlex(uint8_t high, uint8_t low) {
  DDRB = high | low;
  PORTB = high;
}

void blinky() {
  uint8_t led = pRNG();
  uint16_t onTime = 8000 + 256 << (pRNG() & 0b111); // 8256 - 40768
  uint16_t offTime = 8000 + 256 << (pRNG() & 0b111); // 8256 - 40768
  // Intentionally allowing overflow by 1 to show no LEDs
  while(led > NUM_LEDS) {
    led -= NUM_LEDS;
  }
  setLed(led);
  _delayCycles(onTime);
  clearAll();
  _delayCycles(offTime);
}

void setup() {
  if(MCUSR & EXTRF) {
    buttonPressed = 1;
  }
  MCUSR = 0;
}

void loop() {
//  blinky();
  
  uint32_t pattern = 0;
  for(uint8_t j = 0; j <= NUM_LEDS; j++) {
    for(uint16_t i = 0; i < 250; i++) {
      drawPattern(pattern);
    }
    pattern = pattern << 1;
    pattern |= 1;
  }
}
