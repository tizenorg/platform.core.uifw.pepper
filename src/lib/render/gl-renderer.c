#include "pepper-gl-renderer.h"
#include "pepper-render-internal.h"
#include <pepper-output-backend.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "eglextwayland.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct gl_renderer      gl_renderer_t;
typedef struct gl_shader        gl_shader_t;
typedef struct gl_surface_state gl_surface_state_t;
typedef struct gl_render_target gl_render_target_t;

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

#define NUM_MAX_PLANES  3

static const char vertex_shader[] =
    "uniform mat4   trans;\n"
    "attribute vec2 position;\n"
    "attribute vec2 texcoord;\n"
    "varying vec2   v_texcoord;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = trans * vec4(position, 0.0, 1.0);\n"
    "   v_texcoord = texcoord;\n"
    "}\n";

static const char fragment_shader_rgba[] =
    "precision mediump  float;\n"
    "varying vec2       v_texcoord;\n"
    "uniform sampler2D  tex;\n"
    "uniform float      alpha;\n"
    "void main()\n"
    "{\n"
    "   gl_FragColor = alpha * texture2D(tex, v_texcoord);\n"
    "}\n";

static const char fragment_shader_rgbx[] =
    "precision mediump float;\n"
    "varying vec2       v_texcoord;\n"
    "uniform sampler2D  tex;\n"
    "uniform float      alpha;\n"
    "void main()\n"
    "{\n"
    "   gl_FragColor.rgb = alpha * texture2D(tex, v_texcoord).rgb;\n"
    "   gl_FragColor.a = alpha;\n"
    "}\n";

static const char fragment_shader_y_uv[] =
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "uniform sampler2D tex1;\n"
    "varying vec2 v_texcoord;\n"
    "uniform float alpha;\n"
    "void main()\n"
    "{\n"
    "   float y = 1.16438356 * (texture2D(tex, v_texcoord).x - 0.0625);\n"
    "   float u = texture2D(tex1, v_texcoord).r - 0.5;\n"
    "   float v = texture2D(tex1, v_texcoord).g - 0.5;\n"
    "   y *= alpha;\n"
    "   u *= alpha;\n"
    "   v *= alpha;\n"
    "   gl_FragColor.r = y + 1.59602678 * v;\n"
    "   gl_FragColor.g = y - 0.39176229 * u - 0.81296764 * v;\n"
    "   gl_FragColor.b = y + 2.01723214 * u;\n"
    "   gl_FragColor.a = alpha;\n"
    "}\n";

static const char fragment_shader_y_xuxv[] =
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "uniform sampler2D tex1;\n"
    "varying vec2 v_texcoord;\n"
    "uniform float alpha;\n"
    "void main()\n"
    "{\n"
    "   float y = 1.16438356 * (texture2D(tex, v_texcoord).x - 0.0625);\n"
    "   float u = texture2D(tex1, v_texcoord).g - 0.5;\n"
    "   float v = texture2D(tex1, v_texcoord).a - 0.5;\n"
    "   y *= alpha;\n"
    "   u *= alpha;\n"
    "   v *= alpha;\n"
    "   gl_FragColor.r = y + 1.59602678 * v;\n"
    "   gl_FragColor.g = y - 0.39176229 * u - 0.81296764 * v;\n"
    "   gl_FragColor.b = y + 2.01723214 * u;\n"
    "   gl_FragColor.a = alpha;\n"
    "}\n";

static const char fragment_shader_y_u_v[] =
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "uniform sampler2D tex1;\n"
    "uniform sampler2D tex2;\n"
    "varying vec2 v_texcoord;\n"
    "uniform float alpha;\n"
    "void main()"
    "{\n"
    "   float y = 1.16438356 * (texture2D(tex, v_texcoord).x - 0.0625);\n"
    "   float u = texture2D(tex1, v_texcoord).x - 0.5;\n"
    "   float v = texture2D(tex2, v_texcoord).x - 0.5;\n"
    "   y *= alpha;\n"
    "   u *= alpha;\n"
    "   v *= alpha;\n"
    "   gl_FragColor.r = y + 1.59602678 * v;\n"
    "   gl_FragColor.g = y - 0.39176229 * u - 0.81296764 * v;\n"
    "   gl_FragColor.b = y + 2.01723214 * u;\n"
    "   gl_FragColor.a = alpha;\n"
    "}\n";

