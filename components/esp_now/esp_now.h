#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/components/network/ip_address.h"
#include <string>
#include <vector>
#include <esp_now.h>

namespace esphome {
namespace esp_now {

class ESPNowPeer {
 public:
  ESPNowPeer() {}
  
  void set_mac_address(const std::string &mac) {
    this->mac_address_ = mac;
    this->parse_mac_address_();
  }
  
  void set_channel(uint8_t channel) { this->channel_ = channel; }
  
  void set_encryption_key(const std::string &key) {
    this->encryption_key_ = key;
  }
  
  esp_now_peer_info_t get_peer_info() {
    esp_now_peer_info_t peer_info = {};
    memcpy(peer_info.peer_addr, this->raw_mac_address_, 6);
    peer_info.channel = this->channel_;
    peer_info.encrypt = !this->encryption_key_.empty();
    return peer_info;
  }
  
  const uint8_t *get_raw_mac_address() { return this->raw_mac_address_; }
  
  const uint8_t *get_encryption_key() {
    return this->encryption_key_.empty() ? nullptr : (const uint8_t *) this->encryption_key_.c_str();
  }
  
 protected:
  void parse_mac_address_() {
    std::vector<std::string> parts;
    size_t start = 0, end = 0;
    while ((end = this->mac_address_.find(':', start)) != std::string::npos) {
      parts.push_back(this->mac_address_.substr(start, end - start));
      start = end + 1;
    }
    parts.push_back(this->mac_address_.substr(start));
    
    for (size_t i = 0; i < 6; i++) {
      this->raw_mac_address_[i] = strtol(parts[i].c_str(), nullptr, 16);
    }
  }
  
  std::string mac_address_;
  uint8_t raw_mac_address_[6] = {0};
  uint8_t channel_ = 1;
  std::string encryption_key_;
};

class ESPNowComponent;

class ESPNowMessageReceivedTrigger : public Trigger<std::string, std::string> {
 public:
  explicit ESPNowMessageReceivedTrigger(ESPNowComponent *parent) {
    parent->add_on_message_callback([this](const std::string &mac, const std::string &data) {
      this->trigger(mac, data);
    });
  }
};

class ESPNowComponent : public Component {
 public:
  ESPNowComponent() {}
  
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }
  
  void add_peer(ESPNowPeer *peer) { this->peers_.push_back(peer); }
  
  bool send_message(const std::string &mac_address, const std::string &data);
  bool broadcast_message(const std::string &data);
  
  void add_on_message_callback(std::function<void(const std::string &, const std::string &)> &&callback) {
    this->message_callbacks_.push_back(std::move(callback));
  }
  
  void on_message(const uint8_t *mac_addr, const uint8_t *data, int len);
  
 protected:
  std::vector<ESPNowPeer *> peers_;
  std::vector<std::function<void(const std::string &, const std::string &)>> message_callbacks_;
  
  static std::string format_mac_address(const uint8_t *mac_addr);
  static void esp_now_receive_cb(const uint8_t *mac_addr, const uint8_t *data, int len);
  static void esp_now_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
  
  static ESPNowComponent *global_esp_now_component;
};

class ESPNowSendAction : public Action<> {
 public:
  ESPNowSendAction(ESPNowComponent *parent) : parent_(parent) {}
  
  void set_data(const std::string &data) { this->data_ = data; }
  void set_mac_address(const std::string &mac_address) { this->mac_address_ = mac_address; }
  
  void play(const ActionVariant &action) override {
    this->parent_->send_message(this->mac_address_, this->data_);
  }
  
 protected:
  ESPNowComponent *parent_;
  std::string data_;
  std::string mac_address_;
};

class ESPNowBroadcastAction : public Action<> {
 public:
  ESPNowBroadcastAction(ESPNowComponent *parent) : parent_(parent) {}
  
  void set_data(const std::string &data) { this->data_ = data; }
  
  void play(const ActionVariant &action) override {
    this->parent_->broadcast_message(this->data_);
  }
  
 protected:
  ESPNowComponent *parent_;
  std::string data_;
};

}  // namespace esp_now
}  // namespace esphome
