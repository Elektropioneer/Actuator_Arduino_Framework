#include <Servo.h>
#include <Arduino.h>
#include <U8x8lib.h>

#define serial_print                                                                                  // uncomment if you want debug through serial

Servo s1; Servo s2; Servo s3; Servo s4; Servo s5;             // servo objects
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // OLEDs without Reset of the Display

Servo _servo[5]                 = {s1, s2, s3, s4, s5};                                               // 6, 9, 10, 11, 3
int _servo_begin[5]             = {0, 0, 0, 0, 0};       

uint8_t _relay[3]               = {4, 5, A5};                                                         // D4, D5, A5
uint8_t _relay_begin[3]         = {0, 0, 0};                                                          // starting values

uint8_t _mosfet[3]              = {A0, A1, 12};                                                       // A6, A7, 12
uint8_t _mosfet_begin[3]        = {0, 0, 0};                                                          // starting values

uint8_t _adc_bat_move_pin       = A6;                                                                 // move battery pin
float   _adc_bat_move_value     = 0;                                                                  // we store the value here
float   _adc_bat_move_min       = 12.7;                                                               // minimum of the battery 

uint8_t _adc_bat_main_pin       = A7;                                                                 // move battery pin
float   _adc_bat_main_value     = 0;                                                                  // we store the value here
float   _adc_bat_main_min       = 12.0;                                                               // minimum of the battery 

uint8_t _adc_bat_servo_pin      = A2;                                                                 // pin for adc servo
float   _adc_servo_min          = 4.85;                                                               // min value

String error_0 = "X"; String error_1 = "X"; String error_2 = "X";                                     // error strings

void setup() {
  Serial.begin(9600);                                                                                 // setup serial

  // Setup all the pins
  setup_servo();
  setup_relay();
  setup_mosfet();

  // for safety
  delay(500);

  // check ADCs
  if(!check_adc_bat_move()) { Serial.print("ADC_BAT_MOVE - don't start...");  error_1="CHNG BAT MOVE";/*while(1);*/ }
  if(!check_adc_bat_main()) { Serial.print("ADC_BAT_MAIN - don't start...");  error_2="CHNG BAT MAIN";/*while(1);*/ }
  
  if(!check_adc_servo())    { Serial.print("ADC_SERVO - don't start...");     error_0="CHK VOLTAGE"; }

  // setup the display
  u8x8.begin(); 
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  
  setup_display();
}

void loop() {   }


/********************************************************************************/
/*                               Setup functions                                */
/********************************************************************************/
void setup_servo() {
  s1.attach(6); s2.attach(9); s3.attach(10); s4.attach(11); s5.attach(3);   //attach to the pins
  for(int i=0; i<5; i++) {
    _servo[i].write(_servo_begin[i]);
  }
}

void setup_relay() {
  for(int i=0; i<3; i++) {
   pinMode(_relay[i], OUTPUT); 
   digitalWrite(_relay[i], _relay_begin[i]);
  }
}

void setup_mosfet() {
  for(int i=0; i<3; i++) {
   pinMode(_mosfet[i], OUTPUT); 
   digitalWrite(_mosfet[i], _mosfet_begin[i]);
  }
}

void setup_display() {
  delay(100);
  
  u8x8.setCursor(0, 1); u8x8.print("Move Bat: "); u8x8.print(_adc_bat_move_real(), 2); 
  u8x8.setCursor(0, 2); u8x8.print("Main Bat: "); u8x8.print(_adc_bat_main_real(), 2);
  u8x8.setCursor(0, 3); u8x8.print("Servo Rg: "); u8x8.print(adc_bat_servo_read(), 2);
  u8x8.setCursor(0, 4); u8x8.print("E:"); u8x8.print(error_0);
  u8x8.setCursor(0, 5); u8x8.print("  "); u8x8.print(error_1);
  u8x8.setCursor(0, 6); u8x8.print("  "); u8x8.print(error_2);

}
/********************************************************************************/

