#include <WiFi.h>
#include <esp_now.h>

uint8_t receiverAddress[] = {0x48, 0xe7, 0x29, 0x96, 0xd0, 0x38}; // 

typedef struct {
  int value;
} Message;

Message msg;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("Sender Ready");
}

void loop() {
  msg.value = 1;  // dummy data

  esp_now_send(receiverAddress, (uint8_t *) &msg, sizeof(msg));

  delay(200); // send 5 times per second
}