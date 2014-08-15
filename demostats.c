/***************************************************************************
 *
 * Copyright (c) 2014 Mathias Thore
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 ***************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "demo.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static void do_stats(char *demoname);
static void monster_stats(demo *demo, int *killed, int *total);
static void secrets_stats(demo *demo, int *found, int *total);
static void map_info(demo *demo, char **mapname, char **maptitle);
static void time_info(demo *demo, float *start_time, float *exit_time);

int main(int argc, char *argv[])
{
  int i;

  if (argc < 2) {
    printf("demostats by Mandel 2014\n"
	   "\n"
	   "usage: demostats <demo> [<demo2> ...]\n");
    exit(1);
  }

  do_stats(argv[1]);
  for (i = 2; i < argc; i++) {
    printf("\n");
    do_stats(argv[i]);
  }

  return 0;
}

static void do_stats(char *demoname)
{
  demo *demo;
  dret_t dret;
  flagfield readflags[]  = {{READFLAG_FILENAME, NULL},
			    {READFLAG_END, READFLAG_END}};
  int monsters_killed, monsters_total;
  int secrets_found, secrets_total;
  char *mapname,  *maptitle;
  float start_time, exit_time;

  // open demo
  readflags[0].value = demoname;
  if ((dret = demo_read(readflags, &demo)) != DEMO_OK) {
    printf("demo %s not opened: %s\n", demoname, demo_error(dret));
    return;
  }

  // gather statistics
  monster_stats(demo, &monsters_killed, &monsters_total);
  secrets_stats(demo, &secrets_found, &secrets_total);
  map_info(demo, &mapname, &maptitle);
  time_info(demo, &start_time, &exit_time);

  // print statistics
  printf("demo:       %s\n", demoname);
  printf("protocol:   %u\n", demo->protocol);
  printf("map bsp:    %s\n", mapname);
  printf("map title:  %s\n", maptitle);
  printf("kills:      %d/%d\n", monsters_killed, monsters_total);
  printf("secrets:    %d/%d\n", secrets_found, secrets_total);
  printf("start time: %f\n", start_time);
  printf("exit time:  %f\n", exit_time);
  printf("duration:   ");
  if (exit_time > start_time) {
    printf("%f\n", exit_time - start_time);
  }
  else {
    printf("\n");
  }

  demo_free(demo);
  free(mapname);
  free(maptitle);
}

static void monster_stats(demo *demo, int *killed, int *total)
{
  block *b;
  message *m;

  int km_count = 0;
  int us_km_count = 0;
  int us_tm_count = 0;

  for (b = demo->blocks; b; b = b->next) {
    for (m = b->messages; m; m = m->next) {
      if (m->type == KILLEDMONSTER) {
	km_count++;
      }
      if (m->type == UPDATESTAT) {
	int index = (int) m->data[0];
	int buf;
	memcpy(&buf, m->data + 1, sizeof(buf));
	buf = dtohl(buf);

	if (index == 12) { // total_monsters
	  us_tm_count = buf;
	}
	if (index == 14) { // killed_monsters
	  us_km_count = buf;
	}
      }
    }
  }

  *killed = MAX(us_km_count, km_count);
  *total = us_tm_count;
}

static void secrets_stats(demo *demo, int *found, int *total)
{
  block *b;
  message *m;

  int fs_count = 0;
  int us_fs_count = 0;
  int us_ts_count = 0;

  for (b = demo->blocks; b; b = b->next) {
    for (m = b->messages; m; m = m->next) {
      if (m->type == FOUNDSECRET) {
	fs_count++;
      }
      if (m->type == UPDATESTAT) {
	int index = (int) m->data[0];
	int buf;
	memcpy(&buf, m->data + 1, sizeof(buf));
	buf = dtohl(buf);

	if (index == 11) { // total_secrets
	  us_ts_count = buf;
	}
	if (index == 13) { // found_secrets
	  us_fs_count = buf;
	}
      }
    }
  }

  *found = MAX(us_fs_count, fs_count);
  *total = us_ts_count;
}

static void map_info(demo *demo, char **mapname, char **maptitle)
{
  block *b;
  message *m;
  char *name = NULL;
  char *title = NULL;
  
  for (b = demo->blocks; b; b = b->next) {
    for (m = b->messages; m; m = m->next) {
      if (m->type == SERVERINFO) {
	char *pbase = (char *)m->data + 6;
	size_t len;
	// map title
	len = strlen(pbase);
	if (len > 0) {
	  title = strdup(pbase);
	}
	pbase += len + 1;
	// map name
	len = strlen(pbase);
	if (len > 0) {
	  name = strdup(pbase);
	}
	break;
      }
    }
  }

  *mapname = name ? name : strdup("");
  *maptitle = title ? title : strdup("");
}

static void time_info(demo *demo, float *start_time, float *exit_time)
{
  block *b;
  message *m;
  float start = 0.0;
  float exit = 0.0;
  float tthis;
  int first_set = 0;

  for (b = demo->blocks; b; b = b->next) {
    for (m = b->messages; m; m = m->next) {
      if (m->type == TIME) {
	memcpy(&tthis, m->data, m->size);

	if (first_set == 0) {
	  start = dtohl(tthis);
	  first_set = 1;
	}
      }
      if (m->type == INTERMISSION) {
	exit = dtohl(tthis);
      }
    }
  }

  *start_time = start;
  *exit_time = exit;
}
