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

#include "pepper-utils.h"

PEPPER_API void
pepper_id_allocator_init(pepper_id_allocator_t *allocator)
{
    memset(allocator, 0x00, sizeof(pepper_id_allocator_t));
    allocator->next_id = 1;
}

PEPPER_API void
pepper_id_allocator_fini(pepper_id_allocator_t *allocator)
{
    if (allocator->free_ids)
        free(allocator->free_ids);
}

PEPPER_API uint32_t
pepper_id_allocator_alloc(pepper_id_allocator_t *allocator)
{
    if (allocator->free_id_count)
        return allocator->free_ids[--allocator->free_id_count];

    return allocator->next_id++;
}

PEPPER_API void
pepper_id_allocator_free(pepper_id_allocator_t *allocator, uint32_t id)
{
    if (allocator->free_id_size <= allocator->free_id_count)
    {
        allocator->free_id_size += 64;
        allocator->free_ids = realloc(allocator->free_ids,
                                      allocator->free_id_size * sizeof(uint32_t));
    }

    allocator->free_ids[allocator->free_id_count++] = id;
}
