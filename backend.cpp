#include <WiFi.h>
#include <WebServer.h>

// Replace with your network credentials
const char* ssid = "JeremyTan";
const char* password = "plsconnect";

// Create an instance of the web server running on port 80
WebServer server(80);

const int en_left = 18;
const int en_right = 19;
const int dirleft = 32;
const int dirright = 33;

const int ultrasoundPin = 12;
const int radioPin = 25;
const int infraredPin = 27;
const int hallSensorPin = 34; 

// Function to handle root path
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void moveMotors(int leftSpeed, int rightSpeed, int leftDir, int rightDir) {
    analogWrite(en_left, leftSpeed);
    analogWrite(en_right, rightSpeed);
    analogWrite(dirleft, leftDir);
    analogWrite(dirright, rightDir);
}

void forward() {
    int speed = server.arg("speed").toInt();
    int pwmValue = map(speed, 0, 100, 0, 255);
    moveMotors(pwmValue, pwmValue, 1000, 1000);
    server.send(200, F("text/plain"), F("forward"));
}

void reverse() {
    int speed = server.arg("speed").toInt();
    int pwmValue = map(speed, 0, 100, 0, 255);
    moveMotors(pwmValue, pwmValue, 0, 0);
    server.send(200, F("text/plain"), F("reverse"));
}

void left() {
    int speed = server.arg("speed").toInt();
    int pwmValue = map(speed, 0, 100, 0, 255);
    moveMotors(0, pwmValue, 1000, 1000);
    server.send(200, F("text/plain"), F("left"));
}

void right() {
    int speed = server.arg("speed").toInt();
    int pwmValue = map(speed, 0, 100, 0, 255);
    moveMotors(pwmValue, 0, 1000, 1000);
    server.send(200, F("text/plain"), F("right"));
}

void off() {
    moveMotors(0, 0, 0, 0);
    server.send(200, F("text/plain"), F("off"));
}

// Function to handle ultrasound data
void handleUltrasound() {
	if (Serial1.available()) {
    String name_received = Serial1.readStringUntil('#');
    Serial.print("Name received: ");
    Serial.println(name_received);
    server.send(200, "text/plain", name_received);
  }
  else{
    server.send(200, "text/plain", "error");
  }
}

void handleInfrared() {
  // Variables to store the number of pulses and the frequency
  unsigned long pulseCount = 0;
  unsigned long previousMillis = 0;
  unsigned long interval = 1000; // Interval for measuring frequency (in milliseconds)
  float frequency = 0.0;
  unsigned long duration = 500;

  // Variables to store the state of the input pin
  int lastState = LOW;
  int currentState;
  // Get the current state of the input pin
  unsigned long startMillis = millis();
  pulseCount = 0; // Reset pulse count at the start

  while (millis() - startMillis < duration) {
    // Get the current state of the input pin
    currentState = digitalRead(infraredPin);

    // Check for a rising edge
    if (currentState == HIGH && lastState == LOW) {
      pulseCount++;
    }

    // Update the last state
    lastState = currentState;
  }

  // Calculate the frequency
  frequency = pulseCount * (1000.0 / duration);
  
  // Send the frequency as a response
  Serial.println("Infrared Frequency: " + String(frequency));
  server.send(200, "text/plain", String(frequency));
}

// Function to handle radio frequency data
void handleRadio() {
  // Variables to store the number of pulses and the frequency
  unsigned long pulseCount = 0;
  unsigned long previousMillis = 0;
  unsigned long interval = 1000; // Interval for measuring frequency (in milliseconds)
  float frequency = 0.0;
  unsigned long duration = 500;

  // Variables to store the state of the input pin
  int lastState = LOW;
  int currentState;
  // Get the current state of the input pin
  unsigned long startMillis = millis();
  pulseCount = 0; // Reset pulse count at the start

  while (millis() - startMillis < duration) {
    // Get the current state of the input pin
    currentState = digitalRead(radioPin);

    // Check for a rising edge
    if (currentState == HIGH && lastState == LOW) {
      pulseCount++;
    }

    // Update the last state
    lastState = currentState;
  }

  // Calculate the frequency
  frequency = pulseCount * (1000.0 / duration);
  
  // Send the frequency as a response
  Serial.println("Radio Frequency: " + String(frequency));
  server.send(200, "text/plain", String(frequency));
}

// Function to handle magnetic field data
void handleMagnetic() {
  const float centerVoltage = 2.02; // Center voltage
  const float tolerance = 0.05; // Tolerance range for the center voltage
  // Read the analog voltage from the Hall effect sensor
  int sensorValue = analogRead(hallSensorPin);
  // Convert the analog reading (0-1023) to a voltage (0-5V)
  float voltage = sensorValue * (3.3 / 4095.0);

  Serial.print("Magnet Voltage: ");
  Serial.println(voltage, 2); // Print the voltage with 2 decimal places

  String magneticField;

  // Determine the magnetic field direction
  if (voltage < (centerVoltage - tolerance)) {
    magneticField = "N";
  } else if (voltage > (centerVoltage + tolerance)) {
    magneticField = "S";
  } else {
    magneticField = "-";
  }

  Serial.println(magneticField);
  server.send(200, "text/plain", magneticField);
}

void setup() {
  // Start serial communication for debugging purposes
  Serial.begin(9600);

  Serial1.begin(600, SERIAL_8N1, ultrasoundPin, 13); // Initialize UART:RX pin 12, TX pin 13, baud rate 600

  // Set the input pin mode
  pinMode(radioPin, INPUT);
  pinMode(infraredPin, INPUT);
  pinMode(hallSensorPin,INPUT);

  // Connect to Wi-Fi network
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print('.');
  }
  Serial.println("Connected to the WiFi network");

  // Print the ESP32 IP address
  Serial.println(WiFi.localIP());

  // Define the root path and other paths
  server.on("/", handleRoot);
  server.on(F("/forward"), forward);
  server.on(F("/reverse"), reverse);
  server.on(F("/right"), right);
  server.on(F("/left"), left);
  server.on(F("/off"), off);
  server.on(F("/ultrasound"), handleUltrasound);
  server.on(F("/infrared"), handleInfrared);
  server.on(F("/radio"), handleRadio);
  server.on(F("/magnetic"), handleMagnetic);

  // Start the server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Handle client requests
  server.handleClient();
}