/*
 *  int adc_bat_move()                  - returns the analog read value from the adc bat move pin (0-1023)
 *  void adc_bat_move_read()            - converts the value into 0-5V range
 *  void adc_bat_move_serial_voltage()  - just loggs the 0-5V range value
 *  float _adc_bat_move_real()          - get the 0V-15V range
 *  boolean check_adc_bat_move()        - returns true if the battery range is good (inside the bar) if not returns false
 */
int adc_bat_move()                  { return analogRead(_adc_bat_move_pin); } 
void adc_bat_move_read()            { _adc_bat_move_value = (adc_bat_move() * (5.0 / 1023.0)); };
float get_adc_bat_move_value()      { adc_bat_move_read(); return _adc_bat_move_value; }
void adc_bat_move_serial_voltage()  { Serial.println(); Serial.print("Battery move adc in V: "); Serial.print(_adc_bat_move_real()); }

float _adc_bat_move_real() {
  // Vs = (Vout * (R1+R2)) / R2
  return ((get_adc_bat_move_value() * (100000 + 50000)) / 50000);
}

boolean check_adc_bat_move() { 
  #ifdef serial_print 
    Serial.println(); Serial.print("ADC_BAT_MOVE - "); Serial.print(get_adc_bat_move_value()); Serial.println(); 
  #endif

  // if it is above minimum
  if( _adc_bat_move_real() > _adc_bat_move_min) {

    #ifdef serial_print
      Serial.println("ADC_BAT_MOVE - good voltage!");
    #endif
    
    return true;
  } else {
    #ifdef serial_print
      Serial.println("ADC_BAT_MOVE - low voltage, change battery!");
    #endif
    
    return false;
  }
}

/********************************************************************************/

/*
 *  int adc_bat_main()                  - returns the analog read value from the adc bat main pin (0-1023)
 *  void adc_bat_main_read()            - converts the value into 0-5V range
 *  void adc_bat_main_serial_voltage()  - just loggs the 0-5V range value
 *  float _adc_bat_main_real()          - get the 0V-15V range
 *  boolean check_adc_bat_main()        - returns true if the battery range is good (inside the bar) if not returns false
 */
int adc_bat_main()                  { return analogRead(_adc_bat_main_pin); } 
void adc_bat_main_read()            { _adc_bat_main_value = (adc_bat_main() * (5.0 / 1023.0)); };
float get_adc_bat_main_value()      { adc_bat_main_read(); return _adc_bat_main_value; }
void adc_bat_main_serial_voltage()  { Serial.println(); Serial.print("Battery main adc in V: "); Serial.print(_adc_bat_main_real()); }

float _adc_bat_main_real() {
  // Vs = (Vout * (R1+R2)) / R2
  return ((get_adc_bat_main_value() * (100000 + 50000)) / 50000);
}

boolean check_adc_bat_main() { 
  #ifdef serial_print 
    Serial.println(); Serial.print("ADC_BAT_MAIN - "); Serial.print(get_adc_bat_main_value()); Serial.println(); 
  #endif

  // if it is above minimum
  if( _adc_bat_main_real() > _adc_bat_main_min) {

    #ifdef serial_print
      Serial.println("ADC_BAT_MAIN - good voltage!");
    #endif
    
    return true;
  } else {
    #ifdef serial_print
      Serial.println("ADC_BAT_MAIN - low voltage, change battery!");
    #endif
    
    return false;
  }
}

/********************************************************************************/

/*
 *  int adc_bat_servo()         - reads the analog of the servo pin
 *  float adc_bat_servo_read()  - convert to 0-5V range
 *  boolean check_adc_servo()   - returns true if it is above 4.85
 */
int adc_bat_servo()                  { return analogRead(_adc_bat_servo_pin); } 
float adc_bat_servo_read()           { return (adc_bat_servo() * (5.0 / 1023.0)); };
boolean check_adc_servo() { 
  if(adc_bat_servo_read() > _adc_servo_min) 
    return true;
  else
    return false;
}

/********************************************************************************/
