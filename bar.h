#ifndef _BAR_H_
#define _BAR_H_

#include <wayland-client.h>

#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "pool-buffer.h"
#include "bar.h"
#include "button.h"

struct bar_output {
	struct wl_output *output;
	struct wl_surface *surface;
	struct wl_list link;
	struct zwlr_layer_surface_v1 *layer_surface;
	struct pool_buffer buffers[2];
	struct pool_buffer *current_buffer;
    bool frame_scheduled;

	uint32_t width, height;
	uint32_t max_width, max_height;
	int32_t scale;
	bool should_extend, extended;
    struct button button_a;
    struct button button_b;
    struct button button_c;
};

struct touch_slot {
	int32_t id;
	uint32_t time;
	struct bar_output *output;
	double start_x, start_y;
	double x, y;
};

struct bar_touch {
	struct wl_touch *touch;
	struct touch_slot slots[16];
};

struct bar_pointer {
	struct wl_pointer *pointer;
	struct wl_cursor_theme *cursor_theme;
	struct wl_cursor_image *cursor_image;
	struct wl_surface *cursor_surface;
	struct bar_output *current;
	int x, y;
	uint32_t serial;
};

struct bar_seat {
	uint32_t wl_name;
	struct wl_seat *wl_seat;
	struct bar_pointer pointer;
	struct bar_touch touch;
	struct wl_list link; // bar_seat:link
};


#endif // _BAR_H_
