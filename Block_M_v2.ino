#include <EEPROM.h>

// Program configuration
#define USE_EEPROM false

// Limits
#define MAX_DELAY (65535)
#define NUM_LEDS (20)
//#define NUM_PATTERNS (6)
#define NUM_PATTERNS (3)

// References
#define EEPROM_ADDR_PATTERN (0)
#define EEPROM_ADDR_RAND_S (1)
#define EEPROM_ADDR_RAND_A (2)

uint8_t patternIndex = 0;
uint8_t buttonPressed = 0;
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
  // https://www.avrfreaks.net/forum/tiny-fast-prng
  // Todo: Update s and a occasionally to increase randomness
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

void drawBitmap(uint32_t bitmap) {
  // Shift bitmap right one bit at a time to determine if each LED should be on
  for(uint8_t i = 0; i < NUM_LEDS; i++) {
    if(bitmap & 1) {
      // Turn on the current LED
      setLed(i);
    }
    else {
      // Displays nothing, but takes the same amount of time
      // to keep the brightness equal regardless of how many LEDs are on
      setLed(NUM_LEDS);
    }
    // Shift right to get the next pixel
    bitmap = bitmap >> 1;
  }
}

void drawBitmapFast(uint32_t bitmap) {
  // The brightness of LEDs will change based on how many are lighted
  // Shift bitmap right one bit at a time to determine if each LED should be on
  for(uint8_t i = 0; i < NUM_LEDS; i++) {
    if(bitmap & 1) {
      // Turn on the current LED
      setLed(i);
    }
    // Shift right to get the next pixel
    bitmap = bitmap >> 1;
  }
}

void drawBitmapFade(uint32_t bitmap, uint16_t highDelay, uint16_t lowDelay) {
  // Shift bitmap right one bit at a time to determine if each LED should be on
  for(uint8_t i = 0; i < NUM_LEDS; i++) {
    if(bitmap & 1) {
      // Turn on the current LED
      setLed(i);
    }
    else {
      // Displays nothing, but takes the same amount of time
      // to keep the brightness equal regardless of how many LEDs are on
      setLed(NUM_LEDS);
    }
    _delayCycles(highDelay);
    // Shift right to get the next pixel
    bitmap = bitmap >> 1;
  }
  clearAll();
  _delayCycles(lowDelay);
}


inline void clearAll() {
  DDRB = 0;
  PORTB = 0;
}

void charliePlex(uint8_t high, uint8_t low) {
  DDRB = high | low;
  PORTB = high;
}

