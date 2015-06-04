#include "pepper-gl-renderer.h"
#include "pepper-render-internal.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "eglextwayland.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct gl_renderer      gl_renderer_t;
typedef struct gl_surface_state gl_surface_state_t;

#ifndef EGL_EXT_platform_base
typedef EGLDisplay  (*PFNEGLGETPLATFORMDISPLAYEXTPROC)(EGLenum          platform,
                                                       void            *native_display,
                                                       const EGLint    *attrib_list);

typedef EGLSurface  (*PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC)(EGLDisplay      display,
                                                                EGLConfig       config,
                                                                void           *native_window,
                                                                const EGLint   *attrib_list);
#endif

#define EGL_PLATFORM_X11_KHR        0x31d5
#define EGL_PLATFORM_GBM_KHR        0x31d7
#define EGL_PLATFORM_WAYLAND_KHR    0x31d8

enum buffer_type
{
    BUFFER_TYPE_NONE,
    BUFFER_TYPE_SHM,
    BUFFER_TYPE_EGL,
};

struct gl_renderer
{
    pepper_renderer_t       base;

    void                   *native_display;
    void                   *native_window;

    EGLDisplay              display;
    EGLSurface              surface;
    EGLContext              context;
    EGLConfig               config;

    /* EGL extensions. */
    PFNEGLCREATEIMAGEKHRPROC    create_image;
    PFNEGLDESTROYIMAGEKHRPROC   destroy_image;

#ifdef EGL_EXT_swap_buffers_with_damage
    PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC  swap_buffers_with_damage;
#endif

    PFNEGLBINDWAYLANDDISPLAYWL      bind_display;
    PFNEGLUNBINDWAYLANDDISPLAYWL    unbind_display;
    PFNEGLQUERYWAYLANDBUFFERWL      query_buffer;

    pepper_bool_t   has_buffer_age;

    PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC    create_platform_window_surface;

    /* GL extensions. */
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC image_target_texture_2d;

    pepper_bool_t   has_read_format_bgra;
    pepper_bool_t   has_unpack_subimage;
};

#define NUM_MAX_PLANES  3

enum shader_sampler
{
    GL_SHADER_SAMPLER_RGBA,
    GL_SHADER_SAMPLER_RGBX,
    GL_SHADER_SAMPLER_Y_UV,
    GL_SHADER_SAMPLER_Y_XUXV,
    GL_SHADER_SAMPLER_Y_U_V,
};

struct gl_surface_state
{
    gl_renderer_t      *renderer;
    pepper_surface_t   *surface;

    pepper_buffer_t    *buffer;
    int                 buffer_width, buffer_height;
    int                 buffer_type;

    int                 num_planes;
    GLuint              textures[NUM_MAX_PLANES];
    int                 sampler;
    int                 y_inverted;

    /* EGL buffer type. */
    EGLImageKHR         images[NUM_MAX_PLANES];

    /* SHM buffer type. */
    struct {
        struct wl_shm_buffer   *buffer;
        pepper_bool_t           need_full_upload;
        GLenum                  format;
        GLenum                  pixel_format;
        int                     pitch;
    } shm;

    struct wl_listener  buffer_destroy_listener;
    struct wl_listener  surface_destroy_listener;
};

static pepper_bool_t
gl_renderer_use(gl_renderer_t *gr)
{
    if (!eglMakeCurrent(gr->display, gr->surface, gr->surface, gr->context))
        return PEPPER_FALSE;

    return PEPPER_TRUE;
}

static void
gl_renderer_destroy(pepper_renderer_t *renderer)
{
    gl_renderer_t *gr = (gl_renderer_t *)renderer;

    if (gr->context != EGL_NO_CONTEXT)
        eglDestroyContext(gr->display, gr->context);

    if (gr->surface != EGL_NO_SURFACE)
        eglDestroySurface(gr->display, gr->surface);

    if (gr->display != EGL_NO_DISPLAY)
        eglTerminate(gr->display);

    free(gr);
}

/* TODO: Similar with pixman renderer. There might be a way of reusing those codes.  */

