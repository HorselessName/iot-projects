/*  Programa para Printar Hello World
    Autor: Raul Chiarella
*/

void setup() {
  Serial.begin(115200); // Initialize serial communication with 115200 baud rate
}

void loop() {
  Serial.println("Hello World"); // Print "Hello World" to the Serial Monitor
  delay(5000); // Wait for 5 seconds
}
