#include <WiFi.h>
#include <esp_now.h>

// --- Original Single Pins ---
#define LED_PIN 19
#define BUZZER_PIN 18
#define ONBOARD_LED 2

// --- New Switch Pin ---
#define SWITCH_PIN 4 

// --- 10-LED Bar Graph Pins ---
const int ledBarPins[10] = {13,12,14,27,26,25,33,32,35,34};

// --- Signal Tracking Variables ---
int lastRSSI = -100;
unsigned long lastReceiveTime = 0;

// --- Blinking/Buzzing State Variables ---
unsigned long previousTime = 0; 
bool outputState = LOW; 

// --- RSSI and Interval Boundaries ---
const int SILENCE_THRESHOLD = -85; // If signal is weaker than -85, stay completely silent
const int MIN_RSSI = -84; // Start beeping slowly at this signal strength
const int MAX_RSSI = -30; // Strongest expected signal

// Interval bounds (in milliseconds)
const int MAX_INTERVAL = 1000; // 1 second gap between beeps for weak signals
const int MIN_INTERVAL = 0;    // 0ms gap = solid ON for strong signals
const int BEEP_DURATION = 50;  // How long the actual beep lasts

// --- Hardcoded Settings ---
const int buzzerPitch = 1000; // 1000 Hz is a standard clear beep
const int ledBrightness = 10; // Set ONBOARD_LED brightness from 0 (off) to 255 (max bright)

// Callback function: Executes when data is received
void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  lastRSSI = info->rx_ctrl->rssi;
  lastReceiveTime = millis();
}

// Helper function to easily update the 10-LED bar graph
void updateLEDBar(int numLedsOn) {
  for (int i = 0; i <= 10; i++) {
    if (i < numLedsOn) {
      digitalWrite(ledBarPins[i], HIGH); 
    } else {
      digitalWrite(ledBarPins[i], LOW);  
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT); 
  
  // Initialize switch pin
  pinMode(SWITCH_PIN, INPUT);

  // Initialize all 10 LED bar pins
  for (int i = 0; i < 10; i++) {
    pinMode(ledBarPins[i], OUTPUT);
  }

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  esp_now_register_recv_cb(onReceive);
  Serial.println("Receiver Ready");
}

void loop() {
  // --- 0. Master Switch Logic ---
  // Check if the switch is connected to GND (LOW)
  if (digitalRead(SWITCH_PIN) == LOW) {
    
    // Safety measure: Force everything OFF
    outputState = LOW;
    digitalWrite(LED_PIN, LOW);
    analogWrite(ONBOARD_LED, 0); // Turn OFF onboard LED
    noTone(BUZZER_PIN);
    updateLEDBar(0);
    
    Serial.println("System Paused. Flip switch to VCC to resume.");

    // Enter infinite loop until switch is flipped to VCC (HIGH)
    while (digitalRead(SWITCH_PIN) == LOW) {
      delay(10); // Feed the watchdog timer
    }
    
    Serial.println("System Resumed!");
  }


  // --- Resume Normal Operation if Switch is HIGH ---
  unsigned long currentMillis = millis();

  // --- 1. Serial Monitor Reporting (Once per second) ---
  static unsigned long lastPrint = 0;
  if (currentMillis - lastPrint > 1000) {
    lastPrint = currentMillis;

    if (currentMillis - lastReceiveTime > 2000) {
      Serial.println("Not detected");
    } else {
      Serial.print("RSSI: ");
      Serial.println(lastRSSI);
    }
  }

  // --- 2. LED Bar Graph Update Logic ---
  int activeLeds = 0; 
  
  if (currentMillis - lastReceiveTime <= 2000 && lastRSSI >= SILENCE_THRESHOLD) {
    activeLeds = map(lastRSSI, SILENCE_THRESHOLD, -40, 0, 10);
    activeLeds = constrain(activeLeds, 0, 10);
  }
  
  updateLEDBar(activeLeds);


  // --- 3. Beep and Single LED Blinking Logic ---
  if (currentMillis - lastReceiveTime > 2000 || lastRSSI < SILENCE_THRESHOLD) {
    outputState = LOW;
    digitalWrite(LED_PIN, LOW);
    analogWrite(ONBOARD_LED, 0); // Turn OFF onboard LED
    noTone(BUZZER_PIN); 
  } 
  else {
    int delayInterval = map(lastRSSI, MIN_RSSI, MAX_RSSI, MAX_INTERVAL, MIN_INTERVAL);
    delayInterval = constrain(delayInterval, MIN_INTERVAL, MAX_INTERVAL);

    if (delayInterval <= 15) {
      digitalWrite(LED_PIN, HIGH);
      analogWrite(ONBOARD_LED, ledBrightness); // Turn ON onboard LED at custom brightness
      tone(BUZZER_PIN, buzzerPitch); 
      outputState = HIGH; 
    } 
    else {
      if (outputState == HIGH) {
        if (currentMillis - previousTime >= BEEP_DURATION) {
          previousTime = currentMillis;
          outputState = LOW;
          digitalWrite(LED_PIN, LOW);
          analogWrite(ONBOARD_LED, 0);  // Turn OFF onboard LED
          noTone(BUZZER_PIN);             
        }
      } else {
        if (currentMillis - previousTime >= delayInterval) {
          previousTime = currentMillis;
          outputState = HIGH;
          digitalWrite(LED_PIN, HIGH);
          analogWrite(ONBOARD_LED, ledBrightness); // Turn ON onboard LED at custom brightness
          tone(BUZZER_PIN, buzzerPitch);  
        }
      }
    }
  }
}