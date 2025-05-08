#include "basic_espnow.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "string.h"

namespace esphome {
namespace basic_espnow {

BasicESPNow *BasicESPNow::instance_ = nullptr;

void BasicESPNow::setup() {
  ESP_LOGI("basic_espnow", "Initializing ESP-NOW (ESP-IDF)");

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

  if (esp_now_init() != ESP_OK) {
    ESP_LOGE("basic_espnow", "ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(&BasicESPNow::recv_cb);
  esp_now_register_send_cb(&BasicESPNow::send_cb);

  instance_ = this;

  ESP_LOGI("basic_espnow", "ESP-NOW setup complete");
}

void BasicESPNow::send_broadcast(const std::string &message) {
  uint8_t broadcast_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_send(broadcast_addr, (const uint8_t *)message.data(), message.size());
}

void BasicESPNow::send_to_peer(const uint8_t *peer_addr, const std::string &message) {
  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, peer_addr, 6);
  peer.channel = 0;
  peer.encrypt = false;

  if (!esp_now_is_peer_exist(peer_addr)) {
    if (esp_now_add_peer(&peer) != ESP_OK) {
      ESP_LOGE("basic_espnow", "Failed to add peer");
      return;
    }
  }

  esp_now_send(peer_addr, (const uint8_t *)message.data(), message.size());
}

void BasicESPNow::set_on_message_callback(OnMessageCallback cb) {
  on_message_callback_ = cb;
}

void BasicESPNow::recv_cb(const uint8_t *mac, const uint8_t *data, int len) {
  if (instance_ && instance_->on_message_callback_) {
    instance_->on_message_callback_(mac, data, len);
  }
}

void BasicESPNow::send_cb(const uint8_t *mac, esp_now_send_status_t status) {
  ESP_LOGD("basic_espnow", "Send to %02x:%02x:%02x:%02x:%02x:%02x %s",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
           status == ESP_NOW_SEND_SUCCESS ? "succeeded" : "failed");
}

}  // namespace basic_espnow
}  // namespace esphome
