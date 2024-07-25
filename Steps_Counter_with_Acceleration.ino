//code to print acceleration values of x,y,z axis , print current date/month/year , time and total no. of steps which will be store in its memory (eeprom) and gets deleted at night . It will share data via bluetooth to bluetooth terminal app . Using MPU6050, connections are 21,22 using i2c method. 

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BluetoothSerial.h>
#include <Preferences.h>
#include <WiFi.h>
#include <time.h>

// Replace with your network credentials
const char* ssid = "Your_SSID";
const char* password = "Your_Password";

// NTP Server and Timezone settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800; // UTC+5:30
const int daylightOffset_sec = 0; // No DST adjustment

Adafruit_MPU6050 mpu;
BluetoothSerial SerialBT;
Preferences preferences;

const float threshold = 1.2; // Adjust this threshold for step detection sensitivity
const int bufferLength = 15; // Number of accelerometer readings in the buffer
float buffer[bufferLength];
int bufferIndex = 0;
int currentStepCount = 0;
int totalStepCount = 0;
bool stepDetected = false;

const unsigned long debounceDelay = 300; // Debounce delay in milliseconds
unsigned long lastStepTime = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialize Adafruit MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1);
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Initialize Bluetooth
  SerialBT.begin("ESP32_Step_Counter"); // Bluetooth device name
  Serial.println("The device started, now you can pair it with Bluetooth!");

  // Initialize preferences
  preferences.begin("step_counter", false);

  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");

  // Initialize time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Time syncing...");

  // Wait for the time to be synchronized
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) { // Wait until we get a valid time (use a valid timestamp)
    delay(1000);
    now = time(nullptr);
    Serial.print(".");
  }
  Serial.println("\nTime synchronized!");

  // Load the last saved total step count from EEPROM
  totalStepCount = preferences.getInt("total_steps", 0); // Default to 0 if not found

  Serial.print("Loaded total step count from EEPROM: ");
  Serial.println(totalStepCount);
}

void loop() {
  // Variables to store accelerometer readings
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float accelerationX = a.acceleration.x;
  float accelerationY = a.acceleration.y;
  float accelerationZ = a.acceleration.z;

  float accelerationMagnitude = sqrt(accelerationX * accelerationX +
                                     accelerationY * accelerationY +
                                     accelerationZ * accelerationZ);

  buffer[bufferIndex] = accelerationMagnitude;
  bufferIndex = (bufferIndex + 1) % bufferLength;

  // Detect a step if the current magnitude is greater than the average of the buffer by the threshold
  float avgMagnitude = 0;
  for (int i = 0; i < bufferLength; i++) {
    avgMagnitude += buffer[i];
  }
  avgMagnitude /= bufferLength;

  unsigned long currentMillis = millis();

  if (accelerationMagnitude > (avgMagnitude + threshold)) {
    if (!stepDetected && (currentMillis - lastStepTime) > debounceDelay) {
      currentStepCount++;
      totalStepCount++;
      stepDetected = true;
      lastStepTime = currentMillis;

      Serial.println("Step detected!");
      Serial.print("Current steps: ");
      Serial.println(currentStepCount);
      Serial.print("Total steps: ");
      Serial.println(totalStepCount);

      // Store the updated total step count in EEPROM
      preferences.putInt("total_steps", totalStepCount);

      // Send step count to Bluetooth terminal
      SerialBT.print("Current steps: ");
      SerialBT.println(currentStepCount);
      SerialBT.print("Total steps: ");
      SerialBT.println(totalStepCount);
    }
  } else {
    stepDetected = false;
  }

  // Print the current date and time every second
  static unsigned long lastTimePrint = 0;
  if (millis() - lastTimePrint >= 1000) {
    lastTimePrint = millis();
    printLocalTime();
    // Print accelerometer readings
    printAccelerometerData();
  }

  // Check if it's 12:00:00 to reset step count
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  if (timeinfo.tm_hour == 12 && timeinfo.tm_min == 0 && timeinfo.tm_sec == 0) {
    resetStepCount();
  }
}

void printLocalTime() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  Serial.print("Current date and time: ");
  Serial.print(timeinfo.tm_year + 1900); // Year since 1900
  Serial.print("-");
  Serial.print(timeinfo.tm_mon + 1);    // Month [0-11]
  Serial.print("-");
  Serial.print(timeinfo.tm_mday);        // Day of the month
  Serial.print(" ");
  Serial.print(timeinfo.tm_hour);
  Serial.print(":");
  Serial.print(timeinfo.tm_min);
  Serial.print(":");
  Serial.print(timeinfo.tm_sec);
  Serial.println();

  // Send current date and time to Bluetooth terminal
  SerialBT.print("Current date and time: ");
  SerialBT.print(timeinfo.tm_year + 1900); // Year since 1900
  SerialBT.print("-");
  SerialBT.print(timeinfo.tm_mon + 1);    // Month [0-11]
  SerialBT.print("-");
  SerialBT.print(timeinfo.tm_mday);        // Day of the month
  SerialBT.print(" ");
  SerialBT.print(timeinfo.tm_hour);
  SerialBT.print(":");
  SerialBT.print(timeinfo.tm_min);
  SerialBT.print(":");
  SerialBT.print(timeinfo.tm_sec);
  SerialBT.println();
}

void printAccelerometerData() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  Serial.print("Accelerometer X: ");
  Serial.print(a.acceleration.x, 2); // Print with 2 decimal places
  Serial.print(" | Y: ");
  Serial.print(a.acceleration.y, 2); // Print with 2 decimal places
  Serial.print(" | Z: ");
  Serial.println(a.acceleration.z, 2); // Print with 2 decimal places

  // Send accelerometer data via Bluetooth
  SerialBT.print("Accelerometer X: ");
  SerialBT.print(a.acceleration.x, 2); // Print with 2 decimal places
  SerialBT.print(" | Y: ");
  SerialBT.print(a.acceleration.y, 2); // Print with 2 decimal places
  SerialBT.print(" | Z: ");
  SerialBT.println(a.acceleration.z, 2); // Print with 2 decimal places
}

void resetStepCount() {
  // Get current date and time
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  // Format the date and time
  String dateTime = String(timeinfo.tm_year + 1900) + "-" +
                    String(timeinfo.tm_mon + 1) + "-" +
                    String(timeinfo.tm_mday) + " " +
                    String(timeinfo.tm_hour) + ":" +
                    String(timeinfo.tm_min) + ":" +
                    String(timeinfo.tm_sec);

  // Print current date, time, and total number of steps before resetting
  Serial.print("Current date and time: ");
  Serial.print(dateTime);
  Serial.print(", Total number of steps: ");
  Serial.println(totalStepCount);

  SerialBT.print("Current date and time: ");
  SerialBT.print(dateTime);
  SerialBT.print(", Total number of steps: ");
  SerialBT.println(totalStepCount);

  // Reset step counts
  currentStepCount = 0;
  totalStepCount = 0; // Reset total step count as well
  preferences.putInt("total_steps", 0); // Clear total step count in EEPROM

  Serial.println("Step counts reset.");
  SerialBT.println("Step counts reset.");
}