static void
surface_state_release_buffer(gl_surface_state_t *state)
{
    int i;

    for (i = 0; i < state->num_planes; i++)
    {
        glDeleteTextures(1, &state->textures[i]);
        state->textures[i] = 0;

        if (state->images[i] != EGL_NO_IMAGE_KHR)
        {
            state->renderer->destroy_image(state->renderer->display, state->images[i]);
            state->images[i] = EGL_NO_IMAGE_KHR;
        }
    }

    state->num_planes = 0;

    if (state->buffer)
    {
        pepper_buffer_unreference(state->buffer);
        state->buffer = NULL;

        wl_list_remove(&state->buffer_destroy_listener.link);
    }
}

static void
surface_state_handle_surface_destroy(struct wl_listener *listener, void *data)
{
    gl_surface_state_t *state =
        pepper_container_of(listener, gl_surface_state_t, surface_destroy_listener);

    surface_state_release_buffer(state);
    wl_list_remove(&state->surface_destroy_listener.link);
    pepper_surface_set_user_data(state->surface, state->renderer, NULL, NULL);
    free(state);
}

static void
surface_state_handle_buffer_destroy(struct wl_listener *listener, void *data)
{
    gl_surface_state_t *state =
        pepper_container_of(listener, gl_surface_state_t, buffer_destroy_listener);

    surface_state_release_buffer(state);
}

static gl_surface_state_t *
get_surface_state(pepper_renderer_t *renderer, pepper_surface_t *surface)
{
    gl_surface_state_t *state = pepper_surface_get_user_data(surface, renderer);

    if (!state)
    {
        state = (gl_surface_state_t *)calloc(1, sizeof(gl_surface_state_t));
        if (!state)
            return NULL;

        state->surface = surface;
        state->buffer_destroy_listener.notify = surface_state_handle_buffer_destroy;
        state->surface_destroy_listener.notify = surface_state_handle_surface_destroy;

        pepper_surface_add_destroy_listener(surface, &state->surface_destroy_listener);
        pepper_surface_set_user_data(surface, renderer, state, NULL);
    }

    return state;
}

static void
surface_state_destroy_images(gl_surface_state_t *state)
{
    int i;

    for (i = 0; i < state->num_planes; i++)
    {
        state->renderer->destroy_image(state->renderer->display, state->images[i]);
        state->images[i] = EGL_NO_IMAGE_KHR;
    }
}

