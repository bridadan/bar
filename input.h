#ifndef _INPUT_H_
#define _INPUT_H_

#include <wayland-client.h>

extern struct wl_seat_listener seat_listener;

enum pointer_event {
    POINTER_MOVE,
    POINTER_DOWN,
    POINTER_UP,
};


#endif //_INPUT_H
