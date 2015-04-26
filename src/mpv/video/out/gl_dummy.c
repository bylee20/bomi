/*
 * This file is part of mpv.
 *
 * mpv is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with mpv.  If not, see <http://www.gnu.org/licenses/>.
 *
 * You can alternatively redistribute this file and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include "gl_common.h"

static bool config_window_dummy(struct MPGLContext *ctx, int flags)
{
    return true;
//     int gl_version = ctx->requested_gl_version;
//     bool success = false;
//     if (!glx_ctx->force_es) {
//         success = create_context_x11_gl3(ctx, flags, gl_version, false);
//         if (!success)
//             success = create_context_x11_old(ctx);
//     }
//     if (!success) // try ES
//         success = create_context_x11_gl3(ctx, flags, 200, true);
//     if (success && !glXIsDirect(vo->x11->display, glx_ctx->context))
//         ctx->gl->mpgl_caps |= MPGL_CAP_SW;
//     return success;
}

static void releaseGlContext_dummy(MPGLContext *ctx)
{

    
}

static void swapGlBuffers_dummy(MPGLContext *ctx)
{

}


static int vo_dummy_init(struct vo *vo)
{
    return 1;
}

static void vo_dummy_uninit(struct vo *vo)
{
}

static int vo_dummy_control(struct vo *vo, int *events, int request, void *arg)
{
    return VO_NOTIMPL;
}

void mpgl_set_backend_dummy(MPGLContext *ctx)
{
    ctx->priv = NULL;
    ctx->config_window = config_window_dummy;
    ctx->releaseGlContext = releaseGlContext_dummy;
    ctx->swapGlBuffers = swapGlBuffers_dummy;
    ctx->vo_init = vo_dummy_init;
    ctx->vo_uninit = vo_dummy_uninit;
    ctx->vo_control = vo_dummy_control;
}
