// https://www.avrfreaks.net/forum/tiny-fast-prng

#define MAX_DELAY (65535)
#define NUM_LEDS (20)

uint8_t buttonPressed = 0;
uint8_t dimDelay = 10;
const uint8_t CHARLIE_DIR[] = {0x00000000, 0x00000000, 0x00000000};
const uint8_t CHARLIE_STATE[] = {0x00000000, 0x00000000, 0x00000000};

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
  switch(led) {
    case 0:
      charliePlex(1 << 0, 1 << 4);
      break;
    case 1:
      charliePlex(1 << 4, 1 << 0);
      break;
    case 2:
      charliePlex(1 << 0, 1 << 3);
      break;
    case 3:
      charliePlex(1 << 3, 1 << 0);
      break;
    case 4:
      charliePlex(1 << 0, 1 << 2);
      break;
    case 5:
      charliePlex(1 << 2, 1 << 0);
      break;
    case 6:
      charliePlex(1 << 0, 1 << 1);
      break;
    case 7:
      charliePlex(1 << 1, 1 << 0);
      break;
    case 8:
      charliePlex(1 << 1, 1 << 4);
      break;
    case 9:
      charliePlex(1 << 4, 1 << 1);
      break;
    case 10:
      charliePlex(1 << 1, 1 << 3);
      break;
    case 11:
      charliePlex(1 << 3, 1 << 1);
      break;
    case 12:
      charliePlex(1 << 1, 1 << 2);
      break;
    case 13:
      charliePlex(1 << 2, 1 << 1);
      break;
    case 14:
      charliePlex(1 << 2, 1 << 4);
      break;
    case 15:
      charliePlex(1 << 4, 1 << 2);
      break;
    case 16:
      charliePlex(1 << 2, 1 << 3);
      break;
    case 17:
      charliePlex(1 << 3, 1 << 2);
      break;
    case 18:
      charliePlex(1 << 3, 1 << 4);
      break;
    case 19:
      charliePlex(1 << 4, 1 << 3);
      break;
    default:
      clearAll();
      // Emperical delay added to match the time of setting an LED
      _delayCycles(7);
      break;
  }
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
#if EFFICIENT_CHARLIE
// This makes the LEDs much dimmer. Todo: Use logic analyzer
  DDRB = high;
  DDRB = low;
  PORTB = high;
#else
  clearAll();
  DDRB |= high;
  DDRB |= low;
  PORTB |= high;
#endif
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
