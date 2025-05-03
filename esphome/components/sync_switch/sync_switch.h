#pragma once
#include "esphome.h"
#include <esp_now.h>
#include <WiFi.h>

namespace esphome {
namespace espnow {

class ESPNowComponent : public Component {
 public:
  void setup() override;
  void send_message(const std::string &msg);
  void set_peer_mac(const uint8_t *mac);

 protected:
  uint8_t peer_mac_[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // default: broadcast
  esp_now_peer_info_t peer_info_ = {};
};

}  // namespace espnow
}  // namespace esphome
