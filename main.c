#define _POSIX_C_SOURCE 200112L
#include <linux/input-event-codes.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wlr/util/log.h>
#include <cairo/cairo.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include "button.h"
#include "pool-buffer.h"
#include "bar.h"
#include "input.h"

static struct wl_display *display;
struct wl_compositor *compositor;
struct wl_shm *shm;
static struct xdg_wm_base *xdg_wm_base;
static struct zwlr_layer_shell_v1 *layer_shell;

struct wl_callback *frame_callback;

const unsigned int regular_height = 32;
bool run_display = true;
static const uint32_t regular_color = 0x808080CC;
static const uint32_t extended_color = 0x222222CC;
static const uint32_t button_side = 100;
static const uint32_t button_margin = 15;

struct wl_list bar_outputs;
struct wl_list bar_seats;

void set_output_dirty(struct bar_output *output) {
    if (output->frame_scheduled) {
        output->dirty = true;
    } else if (output->surface) {
        draw(output);
    }
}

static void surface_frame_callback(
		void *data, struct wl_callback *cb, uint32_t time) {
    struct bar_output *output = data;
	wl_callback_destroy(cb);
	frame_callback = NULL;
    output->frame_scheduled = false;
    if (output->dirty) {
        draw(output);
        output->dirty = false;
    }

}

static struct wl_callback_listener output_frame_listener = {
	.done = surface_frame_callback
};

static void draw_extended(cairo_t *cairo, struct bar_output *output) {
    cairo_set_source_u32(cairo, extended_color);
	cairo_paint(cairo);

	button_draw(&(output->button_a), cairo, output);
	button_draw(&(output->button_b), cairo, output);
	button_draw(&(output->button_c), cairo, output);
}

static void draw_regular(cairo_t *cairo, struct bar_output *output) {
    cairo_set_source_u32(cairo, regular_color);
	cairo_paint(cairo);
}

void draw(struct bar_output *output) {
    wlr_log(WLR_DEBUG, "Drawing");
	cairo_surface_t *recorder = cairo_recording_surface_create(
			CAIRO_CONTENT_COLOR_ALPHA, NULL);
	cairo_t *cairo = cairo_create(recorder);
	cairo_set_antialias(cairo, CAIRO_ANTIALIAS_BEST);
    /*
	cairo_font_options_t *fo = cairo_font_options_create();
	cairo_font_options_set_hint_style(fo, CAIRO_HINT_STYLE_FULL);
	cairo_font_options_set_antialias(fo, CAIRO_ANTIALIAS_SUBPIXEL);
	cairo_font_options_set_subpixel_order(fo, to_cairo_subpixel_order(output->subpixel));
	cairo_set_font_options(cairo, fo);
	cairo_font_options_destroy(fo);
    */
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cairo);
	cairo_restore(cairo);

    // do the render
	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    if (output->extended) {
        draw_extended(cairo, output);
    } else {
        draw_regular(cairo, output);
    }

    // Replay recording into shm and send it off
    output->current_buffer = get_next_buffer(shm,
            output->buffers,
            output->width * output->scale,
            output->height * output->scale);
    if (!output->current_buffer) {
        cairo_surface_destroy(recorder);
        cairo_destroy(cairo);
        wlr_log(WLR_DEBUG, "Null buffer");
        return;
    }
    cairo_t *shm = output->current_buffer->cairo;

    cairo_save(shm);
    cairo_set_operator(shm, CAIRO_OPERATOR_CLEAR);
    cairo_paint(shm);
    cairo_restore(shm);

    cairo_set_source_surface(shm, recorder, 0.0, 0.0);
    cairo_paint(shm);

    wl_surface_set_buffer_scale(output->surface, output->scale);
    wl_surface_attach(output->surface,
            output->current_buffer->buffer, 0, 0);
    wl_surface_damage(output->surface, 0, 0,
            output->width, output->height);

    struct wl_callback *frame_callback = wl_surface_frame(output->surface);
    wl_callback_add_listener(frame_callback, &output_frame_listener, output);
    output->frame_scheduled = true;

    wl_surface_commit(output->surface);
	cairo_surface_destroy(recorder);
	cairo_destroy(cairo);
}

static void layer_surface_configure(void *data,
		struct zwlr_layer_surface_v1 *surface,
		uint32_t serial, uint32_t w, uint32_t h) {
	struct bar_output *output = data;
	output->width = w;
	output->height = h;
	wlr_log(WLR_DEBUG, "layer_surface_configure: %u, %u", output->width, output->height);

    output->extended = output->should_extend;
	zwlr_layer_surface_v1_ack_configure(output->layer_surface, serial);
    set_output_dirty(output);
}

static void layer_surface_closed(void *data,
		struct zwlr_layer_surface_v1 *surface) {
	struct bar_output *output = data;
	zwlr_layer_surface_v1_destroy(output->layer_surface);
	wl_surface_destroy(output->surface);
	run_display = false;
}

struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_configure,
	.closed = layer_surface_closed,
};

static void output_geometry(void *data, struct wl_output *wl_output, int32_t x,
		int32_t y, int32_t width_mm, int32_t height_mm, int32_t subpixel,
		const char *make, const char *model, int32_t transform) {
	// Who cares
}

