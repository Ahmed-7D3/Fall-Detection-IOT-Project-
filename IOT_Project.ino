#define BLYNK_TEMPLATE_ID "TMPL2EuW4OrzG"
#define BLYNK_TEMPLATE_NAME "IOT Project"
#define BLYNK_AUTH_TOKEN "DiqBvQc_hE97QMAagPfu3M-txFgk5460"

#include <Wire.h>
#include <MPU6050.h>
#include <BlynkSimpleStream.h> // For USB connection. Use BlynkSimpleEsp32 for WiFi.

#define TRIG_PIN 9      // Pin for the ultrasonic sensor trigger
#define ECHO_PIN 10     // Pin for the ultrasonic sensor echo
#define BUZZER_PIN 8    // Pin for the buzzer

// MPU6050 setup
MPU6050 mpu;

// Blynk credentials
char auth[] = BLYNK_AUTH_TOKEN;  // Blynk authentication token

// Constants for fall detection thresholds
const float fallThreshold = 2.5;     // Accelerometer value for detecting a fall
const float impactThreshold = 3.0;   // Acceleration threshold for impact detection (g)
const float distanceThreshold = 50.0; // Distance (cm) threshold
const float gyroThreshold = 200.0;   // Angular velocity threshold (degrees/sec)

// Variables
float distance_cm = 0.0;  // Distance variable for ultrasonic sensor

// Blynk timer
BlynkTimer timer;

// Function to read MPU6050 and send data to Blynk
void sendSensorData() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);  // Get sensor data from MPU6050

  // Convert raw values to g and deg/s
  float accelX = ax / 16384.0;
  float accelY = ay / 16384.0;
  float accelZ = az / 16384.0;
  float gyroX = gx / 131.0; // Gyro raw data to degrees/second
  float gyroY = gy / 131.0;
  float gyroZ = gz / 131.0;

  // Calculate total acceleration
  float totalAccel = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);

  // Send accelerometer data to Blynk
  Blynk.virtualWrite(V0, accelX);
  Blynk.virtualWrite(V1, accelY);
  Blynk.virtualWrite(V2, accelZ);

  // Send gyroscope data to Blynk
  Blynk.virtualWrite(V3, gyroX);
  Blynk.virtualWrite(V4, gyroY);
  Blynk.virtualWrite(V5, gyroZ);

  // Send smoothed acceleration data to Blynk
  Blynk.virtualWrite(V6, totalAccel);
}

// Function to read ultrasonic sensor and send data to Blynk
void sendUltrasonicData() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure distance using ultrasonic sensor
  float duration = pulseIn(ECHO_PIN, HIGH, 30000); // Timeout after 30ms (~5m max distance)
  if (duration == 0) {
    distance_cm = -1; // Invalid distance
  } else {
    distance_cm = duration * 0.01723;  // Convert duration to distance in cm
  }

  // Send distance to Blynk
  Blynk.virtualWrite(V7, distance_cm);
}

// Function to check for falls
void checkForFalls() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);  // Get sensor data from MPU6050

  // Convert raw accelerometer and gyroscope data to meaningful values
  float accelX = ax / 16384.0;
  float accelY = ay / 16384.0;
  float accelZ = az / 16384.0;
  float gyroX = gx / 131.0; 
  float gyroY = gy / 131.0;
  float gyroZ = gz / 131.0;    
  float totalAccel = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);  // Calculate total acceleration

  // Check for a fall based on acceleration and distance
  if (totalAccel >= impactThreshold) {
    Blynk.virtualWrite(V8, "High-impact Fall Detected!");  // Notify of high-impact fall
    soundBuzzer();  // Trigger buzzer alert
    Blynk.logEvent("fall_alert", "High-impact fall detected! Emergency attention needed.");
  } else if (totalAccel > fallThreshold && distance_cm < distanceThreshold) {
    Blynk.virtualWrite(V8, "Fall Detected!");  // Notify of detected fall
    soundBuzzer();  // Trigger buzzer alert
    Blynk.logEvent("fall_alert", "Fall detected! Immediate action required.");
  } else {
    Blynk.virtualWrite(V8, "No Fall");  // No fall detected
  }
}

// Buzzer alert function
void soundBuzzer() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

void setup() {
  // Initialize Serial and Blynk
  Serial.begin(9600);
  Blynk.begin(Serial, auth);  // Connect to Blynk using Serial (for USB connection)

  // Initialize MPU6050
  Wire.begin();
  mpu.initialize();  // Initialize the MPU6050 sensor
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");  // Check if MPU6050 is connected properly
    while (1); // Stay here if MPU fails
  }

  // Initialize ultrasonic sensor and buzzer
  pinMode(TRIG_PIN, OUTPUT);   // Set trigger pin as output
  pinMode(ECHO_PIN, INPUT);    // Set echo pin as input
  pinMode(BUZZER_PIN, OUTPUT); // Set buzzer pin as output
  digitalWrite(BUZZER_PIN, LOW);  // Ensure buzzer is off initially

  // Set up Blynk timers
  timer.setInterval(200L, sendSensorData);   // Send MPU6050 data every 200 ms
  timer.setInterval(500L, sendUltrasonicData); // Send ultrasonic data every 500 ms
  timer.setInterval(1000L, checkForFalls);   // Check for falls every 1 second
}

void loop() {
  if (Blynk.connected()) {
    Blynk.run();  // Run Blynk tasks
  }
  timer.run();  // Run Blynk timer tasks
}

