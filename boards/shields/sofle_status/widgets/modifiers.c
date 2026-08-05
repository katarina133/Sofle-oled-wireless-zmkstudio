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
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/modifiers.h>

#include "modifiers.h"

struct modifiers_state {
    uint8_t modifiers;
};

/* Slot order is the order the letters appear on screen. Left and right variants of
 * each modifier share a slot, so either Ctrl lights the same "C". */
static const struct modifier_slot {
    uint8_t mask;
    char letter;
} modifier_slots[] = {
    {MOD_LCTL | MOD_RCTL, 'C'},
    {MOD_LSFT | MOD_RSFT, 'S'},
    {MOD_LALT | MOD_RALT, 'A'},
    {MOD_LGUI | MOD_RGUI, 'G'},
};

#define NUM_SLOTS ARRAY_SIZE(modifier_slots)

/* "C S A G", i.e. one char per slot plus a separating space, minus the trailing
 * separator, plus the NUL. */
#define MODIFIERS_TEXT_LEN (NUM_SLOTS * 2)

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static void set_modifiers(lv_obj_t *label, struct modifiers_state state) {
    char text[MODIFIERS_TEXT_LEN];

    for (int i = 0; i < NUM_SLOTS; i++) {
        bool active = (state.modifiers & modifier_slots[i].mask) != 0;

        text[i * 2] = active ? modifier_slots[i].letter : '_';
        text[i * 2 + 1] = ' ';
    }

    text[MODIFIERS_TEXT_LEN - 1] = '\0';

    lv_label_set_text(label, text);
}

static void modifiers_update_cb(struct modifiers_state state) {
    struct zmk_widget_modifiers *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_modifiers(widget->obj, state); }
}

static struct modifiers_state modifiers_get_state(const zmk_event_t *eh) {
    return (struct modifiers_state){.modifiers = zmk_hid_get_explicit_mods()};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_modifiers, struct modifiers_state, modifiers_update_cb,
                            modifiers_get_state)

/* Every press and release raises this, which is what makes the letters follow a
 * held modifier rather than latching. */
ZMK_SUBSCRIPTION(widget_modifiers, zmk_keycode_state_changed);

int zmk_widget_modifiers_init(struct zmk_widget_modifiers *widget, lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);

    lv_obj_set_style_text_font(widget->obj, lv_theme_get_font_small(parent), LV_PART_MAIN);
    set_modifiers(widget->obj, (struct modifiers_state){.modifiers = 0});

    sys_slist_append(&widgets, &widget->node);

    widget_modifiers_init();

    return 0;
}

lv_obj_t *zmk_widget_modifiers_obj(struct zmk_widget_modifiers *widget) { return widget->obj; }
