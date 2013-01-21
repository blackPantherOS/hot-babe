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

/* general includes */
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#ifdef __FreeBSD__
  #include <sys/time.h>
  #include <sys/resource.h>
  #include <sys/types.h>
  #include <sys/sysctl.h>
  #ifndef CPUSTATES
    #include <sys/dkstat.h>
  #endif                          /* CPUSTATES */
#endif                          /* __FreeBSD__ */

/* x11 includes */
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

#include "def.h"
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

static HotBabeData bm = {0}; /* Zero initialize */

/* returns current CPU load in percent, 0 to 1 */
static double system_cpu(void) {
  unsigned long long load, total, oload, ototal;
  unsigned long long ab, ac, ad, ae;
#ifdef __linux__
  FILE *stat = fopen("/proc/stat", "r");
  if (stat == NULL) {
    perror("Error opening /proc/stat for reading.");
    exit(1);
  }
  if (fscanf(stat, "cpu %Lu %Lu %Lu %Lu", &ab, &ac, &ad, &ae) < 4) {
    fprintf(stderr, "Error reading cpu data from /proc/stat.\n");
    exit(1);
  }
  fclose(stat);
#else                           /* __linux__ */
#ifdef __FreeBSD__
  long cp_time[CPUSTATES];
  size_t len = sizeof(cp_time);

  if (sysctlbyname("kern.cp_time", &cp_time, &len, NULL, 0) < 0)
    (void) fprintf(stderr, "Cannot get kern.cp_time");

  ab = cp_time[CP_USER];
  ac = cp_time[CP_NICE];
  ad = cp_time[CP_SYS];
  ae = cp_time[CP_IDLE];
#endif                          /* __FreeBSD__ */
#endif                          /* !__linux__ */

  oload = bm.load[bm.loadIndex];
  ototal = bm.total[bm.loadIndex];

  /* Find out the CPU load
   * user + sys = load
   * total = total */
  load = ab + ad;               /* cpu.user + cpu.sys; */
  if (!bm.noNice)
    load += ac;
  total = ab + ac + ad + ae;    /* cpu.total; */

  bm.load[bm.loadIndex] = load;
  bm.total[bm.loadIndex] = total;

  bm.loadIndex = (bm.loadIndex + 1) % bm.samples;

  /*   Because the load returned from libgtop is a value accumulated
   *   over time, and not the current load, the current load percentage
   *   is calculated as the extra amount of work that has been performed
   *   since the last sample. yah, right, what the fuck does that mean?
   */
  if (ototal == 0 || total == ototal)   /* ototal == 0 means that this is the first time we get here */
    return 0;
  else
    return (256.0 * (load - oload)) / (total - ototal);
}

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
  if (bm.composited == FORCE_COMPOSITE || bm.composited == AUTO_COMPOSITE) {
    attr.visual = gdk_screen_get_rgba_visual(defscrn);
    attr.colormap = gdk_screen_get_rgba_colormap(defscrn);
    if (bm.composited == AUTO_COMPOSITE && !(attr.visual && attr.colormap))
        bm.composited = AUTO_NOCOMPOSITE;
  }
  if (bm.composited == FORCE_NOCOMPOSITE || bm.composited == AUTO_NOCOMPOSITE) {
    attr.visual = gdk_screen_get_rgb_visual(defscrn);
    attr.colormap = gdk_screen_get_rgb_colormap(defscrn);
  }
  if (!attr.visual || !attr.colormap) {
    g_printerr("Error getting gdk screen visual and colormap.\n");
    exit(1);
  }

  g_print("Compositing: %s %sCompositing\n",
      bm.composited==FORCE_NOCOMPOSITE||bm.composited==FORCE_COMPOSITE?"Force":"Auto",
      bm.composited==FORCE_NOCOMPOSITE||bm.composited==AUTO_NOCOMPOSITE?"No ":"");

  bm.win = gdk_window_new(NULL, &attr,
      GDK_WA_TITLE | GDK_WA_WMCLASS | GDK_WA_TYPE_HINT |
      GDK_WA_VISUAL | GDK_WA_COLORMAP | GDK_WA_X | GDK_WA_Y);
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
    gdk_window_shape_combine_mask(bm.win, bm.anim.mask, 0, 0);
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
    GdkPixmap *next;
    cairo_t *cr;
    cairo_surface_t *p3;

    bm.oldPercentage = loadPercentage;
    size_t range = 256 / (bm.anim.samples - 1);
    size_t index = loadPercentage / range;

    if (index > bm.anim.samples - 1)
      index = bm.anim.samples - 1;

    p3 = bm.anim.surface[index+((index == bm.anim.samples-1)?0:1)];

    loadPercentage -= range * floor(loadPercentage/range); /* modulo */

    next = gdk_pixmap_new(bm.win, bm.anim.width, bm.anim.height, -1);
    if (!next) {
      g_printerr("Error creating gdk pixmap\n");
      exit(1);
    }
    if (!(cr = gdk_cairo_create(next))) {
      g_printerr("Error creating cairo surface\n");
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

    gdk_window_set_back_pixmap(bm.win, next, FALSE);
    g_object_unref(next);

    gdk_window_clear(bm.win);
  }
}