static void
surface_state_ensure_textures(gl_surface_state_t *state, int num_planes)
{
    int i;

    for (i = 0; i < NUM_MAX_PLANES; i++)
    {
        if (state->textures[i] == 0 && i < num_planes)
        {
            glGenTextures(1, &state->textures[i]);
            glBindTexture(GL_TEXTURE_2D, state->textures[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        else if (state->textures[i] != 0 && i >= num_planes)
        {
            glDeleteTextures(1, &state->textures[i]);
            state->textures[i] = 0;
        }
    }

    state->num_planes = num_planes;
}

static pepper_bool_t
surface_state_attach_shm(gl_surface_state_t *state, pepper_buffer_t *buffer)
{
    struct wl_shm_buffer   *shm_buffer = wl_shm_buffer_get(pepper_buffer_get_resource(buffer));
    int                     w, h;
    int                     sampler;
    GLenum                  format;
    GLenum                  pixel_format;
    int                     pitch;

    if (!shm_buffer)
        return PEPPER_FALSE;

    switch (wl_shm_buffer_get_format(shm_buffer))
    {
        case WL_SHM_FORMAT_XRGB8888:
            sampler = GL_SHADER_SAMPLER_RGBX;
            format = GL_BGRA_EXT;
            pixel_format = GL_UNSIGNED_BYTE;
            pitch = wl_shm_buffer_get_stride(shm_buffer) / 4;
            break;
        case WL_SHM_FORMAT_ARGB8888:
            sampler = GL_SHADER_SAMPLER_RGBA;
            format = GL_BGRA_EXT;
            pixel_format = GL_UNSIGNED_BYTE;
            pitch = wl_shm_buffer_get_stride(shm_buffer) / 4;
            break;
        case WL_SHM_FORMAT_RGB565:
            sampler = GL_SHADER_SAMPLER_RGBA;
            format = GL_RGB;
            pixel_format = GL_UNSIGNED_SHORT_5_6_5;
            pitch = wl_shm_buffer_get_stride(shm_buffer) / 2;
            break;
        default:
            PEPPER_ERROR("Unknown shm buffer format.\n");
            return PEPPER_FALSE;
    }

    w = wl_shm_buffer_get_width(shm_buffer);
    h = wl_shm_buffer_get_height(shm_buffer);

    if (state->buffer_type != BUFFER_TYPE_SHM ||
        w != state->buffer_width || h != state->buffer_height ||
        pitch != state->shm.pitch || format != state->shm.format ||
        pixel_format != state->shm.pixel_format)
    {
        state->buffer_type      = BUFFER_TYPE_SHM;
        state->buffer_width     = w;
        state->buffer_height    = h;

        /* SHM buffer's origin is upper-left. */
        state->y_inverted       = 1;

        state->shm.buffer       = shm_buffer;
        state->shm.format       = format;
        state->shm.pixel_format = pixel_format;
        state->shm.pitch        = pitch;

        /* Don't use glTexSubImage2D() for shm buffers in this case. */
        state->shm.need_full_upload = PEPPER_TRUE;

        surface_state_ensure_textures(state, 1);
    }

    state->sampler = sampler;

    return PEPPER_TRUE;
}

static pepper_bool_t
surface_state_attach_egl(gl_surface_state_t *state, pepper_buffer_t *buffer)
{
    gl_renderer_t      *gr = state->renderer;
    EGLDisplay          display = gr->display;
    struct wl_resource *resource = pepper_buffer_get_resource(buffer);
    int                 num_planes;
    int                 sampler;
    EGLint              attribs[3];
    int                 texture_format;
    int                 i;

    if (!gr->query_buffer(display, resource, EGL_TEXTURE_FORMAT, &texture_format))
        return PEPPER_FALSE;

    switch (texture_format)
    {
    case EGL_TEXTURE_RGB:
        sampler = GL_SHADER_SAMPLER_RGBX;
        num_planes = 1;
        break;
    case EGL_TEXTURE_RGBA:
        sampler = GL_SHADER_SAMPLER_RGBA;
        num_planes = 1;
        break;
    case EGL_TEXTURE_Y_UV_WL:
        sampler = GL_SHADER_SAMPLER_Y_UV;
        num_planes = 2;
        break;
    case EGL_TEXTURE_Y_U_V_WL:
        sampler = GL_SHADER_SAMPLER_Y_U_V;
        num_planes = 3;
        break;
    case EGL_TEXTURE_Y_XUXV_WL:
        sampler = GL_SHADER_SAMPLER_Y_XUXV;
        num_planes = 2;
        break;
    default:
        PEPPER_ERROR("unknown EGL buffer format.\n");
        return PEPPER_FALSE;
    }

    if (state->buffer_type != BUFFER_TYPE_EGL || num_planes != state->num_planes)
        surface_state_ensure_textures(state, num_planes);

    attribs[0] = EGL_WAYLAND_PLANE_WL;
    attribs[2] = EGL_NONE;

    for (i = 0; i < num_planes; i++)
    {
        attribs[1] = i;
        state->images[i] = gr->create_image(display, NULL, EGL_WAYLAND_BUFFER_WL,
                                            resource, attribs);

        PEPPER_ASSERT(state->images[i] != EGL_NO_IMAGE_KHR);

        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, state->textures[i]);
        gr->image_target_texture_2d(GL_TEXTURE_2D, state->images[i]);
    }

    gr->query_buffer(display, resource, EGL_WIDTH, &state->buffer_width);
    gr->query_buffer(display, resource, EGL_HEIGHT, &state->buffer_height);
    gr->query_buffer(display, resource, EGL_WAYLAND_Y_INVERTED_WL, &state->y_inverted);

    state->buffer_type = BUFFER_TYPE_EGL;
    state->sampler = sampler;

    return PEPPER_TRUE;
}

static pepper_bool_t
gl_renderer_attach_surface(pepper_renderer_t *renderer, pepper_surface_t *surface, int *w, int *h)
{
    gl_surface_state_t *state = get_surface_state(renderer, surface);
    pepper_buffer_t    *buffer = pepper_surface_get_buffer(surface);

    if (!buffer)
    {
        *w = 0;
        *h = 0;

        surface_state_release_buffer(state);
        return PEPPER_FALSE;
    }

    surface_state_destroy_images(state);

    if (surface_state_attach_shm(state, buffer))
        goto done;

    if (surface_state_attach_egl(state, buffer))
        goto done;

    /* Assert not reached. */
    PEPPER_ASSERT(PEPPER_FALSE);
    return PEPPER_FALSE;

done:
    pepper_buffer_reference(buffer);

    /* Release previous buffer. */
    if (state->buffer)
    {
        pepper_buffer_unreference(state->buffer);
        wl_list_remove(&state->buffer_destroy_listener.link);
    }

    /* Set new buffer. */
    state->buffer = buffer;
    pepper_buffer_add_destroy_listener(buffer, &state->buffer_destroy_listener);

    /* Output buffer size info. */
    *w = state->buffer_width;
    *h = state->buffer_height;

    return PEPPER_TRUE;
}

static void
gl_renderer_flush_surface_damage(pepper_renderer_t *renderer, pepper_surface_t *surface)
{
    gl_surface_state_t *state = get_surface_state(renderer, surface);

    if (state->buffer_type != BUFFER_TYPE_SHM)
        return;

    /* TODO: Texture upload. */
}

static pepper_bool_t
gl_renderer_read_pixels(pepper_renderer_t *renderer,
                        int x, int y, int w, int h,
                        void *pixels, pepper_format_t format)
{
    gl_renderer_t  *gr = (gl_renderer_t *)renderer;
    GLenum          gl_format;
    GLenum          gl_type;

    if (!gl_renderer_use(gr))
        return PEPPER_FALSE;

    switch (format)
    {
    case PEPPER_FORMAT_ARGB8888:
        gl_format = GL_BGRA_EXT;
        gl_type = GL_UNSIGNED_BYTE;
        break;
    case PEPPER_FORMAT_ABGR8888:
        gl_format = GL_RGBA;
        gl_type = GL_UNSIGNED_BYTE;
        break;
    default:
        return PEPPER_FALSE;
    }

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, y, w, h, gl_format, gl_type, pixels);
    return PEPPER_TRUE;
}

static void
gl_renderer_repaint_output(pepper_renderer_t *renderer, pepper_output_t *output)
{
    gl_renderer_t  *gr = (gl_renderer_t *)renderer;

    if (!gl_renderer_use(gr))
        return;

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(gr->display, gr->surface);
}

static PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display = NULL;

pepper_bool_t
gl_renderer_support_platform(const char *platform)
{
    const char *extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    char        str[64];

    if (!extensions)
    {
        PEPPER_ERROR("Failed to get EGL extension string.\n");
        return PEPPER_FALSE;
    }

    if (!strstr(extensions, "EGL_EXT_platform_base"))
        return PEPPER_TRUE;

    snprintf(str, sizeof(str), "EGL_KHR_platform_%s", platform);
    if (strstr(extensions, str))
        return PEPPER_TRUE;

    snprintf(str, sizeof(str), "EGL_EXT_platform_%s", platform);
    if (strstr(extensions, str))
        return PEPPER_TRUE;

    snprintf(str, sizeof(str), "EGL_MESA_platform_%s", platform);
    if (strstr(extensions, str))
        return PEPPER_TRUE;

    return PEPPER_FALSE;
}

static pepper_bool_t
setup_egl_extensions(gl_renderer_t *gr)
{
    const char *extensions = eglQueryString(gr->display, EGL_EXTENSIONS);

    if (!extensions)
    {
        PEPPER_ERROR("Failed to get EGL extension string.\n");
        return PEPPER_FALSE;
    }

    if (strstr(extensions, "EGL_KHR_image"))
    {
        gr->create_image  = (void *)eglGetProcAddress("eglCreateImageKHR");
        gr->destroy_image = (void *)eglGetProcAddress("eglDestroyImageKHR");
    }
    else
    {
        PEPPER_ERROR("EGL_KHR_image not supported.\n");
        return PEPPER_FALSE;
    }

#ifdef EGL_EXT_swap_buffers_with_damage
    if (strstr(extensions, "EGL_EXT_swap_buffers_with_damage"))
    {
        gr->swap_buffers_with_damage =
            (void *)eglGetProcAddress("eglSwapBuffersWithDamageEXT");
    }
    else
    {
        PEPPER_ERROR("Performance Warning: EGL_EXT_swap_buffers_with_damage not supported.\n");
    }
#endif

    if (strstr(extensions, "EGL_WL_bind_wayland_display"))
    {
        gr->bind_display      = (void *)eglGetProcAddress("eglBindWaylandDisplayWL");
        gr->unbind_display    = (void *)eglGetProcAddress("eglUnbindWaylandDisplayWL");
        gr->query_buffer      = (void *)eglGetProcAddress("eglQueryWaylandBufferWL");

        if (!gr->bind_display(gr->display,
                                    pepper_compositor_get_display(gr->base.compositor)))
        {
            gr->bind_display = NULL;
            gr->unbind_display = NULL;
            gr->query_buffer = NULL;
        }
    }

    if (strstr(extensions, "EGL_EXT_buffer_age"))
        gr->has_buffer_age = PEPPER_TRUE;
    else
        PEPPER_ERROR("Performance Warning: EGL_EXT_buffer_age not supported.\n");

    if (strstr(extensions, "EGL_EXT_platform_base"))
    {
        gr->create_platform_window_surface =
            (void *)eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT");
    }
    else
    {
        PEPPER_ERROR("Warning: EGL_EXT_platform_base not supported.\n");
    }

    return PEPPER_TRUE;
}

static pepper_bool_t
setup_gl_extensions(gl_renderer_t *gr)
{
    const char *extensions = (const char *)glGetString(GL_EXTENSIONS);

    if (!extensions)
    {
        PEPPER_ERROR("Failed to get GL extension string.\n");
        return PEPPER_FALSE;
    }

    gr->image_target_texture_2d = (void *)eglGetProcAddress("glEGLImageTargetTexture2DOES");

    if (!gr->image_target_texture_2d)
    {
        PEPPER_ERROR("glEGLImageTargetTexture2DOES not supported.\n");
        return PEPPER_FALSE;
    }

    if (!strstr(extensions, "GL_EXT_texture_format_BGRA8888"))
    {
        PEPPER_ERROR("GL_EXT_texture_format_BGRA8888 not supported.\n");
        return PEPPER_FALSE;
    }

    if (strstr(extensions, "GL_EXT_read_format_bgra"))
        gr->has_read_format_bgra = PEPPER_TRUE;

    if (strstr(extensions, "GL_EXT_unpack_subimage"))
        gr->has_unpack_subimage = PEPPER_TRUE;

    return PEPPER_TRUE;
}

static pepper_bool_t
init_egl(gl_renderer_t *gr, void *dpy, void *win, EGLenum platform,
         pepper_format_t format, const void *visual_id)
{
    EGLDisplay      display = EGL_NO_DISPLAY;
    EGLSurface      surface = EGL_NO_SURFACE;
    EGLContext      context = EGL_NO_CONTEXT;
    EGLConfig       config = NULL;
    EGLint          config_size = 0;
    EGLint          num_configs = 0;
    EGLint          major, minor;
    EGLConfig      *configs = NULL;
    int             i;

    EGLint context_attribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLint config_attribs[] =
    {
        EGL_SURFACE_TYPE,       EGL_WINDOW_BIT,
        EGL_RED_SIZE,           0,
        EGL_GREEN_SIZE,         0,
        EGL_BLUE_SIZE,          0,
        EGL_ALPHA_SIZE,         0,
        EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    config_attribs[3] = PEPPER_FORMAT_R(format);
    config_attribs[5] = PEPPER_FORMAT_G(format);
    config_attribs[7] = PEPPER_FORMAT_B(format);
    config_attribs[9] = PEPPER_FORMAT_A(format);

    if (get_platform_display)
        display = get_platform_display(platform, (EGLNativeDisplayType)dpy, NULL);
    else
        display = eglGetDisplay((EGLNativeDisplayType)dpy);

    if (display == EGL_NO_DISPLAY)
    {
        PEPPER_ERROR("eglGetDisplay(%p) failed.\n", display);
        goto error;
    }

    if (!eglInitialize(display, &major, &minor))
    {
        PEPPER_ERROR("eglInitialize() failed.\n");
        goto error;
    }

    if (!eglBindAPI(EGL_OPENGL_ES_API))
    {
        PEPPER_ERROR("eglBindAPI() failed.\n");
        return PEPPER_FALSE;
    }

    if (!eglChooseConfig(display, config_attribs, NULL, 0, &config_size))
    {
        PEPPER_ERROR("eglChooseConfig() failed.\n");
        goto error;
    }

    if (config_size < 1)
    {
        PEPPER_ERROR("eglChooseConfig() returned no config.\n");
        goto error;
    }

    if ((configs = (EGLConfig *)calloc(config_size, sizeof(EGLConfig))) == NULL)
        goto error;

    eglChooseConfig(display, config_attribs, configs, config_size, &num_configs);
    PEPPER_ASSERT(config_size == num_configs); /* Paranoid check. */

    if (num_configs < 1)
        goto error;

    for (i = 0; i < num_configs; i++)
    {
        EGLint attrib;

        if (visual_id)
        {
            /* Native visual id have privilege. */
            if (eglGetConfigAttrib(display, configs[i], EGL_NATIVE_VISUAL_ID, &attrib))
            {
                if (attrib == *((EGLint *)visual_id))
                {
                    config = configs[i];
                    break;
                }
            }

            continue;
        }

        if (eglGetConfigAttrib(display, configs[i], EGL_BUFFER_SIZE, &attrib))
        {
            if (attrib == PEPPER_FORMAT_BPP(format))
            {
                config = configs[i];
                break;
            }
        }
    }

    free(configs);
    configs = NULL;

    if (!config)
    {
        PEPPER_ERROR("No matched config.\n");
        goto error;
    }

    surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)win, NULL);
    if (surface == EGL_NO_SURFACE)
    {
        PEPPER_ERROR("eglCreateWindowSurface() failed.\n");
        goto error;
    }

    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribs);
    if (context == EGL_NO_CONTEXT)
    {
        PEPPER_ERROR("eglCreateContext() failed.\n");
        goto error;
    }

    if (!eglMakeCurrent(display, surface, surface, context))
    {
        PEPPER_ERROR("eglMakeCurrent() failed.\n");
        goto error;
    }

    gr->display = display;
    gr->surface = surface;
    gr->context = context;
    gr->config  = config;

    if (!setup_egl_extensions(gr))
        goto error;

    if (!setup_gl_extensions(gr))
        goto error;

    return PEPPER_TRUE;