static const char *vertex_shaders[] =
{
    vertex_shader,
    vertex_shader,
    vertex_shader,
    vertex_shader,
    vertex_shader,
};

static const char *fragment_shaders[] =
{
    fragment_shader_rgba,
    fragment_shader_rgbx,
    fragment_shader_y_uv,
    fragment_shader_y_xuxv,
    fragment_shader_y_u_v,
};

enum shader_sampler
{
    GL_SHADER_SAMPLER_RGBA,
    GL_SHADER_SAMPLER_RGBX,
    GL_SHADER_SAMPLER_Y_UV,
    GL_SHADER_SAMPLER_Y_XUXV,
    GL_SHADER_SAMPLER_Y_U_V,
    GL_SHADER_SAMPLER_NONE,
};

struct gl_shader
{
    GLuint      program;
    GLuint      vertex_shader;
    GLuint      fragment_shader;
    GLint       texture_uniform[3];
    GLint       alpha_uniform;
    GLint       trans_uniform;
    const char *vertex_shader_source;
    const char *fragment_shader_source;
};

enum buffer_type
{
    BUFFER_TYPE_NONE,
    BUFFER_TYPE_SHM,
    BUFFER_TYPE_EGL,
};

#define MAX_BUFFER_COUNT    3

struct gl_render_target
{
    pepper_render_target_t      base;

    EGLSurface                  surface;
    EGLConfig                   config;

    void                       *native_window;

    int32_t                     width;
    int32_t                     height;

    pepper_mat4_t               proj_mat;

    pixman_region32_t           damages[MAX_BUFFER_COUNT];
    int32_t                     damage_index;
};

struct gl_renderer
{
    pepper_renderer_t       base;

    void                   *native_display;

    EGLDisplay              display;
    EGLContext              context;

    gl_shader_t             shaders[GL_SHADER_SAMPLER_NONE];

    /* EGL_EXT_platform. */
    PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC create_platform_window_surface;

    /* EGL extensions. */
    PFNEGLCREATEIMAGEKHRPROC        create_image;
    PFNEGLDESTROYIMAGEKHRPROC       destroy_image;

#ifdef EGL_EXT_swap_buffers_with_damage
    PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC  swap_buffers_with_damage;
#endif

    PFNEGLBINDWAYLANDDISPLAYWL      bind_display;
    PFNEGLUNBINDWAYLANDDISPLAYWL    unbind_display;
    PFNEGLQUERYWAYLANDBUFFERWL      query_buffer;

    pepper_bool_t   has_buffer_age;

    /* GL extensions. */
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC image_target_texture_2d;

    pepper_bool_t   has_read_format_bgra;
    pepper_bool_t   has_unpack_subimage;

    gl_shader_t        *current_shader;

    pepper_bool_t       clear_background;
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

    pepper_event_listener_t *buffer_destroy_listener;
    pepper_event_listener_t *surface_destroy_listener;
};

