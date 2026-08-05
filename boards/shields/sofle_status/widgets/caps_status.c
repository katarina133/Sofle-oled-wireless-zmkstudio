/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/hid_indicators_types.h>

#include "caps_status.h"

/* HID keyboard LED report bits. Bit 0 is Num Lock, bit 1 Caps Lock, bit 2 Scroll Lock. */
#define LED_CAPS_LOCK 0x02

struct caps_status_state {
    zmk_hid_indicators_t indicators;
};

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static void set_caps_status(lv_obj_t *label, struct caps_status_state state) {
    lv_label_set_text(label, (state.indicators & LED_CAPS_LOCK) ? "CAPS" : "____");
}

static void caps_status_update_cb(struct caps_status_state state) {
    struct zmk_widget_caps_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_caps_status(widget->obj, state); }
}

static struct caps_status_state caps_status_get_state(const zmk_event_t *eh) {
    const struct zmk_hid_indicators_changed *ev = as_zmk_hid_indicators_changed(eh);

    return (struct caps_status_state){.indicators = ev->indicators};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_caps_status, struct caps_status_state, caps_status_update_cb,
                            caps_status_get_state)

/* On the central this is raised locally from the host's LED report. On a peripheral
 * it is raised by the split GATT service, but only when the central was built with
 * CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS=y to forward it. */
ZMK_SUBSCRIPTION(widget_caps_status, zmk_hid_indicators_changed);

int zmk_widget_caps_status_init(struct zmk_widget_caps_status *widget, lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);

    lv_obj_set_style_text_font(widget->obj, lv_theme_get_font_small(parent), LV_PART_MAIN);
    set_caps_status(widget->obj, (struct caps_status_state){.indicators = 0});

    sys_slist_append(&widgets, &widget->node);

    widget_caps_status_init();

    return 0;
}

lv_obj_t *zmk_widget_caps_status_obj(struct zmk_widget_caps_status *widget) { return widget->obj; }
