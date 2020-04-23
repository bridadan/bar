#include <assert.h>
#include <linux/input-event-codes.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wlr/util/log.h>

#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "input.h"
#include "bar.h"
#include "button.h"

extern const unsigned int regular_height;
extern struct wl_shm *shm;
extern struct wl_list bar_outputs;
extern struct wl_list bar_seats;
extern struct wl_compositor *compositor;
extern bool run_display;

static void process_hotspots(struct bar_output *output,
		double x, double y, uint32_t button, bool down, bool touch) {
	//double new_x = x * output->scale;
	//double new_y = y * output->scale;

	/* click */
	wlr_log(WLR_DEBUG, "%s at (%f, %f) with button %u", down ? "down" : "up", x, y, button);

    if (down) {
        button_pointer_moved(&output->button_a, x, y);
        button_pointer_moved(&output->button_b, x, y);
        button_pointer_moved(&output->button_c, x, y);

        // TODO mark the output as needing a new frame
        bool button_clicked = false;
        if (output->button_a.hover && !output->button_a.active) {
            button_clicked = true;
            output->button_a.active = true;
            output->button_a.render_required = true;
        }

        if (output->button_b.hover && !output->button_b.active) {
            button_clicked = true;
            output->button_b.active = true;
            output->button_b.render_required = true;
        }

        if (output->button_c.hover && !output->button_c.active) {
            button_clicked = true;
            output->button_c.active = true;
            output->button_c.render_required = true;
        }

        if (!button_clicked) {
            output->should_extend = !output->should_extend;
            zwlr_layer_surface_v1_set_size(output->layer_surface, output->width, output->should_extend ? output->max_height : regular_height);
            wl_surface_commit(output->surface);
        }
    } else {
        if (output->button_a.active) {
            output->button_a.active = false;
            output->button_a.render_required = true;
        }

        if (output->button_b.active) {
            output->button_b.active = false;
            output->button_b.render_required = true;
        }

        if (output->button_c.active) {
            output->button_c.active = false;
            output->button_c.render_required = true;
        }

        if (touch) {
            output->button_a.hover = false;
            output->button_b.hover = false;
            output->button_c.hover = false;
        }
    }
}

static void update_cursor(struct bar_seat *seat) {
	struct bar_pointer *pointer = &seat->pointer;
	if (!pointer || !pointer->cursor_surface) {
		return;
	}
	if (pointer->cursor_theme) {
		wl_cursor_theme_destroy(pointer->cursor_theme);
	}
	//const char *cursor_theme = getenv("XCURSOR_THEME");
	unsigned cursor_size = 24;
	const char *env_cursor_size = getenv("XCURSOR_SIZE");
	if (env_cursor_size && strlen(env_cursor_size) > 0) {
		errno = 0;
		char *end;
		unsigned size = strtoul(env_cursor_size, &end, 10);
		if (!*end && errno == 0) {
			cursor_size = size;
		}
	}
	int scale = pointer->current ? pointer->current->scale : 1;
	pointer->cursor_theme = wl_cursor_theme_load(
		NULL, cursor_size * scale, shm);
	assert(pointer->cursor_theme);
	struct wl_cursor *cursor;
	cursor = wl_cursor_theme_get_cursor(pointer->cursor_theme, "left_ptr");
	pointer->cursor_image = cursor->images[0];
	wl_surface_set_buffer_scale(pointer->cursor_surface, scale);
	wl_surface_attach(pointer->cursor_surface,
			wl_cursor_image_get_buffer(pointer->cursor_image), 0, 0);
	wl_pointer_set_cursor(pointer->pointer, pointer->serial,
			pointer->cursor_surface,
			pointer->cursor_image->hotspot_x / scale,
			pointer->cursor_image->hotspot_y / scale);
	wl_surface_damage_buffer(pointer->cursor_surface, 0, 0,
			INT32_MAX, INT32_MAX);
	wl_surface_commit(pointer->cursor_surface);
}

