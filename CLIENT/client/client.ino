#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_sleep.h>


// rtc clock pins -> sda = gpio21 && scl = gpio22
#define SENSOR_PIN 13 // GPIO13 for presence sensor
#define SOUND_SENSOR_PIN 35 // GPIO35 for sound sensor (ADC1_CH7)

const unsigned long timeout = 2000; // Timeout period in milliseconds
const int soundThreshold = 1000; // Adjust this value based on your sound sensor
const unsigned long thresholdWindow = 5 * 60 * 1000; // 5 minutes in milliseconds

unsigned long latestDetectionTime = 0; // Variable to store the last detection time
unsigned long presenceDuration = 0; // Variable to store the duration of human presence
bool isPresenceDetected = false; // Flag to indicate if presence is currently detected

int soundThresholdCount = 0; // Counter for threshold crossings within the 5-minute window
unsigned long firstThresholdTime = 0; // Time of the first threshold crossing
unsigned long eventStartTime = 0; // Time when the event starts
int totalSoundTriggers = 0; // Total sound triggers during the event

RTC_DS3231 rtc;

// WiFi credentials
const char* ssid = "Young-Mobile";
const char* password = "ttjsnz23";

const char* serverUrl = "http://localhost:5555/events"; // Replace with your backend URL

void setup() {
  Serial.begin(115200); 
  pinMode(SENSOR_PIN, INPUT); // Set the presence sensor pin as input
  pinMode(SOUND_SENSOR_PIN, INPUT); // Set the sound sensor pin as input
  
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1); // Stop execution if RTC is not found
  }

  // Initially, do not connect to Wi-Fi
  WiFi.disconnect(true); // Ensure Wi-Fi is disconnected at startup

  // Configure wakeup sources for deep sleep
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 1); // Wake up when the presence sensor detects a human
  esp_sleep_enable_ext1_wakeup((1ULL << SENSOR_PIN) | (1ULL << SOUND_SENSOR_PIN), ESP_EXT1_WAKEUP_ANY_HIGH); // Wake up when the sound sensor or presence sensor is triggered

  // Print wakeup reason
  printWakeupReason();
}

void loop() {
  int sensorValue = digitalRead(SENSOR_PIN);
  int analogValue = analogRead(SOUND_SENSOR_PIN);

  if (sensorValue == HIGH) {
    if (!isPresenceDetected) {
      latestDetectionTime = millis(); // Record the time of detection
      isPresenceDetected = true; // Set the presence detected flag
      DateTime now = rtc.now(); // Get the current date and time
      Serial.printf("%04d/%02d/%02d %02d:%02d:%02d - Human detected\n", 
                    now.year(), now.month(), now.day(), 
                    now.hour(), now.minute(), now.second()); // Print detection message
    }
  }

  if (analogValue > soundThreshold) {
    if (soundThresholdCount == 0) {
      firstThresholdTime = millis(); // Record the time of the first threshold crossing
    }
    soundThresholdCount++; // Increment the threshold count
  }

  if (isPresenceDetected && soundThresholdCount >= 3 && millis() - firstThresholdTime <= thresholdWindow) {
    if (eventStartTime == 0) {
      eventStartTime = millis(); // Record the start time of the event
    }
    totalSoundTriggers++; // Increment the sound trigger count
  }

  if ((isPresenceDetected && sensorValue == LOW) || (soundThresholdCount >= 3 && millis() - firstThresholdTime > thresholdWindow)) {
    if (eventStartTime != 0) {
      presenceDuration = millis() - eventStartTime; // Calculate the presence duration
      sendEvent(); // Send the event data
      resetEventState(); // Reset the event state variables
    }
  }

  if (millis() - firstThresholdTime > thresholdWindow) {
    soundThresholdCount = 0;
    firstThresholdTime = millis(); // Reset the timer
  }

  delay(500); // Add a small delay to avoid flooding the serial output

  // Enter deep sleep to save power
  enterDeepSleep();
}

void resetEventState() {
  isPresenceDetected = false; // Reset presence detected flag
  soundThresholdCount = 0; // Reset sound threshold count
  eventStartTime = 0; // Reset event start time
  totalSoundTriggers = 0; // Reset total sound triggers count
}

void connectToWiFi() {
  WiFi.begin(ssid, password); // Start the Wi-Fi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000); // Wait for 1 second before checking again
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi"); // Print message when connected to Wi-Fi
}

void disconnectFromWiFi() {
  WiFi.disconnect(true); // Disconnect from Wi-Fi
  Serial.println("Disconnected from WiFi"); // Print message when disconnected from Wi-Fi
}

void sendEvent() {
  connectToWiFi(); // Connect to Wi-Fi before sending the event

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http; // Create an HTTP client object
    http.begin(serverUrl); // Initialize the HTTP client with the server URL
    http.addHeader("Content-Type", "application/json"); // Set the content type to JSON

    DateTime now = rtc.now(); // Get the current date and time
    // Construct the JSON payload
    String payload = "{";
    payload += "\"timestamp\":\"" + String(now.year()) + "-" + String(now.month()) + "-" + String(now.day()) + "T" + 
               String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + "\",";
    payload += "\"duration\":" + String(presenceDuration / 1000.0) + ",";
    payload += "\"soundTriggers\":" + String(totalSoundTriggers);
    payload += "}";

    int httpResponseCode = http.POST(payload); // Send the POST request with the JSON payload

    if (httpResponseCode > 0) {
      String response = http.getString(); // Get the response from the server
      Serial.println("HTTP Response code: " + String(httpResponseCode)); // Print the HTTP response code
      Serial.println("Response: " + response); // Print the server response
    } else {
      Serial.println("Error on sending POST: " + String(httpResponseCode)); // Print error message
    }

    http.end(); // End the HTTP connection
  } else {
    Serial.println("WiFi not connected"); // Print error message if Wi-Fi is not connected
  }

  disconnectFromWiFi(); // Disconnect from Wi-Fi after sending the event
}

void enterDeepSleep() {
  Serial.println("Entering deep sleep");
  esp_deep_sleep_start(); // Enter deep sleep
}

void printWakeupReason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Wakeup caused by external signal using RTC_IO");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("Wakeup caused by external signal using RTC_CNTL");
      break;
    default:
      Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
      break;
  }
}
