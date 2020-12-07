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

#include <LiquidCrystal.h>
#include <Adafruit_Sensor.h>
#include <RunningMedian.h>
#include <DHT.h>
#include <DHT_U.h>

#define SERIAL_DEBUG 0 //Mode for serial debug, 0 -> No debug, 1 -> Debug over serial is on

/**
 * Define all the pins
 */
// LCD pins
#define LCD_D4 9
#define LCD_D3 8
#define LED_D2 7
#define LED_D1 6
#define LCD_RS 5
#define LCD_E 4

// Pins for sensors
#define DHT_PIN 10
#define TRIGGER_PIN 11
#define ECHO_PIN 12

// Interupt Pins
#define ENGINE_INT 2
#define BUZZER_INT 3

// Constants for distances
#define SPEED_OF_SOUND 0.034 // unit(cm/us)
#define ITER_M 10

// Tank dimention
#define TANK_BUTTOM_DISTANCE 104    // Distance of the water lavel, when the tank is empty
#define TANK_TOP_DISTANCE 25    // Distance of the water lavel, when the tank is full
#define TANK_REDIUS 50

#define MAX_DELAY_ON_ENGINE_OFF 600000UL


// Initializing the library with the numbers of the interface pins
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D3, LED_D2, LED_D1);

// Initializing the DHT library
#define DHTTYPE DHT11
DHT_Unified dht(DHT_PIN, DHTTYPE);

// running medium for storing distance 
RunningMedian running_durations = RunningMedian(ITER_M);

//Global Variables
byte engine_state = HIGH;
unsigned long delay_on_engine_off = 0;
float temp = 0;
float humidity = 0;
float water_percentage = 0;
float ltr = 0;
float distance = 0;

// Degree character for LCD display
byte degree[] = {
  B11100,
  B10100,
  B11100,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

void setup(){
  // set up the LCD's number of columns and rows:
  analogWrite(6, 75);

  pinMode(TRIGGER_PIN, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(ECHO_PIN, INPUT);     // Sets the echoPin as an INPUT
  pinMode(ENGINE_INT, INPUT_PULLUP);   //Sets the ENGINE_INT pin as INPUT_PULLUP
  pinMode(BUZZER_INT, INPUT_PULLUP);    //Sets the BUZZER_INT pin as INPUT_PULLUP

  attachInterrupt(digitalPinToInterrupt(ENGINE_INT), engine_isr, FALLING);   //Setting the ISR for ENGINE_INT to engine_isr function
  attachInterrupt(digitalPinToInterrupt(BUZZER_INT), buzzer_isr, FALLING);     //Setting the ISR for BUZZER_INT to buzzer_isr function
  
  lcd.begin(16, 2);
  lcd.createChar(0, degree);

  dht.begin();

  // Print a message to the LCD.
  print_copyright();
  delay(2000);
  lcd.clear();

  #if SERIAL_DEBUG
    Serial.begin(9600);
  #endif
}

void loop(){
  #if SERIAL_DEBUG
    Serial.println(engine_state);
  #endif
  if(engine_state){
    // Read data from the Sonar sensor and calculate the water volume, modify the global varibles (distance, ltr)
    set_tank_distance();
    calulate_volume();
    calculate_water_percentage();
    #if SERIAL_DEBUG
      Serial.println(F("Tank Calculation Done!"));
    #endif
    
    // Read data from the DHT sensor, modify the global varibles (temp, humidity)
    get_temp_humidity();
    #if SERIAL_DEBUG
      Serial.println(F("Temp Humidity Done!"));
    #endif
    
    // clears the display and print the global variables in the display, uses (distance, ltr, temp, humidity)
    lcd.clear();
    print_data_to_lcd();
    #if SERIAL_DEBUG
      Serial.println(F("Printing Done!"));
    #endif
    
    // Wait for 1.2 sec
    delay(1200);
  }
  else {
    set_tank_distance();
    calulate_volume();
    calculate_water_percentage();
    delay(delay_on_engine_off);
  }
}

void engine_isr(){
  engine_state = !engine_state;
  #if SERIAL_DEBUG
    Serial.println(F("Engine Flipped"));
  #endif
  if(engine_state){
    lcd.display();
    delay_on_engine_off = 0UL;
  }
  else{
    lcd.clear();
    lcd.noDisplay();
    delay_on_engine_off = MAX_DELAY_ON_ENGINE_OFF;
  }
  
}

void buzzer_isr(){
  #if SERIAL_DEBUG
    Serial.println(F("Buzzer Flipped"));
  #endif
  return;
}

void print_copyright(){
  lcd.setCursor(3, 0);
  lcd.print("Hello From");
  lcd.setCursor(4, 1);
  lcd.print("Arkadip");
}

void print_data_to_lcd(){
  lcd.setCursor(0, 0);
  lcd.print(temp);
  lcd.write(byte(0));
  lcd.print("C");

  lcd.setCursor(8, 0);
  lcd.print(ltr);
  lcd.print("L");

  lcd.setCursor(0, 1);
  lcd.print(humidity);
  lcd.print("%");

  lcd.setCursor(8, 1);
  lcd.print(water_percentage);
  lcd.print("%");
}

void get_temp_humidity(){
  //Sensor Event for DHT11
  sensors_event_t event;

  // Get temperature event and print its value.
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)){   
    temp = -1;      
    #if SERIAL_DEBUG
      Serial.println(F("Error reading temperature!"));
    #endif
  }
  else{
    temp = event.temperature;
    #if SERIAL_DEBUG
      Serial.print(F("Temperature: "));
      Serial.print(temp);
      Serial.println(F("°C"));
    #endif
  }

  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)){ 
    humidity = -1;
    #if SERIAL_DEBUG
      Serial.println(F("Error reading humidity!"));
    #endif
  }
  else{
    humidity = event.relative_humidity;
    #if SERIAL_DEBUG
      Serial.print(F("Humidity: "));
      Serial.print(humidity);
      Serial.println(F("%"));
    #endif
  }
}

void calculate_water_percentage(){
  water_percentage = (TANK_BUTTOM_DISTANCE - distance)/(TANK_BUTTOM_DISTANCE - TANK_TOP_DISTANCE) * 100;
}

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


void set_tank_distance(){
  distance = get_distance_median();
  
  #if SERIAL_DEBUG
    Serial.print(distance);
    Serial.println("cm");
  #endif
}

float get_distance_median(){
  for(int i=0; i<ITER_M; i++){
    running_durations.add((float)measure_single_duration());
    delay(300);
  }
  float duration = running_durations.getMedian();
  running_durations.clear();
  
  Serial.print("Duration: ");
  Serial.print(duration);
  Serial.print(" us ");
  // Calculating the distance
  float distance = duration * SPEED_OF_SOUND / 2; // Speed of sound wave divided by 2 (go and back)
  // Displays the distance on the Serial Monitor
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  return distance;
}

unsigned long measure_single_duration(){
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);
    // Reads the echoPin, returns the sound wave travel time in microseconds
    long d = pulseIn(ECHO_PIN, HIGH);
    Serial.print("duration");
    Serial.println(d);
    return d;
}
