#ifndef PEPPER_WL_SHELL_H
#define PEPPER_WL_SHELL_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

PEPPER_API pepper_bool_t
pepper_desktop_shell_init(pepper_object_t *compositor);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_WL_SHELL_H */
