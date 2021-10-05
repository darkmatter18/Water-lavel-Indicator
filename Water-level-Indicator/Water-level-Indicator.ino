#include <RunningMedian.h>
#include <DHT.h>

#include "wiring.h"
#include "constants.h"
#include <EEPROM.h>

// Select the appropiate type of display needed for the development
// Comment out the unused one
// uncomment the using one
// #define DISPLAY DISPLAY_I2C    // Uncomment this if you are using a I2C diaplay
#define DISPLAY DISPLAY_SEEED // Uncomment this if you are using a seeed grove display
// Comment both if you are using a normal display

// *********** Interrupts ******************************************
// Select the appropiate type of inturrept for the development
// Comment out the unused one
// uncomment the using one
#define INTERRUPT INTERRUPT_PUSH_BUTTON // Uncomment this if you are using push buttons in interrupt
//#define INTERRUPT INTERRUPT_SLIDE_BUTTON  // Uncomment this if you are using sliding buttons in interrupt

#if DISPLAY == DISPLAY_I2C
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
// Initializing the library with the numbers of the interface pins
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_WIDTH, LCD_HEIGHT);

#elif DISPLAY == DISPLAY_SEEED
#include <Wire.h>
#include "rgb_lcd.h"
rgb_lcd lcd;

#else
#include <LiquidCrystal.h>
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
#endif

// Initializing the DHT library
DHT dht(DHT_PIN, DHT11);

// running medium for storing distance
RunningMedian running_distance = RunningMedian(ITER_M);

//Global Variables
byte buzzer_state = LOW;
byte self_stop_state = LOW;
float distance = 0;
float ltr = 0;
float water_percentage = 0;
float temp = 0;
float heat_index = 0;
float humidity = 0;

void setup()
{
  pinMode(TRIGGER_PIN, OUTPUT);         // Sets the TRIGGER_PIN as an OUTPUT (6)
  pinMode(ECHO_PIN, INPUT);             // Sets the ECHO_PIN as an INPUT (5)
  pinMode(BUZZER_PIN, OUTPUT);          // Sets the BUZZER_PIN as an INPUT (A1)
  pinMode(SELF_STOP_RELAY_PIN, OUTPUT); // Sets the SELF_STOP_RELAY_PIN as an INPUT (4)
  pinMode(BUZZER_INT, INPUT_PULLUP);    // Sets the BUZZER_INT pin as INPUT_PULLUP (2)
  pinMode(SELF_STOP_INT, INPUT_PULLUP); // Sets the BUZZER_INT pin as INPUT_PULLUP (3)

  randomSeed(analogRead(A2));

#if INTERRUPT == INTERRUPT_PUSH_BUTTON
  attachInterrupt(digitalPinToInterrupt(BUZZER_INT), buzzer_Isr, FALLING);
  attachInterrupt(digitalPinToInterrupt(SELF_STOP_INT), self_stop_Isr, FALLING);
  buzzer_state = EEPROM.read(EEPROM_BUZZER_INT_ADDR);
  self_stop_state = EEPROM.read(EEPROM_SELF_STOP_INT_ADDR);
#elif INTERRUPT == INTERRUPT_SLIDE_BUTTON
  attachInterrupt(digitalPinToInterrupt(BUZZER_INT), buzzer_Isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SELF_STOP_INT), self_stop_Isr, CHANGE);

  buzzer_state = digitalRead(BUZZER_INT);
  self_stop_state = digitalRead(SELF_STOP_INT);
#endif

// Initialize the LCD
#if DISPLAY == DISPLAY_I2C
  lcd.begin();
  lcd.backlight();
#elif DISPLAY == DISPLAY_SEEED
  lcd.begin(16, 2);
#else
  lcd.begin(16, 2);
#endif

  // Start the DHT11
  dht.begin();

  print_start_msg();
  print_calculating();

#if SERIAL_DEBUG
  Serial.begin(9600);
#endif

#if SERIAL_DEBUG
#if INTERRUPT == INTERRUPT_PUSH_BUTTON
  Serial.println("At Initialization:");
  Serial.print("BUZZER_INT:  ");
  Serial.println(buzzer_state);
  Serial.print("SELF_STOP_INT:  ");
  Serial.println(self_stop_state);
#elif INTERRUPT == INTERRUPT_SLIDE_BUTTON
  Serial.println("At Initialization:");
  Serial.print("BUZZER_INT:  ");
  Serial.println(digitalRead(BUZZER_INT));
  Serial.print("SELF_STOP_INT:  ");
  Serial.println(digitalRead(SELF_STOP_INT));
#else
  Serial.println("No interrupt is added");
#endif
#endif
}

void loop()
{
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

  buzzer_routine();
  self_stop_routine();
}

/**
 * @brief Buzzer Interrupt service routine
 * 
 */
void buzzer_Isr()
{
#if INTERRUPT == INTERRUPT_PUSH_BUTTON
  buzzer_state = buzzer_state == LOW ? HIGH : LOW;
  EEPROM.write(EEPROM_BUZZER_INT_ADDR, buzzer_state);
#elif INTERRUPT == INTERRUPT_SLIDE_BUTTON
  buzzer_state = digitalRead(BUZZER_INT);
  digitalWrite(BUZZER_PIN, LOW);
#endif
#if SERIAL_DEBUG
  Serial.println("Buzzer Interrupt Triggered");
  Serial.print("BUZZER_INT:  ");
  Serial.println(buzzer_state);
#endif
}