error:
    if (context)
        eglDestroyContext(display, context);

    if (surface)
        eglDestroySurface(display, surface);

    if (display)
        eglTerminate(display);

    if (configs)
        free(configs);

    return PEPPER_FALSE;
}

static EGLenum
platform_string_to_egl(const char *str)
{
    if (!strcmp(str, "gbm"))
        return EGL_PLATFORM_GBM_KHR;

    if (!strcmp(str, "wayland"))
        return EGL_PLATFORM_WAYLAND_KHR;

    if (!strcmp(str, "x11"))
        return EGL_PLATFORM_X11_KHR;

    return EGL_NONE;
}

PEPPER_API pepper_renderer_t *
pepper_gl_renderer_create(pepper_compositor_t *compositor, void *display, void *window,
                          const char *platform_str, pepper_format_t format,
                          const void *visual_id)
{
    gl_renderer_t  *gr;
    EGLenum         platform;

    if (!gl_renderer_support_platform(platform_str))
    {
        PEPPER_ERROR("Unsupported platform %s.\n", platform_str);
        return NULL;
    }

    platform = platform_string_to_egl(platform_str);
    if (platform == EGL_NONE)
        return NULL;

    if (!get_platform_display)
        get_platform_display = (void *)eglGetProcAddress("eglGetPlatformDisplayEXT");

    gr = (gl_renderer_t *)calloc(1, sizeof(gl_renderer_t));
    if (!gr)
        return NULL;

    gr->base.compositor = compositor;
    gr->native_display = display;
    gr->native_window  = window;

    if (!init_egl(gr, display, window, platform, format, visual_id))
        goto error;

    gr->base.destroy                =   gl_renderer_destroy;
    gr->base.attach_surface         =   gl_renderer_attach_surface;
    gr->base.flush_surface_damage   =   gl_renderer_flush_surface_damage;
    gr->base.read_pixels            =   gl_renderer_read_pixels;
    gr->base.repaint_output         =   gl_renderer_repaint_output;

    return &gr->base;

error:
    if (gr)
        gl_renderer_destroy(&gr->base);

    return NULL;
}
