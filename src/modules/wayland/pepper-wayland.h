#ifndef PEPPER_WAYLAND_H
#define PEPPER_WAYLAND_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_wayland_output_info   pepper_wayland_output_info_t;

struct pepper_wayland_output_info
{
    const char  *socket_name;
};

PEPPER_API pepper_bool_t
pepper_wayland_init(pepper_compositor_t *compositor);

PEPPER_API const pepper_output_interface_t *
pepper_wayland_get_output_interface();

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_WAYLAND_H */
