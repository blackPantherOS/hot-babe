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

#include "stats.h"

/* general includes */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef __FreeBSD__
  #include <sys/time.h>
  #include <sys/resource.h>
  #include <sys/types.h>
  #include <sys/sysctl.h>
  #ifndef CPUSTATES
    #include <sys/dkstat.h>
  #endif                          /* CPUSTATES */
#endif                          /* __FreeBSD__ */

#include "hot-babe.h"

/* returns current CPU load in percent, 0 to 1 */
double system_cpu(void) {
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
