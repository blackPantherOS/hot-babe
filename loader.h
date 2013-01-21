/* Hot-babe
 * Copyright (C) 2002 DindinX & Cyprien
 * Copyright (C) 2002 Bruno Bellamy.
 * Copyright (C) 2012 Allan Wirth <allan@allanwith.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the artistic License
 *
 * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES
 * OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License for more details.
 *
 */
#ifndef _LOADER_H
#define _LOADER_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

typedef struct {
  gint height, width;
  gsize samples;
  cairo_surface_t **surface;
  GdkBitmap *mask;
} HotBabeAnim;

gboolean load_anim(HotBabeAnim * anim, const gchar *dirname);

#endif                          /* _LOADER_H */
