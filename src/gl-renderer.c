#include "pepper.h"
#include "common.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

typedef struct gl_renderer  gl_renderer_t;

struct gl_renderer
{
    pepper_renderer_t   base;

    EGLDisplay          display;
    EGLSurface          surface;
    EGLContext          context;
    EGLConfig           config;
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
gl_renderer_read_pixels(pepper_renderer_t *r, int x, int y, int w, int h,
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

static pepper_bool_t
gl_renderer_set_render_target(pepper_renderer_t *r, void *target)
{
    /* Can't change gl renderer's render target. */
    return PEPPER_FALSE;
}

static void
gl_renderer_draw(pepper_renderer_t *r, void *data)
{
    gl_renderer_t  *renderer = (gl_renderer_t *)r;

    if (!gl_renderer_use(renderer))
        return;

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(renderer->display, renderer->surface);
}

static pepper_bool_t
init_egl(gl_renderer_t *renderer, void *dpy, void *win,
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

    if ((display = eglGetDisplay((EGLNativeDisplayType)dpy)) == EGL_NO_DISPLAY)
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

PEPPER_API pepper_renderer_t *
pepper_gl_renderer_create(void *display, void *window,
                          pepper_format_t format, const uint32_t *native_visual_id)
{
    gl_renderer_t  *renderer;

    renderer = (gl_renderer_t *)pepper_calloc(1, sizeof(gl_renderer_t));
    if (!renderer)
        return NULL;

    pepper_renderer_init(&renderer->base);

    if (!init_egl(renderer, display, window, format, native_visual_id))
        goto error;

    renderer->base.destroy              =   gl_renderer_destroy;
    renderer->base.read_pixels          =   gl_renderer_read_pixels;
    renderer->base.set_render_target    =   gl_renderer_set_render_target;
    renderer->base.draw                 =   gl_renderer_draw;

    return &renderer->base;

error:
    if (renderer)
        gl_renderer_destroy(&renderer->base);

    return NULL;
}