static pepper_bool_t
init_gl_shader(gl_renderer_t *gr, gl_shader_t *shader, const char *vs, const char *fs)
{
    GLint status;
    char msg[512];

    shader->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(shader->vertex_shader, 1, &vs, NULL);
    glCompileShader(shader->vertex_shader);
    glGetShaderiv(shader->vertex_shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        glGetShaderInfoLog(shader->vertex_shader, sizeof(msg), NULL, msg);
        PEPPER_ERROR("Failed to compile vertex shader: %s\n", msg);
        return PEPPER_FALSE;
    }

    shader->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shader->fragment_shader, 1, &fs, NULL);
    glCompileShader(shader->fragment_shader);
    glGetShaderiv(shader->fragment_shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        glGetShaderInfoLog(shader->fragment_shader, sizeof(msg), NULL, msg);
        PEPPER_ERROR("Failed to compile fragment shader: %s\n", msg);
        return PEPPER_FALSE;
    }

    shader->program = glCreateProgram();
    glAttachShader(shader->program, shader->vertex_shader);
    glAttachShader(shader->program, shader->fragment_shader);

    glBindAttribLocation(shader->program, 0, "position");
    glBindAttribLocation(shader->program, 1, "texcoord");

    glLinkProgram(shader->program);
    glGetProgramiv(shader->program, GL_LINK_STATUS, &status);
    if (!status)
    {
        glGetProgramInfoLog(shader->program, sizeof(msg), NULL, msg);
        PEPPER_ERROR("Failed to link program: %s\n", msg);
        return PEPPER_FALSE;
    }

    shader->texture_uniform[0] = glGetUniformLocation(shader->program, "tex");
    shader->texture_uniform[1] = glGetUniformLocation(shader->program, "tex1");
    shader->texture_uniform[2] = glGetUniformLocation(shader->program, "tex2");
    shader->alpha_uniform = glGetUniformLocation(shader->program, "alpha");
    shader->trans_uniform = glGetUniformLocation(shader->program, "trans");
    shader->vertex_shader_source = vs;
    shader->fragment_shader_source = fs;

    return PEPPER_TRUE;
}

static void
fini_gl_shaders(gl_renderer_t *gr)
{
    int i;

    for (i = 0; i < GL_SHADER_SAMPLER_NONE; i++)
    {
        gl_shader_t *shader = &gr->shaders[i];
        glDeleteShader(shader->vertex_shader);
        glDeleteShader(shader->fragment_shader);
        glDeleteProgram(shader->program);
        memset(shader, 0, sizeof(gl_shader_t));
    }
}

static pepper_bool_t
init_gl_shaders(gl_renderer_t *gr)
{
    int i;

    for (i = 0; i < GL_SHADER_SAMPLER_NONE; i++)
    {
        if (!init_gl_shader(gr, &gr->shaders[i], vertex_shaders[i], fragment_shaders[i]))
        {
            fini_gl_shaders(gr);
            return PEPPER_FALSE;
        }
    }

    return PEPPER_TRUE;
}

static pepper_bool_t
gl_renderer_use(gl_renderer_t *gr)
{
    gl_render_target_t *gt = (gl_render_target_t *)gr->base.target;

    if (!eglMakeCurrent(gr->display, gt->surface, gt->surface, gr->context))
        return PEPPER_FALSE;

    return PEPPER_TRUE;
}

static void
gl_renderer_destroy(pepper_renderer_t *renderer)
{
    gl_renderer_t *gr = (gl_renderer_t *)renderer;

    fini_gl_shaders(gr);

    if (gr->unbind_display)
        gr->unbind_display(gr->display, pepper_compositor_get_display(gr->base.compositor));

    if (gr->display != EGL_NO_DISPLAY)
        eglMakeCurrent(gr->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    if (gr->context != EGL_NO_CONTEXT)
        eglDestroyContext(gr->display, gr->context);

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

        pepper_event_listener_remove(state->buffer_destroy_listener);
    }
}

static void
surface_state_handle_surface_destroy(pepper_event_listener_t    *listener,
                                    pepper_object_t            *object,
                                    uint32_t                    id,
                                    void                       *info,
                                    void                       *data)
{
    gl_surface_state_t *state = data;
    surface_state_release_buffer(state);
    pepper_event_listener_remove(state->surface_destroy_listener);
    pepper_object_set_user_data((pepper_object_t *)state->surface, state->renderer, NULL, NULL);
    free(state);
}

static void
surface_state_handle_buffer_destroy(pepper_event_listener_t    *listener,
                                    pepper_object_t            *object,
                                    uint32_t                    id,
                                    void                       *info,
                                    void                       *data)
{
    gl_surface_state_t *state = data;
    surface_state_release_buffer(state);
}

