#include "ESP32_NOW.h"
#include "WiFi.h"
#include <esp_mac.h>  // For the MAC2STR and MACSTR macros
#include <vector>

/* Definitions */
#define ESPNOW_WIFI_CHANNEL 69

// Motor driver pins
#define IN1 19
#define IN2 18
#define IN3 5
#define IN4 4

/* Setup Function */
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  // Initialize the Wi-Fi module
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) delay(100);

  Serial.println("ESP Receiver");
  Serial.println("Wi-Fi parameters:");
  Serial.println("  Mode: STA");
  Serial.println("  MAC Address: " + WiFi.macAddress());
  Serial.printf("  Channel: %d\n", ESPNOW_WIFI_CHANNEL);

  // Initialize ESP-NOW
  if (!ESP_NOW.begin()) {
    Serial.println("PROTOCOL FAILED");
    Serial.println("Rebooting in 2 seconds...");
    delay(2000);
    ESP.restart();
  }

  // Register the peer callback
  ESP_NOW.onNewPeer(register_new_master, NULL);

  // Set motor pins as outputs
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  Serial.println("Setup complete.");
}

/* Motor Control Functions */
void stop() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void backward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void left() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void right() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

/* Motor Control Command Processing */
void controls(const char* data) {
  if (strcmp(data, "") == 0) stop();
  else if (strcmp(data, "W") == 0) forward();
  else if (strcmp(data, "S") == 0) backward();
  else if (strcmp(data, "A") == 0) left();
  else if (strcmp(data, "D") == 0) right();
  delay(200);
}

class ESP_NOW_Peer_Class : public ESP_NOW_Peer {
public:
  ESP_NOW_Peer_Class(const uint8_t *mac_addr, uint8_t channel, wifi_interface_t iface, const uint8_t *lmk) 
    : ESP_NOW_Peer(mac_addr, channel, iface, lmk) {}

  ~ESP_NOW_Peer_Class() {}

  bool add_peer() {
    if (!add()) {
      log_e("Failed connection to controller");
      return false;
    }
    return true;
  }
  
  void onReceive(const uint8_t *data, size_t len, bool broadcast) {
    Serial.printf("Trasmission with " MACSTR " is open(%s)\n", MAC2STR(addr()), broadcast ? "broadcast" : "unicast");
    Serial.printf("  Message: %s\n", (char *)data);
    controls((char *)data);
  }
};

/* Global Variables */
std::vector<ESP_NOW_Peer_Class> masters;

/* Callbacks */
void register_new_master(const esp_now_recv_info_t *info, const uint8_t *data, int len, void *arg) {
  if (memcmp(info->des_addr, ESP_NOW.BROADCAST_ADDR, 6) == 0) {
    Serial.printf("Unknown peer " MACSTR " sent a broadcast message\n", MAC2STR(info->src_addr));
    Serial.println("Registering the peer as a master");

    ESP_NOW_Peer_Class new_master(info->src_addr, ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, NULL);
    masters.push_back(new_master);

    if (!masters.back().add_peer()) {
      Serial.println("Controller Register Failed");
      return;
    }
  } else {
    log_v("Received a unicast message from " MACSTR, MAC2STR(info->src_addr));
    log_v("Ignoring the message");
  }
}

/* Main Loop */
void loop() {
  delay(500);
}
