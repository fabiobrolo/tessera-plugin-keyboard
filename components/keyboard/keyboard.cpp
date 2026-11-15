#include "keyboard.h"
#include <algorithm>
#include <cctype>
#include <cstring>

namespace esphome::keyboard {

Keyboard *global_keyboard = nullptr;

namespace {

// UTF-8 sequences for Material Design Icons (included in materialdesign_icons and materialdesign_icons_mini).
constexpr const char *MDI_BACKSPACE = "\xF3\xB0\xAD\x9C";  // \U000F0B5C backspace-outline
constexpr const char *MDI_RETURN    = "\xF3\xB0\x8C\x91";  // \U000F0311 keyboard-return
constexpr const char *MDI_KEYBOARD  = "\xF3\xB0\x8C\x92";  // \U000F0312 keyboard-outline
constexpr const char *MDI_LEFT      = "\xF3\xB0\x85\x81";  // \U000F0141 chevron-left
constexpr const char *MDI_RIGHT     = "\xF3\xB0\x85\x82";  // \U000F0142 chevron-right
constexpr const char *MDI_CHECK     = "\xF3\xB0\x84\xAC";  // \U000F012C check

// Helper to cast bitwise-combined buttonmatrix control flags back to lv_buttonmatrix_ctrl_t.
constexpr lv_buttonmatrix_ctrl_t ctrl(int v) {
  return static_cast<lv_buttonmatrix_ctrl_t>(v);
}

constexpr int KB_CTRL_MODE =
    LV_BUTTONMATRIX_CTRL_CHECKED | LV_BUTTONMATRIX_CTRL_NO_REPEAT | LV_BUTTONMATRIX_CTRL_CLICK_TRIG;
constexpr int KB_CTRL_REPEAT =
    LV_BUTTONMATRIX_CTRL_CHECKED;
constexpr int KB_KEY =
    LV_BUTTONMATRIX_CTRL_POPOVER;

// Standard EN-US lowercase QWERTY button map (12 + 11 + 12 + 5 = 40 buttons).
const char *const KB_MAP_LC[] = {
    "1#", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", MDI_BACKSPACE, "\n",
    "ABC", "a", "s", "d", "f", "g", "h", "j", "k", "l", MDI_RETURN, "\n",
    "_", "-", "z", "x", "c", "v", "b", "n", "m", ".", ",", ":", "\n",
    MDI_KEYBOARD, MDI_LEFT, " ", MDI_RIGHT, MDI_CHECK, ""
};

// Standard EN-US uppercase QWERTY button map (12 + 11 + 12 + 5 = 40 buttons).
const char *const KB_MAP_UC[] = {
    "1#", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", MDI_BACKSPACE, "\n",
    "abc", "A", "S", "D", "F", "G", "H", "J", "K", "L", MDI_RETURN, "\n",
    "_", "-", "Z", "X", "C", "V", "B", "N", "M", ".", ",", ":", "\n",
    MDI_KEYBOARD, MDI_LEFT, " ", MDI_RIGHT, MDI_CHECK, ""
};

// Control flags for the EN-US lowercase and uppercase maps (12 + 11 + 12 + 5 = 40 buttons).
const lv_buttonmatrix_ctrl_t KB_CTRL_ALPHA[] = {
    ctrl(KB_CTRL_MODE | 5), ctrl(KB_KEY | 4), ctrl(KB_KEY | 4), ctrl(KB_KEY | 4), ctrl(KB_KEY | 4), ctrl(KB_KEY | 4), ctrl(KB_KEY | 4), ctrl(KB_KEY | 4), ctrl(KB_KEY | 4), ctrl(KB_KEY | 4), ctrl(KB_KEY | 4), ctrl(KB_CTRL_REPEAT | 7),
    ctrl(KB_CTRL_MODE | 6), ctrl(KB_KEY | 3), ctrl(KB_KEY | 3), ctrl(KB_KEY | 3), ctrl(KB_KEY | 3), ctrl(KB_KEY | 3), ctrl(KB_KEY | 3), ctrl(KB_KEY | 3), ctrl(KB_KEY | 3), ctrl(KB_KEY | 3), ctrl(KB_CTRL_MODE | 7),
    ctrl(KB_CTRL_REPEAT | 1), ctrl(KB_CTRL_REPEAT | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_CTRL_REPEAT | 1), ctrl(KB_CTRL_REPEAT | 1), ctrl(KB_CTRL_REPEAT | 1),
    ctrl(KB_CTRL_MODE | 2), ctrl(KB_CTRL_REPEAT | 2), ctrl(6), ctrl(KB_CTRL_REPEAT | 2), ctrl(KB_CTRL_MODE | 2)
};

// Numbers and special symbols button map (11 + 12 + 12 + 5 = 40 buttons).
const char *const KB_MAP_SPEC[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", MDI_BACKSPACE, "\n",
    "abc", "+", "&", "/", "*", "=", "%", "!", "?", "#", "<", ">", "\n",
    "\\", "@", "$", "(", ")", "{", "}", "[", "]", ";", "\"", "'", "\n",
    MDI_KEYBOARD, MDI_LEFT, " ", MDI_RIGHT, MDI_CHECK, ""
};

// Control flags for the numbers and special symbols map.
const lv_buttonmatrix_ctrl_t KB_CTRL_SPEC[] = {
    ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_CTRL_REPEAT | 2),
    ctrl(KB_CTRL_MODE | 2), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1),
    ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1), ctrl(KB_KEY | 1),
    ctrl(KB_CTRL_MODE | 2), ctrl(KB_CTRL_REPEAT | 2), ctrl(6), ctrl(KB_CTRL_REPEAT | 2), ctrl(KB_CTRL_MODE | 2)
};