void patternSingleBlink() {
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


void patternRandomNoise() {
  // Only half the LEDs can possibly be on at a time, alternating every cycle.
  // This produces more "motion".
  uint32_t mask = 0x55555555;
  uint32_t oldBitmap = 0;
  uint32_t bitmap = 0;
  for(uint8_t i = 0; i < 255; i++) {
    bitmap = pRNG();
    bitmap <<= 8;
    bitmap |= pRNG();
    bitmap <<= 8;
    bitmap |= pRNG();
    bitmap ^= oldBitmap;
    mask = ~mask;
    for(uint16_t i = 0; i < 250; i++) {
      drawBitmapFast(bitmap & mask);
    }
  }
}

void patternFadingBlink() {
  const uint8_t BRIGHTNESS_STEPS = 16;
  const uint8_t NUM_REPS = 4;
  const uint8_t NUM_DIFF_DELAYS = 8;
  uint8_t newLed;
  uint8_t leds[NUM_LEDS] = {0};
  uint8_t newLedDelays[NUM_DIFF_DELAYS] = {4, 5, 6, 7, 8, 10, 12, 16};
  uint8_t newLedDelay = newLedDelays[0];
  uint16_t newLedCounter = 0;
  uint8_t delayChanger = 0;
  uint8_t temp;
  while(1) {
    delayChanger = pRNG();
    if(delayChanger == newLedDelay) {
      temp = (pRNG() % NUM_DIFF_DELAYS);
      newLedDelay = newLedDelays[temp];
    }
    for(uint16_t rep = 0; rep < NUM_REPS; rep++) {
      newLedCounter++;
      if(newLedCounter >= newLedDelay) {
        newLedCounter = 0;
        newLed = pRNG() % 32;
        if(newLed < NUM_LEDS && leds[newLed] == 0) { 
          leds[newLed] = BRIGHTNESS_STEPS;
        }
      }
      for(uint8_t brightness = 0; brightness < BRIGHTNESS_STEPS; brightness++) {
        for(uint8_t led = 0; led < NUM_LEDS; led++) {
          if(leds[led] > 0) {
            if(leds[led] > brightness) { setLed(led); }
            else { clearAll(); }
          }
        }
      }
    }
    for(uint8_t led = 0; led < NUM_LEDS; led++) {
      if(leds[led] > 0) {
        leds[led]--;
      }
    }
  }
}

void patternBoomIn(uint8_t* leds) {
  const uint8_t BRIGHTNESS_STEPS = 32;
  const uint8_t NUM_REPS = 1;
  const uint8_t MAX_STAGES = 10;
//  uint8_t leds[NUM_LEDS] = {0};
  uint8_t inFade[NUM_LEDS] = {5, 2, 4, 3, 2, 3, 5, 4, 2, 1, 1, 2, 4, 5, 3, 2, 3, 4, 2, 5};
  uint8_t stages[NUM_LEDS] = {10, 4, 8, 6, 4, 6, 10, 8, 4, 1, 1, 4, 8, 10, 6, 4, 6, 8, 4, 10};
  uint8_t altStages[NUM_LEDS] = {8, 8, 7, 6, 5, 4, 4, 3, 2, 1, 1, 2, 3, 4, 4, 5, 6, 7, 8, 8};
  uint8_t altStages2[NUM_LEDS] = {1, 1, 2, 3, 4, 5, 6, 6, 5, 4, 4, 5, 6, 6, 5, 4, 3, 2, 1, 1};
  uint8_t stage = 0;
  uint8_t offDelay = 0;
  bool done = false;
  bool doneEarly = false;
  while(!(done || doneEarly)) {
    // Light the LEDs proportional to the value at leds[i].
    for(uint16_t rep = 0; rep < NUM_REPS; rep++) {
      for(uint8_t brightness = 0; brightness < BRIGHTNESS_STEPS; brightness++) {
        for(uint8_t led = 0; led < NUM_LEDS; led++) {
            if(leds[led] > brightness) { setLed(led); }
            else { clearAll(); }
        }
      }
    }
    // Adjust brightness of each led
    done = true;
    if(stage < MAX_STAGES) { stage++; }
    for(uint8_t led = 0; led < NUM_LEDS; led++) {
      if(leds[led] < BRIGHTNESS_STEPS) {
        if(stage >= altStages2[led]) {
          leds[led] += 8;
        }
        done = false;
      }
      else {
        leds[led] = BRIGHTNESS_STEPS;
      }
    }
    if(pRNG() < 16) { doneEarly = true; }
  }
}

void patternBoomOut(uint8_t* leds) {
  const uint8_t BRIGHTNESS_STEPS = 32;
  const uint8_t NUM_REPS = 1;
//  uint8_t leds[NUM_LEDS] = {0};
  uint8_t inFade[NUM_LEDS] = {5, 2, 4, 3, 2, 3, 5, 4, 2, 1, 1, 2, 4, 5, 3, 2, 3, 4, 2, 5};
//  uint8_t outFade[NUM_LEDS] = {1, 4, 2, 3, 4, 3, 1, 2, 4, 5, 5, 4, 2, 1, 3, 4, 3, 2, 4, 1};
  uint8_t fade[NUM_LEDS] = {8, 4, 7, 6, 3, 6, 8, 7, 2, 1, 1, 2, 7, 8, 6, 3, 6, 7, 4, 8};
  uint8_t altFade[NUM_LEDS] = {8, 8, 7, 6, 5, 4, 4, 3, 2, 1, 1, 2, 3, 4, 4, 5, 6, 7, 8, 8};
  uint8_t altFade2[NUM_LEDS] = {1, 1, 2, 3, 4, 5, 6, 6, 5, 4, 4, 5, 6, 6, 5, 4, 3, 2, 1, 1};
  uint8_t offDelay = 0;
  bool done = false;
  bool doneEarly = false;
  for(uint8_t led = 0; led < NUM_LEDS; led++) {
    leds[led] = BRIGHTNESS_STEPS;
  }
  while(!(done || doneEarly)) {
    // Light the LEDs proportional to the value at leds[i].
    for(uint16_t rep = 0; rep < NUM_REPS; rep++) {
      for(uint8_t brightness = 0; brightness < BRIGHTNESS_STEPS; brightness++) {
        for(uint8_t led = 0; led < NUM_LEDS; led++) {
            if(leds[led] > brightness) { setLed(led); }
            else { clearAll(); }
        }
      }
    }
    // Adjust brightness of each led
    done = true;
    for(uint8_t led = 0; led < NUM_LEDS; led++) {
      if(leds[led] > altFade2[led]) {
        leds[led] -= altFade2[led];
        done = false;
      }
      else {
        leds[led] = 0;
      }
    }
    if(pRNG() < 12) { doneEarly = true; }
  }
  if(done) {
    clearAll();
    offDelay = pRNG();
    if(offDelay & 1) {
      while(offDelay > 0) {
        _delayCycles(150);
        offDelay--;
      }
    }
  }
}

void patternBoom(uint8_t* leds, bool firstHalf, uint8_t endEarlyFreq, uint8_t brightnessStep) {
  const uint8_t BRIGHTNESS_STEPS = 24;
  const uint8_t NUM_REPS = 2;
  const uint8_t MAX_STAGE = 11;
  const bool secondHalf = !firstHalf;
//  uint8_t leds[NUM_LEDS] = {0};
//  uint8_t inFade[NUM_LEDS] = {5, 2, 4, 3, 2, 3, 5, 4, 2, 1, 1, 2, 4, 5, 3, 2, 3, 4, 2, 5};
  uint8_t stages[NUM_LEDS] = {10, 4, 8, 6, 4, 6, 10, 8, 4, 1, 1, 4, 8, 10, 6, 4, 6, 8, 4, 10};
  uint8_t altStages[NUM_LEDS] = {8, 8, 7, 6, 5, 4, 4, 3, 2, 1, 1, 2, 3, 4, 4, 5, 6, 7, 8, 8};
  uint8_t altStages2[NUM_LEDS] = {1, 1, 2, 3, 4, 5, 6, 6, 5, 4, 4, 5, 6, 6, 5, 4, 3, 2, 1, 1};
  uint8_t stage = (firstHalf ? 0 : MAX_STAGE);
  uint8_t offDelay = 0;
  bool skipFirstDraw = secondHalf;
  bool done = false;
  bool doneEarly = false;
  while(!(done || doneEarly)) {
    if(!skipFirstDraw) {
      // Light the LEDs proportional to the value at leds[i].
      for(uint16_t rep = 0; rep < NUM_REPS; rep++) {
        for(uint8_t brightness = 0; brightness < BRIGHTNESS_STEPS; brightness++) {
          for(uint8_t led = 0; led < NUM_LEDS; led++) {
              if(leds[led] > brightness) { setLed(led); }
              else { clearAll(); }
          }
        }
      }
    }
    else {
      skipFirstDraw = false;
    }
    // Adjust brightness of each led
    done = true;
    if(firstHalf && stage < MAX_STAGE) { stage++; }
    else if(secondHalf && stage > 0) { stage--; }
    for(uint8_t led = 0; led < NUM_LEDS; led++) {
      if(firstHalf) {
        if(leds[led] < BRIGHTNESS_STEPS) {
          if(stage >= altStages2[led]) {
            leds[led] += brightnessStep;
          }
          done = false;
        }
        else {
          leds[led] = BRIGHTNESS_STEPS;
        }
      }
      else {
        if(leds[led] > brightnessStep) {
          if(stage <= altStages2[led]) {
            leds[led] -= brightnessStep;
          }
          done = false;
        }
        else {
          leds[led] = 0;
        }
      }
    }
    if(pRNG() < endEarlyFreq) { doneEarly = true; }
  }
  if(done && secondHalf) {
    clearAll();
    offDelay = pRNG();
    if(offDelay & 1) {
      while(offDelay > 0) {
        _delayCycles(600);
        offDelay--;
      }
    }
  }
}

void patternLoop() {
  uint32_t bitmap = 0;
  for(uint8_t j = 0; j <= NUM_LEDS * 2; j++) {
    for(uint16_t i = 0; i < 50; i++) {
      drawBitmap(bitmap);
    }
    bitmap = bitmap << 1;
    if(j <= NUM_LEDS) {
      bitmap |= 1;
    }
  }
}

void patternFade() {
  uint32_t onBitmap = 0xFFFFFFFF;
  uint16_t fadeSteps = 50;
  uint8_t i = 0;
  uint16_t onDelay = 0;
  uint16_t offDelay = fadeSteps * NUM_LEDS;
  while(i < fadeSteps * 2) {
    drawBitmapFade(onBitmap, onDelay, offDelay);
    if(i < fadeSteps) { 
      onDelay += 1;
      offDelay -= NUM_LEDS;
    }
    else {
      onDelay -= 1;
      offDelay += NUM_LEDS;
    }
    i++;
  }
  for(uint8_t i = 0; i < 16; i++) {
    drawBitmap(0);
  }
}

void patternFlash() {
  uint32_t onBitmap = 0xFFFFFFFF;
  uint32_t offBitmap = 0x00000000;
  for(uint16_t i = 0; i < 512; i++) {
    if(i < 255) {
      drawBitmap(onBitmap);
    }
    else {
      drawBitmap(offBitmap);
    }
  }
}

void patternWipe() {
  uint32_t rows[] = {
    0x00000041,
    0x000000BC,
    0x00000102,
    0x00000200,
    0x00000400,
    0x00040800,
    0x0003D000,
    0x00082000,
  };
  uint32_t pattern = 0;
  for(uint8_t i = 0; i < 18; i++) {
    for(uint8_t j = 0; j < 64; j++) {
      drawBitmap(pattern);
    }
    if(i < 8) {
      pattern |= rows[i];
    }
    else if(i < 16) {
      pattern &= ~rows[i-8];
    }
  }
}

void patternCrawl() {
  uint8_t SPACING = 5;
  uint8_t startIndex = 0;
  uint8_t counter = 0;
  uint32_t bitmap = 0;
  while(startIndex < SPACING) {
    bitmap = 0;
    counter = startIndex;
    for(uint8_t led = 0; led < NUM_LEDS; led++) {
      bitmap <<= 1;
      if(counter == 0) {
        bitmap |= 1;
      }
      if(counter < SPACING - 1) {
        counter += 1;
      }
      else {
        counter = 0;
      }
    }
    for(uint16_t i = 0; i < 300; i++) {
      drawBitmapFast(bitmap);
    }
    startIndex++;
  }
}

void setup() {
  // Determine if the button was pressed (external reset).
  if(MCUSR & (1 << EXTRF)) {
    buttonPressed = 1;
  }
  // Reset the status register.
  MCUSR = 0;
#if USE_EEPROM
  // Read the saved pattern index.
  patternIndex = EEPROM.read(EEPROM_ADDR_PATTERN);
  // If the button was pressed, increment pattern.
  if(buttonPressed || patternIndex >= NUM_PATTERNS) {
    patternIndex += 1;
    if(patternIndex >= NUM_PATTERNS) {
      patternIndex = 0;
    }
    EEPROM.write(EEPROM_ADDR_PATTERN, patternIndex);
  }
#else
  patternIndex = 4;
#endif
  ACSR |= _BV(ACD); // Disable ADC.
  ADCSRA &= ~_BV(ADEN); // Disable ADC.
  // Unlock watchdog to allow changes (see datasheet section 8.5.2).
  WDTCR |= (_BV(WDCE) | _BV(WDE));
  // Disable watchdog.
  WDTCR &= ~_BV(WDE);
  WDTCR &= ~_BV(WDIE);
  // Shut down all timers (none are used).
  PRR |= _BV(PRTIM0) | _BV(PRTIM1) | _BV(PRUSI) | _BV(PRADC);
}

void loop() {
  uint8_t leds[NUM_LEDS] = {0};
  switch(patternIndex) {
    case 0:
      patternFadingBlink();
      break;
    case 1:
      patternCrawl();
      break;
    case 2:
      patternLoop();
      break;
    case 3:
      patternWipe();
      break;
    case 4:
      while(1) {
        patternBoomIn(leds);
        patternBoomOut(leds);
//        patternBoom(leds, true, 16, 6);
//        patternBoom(leds, false, 32, 4);
      }
      break;
    case 5:
      patternFlash();
      break;
    case 6:
      patternFade();
      break;
    default:
      _delayCycles(MAX_DELAY);
      break;
  }
}
