#ifndef PEPPER_H
#define PEPPER_H

#include <stdint.h>

#define PEPPER_FALSE    0
#define PEPPER_TRUE     1

typedef uint32_t                    pepper_bool_t;

typedef struct pepper_compositor    pepper_compositor_t;
typedef struct pepper_output        pepper_output_t;
typedef struct pepper_output_info   pepper_output_info_t;
typedef struct pepper_client        pepper_client_t;
typedef struct pepper_surface       pepper_surface_t;
typedef struct pepper_shell_surface pepper_shell_surface_t;

typedef struct pepper_seat          pepper_seat_t;
typedef struct pepper_pointer       pepper_pointer_t;
typedef struct pepper_keyboard      pepper_keyboard_t;
typedef struct pepper_touch         pepper_touch_t;

struct pepper_output_info
{
    int x, y, w, h;
    void *data;
};

/* Compositor functions. */
pepper_compositor_t *
pepper_compositor_create(const char *socket_name,
                         const char *backend_name,
                         const char *input_name,
                         const char *shell_name,
                         const char *renderer_name);

void
pepper_compositor_destroy(pepper_compositor_t *compositor);

pepper_output_t *
pepper_compositor_add_output(pepper_compositor_t  *compositor,
                             pepper_output_info_t *info);

int
pepper_compositor_get_output_count(pepper_compositor_t *compositor);

pepper_output_t *
pepper_compositor_get_output(pepper_compositor_t *compositor, int index);

pepper_client_t *
pepper_compositor_get_client(pepper_compositor_t *compositor, int index);

void
pepper_compositor_frame(pepper_compositor_t *compositor);

/* Output functions. */
pepper_bool_t
pepper_output_move(pepper_output_t *output, int x, int y, int w, int h);

void
pepper_output_get_geometry(pepper_output_t *output, int *x, int *y, int *w, int *h);

pepper_compositor_t *
pepper_output_get_compositor(pepper_output_t *output);

pepper_bool_t
pepper_output_destroy(pepper_output_t *output);

/* Client functions. */
pepper_surface_t *
pepper_client_get_surface(pepper_client_t *client, int index);

/* Surface functions. */
void *
pepper_surface_get_buffer(pepper_surface_t *surface);

#endif /* PEPPER_H */