// Composite LVGL fonts:
// - Small/medium screens: Font::TITLE (18px base) + Font::ICON_SMALL (26px base) fallback.
// - Large screens (>= 700px): Font::VALUE (38px base, ~2x larger) + Font::ICON (42px base, ~2x larger) fallback.
lv_font_t kb_font_small_;
lv_font_t kb_icon_font_small_;
bool kb_font_small_ready_ = false;

lv_font_t kb_font_large_;
lv_font_t kb_icon_font_large_;
bool kb_font_large_ready_ = false;

bool center_icon_glyph(const lv_font_t *icon_font, const lv_font_t *text_font, const lv_font_t *fallback_font,
                       lv_font_glyph_dsc_t *dsc, uint32_t letter, uint32_t letter_next) {
  if (!icon_font || !icon_font->get_glyph_dsc(icon_font, dsc, letter, letter_next)) return false;
  int32_t lh = text_font ? text_font->line_height : fallback_font->line_height;
  int32_t bl = text_font ? text_font->base_line : fallback_font->base_line;
  int32_t base_from_top = lh - bl;
  int32_t target_top = (lh - static_cast<int32_t>(dsc->box_h)) / 2;
  dsc->ofs_y = base_from_top - static_cast<int32_t>(dsc->box_h) - target_top;
  return true;
}

bool kb_icon_get_glyph_dsc_small(const lv_font_t *font, lv_font_glyph_dsc_t *dsc, uint32_t letter, uint32_t letter_next) {
  return center_icon_glyph(ui::font(Font::ICON_SMALL), ui::font(Font::TITLE), font, dsc, letter, letter_next);
}

bool kb_icon_get_glyph_dsc_large(const lv_font_t *font, lv_font_glyph_dsc_t *dsc, uint32_t letter, uint32_t letter_next) {
  return center_icon_glyph(ui::font(Font::ICON), ui::font(Font::VALUE), font, dsc, letter, letter_next);
}

// Returns the composite keyboard font scaled for the current screen size.
const lv_font_t *get_kb_font(bool large_screen) {
  if (large_screen) {
    const lv_font_t *val = ui::font(Font::VALUE);
    const lv_font_t *icon = ui::font(Font::ICON);
    if (val) {
      if (!kb_font_large_ready_ && icon) {
        kb_icon_font_large_ = *icon;
        kb_icon_font_large_.get_glyph_dsc = kb_icon_get_glyph_dsc_large;
        kb_icon_font_large_.fallback = nullptr;
        kb_font_large_ = *val;
        kb_font_large_.fallback = &kb_icon_font_large_;
        kb_font_large_ready_ = true;
      }
      return kb_font_large_ready_ ? &kb_font_large_ : val;
    }
  }

  const lv_font_t *title = ui::font(Font::TITLE);
  const lv_font_t *icon = ui::font(Font::ICON_SMALL);
  if (!title) return nullptr;
  if (!kb_font_small_ready_ && icon) {
    kb_icon_font_small_ = *icon;
    kb_icon_font_small_.get_glyph_dsc = kb_icon_get_glyph_dsc_small;
    kb_icon_font_small_.fallback = nullptr;
    kb_font_small_ = *title;
    kb_font_small_.fallback = &kb_icon_font_small_;
    kb_font_small_ready_ = true;
  }
  return kb_font_small_ready_ ? &kb_font_small_ : title;
}

}  // namespace

