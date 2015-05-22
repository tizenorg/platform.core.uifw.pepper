#include "pepper-gl-renderer.h"
#include "common.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "eglextwayland.h"
#include <string.h>

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
    pepper_renderer_t   base;

    void               *native_display;
    void               *native_window;

    EGLDisplay          display;
    EGLSurface          surface;
    EGLContext          context;
    EGLConfig           config;

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
gl_renderer_use(gl_renderer_t *renderer)
{
    if (!eglMakeCurrent(renderer->display, renderer->surface, renderer->surface, renderer->context))
        return PEPPER_FALSE;

    return PEPPER_TRUE;
}

static void
gl_renderer_destroy(pepper_renderer_t *r)
{
    gl_renderer_t *renderer = (gl_renderer_t *)r;

    if (renderer->context != EGL_NO_CONTEXT)
        eglDestroyContext(renderer->display, renderer->context);

    if (renderer->surface != EGL_NO_SURFACE)
        eglDestroySurface(renderer->display, renderer->surface);

    if (renderer->display != EGL_NO_DISPLAY)
        eglTerminate(renderer->display);

    pepper_free(renderer);
}

static pepper_bool_t
gl_renderer_read_pixels(pepper_renderer_t *r, void *target,
                        int x, int y, int w, int h,
                        void *pixels, pepper_format_t format)
{
    gl_renderer_t  *renderer = (gl_renderer_t *)r;
    GLenum          gl_format;
    GLenum          gl_type;

    if (!gl_renderer_use(renderer))
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
    gl_surface_state_t *state = wl_container_of(listener, state, surface_destroy_listener);

    surface_state_release_buffer(state);
    wl_list_remove(&state->surface_destroy_listener.link);
    pepper_surface_set_user_data(state->surface, state->renderer, NULL, NULL);
    pepper_free(state);
}

static void
surface_state_handle_buffer_destroy(struct wl_listener *listener, void *data)
{
    gl_surface_state_t *state = wl_container_of(listener, state, buffer_destroy_listener);

    surface_state_release_buffer(state);
}

static gl_surface_state_t *
get_surface_state(pepper_renderer_t *renderer, pepper_surface_t *surface)
{
    gl_surface_state_t *state = pepper_surface_get_user_data(surface, renderer);

    if (!state)
    {
        state = pepper_calloc(1, sizeof(gl_surface_state_t));
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
    gl_renderer_t      *renderer = state->renderer;
    EGLDisplay          display = renderer->display;
    struct wl_resource *resource = pepper_buffer_get_resource(buffer);
    int                 num_planes;
    int                 sampler;
    EGLint              attribs[3];
    int                 texture_format;
    int                 i;

    if (!renderer->query_buffer(display, resource, EGL_TEXTURE_FORMAT, &texture_format))
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
        state->images[i] = renderer->create_image(display, NULL, EGL_WAYLAND_BUFFER_WL,
                                                  resource, attribs);

        PEPPER_ASSERT(state->images[i] != EGL_NO_IMAGE_KHR);

        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, state->textures[i]);
        renderer->image_target_texture_2d(GL_TEXTURE_2D, state->images[i]);
    }

    renderer->query_buffer(display, resource, EGL_WIDTH, &state->buffer_width);
    renderer->query_buffer(display, resource, EGL_HEIGHT, &state->buffer_height);
    renderer->query_buffer(display, resource, EGL_WAYLAND_Y_INVERTED_WL, &state->y_inverted);

    state->buffer_type = BUFFER_TYPE_EGL;
    state->sampler = sampler;

    return PEPPER_TRUE;
}

static void
gl_renderer_attach_surface(pepper_renderer_t *renderer, pepper_surface_t *surface, int *w, int *h)
{
    gl_surface_state_t *state = get_surface_state(renderer, surface);
    pepper_buffer_t    *buffer = pepper_surface_get_buffer(surface);

    if (!buffer)
    {
        surface_state_release_buffer(state);
        return;
    }

    surface_state_destroy_images(state);

    if (surface_state_attach_shm(state, buffer))
        goto done;

    if (surface_state_attach_egl(state, buffer))
        goto done;

    /* Assert not reached. */
    PEPPER_ASSERT(PEPPER_FALSE);
    return;

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
}

static void
gl_renderer_flush_surface_damage(pepper_renderer_t *renderer, pepper_surface_t *surface)
{
    gl_surface_state_t *state = get_surface_state(renderer, surface);

    if (state->buffer_type != BUFFER_TYPE_SHM)
        return;

    /* TODO: Texture upload. */
}

