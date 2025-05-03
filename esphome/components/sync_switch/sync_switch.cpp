#include "sync_switch.h"

namespace esphome {
namespace espnow {

void ESPNowComponent::setup() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    ESP_LOGE("espnow", "ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb([](const uint8_t *mac, const uint8_t *data, int len) {
    char buf[256];
    memcpy(buf, data, len);
    buf[len] = 0;
    ESP_LOGD("espnow", "Received from %02X:%02X:%02X:%02X:%02X:%02X: %s",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], buf);
  });

  memcpy(peer_info_.peer_addr, peer_mac_, 6);
  peer_info_.channel = 0;
  peer_info_.encrypt = false;

  if (esp_now_add_peer(&peer_info_) != ESP_OK) {
    ESP_LOGE("espnow", "Failed to add peer");
  }
}

void ESPNowComponent::send_message(const std::string &msg) {
  esp_now_send(peer_mac_, (const uint8_t *)msg.c_str(), msg.length());
}

void ESPNowComponent::set_peer_mac(const uint8_t *mac) {
  memcpy(peer_mac_, mac, 6);
}

}  // namespace espnow
}  // namespace esphome
