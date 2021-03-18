/*The circuit:
 * LCD D4 pin to digital pin 9
 * LCD D5 pin to digital pin 8
 * LCD D6 pin to digital pin 7
 * LCD D7 pin to digital pin 6
 * LCD RS pin to digital pin 5
 * LCD E  pin to digital pin 4
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 * 
 * Custom Chars
 * 0 - Degree
 */

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <RunningMedian.h>
#include <DHT.h>

#include "wiring.h"
#include "constants.h"


// Initializing the library with the numbers of the interface pins
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_WIDTH, LCD_HEIGHT);

// Initializing the DHT library
DHT dht(DHT_PIN, DHT11);

// running medium for storing distance 
RunningMedian running_durations = RunningMedian(ITER_M);

//Global Variables
byte buzzer_state = LOW;
byte self_stop_state = LOW;
// byte stop_delay_at_engine_off = LOW;
// byte buzzer_state = HIGH;
// float temp = 0;
// float humidity = 0;
// float water_percentage = 0;
// float ltr = 0;
// float distance = 0;

// Degree character for LCD display
// byte degree[] = {
//   B11100,
//   B10100,
//   B11100,
//   B00000,
//   B00000,
//   B00000,
//   B00000,
//   B00000
// };

void setup(){
  pinMode(TRIGGER_PIN, OUTPUT);       // Sets the TRIGGER_PIN as an OUTPUT (5)
  pinMode(ECHO_PIN, INPUT);           // Sets the ECHO_PIN as an INPUT (4)
  pinMode(BUZZER_INT, INPUT_PULLUP);  // Sets the BUZZER_INT pin as INPUT_PULLUP (2)
  pinMode(SELF_STOP_INT, INPUT_PULLUP);   // Sets the BUZZER_INT pin as INPUT_PULLUP (3)

  attachInterrupt(digitalPinToInterrupt(BUZZER_INT), buzzer_Isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SELF_STOP_INT), self_stop_Isr, CHANGE);

  buzzer_state = digitalRead(BUZZER_INT);
  self_stop_state = digitalRead(SELF_STOP_INT);

  // Start the LCD
  lcd.begin();
  lcd.backlight();

  // Start the DHT11
  dht.begin();

  print_start_msg();
  print_calculating();

  #if SERIAL_DEBUG
    Serial.begin(9600);
  #endif
}

void loop(){
  // #if SERIAL_DEBUG
  //   Serial.println(engine_state);
  // #endif
  // if(engine_state){
  //   // Read data from the Sonar sensor and calculate the water volume, modify the global varibles (distance, ltr)
  //   set_tank_distance();
  //   calulate_volume();
  //   calculate_water_percentage();
  // }

}

void buzzer_Isr(){
  buzzer_state = digitalRead(BUZZER_INT);
}
void self_stop_Isr(){
  self_stop_state = digitalRead(SELF_STOP_INT);
}

void print_start_msg(){
  lcd.setCursor(2, 0);
  lcd.print("Water Level");
  lcd.setCursor(5, 1);
  lcd.print("Meter");

  delay(1500);
  lcd.clear();
}

void print_calculating(){
  lcd.setCursor(0, 0);
  lcd.print("Calculating...");
}


// void calculate_water_percentage(){
//   water_percentage = (TANK_BUTTOM_DISTANCE - distance)/(TANK_BUTTOM_DISTANCE - TANK_TOP_DISTANCE) * 100;
// }

// void calulate_volume(){
//   double volume = PI * TANK_REDIUS * TANK_REDIUS * (TANK_BUTTOM_DISTANCE - distance);
//   ltr = volume / 1000.0;

//   #if SERIAL_DEBUG
//     Serial.print("Volume: ");
//     Serial.print(volume);
//     Serial.println("cc");

//     Serial.print("LTR: ");
//     Serial.print(ltr);
//     Serial.println("ltr");
//   #endif
// }


// void set_tank_distance(){
//   distance = get_distance_median();
  
//   #if SERIAL_DEBUG
//     Serial.print(distance);
//     Serial.println("cm");
//   #endif
// }

// float get_distance_median(){
//   for(int i=0; i<ITER_M; i++){
//     running_durations.add((float)measure_single_duration());
//     delay(300);
//   }
//   float duration = running_durations.getMedian();
//   running_durations.clear();
  
//   #if SERIAL_DEBUG
//     Serial.print("Duration: ");
//     Serial.print(duration);
//     Serial.print(" us ");
//   #endif
  
//   // Calculating the distance
//   float distance = duration * SPEED_OF_SOUND / 2; // Speed of sound wave divided by 2 (go and back)
//   // Displays the distance on the Serial Monitor
  
//   #if SERIAL_DEBUG
//     Serial.print("Distance: ");
//     Serial.print(distance);
//     Serial.println(" cm");
//   #endif
  
//   return distance;
// }

// unsigned long measure_single_duration(){
//     digitalWrite(TRIGGER_PIN, LOW);
//     delayMicroseconds(2);
//     // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
//     digitalWrite(TRIGGER_PIN, HIGH);
//     delayMicroseconds(10);
//     digitalWrite(TRIGGER_PIN, LOW);
//     // Reads the echoPin, returns the sound wave travel time in microseconds
//     long d = pulseIn(ECHO_PIN, HIGH);
    
//     #if SERIAL_DEBUG
//       Serial.print("duration");
//       Serial.println(d);
//     #endif
//     return d;
// }