static gl_surface_state_t *
get_surface_state(pepper_renderer_t *renderer, pepper_surface_t *surface)
{
    gl_surface_state_t *state = pepper_object_get_user_data((pepper_object_t *)surface, renderer);

    if (!state)
    {
        state = (gl_surface_state_t *)calloc(1, sizeof(gl_surface_state_t));
        if (!state)
            return NULL;

        state->renderer = (gl_renderer_t *)renderer;
        state->surface = surface;
        state->surface_destroy_listener =
            pepper_object_add_event_listener((pepper_object_t *)surface,
                                             PEPPER_EVENT_OBJECT_DESTROY, 0,
                                             surface_state_handle_surface_destroy, state);

        pepper_object_set_user_data((pepper_object_t *)surface, renderer, state, NULL);
    }

    return state;
}

static void
surface_state_destroy_images(gl_surface_state_t *state)
{
    int i;

    for (i = 0; i < state->num_planes; i++)
    {
        if (state->images[i] != EGL_NO_IMAGE_KHR)
        {
            state->renderer->destroy_image(state->renderer->display, state->images[i]);
            state->images[i] = EGL_NO_IMAGE_KHR;
        }
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
        /* Don't use glTexSubImage2D() for shm buffers in this case. */
        state->shm.need_full_upload = PEPPER_TRUE;
    }

    state->buffer_type      = BUFFER_TYPE_SHM;
    state->buffer_width     = w;
    state->buffer_height    = h;

    /* SHM buffer's origin is upper-left. */
    state->y_inverted       = 1;

    state->shm.buffer       = shm_buffer;
    state->shm.format       = format;
    state->shm.pixel_format = pixel_format;
    state->shm.pitch        = pitch;

    surface_state_ensure_textures(state, 1);

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
        pepper_event_listener_remove(state->buffer_destroy_listener);
    }

    /* Set new buffer. */
    state->buffer = buffer;
    state->buffer_destroy_listener =
        pepper_object_add_event_listener((pepper_object_t *)buffer, PEPPER_EVENT_OBJECT_DESTROY, 0,
                                         surface_state_handle_buffer_destroy, state);

    /* Output buffer size info. */
    *w = state->buffer_width;
    *h = state->buffer_height;

    return PEPPER_TRUE;
}

static pepper_bool_t
gl_renderer_flush_surface_damage(pepper_renderer_t *renderer, pepper_surface_t *surface)
{
    gl_renderer_t      *gr = (gl_renderer_t *)renderer;
    gl_surface_state_t *state = get_surface_state(renderer, surface);

    if (!state->buffer)
        goto done;

    if (state->buffer_type != BUFFER_TYPE_SHM)
        goto done;

    glBindTexture(GL_TEXTURE_2D, state->textures[0]);

    if (!gr->has_unpack_subimage)
    {
        wl_shm_buffer_begin_access(state->shm.buffer);
        glTexImage2D(GL_TEXTURE_2D, 0, state->shm.format,
                     state->buffer_width, state->buffer_height, 0,
                     state->shm.format, state->shm.pixel_format,
                     wl_shm_buffer_get_data(state->shm.buffer));
        wl_shm_buffer_end_access(state->shm.buffer);
        goto done;
    }

    /* Texture upload. */
    if (state->shm.need_full_upload)
    {
        glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, state->shm.pitch);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);

        wl_shm_buffer_begin_access(state->shm.buffer);
        glTexImage2D(GL_TEXTURE_2D, 0, state->shm.format,
                     state->buffer_width, state->buffer_height, 0,
                     state->shm.format, state->shm.pixel_format,
                     wl_shm_buffer_get_data(state->shm.buffer));
        wl_shm_buffer_end_access(state->shm.buffer);
        state->shm.need_full_upload = PEPPER_FALSE;
    }
    else
    {
        int                 i, nrects;
        pixman_box32_t     *rects;
        pixman_region32_t  *damage;

        damage = pepper_surface_get_damage_region(surface);
        rects = pixman_region32_rectangles(damage, &nrects);

        glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, state->shm.pitch);
        wl_shm_buffer_begin_access(state->shm.buffer);
        for (i = 0; i < nrects; i++)
        {
            glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, rects[i].x1);
            glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, rects[i].y1);
            glTexSubImage2D(GL_TEXTURE_2D, 0, rects[i].x1, rects[i].y1,
                            rects[i].x2 - rects[i].x1, rects[i].y2 - rects[i].y1,
                            state->shm.format, state->shm.pixel_format,
                            wl_shm_buffer_get_data(state->shm.buffer));
        }
        wl_shm_buffer_end_access(state->shm.buffer);
    }

