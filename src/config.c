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

#include "config.h"

#include <stdlib.h>

#include "hot-babe.h"
#include "def.h"

gboolean hotbabe_load_pics(void) {
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

static gboolean geometry_parse(const gchar *option_name,
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

GOptionEntry cmd_options[] = {
  {"threshold", 't', 0, G_OPTION_ARG_DOUBLE, &bm.threshold, "Use only the first picture before N%", "N" },
  {"incremental", 'i', 0, G_OPTION_ARG_NONE, &bm.incremental, "Incremental (slow) mode.", NULL },
  {"delay", 'd', 0, G_OPTION_ARG_INT, &bm.delay, "Update every N milliseconds", "N"},
  {"noNice", 'N', 0, G_OPTION_ARG_NONE, &bm.noNice, "Don't count nice time in usage.", NULL},
  {"nice", 'n', 0, G_OPTION_ARG_INT, &bm.nice, "Set self-nice to N", "N"},
  {"dir", 'D', 0, G_OPTION_ARG_FILENAME, &bm.dir, "Use images from directory.", "D"},
  {"geometry", 'g', 0, G_OPTION_ARG_CALLBACK, geometry_parse, "Set geometry", "{+|-}x{+|-}y"},
  {"composite", 'c', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, force_composite, "Force compositing", NULL},
  {"nocomposite", 'C', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, force_nocomposite, "Force no compositing", NULL},
  {"version", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, show_version, "Show version and exit", NULL},
  {NULL}
};

/* Hacky conf file parsing to not write logic twice */
void parse_conf(void) {
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
