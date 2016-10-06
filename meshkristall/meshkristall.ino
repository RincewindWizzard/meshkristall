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

void setup() {
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


    // Enable Pin Change Interrupts for Buttons
    cli();
    PCMSK2 |= _BV(PCINT22) | _BV(PCINT21) | _BV(PCINT20) | _BV(PCINT18);
    PCICR  |= _BV(PCIE2);

    // Initialise the IO and ISR
    vw_set_tx_pin(TX_PIN);
    vw_set_rx_pin(RX_PIN);
    vw_set_ptt_pin(TX_EN_PIN);
    vw_set_ptt_inverted(false); // Required for DR3100
    vw_setup(300);   // Bits per sec

    vw_rx_start();       // Start the receiver PLL running
    sei();
}

bool data_available = false;
uint8_t data = 0;
void loop() {
  
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



ISR (PCINT2_vect)
{
  uint8_t red = !digitalRead(BTN_0);
  uint8_t green = !digitalRead(BTN_1);
  uint8_t blue = !digitalRead(BTN_2);
  uint8_t alarm = !digitalRead(BTN_3);
  set_rgb(red, green, blue);
  
  data = (alarm << 3) | (red << 2) | (green << 1) | blue;
  data_available = true;
}