done:
    pepper_buffer_unreference(state->buffer);
    pepper_event_listener_remove(state->buffer_destroy_listener);
    state->buffer = NULL;

    return PEPPER_TRUE;
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
gl_shader_use(gl_renderer_t *gr, gl_shader_t *shader)
{
    if (shader != gr->current_shader)
    {
        glUseProgram(shader->program);
        gr->current_shader = shader;
    }
}

static void
repaint_view(pepper_renderer_t *renderer, pepper_output_t *output,
             pepper_render_item_t *node, pixman_region32_t *damage)
{
    gl_renderer_t      *gr = (gl_renderer_t *)renderer;
    gl_render_target_t *gt = (gl_render_target_t *)renderer->target;

    pepper_surface_t   *surface = pepper_view_get_surface(node->view);
    gl_surface_state_t *state = get_surface_state(renderer, surface);

    gl_shader_t        *shader;
    pixman_region32_t   repaint;

    double              x, y;
    int                 i, w, h;
    int                 nrects;
    pixman_box32_t     *rects;
    GLfloat             vertex_array[16];
    float               trans[16];

    pixman_region32_init(&repaint);
    pixman_region32_intersect(&repaint, &node->visible_region, damage);

    if (!pixman_region32_not_empty(&repaint))
        goto done;

    if (node->transform.flags <= PEPPER_MATRIX_TRANSLATE)
    {
        double tx, ty, tz;

        tx = node->transform.m[12];
        ty = node->transform.m[13];
        tz = node->transform.m[14];

        for (i = 0; i < 16; i++)
            trans[i] = (float)gt->proj_mat.m[i];

        trans[12] += trans[0] * tx + trans[4] * ty + trans[8] * tz;
        trans[13] += trans[1] * tx + trans[5] * ty + trans[9] * tz;
        trans[14] += trans[2] * tx + trans[6] * ty + trans[10] * tz;
        trans[15] += trans[3] * tx + trans[7] * ty + trans[11] * tz;
    }
    else
    {
        pepper_mat4_t tmp;
        pepper_mat4_multiply(&tmp, &gt->proj_mat, &node->transform);
        for (i = 0; i < 16; i++)
            trans[i] = (float)tmp.m[i];
    }

    for (i = 0; i < state->num_planes; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, state->textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST /* FIXME */);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST /* FIXME */);
    }

    pepper_view_get_position(node->view, &x, &y);
    pepper_view_get_size(node->view, &w, &h);

    vertex_array[ 0] = 0;
    vertex_array[ 1] = 0;
    vertex_array[ 4] = w;
    vertex_array[ 5] = 0;
    vertex_array[ 8] = w;
    vertex_array[ 9] = h;
    vertex_array[12] = 0;
    vertex_array[13] = h;

    vertex_array[ 2] = 0;
    vertex_array[ 3] = 0;
    vertex_array[ 6] = 1.0f;
    vertex_array[ 7] = 0;
    vertex_array[10] = 1.0f;
    vertex_array[11] = 1.0f;
    vertex_array[14] = 0;
    vertex_array[15] = 1.0f;

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertex_array[0]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertex_array[2]);
    glEnableVertexAttribArray(1);

    shader = &gr->shaders[state->sampler];

    gl_shader_use(gr, shader);

    glUniform1f(shader->alpha_uniform, 1.0f /* FIXME: view->alpha? */);
    for (i = 0; i < state->num_planes; i++)
        glUniform1i(shader->texture_uniform[i], i);
    glUniformMatrix4fv(shader->trans_uniform, 1, GL_FALSE, trans);

    rects = pixman_region32_rectangles(&repaint, &nrects);
    for (i = 0; i < nrects; i++)
    {
        glScissor(rects[i].x1, gt->height - rects[i].y2,
                  rects[i].x2 - rects[i].x1, rects[i].y2 - rects[i].y1);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

done:
    pixman_region32_fini(&repaint);
}

