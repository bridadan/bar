#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <cairo/cairo.h>

#include "input.h"

enum button_state {
    BUTTON_IDLE,
    BUTTON_HOVER,
    BUTTON_ACTIVE,
};

struct button {
    unsigned int x, y, width, height;
    enum button_state state;
    bool render_required;
};

void button_init(
    struct button *self,
    unsigned int x,
    unsigned int y,
    unsigned int width,
    unsigned int height
);

bool button_state_update(
    struct button *self,
    unsigned int pointer_x,
    unsigned int pointer_y,
    enum pointer_event event,
    bool pointer_active
);

struct bar_output;

void button_draw(struct button *self, cairo_t *cairo, struct bar_output *output);

#endif // _BUTTON_H_