/**
 * @brief Self Stop Interrupt service routine
 * 
 */
void self_stop_Isr()
{
#if INTERRUPT == INTERRUPT_PUSH_BUTTON
  self_stop_state = self_stop_state == LOW ? HIGH : LOW;
  EEPROM.write(EEPROM_SELF_STOP_INT_ADDR, self_stop_state);
#elif INTERRUPT == INTERRUPT_SLIDE_BUTTON
  self_stop_state = digitalRead(SELF_STOP_INT);
#endif

#if SERIAL_DEBUG
  Serial.println("Self Stop Interrupt Triggered");
  Serial.print("SELF_STOP_INT:  ");
  Serial.println(self_stop_state);
#endif
}

/**
 * @brief Routine for buzzer blow
 * Checks for the buzzer state
 * If the state is HIGH
 * And if the water lavel is more than BUZZER_THRESHOLD_PERCENTAGE
 * Then blow the buzzer
 * 
 */
void buzzer_routine()
{
  if (buzzer_state == HIGH)
  {
    if (water_percentage >= BUZZER_THRESHOLD_PERCENTAGE)
    {
      digitalWrite(BUZZER_PIN, HIGH);
    }
  }
}

/**
 * @brief Routine for Self Stop
 * Checks for the self stop state
 * If the state is HIGH
 * And if the water lavel is more than STOP_THRESHOLD_PERCENTAGE
 * Then make the self slow command
 * 
 */
void self_stop_routine()
{
  if (self_stop_state == HIGH)
  {
    if (water_percentage >= STOP_THRESHOLD_PERCENTAGE)
    {
      self_stop_command();
    }
  }
}

/**
 * @brief The command for self stop Relay
 * 
 * First HIGH the RELAY PIN
 * wait for 1 sec
 * Then, LOW the RELAY PIN
 */
void self_stop_command()
{
  digitalWrite(SELF_STOP_RELAY_PIN, HIGH);
  delay(1000);
  digitalWrite(SELF_STOP_RELAY_PIN, LOW);
}

/**
 * @brief Printing DATA in LCD
 * 
 */
void print_data_to_lcd()
{
  lcd.setCursor(0, 0);
  lcd.print(temp, 1);
  lcd.print("C");

  lcd.setCursor(7, 0);
  lcd.print(round(humidity));
  lcd.print("%");

  lcd.setCursor(11, 0);
  lcd.print(heat_index, 1);
  lcd.print("C");

  if (buzzer_state == HIGH)
  {
    lcd.setCursor(0, 1);
    lcd.print("B");
  }

  lcd.setCursor(3, 1);
  lcd.print(round(ltr));
  lcd.print("L");

  lcd.setCursor(9, 1);
  lcd.print(round(water_percentage));
  lcd.print("%");

  if (self_stop_state == HIGH)
  {
    lcd.setCursor(15, 1);
    lcd.print("S");
  }
}

/**
 * @brief Print the start msg
 * Then wait 1.5sec and then clears the display
 * 
 */
void print_start_msg()
{
  switch (random(1, 5))
  {
  case 1:
    lcd.setCursor(2, 0);
    lcd.print("Water Level");
    lcd.setCursor(5, 1);
    lcd.print("Meter");
    break;

  case 2:
    lcd.setCursor(2, 0);
    lcd.print("Save Water");
    lcd.setCursor(3, 1);
    lcd.print("Save Life");
    break;

  case 3:
    lcd.setCursor(3, 0);
    lcd.print("No Water");
    lcd.setCursor(4, 1);
    lcd.print("No Life");
    break;

  case 4:
    lcd.setCursor(1, 0);
    lcd.print("Water is Life");
    lcd.setCursor(0, 1);
    lcd.print("Don't waste it");
    break;

  default:
    lcd.setCursor(2, 0);
    lcd.print("Water Level");
    lcd.setCursor(5, 1);
    lcd.print("Meter");
    break;
  }

  delay(1500);
  lcd.clear();
}

/**
 * @brief Print Calculating 
 * 
 */
void print_calculating()
{
  lcd.setCursor(0, 0);
  lcd.print("Calculating...");
}

/**
 * @brief Read the sensor value from DHT11 and
 * Set the temp, humidity and heat_index global variable
 */
void get_temp_humidity()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t))
  {
    Serial.println("Failed to read from DHT sensor!");
    temp = humidity = -1;
    heat_index = -1;
    return;
  }
  else
  {
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
void calculate_water_percentage()
{
  int x = (TANK_BUTTOM_DISTANCE - distance) / (TANK_BUTTOM_DISTANCE - TANK_TOP_DISTANCE) * 100;
  if (x >= 100)
  {
    water_percentage = 100;
  }
  else
  {
    water_percentage = x;
  }
}

/**
 * @brief Calculates the volume of water in liters
 * Uses the distance global variable
 * 
 * Writes the ltr global variable
 */
void calulate_volume()
{
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
void set_tank_distance()
{
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
float get_distance_median()
{
  for (int i = 0; i < ITER_M; i++)
  {
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
unsigned long measure_single_duration()
{
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