static void
gl_renderer_repaint_output(pepper_renderer_t *renderer, pepper_output_t *output,
                           const pepper_list_t *list, pixman_region32_t *damage)
{
    gl_renderer_t                  *gr = (gl_renderer_t *)renderer;
    gl_render_target_t             *gt = (gl_render_target_t *)renderer->target;
    const pepper_output_geometry_t *geom = pepper_output_get_geometry(output);

    int                             i;
    EGLint                          buffer_age = 0;
    pixman_region32_t               total_damage;

    if (!gl_renderer_use(gr))
        return;

    glViewport(0, 0, geom->w, geom->h);

    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    if (gr->has_buffer_age)
        eglQuerySurface(gr->display, ((gl_render_target_t *)renderer->target)->surface,
                        EGL_BUFFER_AGE_EXT, &buffer_age);

    if (!buffer_age || buffer_age - 1 > MAX_BUFFER_COUNT)
    {
        pixman_region32_init_rect(&total_damage, geom->x, geom->y, geom->w, geom->h);
    }
    else
    {
        int first = gt->damage_index + MAX_BUFFER_COUNT - (buffer_age - 1);

        pixman_region32_init(&total_damage);
        pixman_region32_copy(&total_damage, damage);

        for (i = 0; i < buffer_age - 1; i++)
            pixman_region32_union(&total_damage, &total_damage,
                                  &gt->damages[(first + i) % MAX_BUFFER_COUNT]);

        pixman_region32_copy(&gt->damages[gt->damage_index], damage);

        gt->damage_index += 1;
        gt->damage_index %= MAX_BUFFER_COUNT;
    }

    if (pixman_region32_not_empty(&total_damage))
    {
        pepper_list_t *l;

        if (gr->clear_background)
        {
            int                 i, nrects;
            pixman_box32_t     *rects;

            glClearColor(0.0, 0.0, 0.0, 1.0);

            rects = pixman_region32_rectangles(&total_damage, &nrects);

            for (i = 0; i < nrects; i++)
            {
                glScissor(rects[i].x1, geom->h - rects[i].y2,
                          rects[i].x2 - rects[i].x1, rects[i].y2 - rects[i].y1);
                glClear(GL_COLOR_BUFFER_BIT);
            }
        }

        pepper_list_for_each_list_reverse(l, list)
            repaint_view(renderer, output, (pepper_render_item_t *)l->item, &total_damage);
    }

    eglSwapBuffers(gr->display, ((gl_render_target_t *)renderer->target)->surface);

    /* TODO: eglSwapBuffersWithDamage. */
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
        PEPPER_ERROR("EGL_KHR_image not supported only wl_shm will be supported.\n");
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
    {
        gr->has_buffer_age = PEPPER_TRUE;
    }
    else
    {
        PEPPER_ERROR("Performance Warning: EGL_EXT_buffer_age not supported.\n");
    }

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

static EGLenum
get_egl_platform(const char *str)
{
    if (!str)
        return EGL_NONE;

    if (!strcmp(str, "gbm"))
        return EGL_PLATFORM_GBM_KHR;

    if (!strcmp(str, "wayland"))
        return EGL_PLATFORM_WAYLAND_KHR;

    if (!strcmp(str, "x11"))
        return EGL_PLATFORM_X11_KHR;

    return EGL_NONE;
}

static PFNEGLGETPLATFORMDISPLAYEXTPROC  get_platform_display = NULL;

static pepper_bool_t
setup_display(gl_renderer_t *gr, void *native_display, const char *platform)
{
    EGLenum     egl_platform = get_egl_platform(platform);
    const char *extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    char        str[64];

    if (!extensions || !strstr(extensions, "EGL_EXT_platform_base"))
    {
        gr->display = eglGetDisplay(native_display);
        return gr->display != EGL_NO_DISPLAY;
    }

    if (!egl_platform)
        goto use_legacy;

    if (!extensions)
        goto use_legacy;

    if (!strstr(extensions, "EGL_EXT_platform_base"))
        goto use_legacy;

    snprintf(str, sizeof(str), "EGL_KHR_platform_%s", platform);
    if (!strstr(extensions, str))
        goto use_legacy;

    snprintf(str, sizeof(str), "EGL_EXT_platform_%s", platform);
    if (!strstr(extensions, str))
        goto use_legacy;

    snprintf(str, sizeof(str), "EGL_MESA_platform_%s", platform);
    if (!strstr(extensions, str))
        goto use_legacy;

    if (!get_platform_display)
        get_platform_display = (void *)eglGetProcAddress("eglGetPlatformDisplayEXT");

    if (!get_platform_display)
        goto use_legacy;

    gr->display = get_platform_display(egl_platform, native_display, NULL);

    if (gr->display == EGL_NO_DISPLAY)
        goto use_legacy;

    gr->create_platform_window_surface =
        (void *)eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT");

    return PEPPER_TRUE;

use_legacy:
    if (gr->display == EGL_NO_DISPLAY)
        gr->display = eglGetDisplay(native_display);

    if (gr->display != EGL_NO_DISPLAY)
        return PEPPER_TRUE;

    return PEPPER_FALSE;
}

PEPPER_API pepper_renderer_t *
pepper_gl_renderer_create(pepper_compositor_t *compositor, void *native_display, const char *platform)
{
    gl_renderer_t  *gr;
    EGLint          major, minor;
    const char     *env;

    gr = calloc(1, sizeof(gl_renderer_t));
    if (!gr)
        return NULL;

    if (!setup_display(gr, native_display, platform))
        goto error;

    gr->base.compositor = compositor;
    gr->native_display = native_display;

    gr->base.destroy                =   gl_renderer_destroy;
    gr->base.attach_surface         =   gl_renderer_attach_surface;
    gr->base.flush_surface_damage   =   gl_renderer_flush_surface_damage;
    gr->base.read_pixels            =   gl_renderer_read_pixels;
    gr->base.repaint_output         =   gl_renderer_repaint_output;

    if (!eglInitialize(gr->display, &major, &minor))
    {
        PEPPER_ERROR("eglInitialize() failed.\n");
        goto error;
    }

    if (!eglBindAPI(EGL_OPENGL_ES_API))
    {
        PEPPER_ERROR("eglBindAPI() failed.\n");
        goto error;
    }

    if (!setup_egl_extensions(gr))
        goto error;

    env = getenv("PEPPER_RENDER_CLEAR_BACKGROUND");

    if (env && atoi(env) == 1)
        gr->clear_background = PEPPER_TRUE;

    return &gr->base;

error:
    if (gr)
        gl_renderer_destroy(&gr->base);

    return NULL;
}

static void
gl_render_target_destroy(pepper_render_target_t *target)
{
    int                 i;
    gl_render_target_t *gt = (gl_render_target_t *)target;
    gl_renderer_t      *gr = (gl_renderer_t *)target->renderer;

    for (i = 0; i < MAX_BUFFER_COUNT; i++)
        pixman_region32_fini(&gt->damages[i]);

    if (gt->surface != EGL_NO_SURFACE)
        eglDestroySurface(gr->display, gt->surface);

    free(gt);
}

PEPPER_API pepper_render_target_t *
pepper_gl_renderer_create_target(pepper_renderer_t *renderer, void *native_window,
                                 pepper_format_t format, const void *visual_id,
                                 int32_t width, int32_t height)
{
    gl_renderer_t      *gr = (gl_renderer_t *)renderer;
    gl_render_target_t *target;
    EGLint              config_size = 0, num_configs = 0;
    EGLConfig          *configs = NULL;
    EGLConfig           config = NULL;
    EGLSurface          surface = EGL_NO_SURFACE;
    EGLContext          context = EGL_NO_CONTEXT;
    int                 i;

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

    target = calloc(1, sizeof(gl_render_target_t));
    if (!target)
        return NULL;

    config_attribs[3] = PEPPER_FORMAT_R(format);
    config_attribs[5] = PEPPER_FORMAT_G(format);
    config_attribs[7] = PEPPER_FORMAT_B(format);
    config_attribs[9] = PEPPER_FORMAT_A(format);

    if (!eglChooseConfig(gr->display, config_attribs, NULL, 0, &config_size))
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

    if (!eglChooseConfig(gr->display, config_attribs, configs, config_size, &num_configs))
    {
        PEPPER_ERROR("eglChooseConfig() failed.\n");
        goto error;
    }

    if (num_configs < 1)
        goto error;

    for (i = 0; i < num_configs; i++)
    {
        EGLint attrib;

        if (visual_id)
        {
            /* Native visual id have privilege. */
            if (eglGetConfigAttrib(gr->display, configs[i], EGL_NATIVE_VISUAL_ID, &attrib))
            {
                if (attrib == *((EGLint *)visual_id))
                {
                    config = configs[i];
                    break;
                }
            }

            continue;
        }

        if (eglGetConfigAttrib(gr->display, configs[i], EGL_BUFFER_SIZE, &attrib))
        {
            if (attrib == (EGLint)PEPPER_FORMAT_BPP(format))
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

    /* Try platform window surface creation first. */
    if (gr->create_platform_window_surface)
        surface = gr->create_platform_window_surface(gr->display, config, native_window, NULL);

    if (target->surface == EGL_NO_SURFACE)
    {
        surface = eglCreateWindowSurface(gr->display, config,
                                         (EGLNativeWindowType)native_window, NULL);
    }

    if (surface == EGL_NO_SURFACE)
    {
        PEPPER_ERROR("eglCreateWindowSurface() failed.\n");
        goto error;
    }

    if (gr->context == EGL_NO_CONTEXT)
    {
        context = eglCreateContext(gr->display, config, EGL_NO_CONTEXT, context_attribs);
        if (context == EGL_NO_CONTEXT)
        {
            PEPPER_ERROR("eglCreateContext() failed.\n");
            goto error;
        }

        if (!eglMakeCurrent(gr->display, surface, surface, context))
        {
            PEPPER_ERROR("eglMakeCurrent() failed.\n");
            goto error;
        }

        if (!setup_gl_extensions(gr))
            goto error;

        if (!init_gl_shaders(gr))
            goto error;
    }
    else
    {
        context = gr->context;
    }

    target->base.renderer   = renderer;
    target->surface         = surface;
    target->config          = config;
    target->native_window   = native_window;
    target->width           = width;
    target->height          = height;

    if (gr->context == EGL_NO_CONTEXT)
        gr->context = context;

    target->base.destroy = gl_render_target_destroy;

    pepper_mat4_init_translate(&target->proj_mat, (double)width / -2, (double)height / -2, 0);
    pepper_mat4_scale(&target->proj_mat, (double)2 / width, (double)(-2) / height, 1);

    for (i = 0; i < MAX_BUFFER_COUNT; i++)
        pixman_region32_init(&target->damages[i]);

    return &target->base;

error:
    if (context != EGL_NO_CONTEXT)
        eglDestroyContext(gr->display, context);

    if (surface != EGL_NO_SURFACE)
        eglDestroySurface(gr->display, surface);

    free(target);
    return NULL;
}
