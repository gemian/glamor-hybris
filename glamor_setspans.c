/*
 * Copyright © 2009 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "glamor_priv.h"

void
glamor_set_spans(DrawablePtr drawable, GCPtr gc, char *src,
		 DDXPointPtr points, int *widths, int n, int sorted)
{
    PixmapPtr dest_pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_screen_private *glamor_priv;
    GLenum format, type;
    int no_alpha, i;
    uint8_t *drawpixels_src = (uint8_t *)src;
    RegionPtr clip = fbGetCompositeClip(gc);
    BoxRec *pbox;
    int x_off, y_off;

    glamor_priv = glamor_get_screen_private(drawable->pScreen);

    if (glamor_priv->gl_flavor == GLAMOR_GL_ES2)
      goto fail;

    if (glamor_get_tex_format_type_from_pixmap(dest_pixmap,
                                               &format, 
                                               &type, 
                                               &no_alpha
                                               )) {
      glamor_fallback("unknown depth. %d \n", 
                     drawable->depth);
      goto fail;
    }
    if (glamor_set_destination_pixmap(dest_pixmap))
	goto fail;

    glamor_validate_pixmap(dest_pixmap);
    if (!glamor_set_planemask(dest_pixmap, gc->planemask))
	goto fail;
    glamor_set_alu(gc->alu);
    if (!glamor_set_planemask(dest_pixmap, gc->planemask))
	goto fail;

    glamor_get_drawable_deltas(drawable, dest_pixmap, &x_off, &y_off);

    for (i = 0; i < n; i++) {

	n = REGION_NUM_RECTS(clip);
	pbox = REGION_RECTS(clip);
	while (n--) {
	    if (pbox->y1 > points[i].y)
		break;
	    glScissor(pbox->x1,
		      points[i].y + y_off,
		      pbox->x2 - pbox->x1,
		      1);
	    glEnable(GL_SCISSOR_TEST);
	    glRasterPos2i(points[i].x + x_off,
			  points[i].y + y_off);
	    glDrawPixels(widths[i],
			 1,
			 format, type,
			 drawpixels_src);
	}
	    drawpixels_src += PixmapBytePad(widths[i], drawable->depth);
    }
    glamor_set_planemask(dest_pixmap, ~0);
    glamor_set_alu(GXcopy);
    glDisable(GL_SCISSOR_TEST);
    return;
fail:

    glamor_fallback("to %p (%c)\n",
		    drawable, glamor_get_drawable_location(drawable));
    if (glamor_prepare_access(drawable, GLAMOR_ACCESS_RW)) {
	fbSetSpans(drawable, gc, src, points, widths, n, sorted);
	glamor_finish_access(drawable);
    }
}