static void output_mode(void *data, struct wl_output *wl_output, uint32_t flags,
		int32_t width, int32_t height, int32_t refresh) {
	struct bar_output *output = data;
	output->max_width = width;
	output->max_height = height;
	// Who cares
}

static void output_done(void *data, struct wl_output *wl_output) {
	// Who cares
}

static void output_scale(void *data, struct wl_output *wl_output,
		int32_t factor) {
	struct bar_output *output = data;
	wlr_log(WLR_DEBUG, "output scale %d", factor);
	output->scale = factor;
}
struct wl_output_listener output_listener = {
	.geometry = output_geometry,
	.mode = output_mode,
	.done = output_done,
	.scale = output_scale,
};

static void handle_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version) {
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		compositor = wl_registry_bind(registry, name,
				&wl_compositor_interface, 4);
	} else if (strcmp(interface, wl_shm_interface.name) == 0) {
		shm = wl_registry_bind(registry, name,
				&wl_shm_interface, 1);
	} else if (strcmp(interface, wl_output_interface.name) == 0) {
		struct bar_output *output = calloc(1, sizeof(struct bar_output));
		output->output = wl_registry_bind(registry, name,
			&wl_output_interface, 3);
		wl_list_insert(&bar_outputs, &output->link);
		wl_output_add_listener(output->output, &output_listener, output);
		wlr_log(WLR_DEBUG, "output listener");
	} else if (strcmp(interface, wl_seat_interface.name) == 0) {
		struct bar_seat *seat = calloc(1, sizeof(struct bar_seat));
		if (!seat) {
			wlr_log(WLR_DEBUG, "Failed to allocate bar_seat");
			return;
		}
		seat->wl_name = name;
		// NOTE this used version 3 in swaybar
		seat->wl_seat = wl_registry_bind(registry, name, &wl_seat_interface, 3);
		wl_seat_add_listener(seat->wl_seat, &seat_listener, seat);
		wl_list_insert(&bar_seats, &seat->link);
	} else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		layer_shell = wl_registry_bind(
				registry, name, &zwlr_layer_shell_v1_interface, 1);
	} else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		xdg_wm_base = wl_registry_bind(
				registry, name, &xdg_wm_base_interface, 2);
	}
}

static void handle_global_remove(void *data, struct wl_registry *registry,
		uint32_t name) {
	// who cares
}

static const struct wl_registry_listener registry_listener = {
	.global = handle_global,
	.global_remove = handle_global_remove,
};

int main(int argc, char **argv) {
	wlr_log_init(WLR_DEBUG, NULL);
	wl_list_init(&bar_outputs);
	wl_list_init(&bar_seats);

	char *namespace = "wlroots";
	int c;
	while ((c = getopt(argc, argv, "h:")) != -1) {
		switch (c) {
		case 'h':
			printf("Help text here\n");
			return 0;
			break;
		default:
			break;
		}
	}

	display = wl_display_connect(NULL);
	if (display == NULL) {
		fprintf(stderr, "Failed to create display\n");
		return 1;
	}

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_display_roundtrip(display);

	if (compositor == NULL) {
		fprintf(stderr, "wl_compositor not available\n");
		return 1;
	}
	if (shm == NULL) {
		fprintf(stderr, "wl_shm not available\n");
		return 1;
	}
	if (layer_shell == NULL) {
		fprintf(stderr, "layer_shell not available\n");
		return 1;
	}

	wlr_log(WLR_DEBUG, "Starting");

	struct bar_output *output;
	wl_list_for_each(output, &bar_outputs, link) {
		output->surface = wl_compositor_create_surface(compositor);
		assert(output->surface);

		output->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
			layer_shell,
			output->surface,
			output->output,
			ZWLR_LAYER_SHELL_V1_LAYER_TOP,
			namespace
		);
		assert(output->layer_surface);

		wlr_log(WLR_DEBUG, "initial height: %u", regular_height);
		zwlr_layer_surface_v1_set_size(
			output->layer_surface,
			output->width,
			regular_height
		);
		zwlr_layer_surface_v1_set_anchor(
			output->layer_surface,
			ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | \
			ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | \
			ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
		);
		zwlr_layer_surface_v1_set_exclusive_zone(output->layer_surface, regular_height);
		zwlr_layer_surface_v1_set_keyboard_interactivity(output->layer_surface, false);
		zwlr_layer_surface_v1_add_listener(
			output->layer_surface,
			&layer_surface_listener,
			output
		);
		wl_surface_commit(output->surface);
		wl_display_roundtrip(display);

        button_init(
            &(output->button_a),
            output->width / 2 - button_side / 2 - button_side - button_margin,
            regular_height,
            button_side,
            button_side
        );

        button_init(
            &(output->button_b),
            output->width / 2 - button_side / 2,
            regular_height,
            button_side,
            button_side
        );

        button_init(
            &(output->button_c),
            output->width / 2 - button_side / 2 + button_side + button_margin,
            regular_height,
            button_side,
            button_side
        );

		draw(output);
	}

	while (wl_display_dispatch(display) != -1 && run_display) {
		// This space intentionally left blank
	}

	return 0;
}
