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
#ifndef _CONFIG_H
#define _CONFIG_H

#include <glib.h>

void parse_conf(void);
extern GOptionEntry cmd_options[];
gboolean hotbabe_load_pics(void);

#endif
