#include "ha_deck.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ha_deck {

void HaDeck::setup() { 
    lvgl_main_screen_ = lv_scr_act();
    switch_screen(main_screen_name_);

    if (inactivity_timeout_) {
        create_inactivity_screen_();
    }
}

void HaDeck::loop() {
    if (inactivity_timeout_ > 0) {
        if (lv_disp_get_inactive_time(NULL) > inactivity_timeout_) {
            set_inactivity_(true);
        } else {
            set_inactivity_(false);
        }
    }
}

float HaDeck::get_setup_priority() const { return setup_priority::AFTER_CONNECTION; }

void HaDeck::set_inactivity_period(uint32_t value) {
    inactivity_timeout_default_ = value * 1000;
}

void HaDeck::set_inactivity_blank_screen(bool value) {
    inactivity_blank_screen_ = value;
}

bool HaDeck::get_inactivity() {
    return inactivity_;
}

void HaDeck::set_main_screen(std::string value) {
    main_screen_name_ = value;
}

void HaDeck::add_screen(HaDeckScreen *screen) {
    ESP_LOGD(this->TAG, "add_screen: %s", screen->get_name().c_str());
    screens_[screen->get_name()] = screen;
    screen_order_.push_back(screen->get_name());
}

void HaDeck::switch_screen(std::string name) {
    if (active_screen_ && active_screen_->get_name() == name)
        return;
    
    if (active_screen_) {
        active_screen_->set_active(false);
        active_screen_ = nullptr;
    }
    
    if (!screens_.count(name))
        return;

    active_screen_ = screens_[name];
    active_screen_->set_active(true);
    inactivity_timeout_ = active_screen_->get_inactivity() > 0
        ? active_screen_->get_inactivity()
        : inactivity_timeout_default_;
}

std::string HaDeck::get_active_screen() {
    return active_screen_ ? active_screen_->get_name() : std::string();
}

size_t HaDeck::current_screen_index_() {
    if (active_screen_) {
        for (size_t i = 0; i < screen_order_.size(); i++) {
            if (screen_order_[i] == active_screen_->get_name())
                return i;
        }
    }
    return 0;
}

void HaDeck::next_screen() {
    if (screen_order_.empty())
        return;
    size_t index = (current_screen_index_() + 1) % screen_order_.size();
    switch_screen(screen_order_[index]);
}

void HaDeck::previous_screen() {
    if (screen_order_.empty())
        return;
    size_t index = (current_screen_index_() + screen_order_.size() - 1) % screen_order_.size();
    switch_screen(screen_order_[index]);
}

void HaDeck::add_on_inactivity_change_callback(std::function<void(bool)> &&callback) {
    inactivity_change_callback_.add(std::move(callback));
}

void HaDeck::set_inactivity_(bool value) {
    if (inactivity_ == value)
        return;
    
    inactivity_ = value;
    if (inactivity_) {
        switch_screen(main_screen_name_);
        if (inactivity_blank_screen_) {
            lv_scr_load_anim(lvgl_inactivity_screen_, LV_SCR_LOAD_ANIM_FADE_OUT, 300, 0, false);
        }
    } else {
        if (inactivity_blank_screen_) {
            lv_scr_load(lvgl_main_screen_);
        }
    }
    inactivity_change_callback_.call(inactivity_);
}

void HaDeck::create_inactivity_screen_() {
    lvgl_inactivity_screen_ = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(lvgl_inactivity_screen_, lv_color_hex(0x0), LV_PART_MAIN | LV_STATE_DEFAULT);
}

}  // namespace ha_deck
}  // namespace esphome