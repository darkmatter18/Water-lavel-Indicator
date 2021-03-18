#define INT_PIN_1 2
#define INT_PIN_2 3

void setup() {
  pinMode(INT_PIN_1, INPUT_PULLUP);
  pinMode(INT_PIN_2, INPUT_PULLUP);

  Serial.begin(9600);

  Serial.println("At Initialization:");
  Serial.print("Int 1:  ");
  Serial.println(digitalRead(INT_PIN_1));
  Serial.print("Int 2:  ");
  Serial.println(digitalRead(INT_PIN_2));
  
  attachInterrupt(digitalPinToInterrupt(INT_PIN_1), Int_1_Isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(INT_PIN_2), Int_2_Isr, CHANGE); 

}

void loop() {
  // put your main code here, to run repeatedly:

}

void Int_1_Isr(){
  Serial.print("Int 1 changed. New Value is:  ");
  Serial.println(digitalRead(INT_PIN_1));
}


void Int_2_Isr(){
  Serial.print("Int 2 changed. New Value is:  ");
  Serial.println(digitalRead(INT_PIN_2));  
}
