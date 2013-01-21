/* Hot-babe
 * Copyright (C) 2002 DindinX & Cyprien
 * Copyright (C) 2002 Bruno Bellamy.
 * Copyright (C) 2012 Allan Wirth <allan@allanwirth.com>
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

#include <glib.h>

#include "def.h"
#include "loader.h"

/* Animation loader from a directory */
gboolean load_anim(HotBabeAnim * anim, const gchar *dirname) {
  GIOChannel *fd = NULL;
  gchar *filename = NULL;
  gchar *line = NULL;
  GError *err = NULL;
  gboolean result = FALSE;
  GdkPixbuf *pixbuf = NULL;
  gint64 temp;

  anim->samples = 0;
  anim->width = -1;
  anim->height = -1;

  /* dirname must be a valid directory */

  if (!dirname || !g_file_test(dirname, G_FILE_TEST_IS_DIR)) {
    /* Don't print an error because we always try invalid directories */
    return FALSE;
  }

  /* description file */
  filename = g_build_filename(dirname, DESCR, NULL);
  if ((fd = g_io_channel_new_file(filename, "r", &err)) == NULL) {
    g_printerr("Failed to open file %s: %s\n", filename, err->message);
    goto cleanup;
  }

  if (g_io_channel_read_line(fd, &line, NULL, NULL, &err) !=
      G_IO_STATUS_NORMAL) {
    g_printerr("Failed to read file %s: %s\n", filename, err->message);
    goto cleanup;
  }

  temp = g_ascii_strtoll(line, NULL, 0);
  if (temp <= 0 || temp >= G_MAXSIZE) {
    g_printerr("%s: bad file format (first line not a number)\n", filename);
    goto cleanup;
  }
  anim->samples = temp;

  /* load images */
  anim->surface = g_malloc_n(anim->samples, sizeof(cairo_surface_t *));
  if (!anim->surface) {
    g_printerr("Error allocating buffer for surfaces.\n");
    goto cleanup;
  }
  for (gsize i = 0; i < anim->samples; i++) {
    gsize n;
    cairo_t *cr;

    if (pixbuf) g_object_unref(pixbuf);

    g_free(line);
    g_free(filename);
    line = NULL;
    filename = NULL;

    if (g_io_channel_read_line(fd, &line, NULL, &n, &err) !=
        G_IO_STATUS_NORMAL) {
      g_printerr("Error reading line from %s: %s\n", filename, err->message);
      goto cleanup;
    }
    line[n] = '\0';
    filename = g_build_filename(dirname, line, NULL);
    if (!(pixbuf = gdk_pixbuf_new_from_file(filename, &err))) {
      g_printerr("Unable to create GDK Pixbuf from %s: %s\n", filename,
          err->message);
      goto cleanup;
    }
    gint w = gdk_pixbuf_get_width(pixbuf);
    gint h = gdk_pixbuf_get_height(pixbuf);
    if (anim->width == -1 && anim->height == -1) {
      anim->width = w;
      anim->height = h;
    } else if (anim->width != w || anim->height != h) {
      g_printerr("Images do not all have the same width.\n");
      goto cleanup;
    }
    anim->surface[i] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);

    if (!(cr = cairo_create(anim->surface[i]))) {
      g_printerr("Error creating cairo surface from %s\n", line);
      goto cleanup;
    }
    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);
    g_print("Loaded image %s (%dx%d, %dbpp)\n", line, w, h,
        gdk_pixbuf_get_bits_per_sample(pixbuf));
  }

  gdk_pixbuf_render_pixmap_and_mask(pixbuf, NULL, &anim->mask, 127);

  result = TRUE;

cleanup:
  if (err) g_error_free(err);

  if (pixbuf) g_object_unref(pixbuf);

  if (fd) {
    g_io_channel_shutdown(fd, TRUE, &err);
    g_io_channel_unref(fd);
  }

  if (line) g_free(line);

  if (filename) g_free(filename);

  return result;
}
