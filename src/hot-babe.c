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

#include "hot-babe.h"

/* general includes */
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include "def.h"
#include "stats.h"
#include "config.h"

HotBabeData bm = {0}; /* Zero initialize */

static void hotbabe_event(GdkEvent *event, gpointer data) {
  if (event != NULL &&
      (event->type == GDK_DESTROY ||
       (event->type == GDK_BUTTON_PRESS &&
        event->button.button == 3))) {
    g_main_loop_quit(bm.loop);
  }
}

/* This is the function that actually creates the display widgets */
static void create_hotbabe_window(void) {
  GdkWindowAttr attr;
  GdkScreen *defscrn;

  if (!(defscrn = gdk_screen_get_default())) {
    g_printerr("Error accessing default screen.\n");
    exit(1);
  }

  if (bm.composited == AUTO) {
    bm.composited = gdk_screen_is_composited(defscrn)?AUTO_COMPOSITE:AUTO_NOCOMPOSITE;
  }

  attr.width = bm.anim.width;
  attr.height = bm.anim.height;
  attr.x = bm.x;
  attr.y = bm.y;
  if (attr.x < 0) attr.x += 1 + gdk_screen_get_width(defscrn) - attr.width;
  if (attr.y < 0) attr.y += 1 + gdk_screen_get_height(defscrn) - attr.height;
  attr.title = PNAME;
  attr.event_mask = (GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK |
      GDK_LEAVE_NOTIFY_MASK);
  attr.wclass = GDK_INPUT_OUTPUT;
  attr.type_hint = GDK_WINDOW_TYPE_HINT_DOCK;
  attr.wmclass_name = PNAME;
  attr.wmclass_class = PNAME;
  attr.window_type = GDK_WINDOW_TOPLEVEL;
  attr.visual = NULL;
  if (bm.composited == FORCE_COMPOSITE || bm.composited == AUTO_COMPOSITE) {
    attr.visual = gdk_screen_get_rgba_visual(defscrn);
    if (bm.composited == AUTO_COMPOSITE && !attr.visual)
        bm.composited = AUTO_NOCOMPOSITE;
  }
  if (bm.composited == FORCE_NOCOMPOSITE || bm.composited == AUTO_NOCOMPOSITE) {
    attr.visual = gdk_screen_get_system_visual(defscrn);
  }
  if (!attr.visual) {
    g_printerr("Error getting gdk screen visual.\n");
    exit(1);
  }

  g_print("Compositing: %s %sCompositing\n",
      bm.composited==FORCE_NOCOMPOSITE||bm.composited==FORCE_COMPOSITE?"Force":"Auto",
      bm.composited==FORCE_NOCOMPOSITE||bm.composited==AUTO_NOCOMPOSITE?"No ":"");

  bm.win = gdk_window_new(NULL, &attr,
      GDK_WA_TITLE | GDK_WA_WMCLASS | GDK_WA_TYPE_HINT |
      GDK_WA_VISUAL | GDK_WA_X | GDK_WA_Y);
  if (!bm.win) {
    g_printerr("Error making toplevel window\n");
    exit(1);
  }
  gdk_window_set_decorations(bm.win, 0);
  gdk_window_set_skip_taskbar_hint(bm.win, TRUE);
  gdk_window_set_skip_pager_hint(bm.win, TRUE);
  gdk_event_handler_set(hotbabe_event, NULL, NULL);
//  gdk_window_set_keep_below(bm.win, TRUE);

  if (bm.composited == FORCE_NOCOMPOSITE || bm.composited == AUTO_NOCOMPOSITE) {
    gdk_window_shape_combine_region(bm.win, bm.anim.mask, 0, 0);
  }

  gdk_window_show(bm.win);
}

static void hotbabe_update(gboolean force) {
  double loadPercentage = system_cpu();

  if (bm.threshold) {
    if (loadPercentage < bm.threshold || bm.threshold > 255)
      loadPercentage = 0;
    else
      loadPercentage = (loadPercentage - bm.threshold) * 256 /
          (256 - bm.threshold);
  }

  if (bm.incremental) {
    loadPercentage = (loadPercentage + (bm.samples-1)*bm.oldPercentage) /
        bm.samples;
  }

  if (loadPercentage != bm.oldPercentage || force) {
    cairo_t *cr;
    cairo_surface_t *p3;

    bm.oldPercentage = loadPercentage;
    size_t range = 256 / (bm.anim.samples - 1);
    size_t index = loadPercentage / range;

    if (index > bm.anim.samples - 1)
      index = bm.anim.samples - 1;

    p3 = bm.anim.surface[index+((index == bm.anim.samples-1)?0:1)];

    loadPercentage -= range * floor(loadPercentage/range); /* modulo */

    cairo_rectangle_int_t rect = {0, 0, bm.anim.width, bm.anim.height};
    cairo_region_t *reg = cairo_region_create_rectangle(&rect);
    gdk_window_begin_paint_region(bm.win, reg);
    cairo_region_destroy(reg);

    if (!(cr = gdk_cairo_create(bm.win))) {
      g_printerr("Error creating cairo object\n");
      exit(1);
    }

    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_surface(cr, p3, 0, 0);
    cairo_paint_with_alpha(cr, 1);
    cairo_set_source_surface(cr, bm.anim.surface[index], 0, 0);
    cairo_paint_with_alpha(cr, 1 - loadPercentage / range);
    cairo_destroy(cr);

    gdk_window_end_paint(bm.win);
  }
}

static gboolean hotbabe_source(gpointer ud) {
  hotbabe_update(FALSE);
  return TRUE;
}

int main(int argc, char **argv) {
  GError *err = NULL;

  /* initialize GDK */
  if (!gdk_init_check(&argc, &argv)) {
    g_printerr("GDK init failed, bye bye.  Check \"DISPLAY\" variable.\n");
    exit(-1);
  }

  bm.samples = NUM_SAMPLES;
  bm.incremental = FALSE;
  bm.delay = 15;
  bm.noNice = FALSE;
  bm.nice = 0;
  bm.dir = DEFAULT_DIR;
  bm.x = -1;
  bm.y = -1;
  bm.composited = AUTO;
  bm.oldPercentage = 0.0;

  bm.loadIndex = 0;
  bm.load = g_malloc0_n(bm.samples, sizeof(unsigned long long));
  bm.total = g_malloc0_n(bm.samples, sizeof(unsigned long long));

  bm.context = g_option_context_new("- interesting CPU Monitor");
  g_option_context_add_main_entries(bm.context, cmd_options, NULL);

  parse_conf();

  if (!g_option_context_parse(bm.context, &argc, &argv, &err)) {
    g_printerr("Error parsing command line arguments: %s\n",
        err->message);
    exit(1);
  }

  bm.threshold = CLAMP(bm.threshold/100.0, 0.0, 1.0);

  if (bm.nice) nice(bm.nice);

  if (!hotbabe_load_pics()) {
    g_printerr("Couldn't load pictures\n");
    return 1;
  }

  create_hotbabe_window();

  hotbabe_update(TRUE);

  bm.loop = g_main_loop_new(NULL, FALSE);
  g_timeout_add(bm.delay, hotbabe_source, NULL);
  g_main_loop_run(bm.loop);

  return 0;
}
