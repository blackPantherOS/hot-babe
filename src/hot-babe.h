/* Hot-babe
 * Copyright (C) 2002 DindinX <David@dindinx.org>
 * Copyright (C) 2002 Bruno Bellamy.
 * Copyright (C) 2012-2013 Allan Wirth <allan@allanwirth.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the artistic License
 *
 * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES
 * OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License for more details.
 *
 * this code is using some ideas from wmbubble (timecop@japan.co.jp)
 *
 */

#ifndef _HOT_BABE_H
#define _HOT_BABE_H

/* x11 includes */
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

#include "loader.h"

/* global variables */
typedef enum {
  AUTO,
  AUTO_COMPOSITE, AUTO_NOCOMPOSITE,
  FORCE_COMPOSITE, FORCE_NOCOMPOSITE
} Compositing;

typedef struct {
  /* X11 stuff */
  GdkWindow *win;               /* main window */
  HotBabeAnim anim;             /* struct describing animation */
  gint x, y;                    /* position of window */
  GOptionContext *context;
  GMainLoop *loop;

  /* CPU percentage stuff */
  guint loadIndex;              /* current location in ring buffer */
  guint samples;                /* size of ring buffer */
  double oldPercentage;         /* last percentage drawn */
  unsigned long long *load, *total; /* ring buffers of load and total samples */

  /* settings stuff */
  double threshold;             /* Threshold for second picture */
  gboolean incremental;         /* TRUE for incremental mode */
  gboolean noNice;              /* TRUE to ignore niced processes */
  guint delay;                  /* delay between updates in microseconds */
  gint nice;                    /* Value to nice() */
  char *dir;                    /* Name of directory to use for loading */
  Compositing composited;       /* TRUE if running in composited mode */
} HotBabeData;

extern HotBabeData bm;

#endif

