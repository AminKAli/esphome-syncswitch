#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esp_now.h"

namespace esphome {
namespace basic_espnow {

class BasicESPNow;

class OnMessageTrigger : public Trigger<std::string> {
 public:
  explicit OnMessageTrigger(BasicESPNow *parent) { this->parent_ = parent; }
 protected:
  BasicESPNow *parent_;
};

class BasicESPNow : public Component {
 public:
  void setup() override;
  void loop() override {}

  void set_peer_mac(std::array<uint8_t, 6> mac) { memcpy(this->peer_mac_, mac.data(), 6); }

  void register_service(const std::string &name, const std::map<std::string, std::string> &args);
  void send_espnow(std::string message);

  void add_on_message_trigger(OnMessageTrigger *trigger) {
    this->triggers_.push_back(trigger);
  }

 protected:
  static void recv_cb(const uint8_t *mac, const uint8_t *data, int len);
  static void send_cb(const uint8_t *mac, esp_now_send_status_t status);

  void handle_received(const std::string &msg);

  uint8_t peer_mac_[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  static BasicESPNow *instance_;
  std::vector<OnMessageTrigger *> triggers_;
};

}  // namespace basic_espnow
}  // namespace esphome