static void wl_pointer_enter(void *data, struct wl_pointer *wl_pointer,
		uint32_t serial, struct wl_surface *surface,
		wl_fixed_t surface_x, wl_fixed_t surface_y) {
	struct bar_seat *seat = data;
	struct bar_pointer *pointer = &seat->pointer;
	pointer->serial = serial;
	struct bar_output *output;
	wl_list_for_each(output, &bar_outputs, link) {
		if (output->surface == surface) {
			pointer->current = output;
			break;
		}
	}
	update_cursor(seat);
}

static void wl_pointer_leave(void *data, struct wl_pointer *wl_pointer,
		uint32_t serial, struct wl_surface *surface) {
	struct bar_seat *seat = data;
	seat->pointer.current = NULL;
}

static void wl_pointer_motion(void *data, struct wl_pointer *wl_pointer,
		uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
	struct bar_seat *seat = data;
	seat->pointer.x = wl_fixed_to_int(surface_x);
	seat->pointer.y = wl_fixed_to_int(surface_y);

	struct bar_output *output = NULL;
    // TODO mark the output as needing a new frame
	wl_list_for_each(output, &bar_outputs, link) {
        button_pointer_moved(&output->button_a, seat->pointer.x, seat->pointer.y);
        button_pointer_moved(&output->button_b, seat->pointer.x, seat->pointer.y);
        button_pointer_moved(&output->button_c, seat->pointer.x, seat->pointer.y);
	}
}

static void wl_pointer_button(void *data, struct wl_pointer *wl_pointer,
		uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
	struct bar_seat *seat = data;
	struct bar_pointer *pointer = &seat->pointer;
	struct bar_output *output = pointer->current;
	assert(output && "button with no active output");

	process_hotspots(output, pointer->x, pointer->y, button, state == WL_POINTER_BUTTON_STATE_PRESSED, false);
}

static void wl_pointer_axis(void *data, struct wl_pointer *wl_pointer,
		uint32_t time, uint32_t axis, wl_fixed_t value) {
	// Who cares
}

static void wl_pointer_frame(void *data, struct wl_pointer *wl_pointer) {
	// Who cares
}

static void wl_pointer_axis_source(void *data, struct wl_pointer *wl_pointer,
		uint32_t axis_source) {
	// Who cares
}

static void wl_pointer_axis_stop(void *data, struct wl_pointer *wl_pointer,
		uint32_t time, uint32_t axis) {
	// Who cares
}

static void wl_pointer_axis_discrete(void *data, struct wl_pointer *wl_pointer,
		uint32_t axis, int32_t discrete) {
	// Who cares
}

struct wl_pointer_listener pointer_listener = {
	.enter = wl_pointer_enter,
	.leave = wl_pointer_leave,
	.motion = wl_pointer_motion,
	.button = wl_pointer_button,
	.axis = wl_pointer_axis,
	.frame = wl_pointer_frame,
	.axis_source = wl_pointer_axis_source,
	.axis_stop = wl_pointer_axis_stop,
	.axis_discrete = wl_pointer_axis_discrete,
};

static struct touch_slot *get_touch_slot(struct bar_touch *touch, int32_t id) {
	ssize_t next = -1;
	for (size_t i = 0; i < sizeof(touch->slots) / sizeof(*touch->slots); ++i) {
		if (touch->slots[i].id == id) {
			return &touch->slots[i];
		}
		if (next == -1 && !touch->slots[i].output) {
			next = i;
		}
	}
	if (next == -1) {
		wlr_log(WLR_DEBUG, "Ran out of touch slots");
		return NULL;
	}
	return &touch->slots[next];
}

static void wl_touch_down(void *data, struct wl_touch *wl_touch,
		  uint32_t serial, uint32_t time, struct wl_surface *surface,
		  int32_t id, wl_fixed_t x, wl_fixed_t y) {
	struct bar_seat *seat = data;
	struct bar_output *_output = NULL, *output = NULL;
	wl_list_for_each(_output, &bar_outputs, link) {
		if (_output->surface == surface) {
			output = _output;
			break;
		}
	}
	if (!output) {
		wlr_log(WLR_DEBUG, "Got touch event for unknown surface");
		return;
	}
	struct touch_slot *slot = get_touch_slot(&seat->touch, id);
	if (!slot) {
		return;
	}
	slot->id = id;
	slot->output = output;
	slot->x = slot->start_x = wl_fixed_to_double(x);
	slot->y = slot->start_y = wl_fixed_to_double(y);
	slot->time = time;

    process_hotspots(slot->output, slot->x, slot->y, BTN_LEFT, true, true);
}

