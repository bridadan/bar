#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "button.h"
#include "bar.h"
#include "cairo.h"

static const uint32_t button_border_color = 0x4C7899FF;
static const uint32_t button_background_color = 0x222222FF;
static const uint32_t button_background_hover_color = 0x285577FF;
static const uint32_t button_background_active_color = 0x5f676aFF;

void button_init(
    struct button *self,
    unsigned int x,
    unsigned int y,
    unsigned int width,
    unsigned int height
) {
    self->x = x;
    self->y = y;
    self->width = width;
    self->height = height;

    self->render_required = true;
    self->hover = false;
    self->active = false;
}

bool button_pointer_moved(
    struct button *self,
    unsigned int pointer_x,
    unsigned int pointer_y
) {
    // No need to scale this since its a common factor of both sides
    if (pointer_x >= self->x &&
        pointer_x <= self->x + self->width &&
        pointer_y >= self->y &&
        pointer_y <= self->y + self->height
    ) {
        if (!self->hover) {
            self->hover = true;
            self->render_required = true;
        }
    } else if (self->hover) {
        self->hover = false;
        self->render_required = true;
    }

    return self->render_required;
}

void button_draw(struct button *self, cairo_t *cairo, struct bar_output *output) {
    if (self->active) {
        cairo_set_source_u32(cairo, button_background_active_color);
    } else if (self->hover) {
        cairo_set_source_u32(cairo, button_background_hover_color);
    } else {
        cairo_set_source_u32(cairo, button_background_color);
    }

    cairo_rectangle(
        cairo,
        self->x * output->scale,
        self->y * output->scale,
        self->height * output->scale,
        self->width * output->scale
    );
	cairo_fill_preserve(cairo);
    cairo_set_source_u32(cairo, button_border_color);
    cairo_stroke(cairo);
}