void open_keyboard() {
  if (global_keyboard) {
    global_keyboard->open_keyboard();
  }
}

// ---------------- Keyboard ----------------

void Keyboard::setup() {
  global_keyboard = this;
  add_tile("keyboard", [this]() { return new KeyboardTile(this); });
  // Set wide = true so the keyboard card spans the full glass width in landscape orientation.
  add_card("keyboard", [this]() { return new KeyboardCard(this); }, true);
  add_tap_action("open_keyboard", [this](const tessera::TapContext &) {
    this->open_keyboard();
  });
}

void Keyboard::open_keyboard() {
  std::string title = text("title");
  if (title.empty()) title = "Keyboard";
  tessera::open_card(plugin_id(), "keyboard", "", -1, title);
}

void Keyboard::submit_text(const std::string &text) {
  if (!script_.empty()) {
    JsonDocument d;
    JsonObject obj = d.to<JsonObject>();
    obj["input_text"] = text;
    ESP_LOGI("keyboard", "Submitting input_text='%s' to script='%s'", text.c_str(), script_.c_str());
    tessera::action(script_.c_str(), "", obj);
  } else {
    ESP_LOGW("keyboard", "No target_script configured; cannot submit '%s'", text.c_str());
  }
  for (auto *trig : submit_triggers_) {
    trig->trigger(text);
  }
}

// ---------------- KeyboardTile ----------------

void KeyboardTile::create(const tessera::TileContext &c) {
  width_ = c.width;
  height_ = c.height;
  tile_ = c.tile;

  // Render centered keyboard-outline icon (\U000F0312).
  icon_label_ = ui::label(c.parent, Font::ICON, theme::INK);
  ui::set_text(icon_label_, "\U000F0312");
  lv_obj_set_width(icon_label_, width_);
  lv_obj_set_style_text_align(icon_label_, LV_TEXT_ALIGN_CENTER, 0);

  // Render tile label below the icon.
  Font lbl_font = (height_ >= ui::px(140)) ? Font::TITLE : Font::BODY;
  text_label_ = ui::label(c.parent, lbl_font, theme::MUTED);
  std::string name = c.name && c.name[0] ? c.name : plugin_->text("tile_name");
  if (name.empty()) name = "Keyboard";
  ui::set_text(text_label_, name);
  lv_obj_set_width(text_label_, width_);
  lv_obj_set_style_text_align(text_label_, LV_TEXT_ALIGN_CENTER, 0);

  int ih = ui::line_height(Font::ICON);
  int th = ui::line_height(lbl_font);
  int gap = ui::px(4);
  int top = (height_ - ih - th - gap) / 2;
  if (top < 0) top = 0;

  lv_obj_set_pos(icon_label_, 0, top);
  lv_obj_set_height(icon_label_, ih);
  lv_obj_set_pos(text_label_, 0, top + ih + gap);
  lv_obj_set_height(text_label_, th);
}

void KeyboardTile::on_theme() {
  ui::set_color(icon_label_, theme::INK);
  ui::set_color(text_label_, theme::MUTED);
}

void KeyboardTile::on_tap() {
  std::string title = plugin_->text("title");
  if (title.empty()) title = "Keyboard";
  tessera::open_card(plugin_->plugin_id(), "keyboard", "", tile_, title);
}

// ---------------- KeyboardCard ----------------

