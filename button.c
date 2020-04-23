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
    self->state = BUTTON_IDLE;
}

// pointer_active in this case means there's a mouse being moved.
// Touch screens don't have to move first before existing at a location.
// On "touch up" events, this should be set to false. On "pointer move"
// and "touch down" events, this should be set to true.
bool button_state_update(
    struct button *self,
    unsigned int pointer_x,
    unsigned int pointer_y,
    enum pointer_event event,
    bool pointer_active
) {
    // No need to scale this since its a common factor of both sides
    enum button_state prev_state = self->state;
    if (
        pointer_active &&
        pointer_x >= self->x &&
        pointer_x <= self->x + self->width &&
        pointer_y >= self->y &&
        pointer_y <= self->y + self->height
    ) {
        if (event == POINTER_DOWN) {
            self->state = BUTTON_ACTIVE;
        } else {
            self->state = BUTTON_HOVER;
        }
    } else {
        self->state = BUTTON_IDLE;
    }

    if (prev_state != self->state) {
        self->render_required = true;
    }

    return self->render_required;
}

void button_draw(struct button *self, cairo_t *cairo, struct bar_output *output) {
    if (self->state == BUTTON_ACTIVE) {
        cairo_set_source_u32(cairo, button_background_active_color);
    } else if (self->state == BUTTON_HOVER) {
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

    self->render_required = false;
}