static gboolean hotbabe_source(gpointer ud) {
  hotbabe_update(FALSE);
  return TRUE;
}

static gboolean hotbabe_load_pics(void) {
  const gchar *const *sys = g_get_system_data_dirs();
  for(const char *const *i = sys; *i; i++) {
    gchar *path = g_build_filename(*i, PNAME, bm.dir, NULL);
    gboolean r = load_anim(&bm.anim, path);
    g_free(path);
    if (r) return TRUE;
  }
  gchar *home = g_build_filename(g_get_user_data_dir(), PNAME, bm.dir, NULL);
  gboolean r2 = load_anim(&bm.anim, home);
  g_free(home);
  if (r2) return TRUE;

  return load_anim(&bm.anim, bm.dir);
}

static gboolean geometry_callback(const gchar *option_name,
    const gchar *value, gpointer data, GError **error) {
  char sign[2];
  guint val[2];

  if (sscanf(value, "%c%u%c%u", &sign[0], &val[0], &sign[1], &val[1]) != 4)
    return FALSE;

  bm.x = val[0] * (sign[0] == '-' ? -1 : 1);
  bm.y = val[1] * (sign[1] == '-' ? -1 : 1);

  return TRUE;
}

static gboolean force_composite(const gchar *option_name,
    const gchar *value, gpointer data, GError **error) {
  bm.composited = FORCE_COMPOSITE;
  return TRUE;
}

static gboolean force_nocomposite(const gchar *option_name,
    const gchar *value, gpointer data, GError **error) {
  bm.composited = FORCE_NOCOMPOSITE;
  return TRUE;
}

static gboolean show_version(const gchar *option_name,
    const gchar *value, gpointer data, GError *error) {
  g_print(PNAME " version " VERSION "\n");
  exit(0);
}

static GOptionEntry entries[] = {
  {"threshold", 't', 0, G_OPTION_ARG_DOUBLE, &bm.threshold, "Use only the first picture before N%", "N" },
  {"incremental", 'i', 0, G_OPTION_ARG_NONE, &bm.incremental, "Incremental (slow) mode.", NULL },
  {"delay", 'd', 0, G_OPTION_ARG_INT, &bm.delay, "Update every N milliseconds", "N"},
  {"noNice", 'N', 0, G_OPTION_ARG_NONE, &bm.noNice, "Don't count nice time in usage.", NULL},
  {"nice", 'n', 0, G_OPTION_ARG_INT, &bm.nice, "Set self-nice to N", "N"},
  {"dir", 'D', 0, G_OPTION_ARG_FILENAME, &bm.dir, "Use images from directory.", "D"},
  {"geometry", 'g', 0, G_OPTION_ARG_CALLBACK, geometry_callback, "Set geometry", "{+|-}x{+|-}y"},
  {"composite", 'c', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, force_composite, "Force compositing", NULL},
  {"nocomposite", 'C', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, force_nocomposite, "Force no compositing", NULL},
  {"version", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, show_version, "Show version and exit", NULL},
  {NULL}
};

/* Hacky conf file parsing to not write logic twice */
static void parse_conf(void) {
  GIOChannel *f;

  gchar *conf = g_build_filename(g_get_user_config_dir(), PNAME, CONFIG_FNAME, NULL);
  if ((f = g_io_channel_new_file(conf, "r", NULL))) {
    char *line = NULL;
    GError *err = NULL;

    while (g_io_channel_read_line(f, &line, NULL, NULL, NULL) ==
        G_IO_STATUS_NORMAL) {
      char *temp = NULL, **argv = NULL;
      gint argc;

      /* strip newlines and comments */
      g_strdelimit(line, "\n#", '\0');
      temp = g_strdup_printf(PNAME " --%s", line);
      if (!g_shell_parse_argv(temp, &argc, &argv, &err) ||
          (argc != 2 && argc != 3) ||
          !g_option_context_parse(bm.context, &argc, &argv, &err)) {
        g_printerr("Error parsing config file arguments: %s (%s)\n",
            err?err->message:"Invalid number of params", line);
        exit(1);
      }
      g_strfreev(argv);
    }
    g_free(line);
    g_io_channel_shutdown(f, TRUE, &err);
    g_io_channel_unref(f);
  }
  g_free(conf);
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
  g_option_context_add_main_entries(bm.context, entries, NULL);

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
