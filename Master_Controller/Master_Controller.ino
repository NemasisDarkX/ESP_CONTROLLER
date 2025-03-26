#include "ESP32_NOW.h"
#include "WiFi.h"

#include <esp_mac.h>  // For the MAC2STR and MACSTR macros

#define ESPNOW_WIFI_CHANNEL 69

#define left 19
#define right 18
#define front 13
#define back 12
#define status 15 //to indicate transmission(not necessary)

class ESP_NOW_Broadcast_Peer : public ESP_NOW_Peer {
public:
  ESP_NOW_Broadcast_Peer(uint8_t channel, wifi_interface_t iface, const uint8_t *lmk)
      : ESP_NOW_Peer(ESP_NOW.BROADCAST_ADDR, channel, iface, lmk) {}

  ~ESP_NOW_Broadcast_Peer() { remove(); }

  bool begin() {
    if (!ESP_NOW.begin() || !add()) {
      log_e("PROTOCOL FAILED!");
      return false;
    }
    return true;
  }

  bool send_message(const uint8_t *data, size_t len) {
    if (!send(data, len)) {
      log_e("TRANSMISSION FAILED");
      return false;
    }
    return true;
  }
};

uint32_t msg_count = 0;
ESP_NOW_Broadcast_Peer broadcast_peer(ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, NULL);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);

  Serial.println("Master Controller");
  Serial.println("Wi-Fi parameters:");
  Serial.println("  Mode: STA");
  Serial.println("  MAC Address: " + WiFi.macAddress());
  Serial.printf("  Channel: %d\n", ESPNOW_WIFI_CHANNEL);

  if (!broadcast_peer.begin()) {
    Serial.println("Failed to initialize broadcast peer");
    Serial.println("Rebooting in 2 seconds...");
    delay(2000);
    ESP.restart();
  }

  Serial.println("Setup complete.");

  pinMode(front, INPUT);
  pinMode(back, INPUT);
  pinMode(left, INPUT);
  pinMode(right, INPUT);
  pinMode(status, OUTPUT);
}

void commands() {
  char data[2] = ""; 

  if (digitalRead(front) == HIGH) {
    data[0] = 'W';
  } else if (digitalRead(back) == HIGH) {
    data[0] = 'S';
  } else if (digitalRead(left) == HIGH) {
    data[0] = 'A';
  } else if (digitalRead(right) == HIGH) {
    data[0] = 'D';
  }

  if (data[0] != '\0') { 
    if (!broadcast_peer.send_message((uint8_t *)data, sizeof(data))) {
      Serial.println("Transmission Failed");
    }
    else {
      Serial.printf("Sent Command: %c\n", data[0]);
      digitalWrite(status,HIGH);
    }
  }

  delay(500);
}

void loop(){
  commands();
}