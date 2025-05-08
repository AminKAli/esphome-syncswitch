#include "basic_espnow.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

namespace esphome {
namespace basic_espnow {

BasicESPNow *BasicESPNow::instance_ = nullptr;

void BasicESPNow::setup() {
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_start();

  esp_now_init();
  esp_now_register_recv_cb(&BasicESPNow::recv_cb);
  esp_now_register_send_cb(&BasicESPNow::send_cb);

  instance_ = this;

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, peer_mac_, 6);
  peer.channel = 0;
  peer.encrypt = false;

  if (!esp_now_is_peer_exist(peer.peer_addr)) {
    esp_now_add_peer(&peer);
  }

  ESP_LOGI("basic_espnow", "ESP-NOW setup complete");
}

void BasicESPNow::send_espnow(std::string message) {
  esp_now_send(this->peer_mac_, (const uint8_t *)message.data(), message.size());
}

void BasicESPNow::register_service(const std::string &name, const std::map<std::string, std::string> &) {
  register_service(&BasicESPNow::send_espnow, name, {"message"});
}

void BasicESPNow::recv_cb(const uint8_t *mac, const uint8_t *data, int len) {
  if (instance_) {
    std::string msg((const char *)data, len);
    ESP_LOGI("basic_espnow", "Received: %s", msg.c_str());
    instance_->handle_received(msg);
  }
}

void BasicESPNow::send_cb(const uint8_t *mac, esp_now_send_status_t status) {
  ESP_LOGD("basic_espnow", "Send %s", status == ESP_NOW_SEND_SUCCESS ? "succeeded" : "failed");
}

void BasicESPNow::handle_received(const std::string &msg) {
  for (auto *trig : this->triggers_) {
    trig->trigger(msg);
  }
}

}  // namespace basic_espnow
}  // namespace esphome
