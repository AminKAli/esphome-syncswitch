#pragma once

#include "esphome/core/component.h"
#include "esp_now.h"

namespace esphome {
namespace basic_espnow {

class BasicESPNow : public Component {
 public:
  typedef std::function<void(const uint8_t *mac_addr, const uint8_t *data, int len)> OnMessageCallback;

  void setup() override;
  void loop() override {}

  void send_broadcast(const std::string &message);
  void send_to_peer(const uint8_t *peer_addr, const std::string &message);
  void set_on_message_callback(OnMessageCallback cb);

 protected:
  static void recv_cb(const uint8_t *mac, const uint8_t *data, int len);
  static void send_cb(const uint8_t *mac, esp_now_send_status_t status);

  static BasicESPNow *instance_;
  OnMessageCallback on_message_callback_;
};

}  // namespace basic_espnow
}  // namespace esphome
