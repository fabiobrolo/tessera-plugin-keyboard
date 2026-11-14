#pragma once
#include <string>
#include <vector>
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/smart_display/plugin_api.h"

namespace esphome::type_text {

using tessera::Font;
namespace ui = tessera::ui;

class TypeText;

class TypeTextTile : public tessera::Tile {
 public:
  explicit TypeTextTile(TypeText *plugin) : plugin_(plugin) {}
  void create(const tessera::TileContext &c) override;
  void on_theme() override;
  void on_tap() override;

 private:
  TypeText *plugin_{};
  int width_{0};
  int height_{0};
  int tile_{-1};
  lv_obj_t *icon_label_{};
  lv_obj_t *text_label_{};
};

class KeyboardCard : public tessera::Card {
 public:
  explicit KeyboardCard(TypeText *plugin) : plugin_(plugin) {}
  void open(const tessera::CardContext &c) override;
  void on_theme() override;

  void submit();
  void clear();

 private:
  TypeText *plugin_{};
  lv_obj_t *ta_{};
  lv_obj_t *kb_{};
  lv_obj_t *btn_send_{};
  lv_obj_t *btn_clear_{};
  lv_obj_t *lbl_send_{};
  lv_obj_t *lbl_clear_{};
};

class TypeText : public Component, public tessera::Plugin {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_script(const std::string &script) { script_ = script; }
  const std::string &get_script() const { return script_; }

  void register_submit_trigger(Trigger<std::string> *trig) {
    submit_triggers_.push_back(trig);
  }

  void submit_text(const std::string &text);
  void open_keyboard();

 private:
  std::string script_;
  std::vector<Trigger<std::string> *> submit_triggers_;
};

template<typename... Ts> class OpenAction : public Action<Ts...>, public Parented<TypeText> {
 public:
  void play(Ts... x) override {
    this->parent_->open_keyboard();
  }
};

extern TypeText *global_type_text;
void open_keyboard();

}  // namespace esphome::type_text
