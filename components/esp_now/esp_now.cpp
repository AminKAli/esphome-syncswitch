#include "esp_now.h"
#include "esphome/core/log.h"
#include <esp_wifi.h>

namespace esphome {
namespace esp_now {

static const char *TAG = "esp_now";

ESPNowComponent *ESPNowComponent::global_esp_now_component = nullptr;

void ESPNowComponent::setup() {
  global_esp_now_component = this;
  
  ESP_LOGCONFIG(TAG, "Setting up ESP-NOW...");
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    ESP_LOGE(TAG, "Error initializing ESP-NOW");
    return;
  }
  
  // Register callbacks
  esp_now_register_recv_cb(esp_now_receive_cb);
  esp_now_register_send_cb(esp_now_send_cb);
  
  // Add peers
  for (auto peer : this->peers_) {
    esp_now_peer_info_t peer_info = peer->get_peer_info();
    
    // Check if peer exists and delete if it does
    if (esp_now_is_peer_exist(peer_info.peer_addr)) {
      esp_now_del_peer(peer_info.peer_addr);
    }
    
    // Add peer
    if (esp_now_add_peer(&peer_info) != ESP_OK) {
      ESP_LOGE(TAG, "Failed to add peer");
      continue;
    }
    
    // Set encryption key if provided
    if (peer_info.encrypt) {
      esp_now_set_pmk(peer->get_encryption_key());
    }
    
    ESP_LOGD(TAG, "Added peer: %s", format_mac_address(peer_info.peer_addr).c_str());
  }
}

void ESPNowComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "ESP-NOW:");
  ESP_LOGCONFIG(TAG, "  Peer count: %d", this->peers_.size());
  
  // Get self MAC address
  uint8_t mac[6];
  esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
  ESP_LOGCONFIG(TAG, "  My MAC address: %s", format_mac_address(mac).c_str());
  
  for (auto peer : this->peers_) {
    ESP_LOGCONFIG(TAG, "  Peer MAC: %s", format_mac_address(peer->get_raw_mac_address()).c_str());
  }
  
  ESP_LOGCONFIG(TAG, "  Broadcast support: enabled");
}

std::string ESPNowComponent::format_mac_address(const uint8_t *mac_addr) {
  char mac_str[18];
  snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", 
          mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  return std::string(mac_str);
}

bool ESPNowComponent::send_message(const std::string &mac_address, const std::string &data) {
  if (data.empty()) {
    ESP_LOGW(TAG, "Empty data, not sending");
    return false;
  }
  
  // Parse MAC address
  uint8_t mac[6];
  int values[6];
  if (sscanf(mac_address.c_str(), "%x:%x:%x:%x:%x:%x", 
            &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) != 6) {
    ESP_LOGE(TAG, "Invalid MAC address format: %s", mac_address.c_str());
    return false;
  }
  
  for (int i = 0; i < 6; ++i) {
    mac[i] = static_cast<uint8_t>(values[i]);
  }
  
  // Send data
  esp_err_t result = esp_now_send(mac, (const uint8_t*) data.data(), data.size());
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Error sending message: %s", esp_err_to_name(result));
    return false;
  }
  
  return true;
}

bool ESPNowComponent::broadcast_message(const std::string &data) {
  if (data.empty()) {
    ESP_LOGW(TAG, "Empty data, not broadcasting");
    return false;
  }
  
  // Broadcast MAC address (FF:FF:FF:FF:FF:FF)
  uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  
  // Check if broadcast peer exists, add if not
  if (!esp_now_is_peer_exist(broadcast_mac)) {
    esp_now_peer_info_t peer_info = {};
    memcpy(peer_info.peer_addr, broadcast_mac, 6);
    peer_info.channel = 0;  // Use current WiFi channel
    peer_info.encrypt = false;  // Can't encrypt broadcasts
    
    esp_err_t add_result = esp_now_add_peer(&peer_info);
    if (add_result != ESP_OK) {
      ESP_LOGE(TAG, "Failed to add broadcast peer: %s", esp_err_to_name(add_result));
      return false;
    }
    
    ESP_LOGD(TAG, "Added broadcast peer");
  }
  
  // Send data to broadcast address
  esp_err_t result = esp_now_send(broadcast_mac, (const uint8_t*) data.data(), data.size());
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Error broadcasting message: %s", esp_err_to_name(result));
    return false;
  }
  
  ESP_LOGD(TAG, "Broadcast message sent: %s", data.c_str());
  return true;
}

void ESPNowComponent::on_message(const uint8_t *mac_addr, const uint8_t *data, int len) {
  std::string mac = format_mac_address(mac_addr);
  std::string payload(reinterpret_cast<const char*>(data), len);
  
  ESP_LOGD(TAG, "Received message from %s: %s", mac.c_str(), payload.c_str());
  
  for (auto &callback : this->message_callbacks_) {
    callback(mac, payload);
  }
}

void ESPNowComponent::esp_now_receive_cb(const uint8_t *mac_addr, const uint8_t *data, int len) {
  if (global_esp_now_component != nullptr) {
    global_esp_now_component->on_message(mac_addr, data, len);
  }
}

void ESPNowComponent::esp_now_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (global_esp_now_component != nullptr) {
    std::string mac = format_mac_address(mac_addr);
    ESP_LOGD(TAG, "Send message to %s %s", mac.c_str(), status == ESP_NOW_SEND_SUCCESS ? "succeeded" : "failed");
  }
}

}  // namespace esp_now
}  // namespace esphome
