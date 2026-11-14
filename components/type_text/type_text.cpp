#include "type_text.h"
#include <algorithm>
#include <cctype>

namespace esphome::type_text {

TypeText *global_type_text = nullptr;

void open_keyboard() {
  if (global_type_text) {
    global_type_text->open_keyboard();
  }
}

// ---------------- TypeText ----------------

void TypeText::setup() {
  global_type_text = this;
  add_tile("type_text", [this]() { return new TypeTextTile(this); });
  add_card("keyboard", [this]() { return new KeyboardCard(this); });
  add_tap_action("open_keyboard", [this](const tessera::TapContext &) {
    this->open_keyboard();
  });
}

void TypeText::open_keyboard() {
  std::string title = text("title");
  if (title.empty()) title = "Digitar Texto";
  tessera::open_card(plugin_id(), "keyboard", "", -1, title);
}

void TypeText::submit_text(const std::string &text) {
  if (!script_.empty()) {
    JsonDocument d;
    d["input_text"] = text;
    tessera::action(script_.c_str(), "", d);
  }
  for (auto *trig : submit_triggers_) {
    trig->trigger(text);
  }
}

// ---------------- TypeTextTile ----------------

void TypeTextTile::create(const tessera::TileContext &c) {
  width_ = c.width;
  height_ = c.height;
  tile_ = c.tile;

  // Icon: keyboard-outline (\U000F0312)
  icon_label_ = ui::label(c.parent, Font::ICON, theme::INK);
  ui::set_text(icon_label_, "\U000F0312");
  lv_obj_set_width(icon_label_, width_);
  lv_obj_set_style_text_align(icon_label_, LV_TEXT_ALIGN_CENTER, 0);

  // Text label: "Digitar Texto"
  text_label_ = ui::label(c.parent, Font::BODY, theme::MUTED);
  std::string name = c.name && c.name[0] ? c.name : plugin_->text("tile_name");
  if (name.empty()) name = "Digitar Texto";
  ui::set_text(text_label_, name);
  lv_obj_set_width(text_label_, width_);
  lv_obj_set_style_text_align(text_label_, LV_TEXT_ALIGN_CENTER, 0);

  int ih = ui::line_height(Font::ICON);
  int th = ui::line_height(Font::BODY);
  int gap = ui::px(4);
  int top = (height_ - ih - th - gap) / 2;
  if (top < 0) top = 0;

  lv_obj_set_pos(icon_label_, 0, top);
  lv_obj_set_height(icon_label_, ih);
  lv_obj_set_pos(text_label_, 0, top + ih + gap);
  lv_obj_set_height(text_label_, th);
}

void TypeTextTile::on_theme() {
  ui::set_color(icon_label_, theme::INK);
  ui::set_color(text_label_, theme::MUTED);
}

void TypeTextTile::on_tap() {
  std::string title = plugin_->text("title");
  if (title.empty()) title = "Digitar Texto";
  tessera::open_card(plugin_->plugin_id(), "keyboard", "", tile_, title);
}

// ---------------- KeyboardCard ----------------

void KeyboardCard::open(const tessera::CardContext &c) {
  int w = c.width;
  int h = c.height;
  int pad = ui::px(16);
  int avail_w = w - 2 * pad;

  // 1. Textarea
  ta_ = lv_textarea_create(c.parent);
  std::string ph = plugin_->text("placeholder");
  if (ph.empty()) ph = "Digite aqui...";
  lv_textarea_set_placeholder_text(ta_, ph.c_str());
  lv_textarea_set_one_line(ta_, true);
  lv_obj_set_size(ta_, avail_w, ui::px(60));
  lv_obj_set_pos(ta_, pad, ui::px(10));
  lv_obj_set_style_text_font(ta_, ui::font(Font::TITLE), 0);
  lv_obj_set_style_border_width(ta_, ui::px(2), 0);
  lv_obj_set_style_border_color(ta_, theme::color(theme::TRACK), 0);
  lv_obj_set_style_radius(ta_, ui::px(10), 0);
  lv_obj_set_style_bg_color(ta_, theme::color(theme::CARD), 0);
  lv_obj_set_style_text_color(ta_, theme::color(theme::INK), 0);

  // 2. Action buttons row
  int btn_y = ui::px(78);
  int btn_h = ui::px(48);
  int gap = ui::px(12);
  int clear_w = (avail_w - gap) / 3;
  int send_w = avail_w - clear_w - gap;

  // Clear button
  btn_clear_ = lv_obj_create(c.parent);
  lv_obj_add_flag(btn_clear_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(btn_clear_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(btn_clear_, clear_w, btn_h);
  lv_obj_set_pos(btn_clear_, pad, btn_y);
  lv_obj_set_style_radius(btn_clear_, ui::px(10), 0);
  lv_obj_set_style_bg_color(btn_clear_, theme::color(theme::TRACK), 0);
  lbl_clear_ = ui::label(btn_clear_, Font::BODY_LARGE, theme::INK);
  std::string clr_txt = plugin_->text("clear");
  ui::set_text(lbl_clear_, clr_txt.empty() ? "Limpar" : clr_txt);
  lv_obj_center(lbl_clear_);

  // Send button
  btn_send_ = lv_obj_create(c.parent);
  lv_obj_add_flag(btn_send_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(btn_send_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(btn_send_, send_w, btn_h);
  lv_obj_set_pos(btn_send_, pad + clear_w + gap, btn_y);
  lv_obj_set_style_radius(btn_send_, ui::px(10), 0);
  lv_obj_set_style_bg_color(btn_send_, lv_color_hex(theme::ha::GREEN), 0);
  lbl_send_ = ui::label(btn_send_, Font::BODY_LARGE, theme::INK);
  lv_obj_set_style_text_color(lbl_send_, lv_color_white(), 0);
  std::string send_txt = plugin_->text("send");
  ui::set_text(lbl_send_, send_txt.empty() ? "Enviar" : send_txt);
  lv_obj_center(lbl_send_);

  // 3. Virtual Keyboard
  int kb_y = ui::px(138);
  int kb_h = std::min(ui::px(580), h - kb_y - ui::px(16));
  if (kb_h < ui::px(350)) kb_h = h - kb_y - ui::px(8);

  kb_ = lv_keyboard_create(c.parent);
  lv_obj_set_size(kb_, avail_w, kb_h);
  lv_obj_set_pos(kb_, pad, kb_y);
  lv_obj_set_style_radius(kb_, ui::px(12), 0);
  lv_obj_set_style_bg_color(kb_, theme::color(theme::CARD), 0);
  lv_obj_set_style_border_width(kb_, 0, 0);
  lv_obj_set_style_text_font(kb_, ui::font(Font::TITLE), 0);
  lv_keyboard_set_textarea(kb_, ta_);

  // Event callbacks
  lv_obj_add_event_cb(btn_clear_, [](lv_event_t *e) {
    auto *card = static_cast<KeyboardCard *>(lv_event_get_user_data(e));
    if (card) card->clear();
  }, LV_EVENT_CLICKED, this);

  lv_obj_add_event_cb(btn_send_, [](lv_event_t *e) {
    auto *card = static_cast<KeyboardCard *>(lv_event_get_user_data(e));
    if (card) card->submit();
  }, LV_EVENT_CLICKED, this);

  lv_obj_add_event_cb(kb_, [](lv_event_t *e) {
    auto *card = static_cast<KeyboardCard *>(lv_event_get_user_data(e));
    if (!card) return;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY) {
      card->submit();
    } else if (code == LV_EVENT_CANCEL) {
      tessera::close_card();
    }
  }, LV_EVENT_ALL, this);
}

void KeyboardCard::on_theme() {
  if (ta_) {
    lv_obj_set_style_border_color(ta_, theme::color(theme::TRACK), 0);
    lv_obj_set_style_bg_color(ta_, theme::color(theme::CARD), 0);
    lv_obj_set_style_text_color(ta_, theme::color(theme::INK), 0);
  }
  if (btn_clear_) {
    lv_obj_set_style_bg_color(btn_clear_, theme::color(theme::TRACK), 0);
    if (lbl_clear_) ui::set_color(lbl_clear_, theme::INK);
  }
  if (kb_) {
    lv_obj_set_style_bg_color(kb_, theme::color(theme::CARD), 0);
  }
}

void KeyboardCard::submit() {
  if (!ta_) return;
  const char *raw = lv_textarea_get_text(ta_);
  std::string query = raw ? raw : "";
  // Trim spaces
  while (!query.empty() && isspace(static_cast<unsigned char>(query.front()))) query.erase(query.begin());
  while (!query.empty() && isspace(static_cast<unsigned char>(query.back()))) query.pop_back();

  if (!query.empty()) {
    plugin_->submit_text(query);
    tessera::close_card();
  }
}

void KeyboardCard::clear() {
  if (ta_) {
    lv_textarea_set_text(ta_, "");
  }
}

}  // namespace esphome::type_text
