/*
* Copyright © 2008-2012 Kristian Høgsberg
* Copyright © 2010-2012 Intel Corporation
* Copyright © 2011 Benjamin Franzke
* Copyright © 2012 Collabora, Ltd.
* Copyright © 2015 S-Core Corporation
* Copyright © 2015-2016 Samsung Electronics co., Ltd. All Rights Reserved.
* 
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#include "drm-internal.h"
#include <stdio.h>

static const char *connector_type_names[] =
{
    "None",
    "VGA",
    "DVI",
    "DVI",
    "DVI",
    "Composite",
    "TV",
    "LVDS",
    "CTV",
    "DIN",
    "DP",
    "HDMI",
    "HDMI",
    "TV",
    "eDP",
};

static inline void
get_connector_name(char *str, drmModeConnector *conn)
{
    const char *type_name;

    if (conn->connector_type < PEPPER_ARRAY_LENGTH(connector_type_names))
        type_name = connector_type_names[conn->connector_type];
    else
        type_name = "UNKNOWN";

    snprintf(str, 32, "%s%d", type_name, conn->connector_type_id);
}

static inline void
add_connector(pepper_drm_t *drm, drmModeConnector *connector)
{
    drm_connector_t *conn;

    conn = calloc(1, sizeof(drm_connector_t));
    PEPPER_CHECK(conn, return, "calloc() failed.\n");

    conn->connector = connector;
}

void
drm_init_connectors(pepper_drm_t *drm)
{
    int i;

    for (i = 0; i < drm->resources->count_connectors; i++)
    {
        drm_connector_t *conn = calloc(1, sizeof(drm_connector_t));
        PEPPER_CHECK(conn, continue, "calloc() failed.\n");

        conn->drm = drm;
        conn->id = drm->resources->connectors[i];
        conn->connector = drmModeGetConnector(drm->fd, conn->id);
        if (!conn->connector)
        {
            PEPPER_ERROR("drmModeGetConnector() failed.\n");
            free(conn);
            continue;
        }

        get_connector_name(&conn->name[0], conn->connector);
        conn->connected = conn->connector->connection == DRM_MODE_CONNECTED;
        pepper_list_insert(drm->connector_list.prev, &conn->link);

        if (conn->connected)
            drm_output_create(conn);
    }
}

void
drm_update_connectors(pepper_drm_t *drm)
{
    drm_connector_t *conn;

    pepper_list_for_each(conn, &drm->connector_list, link)
    {
        if (conn->connector)
            drmModeFreeConnector(conn->connector);

        /* XXX: Do I have to get connector again here??? */
        conn->connector = drmModeGetConnector(drm->fd, conn->id);
        PEPPER_CHECK(conn->connector, continue, "drmModeGetConnector() failed.\n");

        if (conn->connected && conn->connector->connection != DRM_MODE_CONNECTED)
        {
            /* Disconnected. */
            if (conn->output)
            {
                pepper_output_destroy(conn->output->base);
                conn->output = NULL;
                conn->connected = PEPPER_FALSE;
            }
        }
        else if (!conn->connected && conn->connector->connection == DRM_MODE_CONNECTED)
        {
            /* Newly connected. */
            PEPPER_ASSERT(conn->output == NULL);
            drm_output_create(conn);
        }
    }
}

void
drm_connector_destroy(drm_connector_t *conn)
{
    if (conn->output)
        pepper_output_destroy(conn->output->base);

    if (conn->connector)
        drmModeFreeConnector(conn->connector);

    pepper_list_remove(&conn->link);
    free(conn);
}
