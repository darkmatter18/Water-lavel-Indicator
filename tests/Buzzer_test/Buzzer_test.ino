#define BUZZER_PIN A1

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(LED_BUILTIN, HIGH);
}

// the loop function runs over and over again forever
void loop() {
                         // wait for a second
}