static void wl_touch_up(void *data, struct wl_touch *wl_touch,
		uint32_t serial, uint32_t time, int32_t id) {
	struct bar_seat *seat = data;
	struct touch_slot *slot = get_touch_slot(&seat->touch, id);
	if (!slot) {
		return;
	}
	if (time - slot->time < 500) {
		// Tap, treat it like a pointer click
		process_hotspots(slot->output, slot->x, slot->y, BTN_LEFT, false, true);
	}
	slot->output = NULL;

}

static void wl_touch_motion(void *data, struct wl_touch *wl_touch,
		    uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y) {
	struct bar_seat *seat = data;
	struct touch_slot *slot = get_touch_slot(&seat->touch, id);
	if (!slot) {
		return;
	}
	int prev_progress = (int)((slot->x - slot->start_x)
			/ slot->output->width * 100);
	slot->x = wl_fixed_to_double(x);
	slot->y = wl_fixed_to_double(y);
	// "progress" is a measure from 0..100 representing the fraction of the
	// output the touch gesture has travelled, positive when moving to the right
	// and negative when moving to the left.
	int progress = (int)((slot->x - slot->start_x)
			/ slot->output->width * 100);
	if (abs(progress) / 20 != abs(prev_progress) / 20) {
		// workspace_next(seat->bar, slot->output, progress - prev_progress < 0);
	}

}

static void wl_touch_frame(void *data, struct wl_touch *wl_touch) {

}

static void wl_touch_cancel(void *data, struct wl_touch *wl_touch) {
	struct bar_seat *seat = data;
	struct bar_touch *touch = &seat->touch;
	for (size_t i = 0; i < sizeof(touch->slots) / sizeof(*touch->slots); ++i) {
		touch->slots[i].output = NULL;
	}
}

struct wl_touch_listener touch_listener = {
	.down = wl_touch_down,
	.up = wl_touch_up,
	.motion = wl_touch_motion,
	.frame = wl_touch_frame,
	.cancel = wl_touch_cancel,
};

static void seat_handle_capabilities(void *data, struct wl_seat *wl_seat,
		enum wl_seat_capability caps) {
	struct bar_seat *seat = data;

	bool have_pointer = caps & WL_SEAT_CAPABILITY_POINTER;
	bool have_touch = caps & WL_SEAT_CAPABILITY_TOUCH;

	if (!have_pointer && seat->pointer.pointer != NULL) {
		wl_pointer_release(seat->pointer.pointer);
		seat->pointer.pointer = NULL;
	} else if (have_pointer && seat->pointer.pointer == NULL) {
		seat->pointer.pointer = wl_seat_get_pointer(wl_seat);
		if (run_display && !seat->pointer.cursor_surface) {
			seat->pointer.cursor_surface =
				wl_compositor_create_surface(compositor);
			assert(seat->pointer.cursor_surface);
		}
		wl_pointer_add_listener(seat->pointer.pointer, &pointer_listener, seat);
	}
	if (!have_touch && seat->touch.touch != NULL) {
		wl_touch_release(seat->touch.touch);
		seat->touch.touch = NULL;
	} else if (have_touch && seat->touch.touch == NULL) {
		seat->touch.touch = wl_seat_get_touch(wl_seat);
		wl_touch_add_listener(seat->touch.touch, &touch_listener, seat);
	}
}

static void seat_handle_name(void *data, struct wl_seat *wl_seat,
		const char *name) {
	// Who cares
}

struct wl_seat_listener seat_listener = {
	.capabilities = seat_handle_capabilities,
	.name = seat_handle_name,
};

