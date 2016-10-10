#include <VirtualWire.h>

#define DEBUG_LED 13
#define RED_PIN 9
#define GREEN_PIN 10
#define BLUE_PIN 11

#define TX_PIN 3
#define RX_PIN 12
#define TX_EN_PIN 7

#define BTN_0 6 // PCINT22
#define BTN_1 5 // PCINT21
#define BTN_2 4 // PCINT20
#define BTN_3 2 // PCINT18
#define SPEAKER 8


//-----------------------------
#define TX_PIN 3
#define RX_PIN 12
#define TX_EN_PIN 7
//-----------------------------


// Colors
#define BLACK   0b0000
#define RED     0b0100
#define GREEN   0b0010
#define BLUE    0b0001
#define BEEP    0b1000

struct frame {
  uint8_t color;
  uint8_t duration;  
};

#define MAX_FRAMES 0xFE
#define TIMEOUT 100
uint8_t animation_length = 0;
uint8_t currentframe = 0;
struct frame animation[MAX_FRAMES];
bool recording = true;
uint8_t ticks = 0;

bool add_frame(bool red, bool green, bool blue, bool speaker) {
  if(currentframe < MAX_FRAMES) {
    animation[currentframe].color = speaker << 3 | red << 2 | green << 1 | blue;
    if(currentframe > 0) {
      animation[currentframe - 1].duration = ticks;
    }
    ticks = 0;
    currentframe++;
    animation_length = currentframe;
    return true;
  }
  else {
    return false;  
  }
}

bool double_frame() {
  if(currentframe > 0) {
    bool done = add_frame(0, 0, 0, 0);
    animation[currentframe - 1].color = animation[currentframe - 2].color;
    return done;
  }
  return false;
}

void start_recording() {
  recording = true;
  ticks = 0;
  currentframe = 0;
  animation_length = 0;
}

void stop_recording() {
  recording = false;
  animation[currentframe-1].duration = 0xFF;
  currentframe = 0;
  ticks = 0;
}

void play_frame() {
  // check if frame has ended
  if(ticks >= animation[currentframe].duration) {
    currentframe = (currentframe + 1) % animation_length;
    ticks = 0;
  }  
  // new frame
  if(ticks == 0) {
    set_rgb(
      RED & animation[currentframe].color, 
      GREEN & animation[currentframe].color, 
      BLUE & animation[currentframe].color
    );
    digitalWrite(SPEAKER, animation[currentframe].color & BEEP);
  }
}


void setup() {
    Serial.begin(9600);
    // LED
    pinMode(DEBUG_LED, OUTPUT);
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    pinMode(SPEAKER, OUTPUT);
    
    set_rgb(0, 0, 0);
    digitalWrite(DEBUG_LED, LOW);

    // Buttons
    pinMode(BTN_0, INPUT);
    pinMode(BTN_1, INPUT);
    pinMode(BTN_2, INPUT);
    pinMode(BTN_3, INPUT);


    
    cli();
    // Enable Pin Change Interrupts for Buttons
    PCMSK2 |= _BV(PCINT22) | _BV(PCINT21) | _BV(PCINT20) | _BV(PCINT18);
    PCICR  |= _BV(PCIE2);

    // Configure Timer 0 for color animations
    TCCR0A = _BV(WGM01); // Clear Timer on Compare Match (CTC) Mode, OC0A and OC0B disconnected
    TCCR0B = 0b101; // prescaler clk/1024
    TIMSK0 = 0b010; // activate Timer/Counter0, Output Compare A Match Interrupt
    OCR0A  = 0xFF; // when should an interrupt occur
    TCNT0  = 0; // set Timer Value to zero
    
    /*// Initialise the IO and ISR
    vw_set_tx_pin(TX_PIN);
    vw_set_rx_pin(RX_PIN);
    vw_set_ptt_pin(TX_EN_PIN);
    vw_set_ptt_inverted(false); // Required for DR3100
    vw_setup(300);   // Bits per sec

    vw_rx_start();       // Start the receiver PLL running*/
    sei();
}



void loop() {
  
}


bool data_available = false;
uint8_t data = 0;
void transceiver() {
  if(data_available) {
    vw_send(&data, 1);
    delay(10);
    vw_send(&data, 1);
    data_available = false;
  }
  else {
    recieve();
  }
}
void recieve() {
    uint8_t buf[VW_MAX_MESSAGE_LEN];
    uint8_t buflen = VW_MAX_MESSAGE_LEN;
    if (vw_get_message(buf, &buflen)) // Non-blocking
    {
      set_rgb(
        (buf[0]) & 1,
        (buf[0] >> 1) & 1,
        (buf[0] >> 2) & 1
      );
      // trigger alarm
      digitalWrite(SPEAKER, (buf[0] >> 3) & 1);
    }      
}

void set_rgb(uint8_t red, uint8_t green, uint8_t blue) {
  digitalWrite(RED_PIN, !red);
  digitalWrite(GREEN_PIN, !green);
  digitalWrite(BLUE_PIN, !blue);
}


// Wait for the end of a recording and play the animation
ISR(TIMER0_COMPA_vect) {
  if(recording) {
    if(animation[currentframe - 1].color == 0) {
      if((ticks > TIMEOUT) && (animation_length > 0)) {
        stop_recording();
      }
    }
    else {
      if((ticks == 0xFF) ) { // timeout occured
        double_frame();
      }
    }
    
  }
  if(!recording) {
    play_frame();
  }
  ticks++;
}
ISR (PCINT2_vect)
{
  bool red = !digitalRead(BTN_0);
  bool green = !digitalRead(BTN_1);
  bool blue = !digitalRead(BTN_2);
  bool alarm = !digitalRead(BTN_3);

  // stop playback, start recording animation
  if(!recording) start_recording();
  if(add_frame(red, green, blue, alarm)) {
    set_rgb(red, green, blue);
  }
  
  data = (alarm << 3) | (red << 2) | (green << 1) | blue;
  data_available = true;
}