static void
gl_renderer_draw(pepper_renderer_t *r, void *target, void *data)
{
    gl_renderer_t  *renderer = (gl_renderer_t *)r;

    if (!gl_renderer_use(renderer))
        return;

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(renderer->display, renderer->surface);
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
setup_egl_extensions(gl_renderer_t *renderer)
{
    const char *extensions = eglQueryString(renderer->display, EGL_EXTENSIONS);

    if (!extensions)
    {
        PEPPER_ERROR("Failed to get EGL extension string.\n");
        return PEPPER_FALSE;
    }

    if (strstr(extensions, "EGL_KHR_image"))
    {
        renderer->create_image  = (void *)eglGetProcAddress("eglCreateImageKHR");
        renderer->destroy_image = (void *)eglGetProcAddress("eglDestroyImageKHR");
    }
    else
    {
        PEPPER_ERROR("EGL_KHR_image not supported.\n");
        return PEPPER_FALSE;
    }

#ifdef EGL_EXT_swap_buffers_with_damage
    if (strstr(extensions, "EGL_EXT_swap_buffers_with_damage"))
    {
        renderer->swap_buffers_with_damage =
            (void *)eglGetProcAddress("eglSwapBuffersWithDamageEXT");
    }
    else
    {
        PEPPER_ERROR("Performance Warning: EGL_EXT_swap_buffers_with_damage not supported.\n");
    }
#endif

    if (strstr(extensions, "EGL_WL_bind_wayland_display"))
    {
        renderer->bind_display      = (void *)eglGetProcAddress("eglBindWaylandDisplayWL");
        renderer->unbind_display    = (void *)eglGetProcAddress("eglUnbindWaylandDisplayWL");
        renderer->query_buffer      = (void *)eglGetProcAddress("eglQueryWaylandBufferWL");

        if (!renderer->bind_display(renderer->display, renderer->native_display))
        {
            renderer->bind_display = NULL;
            renderer->unbind_display = NULL;
            renderer->query_buffer = NULL;
        }
    }

    if (strstr(extensions, "EGL_EXT_buffer_age"))
        renderer->has_buffer_age = PEPPER_TRUE;
    else
        PEPPER_ERROR("Performance Warning: EGL_EXT_buffer_age not supported.\n");

    if (strstr(extensions, "EGL_EXT_platform_base"))
    {
        renderer->create_platform_window_surface =
            (void *)eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT");
    }
    else
    {
        PEPPER_ERROR("Warning: EGL_EXT_platform_base not supported.\n");
    }

    return PEPPER_TRUE;
}

static pepper_bool_t
setup_gl_extensions(gl_renderer_t *renderer)
{
    const char *extensions = (const char *)glGetString(GL_EXTENSIONS);

    if (!extensions)
    {
        PEPPER_ERROR("Failed to get GL extension string.\n");
        return PEPPER_FALSE;
    }

    renderer->image_target_texture_2d = (void *)eglGetProcAddress("glEGLImageTargetTexture2DOES");

    if (!renderer->image_target_texture_2d)
    {
        PEPPER_ERROR("glEGLImageTargetTexture2DOES not supported.\n");
        return PEPPER_FALSE;
    }

    if (strstr(extensions, "GL_EXT_texture_format_BGRA8888"))
    {
        PEPPER_ERROR("GL_EXT_texture_format_BGRA8888 not supported.\n");
        return PEPPER_FALSE;
    }

    if (strstr(extensions, "GL_EXT_read_format_bgra"))
        renderer->has_read_format_bgra = PEPPER_TRUE;

    if (strstr(extensions, "GL_EXT_unpack_subimage"))
        renderer->has_unpack_subimage = PEPPER_TRUE;

    return PEPPER_TRUE;
}

static pepper_bool_t
init_egl(gl_renderer_t *renderer, void *dpy, void *win, EGLenum platform,
         pepper_format_t format, const uint32_t *native_visual_id)
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

    if ((configs = (EGLConfig *)pepper_calloc(config_size, sizeof(EGLConfig))) == NULL)
        goto error;

    if (num_configs < 1)
        goto error;

    eglChooseConfig(display, config_attribs, configs, config_size, &num_configs);
    PEPPER_ASSERT(config_size == num_configs); /* Paranoid check. */

    for (i = 0; i < num_configs; i++)
    {
        EGLint attrib;

        if (native_visual_id)
        {
            /* Native visual id have privilege. */
            if (eglGetConfigAttrib(display, configs[i], EGL_NATIVE_VISUAL_ID, &attrib))
            {
                if (attrib == (EGLint)(*native_visual_id))
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

    pepper_free(configs);
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

    renderer->display = display;
    renderer->surface = surface;
    renderer->context = context;
    renderer->config  = config;

    if (!setup_egl_extensions(renderer))
        goto error;

    if (!setup_gl_extensions(renderer))
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
        pepper_free(configs);

    return PEPPER_FALSE;
}

static EGLenum
platform_string_to_egl(const char *str)
{
    if (strcmp(str, "gbm"))
        return EGL_PLATFORM_GBM_KHR;

    if (strcmp(str, "wayland"))
        return EGL_PLATFORM_WAYLAND_KHR;

    if (strcmp(str, "x11"))
        return EGL_PLATFORM_X11_KHR;

    return EGL_NONE;
}

PEPPER_API pepper_renderer_t *
pepper_gl_renderer_create(void *display, void *window, const char *platform_str,
                          pepper_format_t format, const uint32_t *native_visual_id)
{
    gl_renderer_t  *renderer;
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

    renderer = (gl_renderer_t *)pepper_calloc(1, sizeof(gl_renderer_t));
    if (!renderer)
        return NULL;

    pepper_renderer_init(&renderer->base);

    renderer->native_display = display;
    renderer->native_window  = window;

    if (!init_egl(renderer, display, window, platform, format, native_visual_id))
        goto error;

    renderer->base.destroy              =   gl_renderer_destroy;
    renderer->base.read_pixels          =   gl_renderer_read_pixels;
    renderer->base.attach_surface       =   gl_renderer_attach_surface;
    renderer->base.flush_surface_damage =   gl_renderer_flush_surface_damage;
    renderer->base.draw                 =   gl_renderer_draw;

    return &renderer->base;

error:
    if (renderer)
        gl_renderer_destroy(&renderer->base);

    return NULL;
}
