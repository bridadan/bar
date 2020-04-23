#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <cairo/cairo.h>

struct button {
    unsigned int x, y, width, height;
    bool render_required, hover, active;
};

void button_init(
    struct button *self,
    unsigned int x,
    unsigned int y,
    unsigned int width,
    unsigned int height
);

bool button_pointer_moved(
    struct button *self,
    unsigned int pointer_x,
    unsigned int pointer_y
);

struct bar_output;

void button_draw(struct button *self, cairo_t *cairo, struct bar_output *output);

#endif // _BUTTON_H_