void KeyboardCard::open(const tessera::CardContext &c) {
  int w = c.width;
  int h = c.height;
  bool landscape = w > h;
  bool large_screen = std::max(w, h) >= 700;

  int pad = ui::px(4);
  int avail_w = std::max(1, w - 2 * pad);
  int gap = ui::px(10);
  int top_y = 0;

  int ta_w, ta_h, clear_x, clear_y, clear_w, btn_h, send_x, send_y, send_w, kb_y, kb_h;

  if (landscape) {
    // Landscape orientation: place textarea, Clear, and Send on a single top row to maximize keyboard height and width.
    ta_h = large_screen ? ui::px(64) : ui::px(54);
    btn_h = ta_h;
    clear_w = std::max(ui::px(140), avail_w / 8);
    send_w = std::max(ui::px(160), avail_w / 6);
    ta_w = std::max(ui::px(200), avail_w - clear_w - send_w - 2 * gap);

    clear_x = pad + ta_w + gap;
    clear_y = top_y;
    send_x = clear_x + clear_w + gap;
    send_y = top_y;

    kb_y = top_y + ta_h + gap;
    kb_h = std::max(ui::px(200), h - kb_y - ui::px(8));
  } else {
    // Portrait orientation: stack textarea on row 1, action buttons on row 2, and virtual keyboard below.
    ta_w = avail_w;
    ta_h = large_screen ? ui::px(64) : ui::px(56);
    btn_h = large_screen ? ui::px(56) : ui::px(48);
    clear_w = (avail_w - gap) / 3;
    send_w = avail_w - clear_w - gap;

    clear_x = pad;
    clear_y = top_y + ta_h + gap;
    send_x = pad + clear_w + gap;
    send_y = clear_y;

    kb_y = clear_y + btn_h + gap;
    int avail_kb_h = std::max(ui::px(220), h - kb_y - ui::px(8));
    kb_h = std::min(avail_kb_h, std::max(ui::px(380), avail_w * 3 / 4));
  }

  Font ta_font = large_screen ? Font::HEADLINE : Font::TITLE;
  Font btn_font = large_screen ? Font::HEADLINE : Font::BODY_LARGE;

  // 1. Single-line text input field (textarea).
  ta_ = lv_textarea_create(c.parent);
  std::string ph = plugin_->text("placeholder");
  if (ph.empty()) ph = "Type here...";
  lv_textarea_set_placeholder_text(ta_, ph.c_str());
  lv_textarea_set_one_line(ta_, true);
  lv_obj_set_size(ta_, ta_w, ta_h);
  lv_obj_align(ta_, LV_ALIGN_TOP_LEFT, pad, top_y);
  lv_obj_set_style_text_font(ta_, ui::font(ta_font), 0);
  lv_obj_set_style_border_width(ta_, ui::px(2), 0);
  lv_obj_set_style_border_color(ta_, theme::color(theme::TRACK), 0);
  lv_obj_set_style_radius(ta_, ui::px(10), 0);
  lv_obj_set_style_bg_color(ta_, theme::color(theme::CARD), 0);
  lv_obj_set_style_text_color(ta_, theme::color(theme::INK), 0);

  // 2. Clear button.
  btn_clear_ = lv_obj_create(c.parent);
  lv_obj_add_flag(btn_clear_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(btn_clear_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(btn_clear_, clear_w, btn_h);
  lv_obj_align(btn_clear_, LV_ALIGN_TOP_LEFT, clear_x, clear_y);
  lv_obj_set_style_radius(btn_clear_, ui::px(10), 0);
  lv_obj_set_style_bg_color(btn_clear_, theme::color(theme::TRACK), 0);
  lbl_clear_ = ui::label(btn_clear_, btn_font, theme::INK);
  std::string clr_txt = plugin_->text("clear");
  ui::set_text(lbl_clear_, clr_txt.empty() ? "Clear" : clr_txt);
  lv_obj_center(lbl_clear_);

  // 3. Send button.
  btn_send_ = lv_obj_create(c.parent);
  lv_obj_add_flag(btn_send_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(btn_send_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(btn_send_, send_w, btn_h);
  lv_obj_align(btn_send_, LV_ALIGN_TOP_LEFT, send_x, send_y);
  lv_obj_set_style_radius(btn_send_, ui::px(10), 0);
  lv_obj_set_style_bg_color(btn_send_, lv_color_hex(theme::ha::GREEN), 0);
  lbl_send_ = ui::label(btn_send_, btn_font, theme::INK);
  lv_obj_set_style_text_color(lbl_send_, lv_color_white(), 0);
  std::string send_txt = plugin_->text("send");
  ui::set_text(lbl_send_, send_txt.empty() ? "Send" : send_txt);
  lv_obj_center(lbl_send_);

  // 4. Virtual keyboard with custom MDI icon button maps and composite fallback font.
  kb_ = lv_keyboard_create(c.parent);
  lv_obj_remove_event_cb(kb_, lv_keyboard_def_event_cb);
  lv_keyboard_set_map(kb_, LV_KEYBOARD_MODE_TEXT_LOWER, KB_MAP_LC, KB_CTRL_ALPHA);
  lv_keyboard_set_map(kb_, LV_KEYBOARD_MODE_TEXT_UPPER, KB_MAP_UC, KB_CTRL_ALPHA);
  lv_keyboard_set_map(kb_, LV_KEYBOARD_MODE_SPECIAL, KB_MAP_SPEC, KB_CTRL_SPEC);
  lv_keyboard_set_mode(kb_, LV_KEYBOARD_MODE_TEXT_LOWER);

  // Override lv_keyboard's default LV_ALIGN_BOTTOM_MID alignment with LV_ALIGN_TOP_LEFT so the bottom row is never clipped.
  lv_obj_set_size(kb_, avail_w, kb_h);
  lv_obj_align(kb_, LV_ALIGN_TOP_LEFT, pad, kb_y);
  lv_obj_set_style_radius(kb_, ui::px(12), 0);
  lv_obj_set_style_bg_color(kb_, theme::color(theme::CARD), 0);
  lv_obj_set_style_border_width(kb_, 0, 0);
  lv_obj_set_style_pad_all(kb_, ui::px(8), 0);
  lv_obj_set_style_pad_row(kb_, ui::px(8), 0);
  lv_obj_set_style_pad_column(kb_, ui::px(8), 0);

  const lv_font_t *kb_font = get_kb_font(large_screen);
  if (kb_font) {
    lv_obj_set_style_text_font(kb_, kb_font, 0);
    lv_obj_set_style_text_font(kb_, kb_font, LV_PART_ITEMS);
  }
  lv_obj_set_style_radius(kb_, ui::px(8), LV_PART_ITEMS);
  lv_obj_set_style_bg_color(kb_, theme::color(theme::BUTTON), LV_PART_ITEMS);
  lv_obj_set_style_text_color(kb_, theme::color(theme::INK), LV_PART_ITEMS);
  lv_obj_set_style_bg_color(kb_, theme::color(theme::TRACK), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(kb_, theme::color(theme::INK), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(kb_, theme::color(theme::ACCENT), LV_PART_ITEMS | LV_STATE_PRESSED);
  lv_obj_set_style_text_color(kb_, theme::color(theme::ON_ACCENT), LV_PART_ITEMS | LV_STATE_PRESSED);

  lv_keyboard_set_textarea(kb_, ta_);

  // Event callbacks for Clear, Send, and virtual keyboard button presses.
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
    if (!card || !card->kb_ || !card->ta_) return;
    uint32_t btn_id = lv_buttonmatrix_get_selected_button(card->kb_);
    if (btn_id == LV_BUTTONMATRIX_BUTTON_NONE) return;
    const char *txt = lv_buttonmatrix_get_button_text(card->kb_, btn_id);
    if (!txt || !txt[0]) return;

    if (std::strcmp(txt, "abc") == 0) {
      lv_keyboard_set_mode(card->kb_, LV_KEYBOARD_MODE_TEXT_LOWER);
    } else if (std::strcmp(txt, "ABC") == 0) {
      lv_keyboard_set_mode(card->kb_, LV_KEYBOARD_MODE_TEXT_UPPER);
    } else if (std::strcmp(txt, "1#") == 0) {
      lv_keyboard_set_mode(card->kb_, LV_KEYBOARD_MODE_SPECIAL);
    } else if (std::strcmp(txt, MDI_BACKSPACE) == 0) {
      lv_textarea_delete_char(card->ta_);
    } else if (std::strcmp(txt, MDI_LEFT) == 0) {
      lv_textarea_cursor_left(card->ta_);
    } else if (std::strcmp(txt, MDI_RIGHT) == 0) {
      lv_textarea_cursor_right(card->ta_);
    } else if (std::strcmp(txt, MDI_RETURN) == 0 || std::strcmp(txt, MDI_CHECK) == 0) {
      card->submit();
    } else if (std::strcmp(txt, MDI_KEYBOARD) == 0) {
      tessera::close_card();
    } else {
      lv_textarea_add_text(card->ta_, txt);
    }
  }, LV_EVENT_VALUE_CHANGED, this);
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
    lv_obj_set_style_bg_color(kb_, theme::color(theme::BUTTON), LV_PART_ITEMS);
    lv_obj_set_style_text_color(kb_, theme::color(theme::INK), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(kb_, theme::color(theme::TRACK), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(kb_, theme::color(theme::INK), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(kb_, theme::color(theme::ACCENT), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(kb_, theme::color(theme::ON_ACCENT), LV_PART_ITEMS | LV_STATE_PRESSED);
  }
}

void KeyboardCard::submit() {
  if (!ta_) return;
  const char *raw = lv_textarea_get_text(ta_);
  std::string query = raw ? raw : "";
  // Trim leading and trailing whitespace before submitting.
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

}  // namespace esphome::keyboard
