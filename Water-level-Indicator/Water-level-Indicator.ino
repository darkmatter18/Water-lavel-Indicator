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
RunningMedian running_distance = RunningMedian(ITER_M);

//Global Variables
byte buzzer_state = LOW;
byte self_stop_state = LOW;
// byte stop_delay_at_engine_off = LOW;
// byte buzzer_state = HIGH;
float distance = 0;
float ltr = 0;
float water_percentage = 0;
float temp = 0;
float heat_index = 0;
float humidity = 0;

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
  // Read data from the Sonar sensor and calculate the water volume, modify the global varibles (distance, ltr)
  set_tank_distance();
  calulate_volume();
  calculate_water_percentage();
  #if SERIAL_DEBUG
    Serial.println("Tank Calculation Done!");
  #endif

  // Read data from the DHT sensor, modify the global varibles (temp, humidity)
  get_temp_humidity();
  #if SERIAL_DEBUG
    Serial.println("Temp Humidity Done!");
  #endif

  // clears the display and print the global variables in the display, uses (distance, ltr, temp, humidity)
  lcd.clear();
  print_data_to_lcd();
  #if SERIAL_DEBUG
    Serial.println("Printing Done!");
  #endif
}

/**
 * @brief Buzzer Interrupt service routine
 * 
 */
void buzzer_Isr(){
  buzzer_state = digitalRead(BUZZER_INT);
}

/**
 * @brief Self Stop Interrupt service routine
 * 
 */
void self_stop_Isr(){
  self_stop_state = digitalRead(SELF_STOP_INT);
}

/**
 * @brief Printing DATA
 * 
 */
void print_data_to_lcd(){
  lcd.setCursor(0, 0);
  lcd.print(temp, 1);
  lcd.print("C");

  lcd.setCursor(7, 0);
  lcd.print(round(humidity));
  lcd.print("%");

  lcd.setCursor(11, 0);
  lcd.print(heat_index, 1);
  lcd.print("C");

  lcd.setCursor(1, 1);
  lcd.print(round(ltr));
  lcd.print("L");

  lcd.setCursor(6, 1);
  lcd.print("TANK");

  lcd.setCursor(11, 1);
  lcd.print(round(water_percentage));
  lcd.print("%");
}

/**
 * @brief Print the start msg
 * Then wait 1.5sec and then clears the display
 * 
 */
void print_start_msg(){
  lcd.setCursor(2, 0);
  lcd.print("Water Level");
  lcd.setCursor(5, 1);
  lcd.print("Meter");

  delay(1500);
  lcd.clear();
}

/**
 * @brief Print Calculating 
 * 
 */
void print_calculating(){
  lcd.setCursor(0, 0);
  lcd.print("Calculating...");
}

/**
 * @brief Read the sensor value from DHT11 and
 * Set the temp, humidity and heat_index global variable
 */
void get_temp_humidity(){
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    temp = humidity = -1;
    heat_index = -1;
    return;
  } else {
    temp = t;
    humidity = h;
    heat_index = dht.computeHeatIndex(t, h, false);

    #if SERIAL_DEBUG
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println("%");
      Serial.print("Temperature: ");
      Serial.print(temp);
      Serial.println("°C");
      Serial.print("Heat Index: ");
      Serial.print(heat_index);
      Serial.println("°C");
    #endif
  }
}

/**
 * @brief Calculates the water percentage
 * Uses distance global variable
 * 
 * Writes the water_percentage global variable
 * 
 * @note If the percentage is greater than 100, make the value 100
 * 
 */
void calculate_water_percentage(){
  int x = (TANK_BUTTOM_DISTANCE - distance)/(TANK_BUTTOM_DISTANCE - TANK_TOP_DISTANCE) * 100;
  if(x >= 100){
    water_percentage = 100;
  } else {
    water_percentage = x;
  }
}


/**
 * @brief Calculates the volume of water in liters
 * Uses the distance global variable
 * 
 * Writes the ltr global variable
 */
void calulate_volume(){
  double volume = PI * TANK_REDIUS * TANK_REDIUS * (TANK_BUTTOM_DISTANCE - distance);
  ltr = volume / 1000.0;

  #if SERIAL_DEBUG
    Serial.print("Volume: ");
    Serial.print(volume);
    Serial.println("cc");

    Serial.print("LTR: ");
    Serial.print(ltr);
    Serial.println("ltr");
  #endif
}

/**
 * @brief Set the tank distance global variable
 * 
 */
void set_tank_distance(){
  distance = get_distance_median();
  
  #if SERIAL_DEBUG
    Serial.print(distance);
    Serial.println("cm");
  #endif
}

/**
 * @brief Get the distance median object
 * using RunningMedian
 * 
 * @return float (distance in cm)
 */
float get_distance_median(){
  for(int i=0; i<ITER_M; i++){
    // Calculating the distance
    unsigned long single_distance = measure_single_duration() * SPEED_OF_SOUND / 2;

    // Adding the calculated distance in running_distance
    running_distance.add((float)single_distance);
    delay(300);
  }

  // Get the median distance and clear the buffer
  float distance = running_distance.getMedian();
  running_distance.clear();
  
  #if SERIAL_DEBUG
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  #endif
  
  return distance;
}

/**
 * @brief This function measures the duration of the sound wave taking to come and go.
 * The function Triggers the sensor by using the trigger pin
 * and uses pulseIn to get the pulse from the sensors echo to get the 
 * 
 * @return unsigned long (duration in micro second)
 */
unsigned long measure_single_duration(){
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);
    // Reads the echoPin, returns the sound wave travel time in microseconds
    unsigned long d = pulseIn(ECHO_PIN, HIGH);
    
    #if SERIAL_DEBUG
      Serial.print("duration");
      Serial.println(d);
    #endif
    return d;
}
