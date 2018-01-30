/*
 * Displays linux /proc/pid/stat in human-readable format
 *
 * Build: gcc -o procstat procstat.c
 * Usage: procstat pid
 *        cat /proc/pid/stat | procstat
 *
 * Homepage: http://www.brokestream.com/procstat.html
 * Version : 2009-03-05
 *
 * Ivan Tikhonov, http://www.brokestream.com, kefeer@netangels.ru
 *
 * 2007-09-19 changed HZ=100 error to warning
 *
 * 2009-03-05 tickspersec are taken from sysconf (Sabuj Pattanayek)
 *
 */


/* Copyright (C) 2009 Ivan Tikhonov

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Ivan Tikhonov, kefeer@brokestream.com

*/

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <linux/limits.h>
#include <sys/times.h>
#include <sstream>
#include <iostream>
#include <cassert>

#include "proc_stat.h"


static void readone(FILE *input, proc_stat_data::num *x) { fscanf(input, "%lld ", x); }
static void readunsigned(FILE *input, unsigned long long *x) { fscanf(input, "%llu ", x); }
static void readstr(FILE *input, char *x) {  fscanf(input, "%s ", x);}
static void readchar(FILE *input, char *x) {  fscanf(input, "%c ", x);}

static void printone(const std::string &name, proc_stat_data::num x) {  printf("%20s: %lld\n", name.c_str(), x);}
static void printonex(const std::string &name, proc_stat_data::num x) {  printf("%20s: %016llx\n", name.c_str(), x);}
//static void printunsigned(const std::string &name, unsigned long long x) {  printf("%20s: %llu\n", name.c_str(), x);}
static void printchar(const std::string &name, char x) {  printf("%20s: %c\n", name.c_str(), x);}
static void printstr(const std::string &name, char *x) {  printf("%20s: %s\n", name.c_str(), x);}
static void printtime(const std::string &name, proc_stat_data::num x, long tickspersec) {  printf("%20s: %f\n", name.c_str(), (((double)x) / tickspersec));}


void read_proc_pid_stats(long int pid, long int tid, proc_stat_data *data)
{
	std::stringstream path;
	path << "/proc/" << pid << "/task/" << tid << "/stat";

	FILE *input = fopen(path.str().c_str(), "r");
	assert(input);

	readone(input,&(data->pid));
	readstr(input,data->tcomm);
	readchar(input,&(data->state));
	readone(input,&(data->ppid));
	readone(input,&(data->pgid));
	readone(input,&(data->sid));
	readone(input,&(data->tty_nr));
	readone(input,&(data->tty_pgrp));
	readone(input,&(data->flags));
	readone(input,&(data->min_flt));
	readone(input,&(data->cmin_flt));
	readone(input,&(data->maj_flt));
	readone(input,&(data->cmaj_flt));
	readone(input,&(data->utime));
	readone(input,&(data->stimev));
	readone(input,&(data->cutime));
	readone(input,&(data->cstime));
	readone(input,&(data->priority));
	readone(input,&(data->nicev));
	readone(input,&(data->num_threads));
	readone(input,&(data->it_real_value));
	readunsigned(input,&(data->start_time));
	readone(input,&(data->vsize));
	readone(input,&(data->rss));
	readone(input,&(data->rsslim));
	readone(input,&(data->start_code));
	readone(input,&(data->end_code));
	readone(input,&(data->start_stack));
	readone(input,&(data->esp));
	readone(input,&(data->eip));
	readone(input,&(data->pending));
	readone(input,&(data->blocked));
	readone(input,&(data->sigign));
	readone(input,&(data->sigcatch));
	readone(input,&(data->wchan));
	readone(input,&(data->zero1));
	readone(input,&(data->zero2));
	readone(input,&(data->exit_signal));
	readone(input,&(data->cpu));
	readone(input,&(data->rt_priority));
	readone(input,&(data->policy));

	data->ticks_per_sec = sysconf(_SC_CLK_TCK);

	fclose(input);

}

void print_proc_pid_stats(proc_stat_data *data)
{

	printone("pid", data->pid);
	printstr("tcomm", data->tcomm);
	printchar("state", data->state);
	printone("ppid", data->ppid);
	printone("pgid", data->pgid);
	printone("sid", data->sid);
	printone("tty_nr", data->tty_nr);
	printone("tty_pgrp", data->tty_pgrp);
	printone("flags", data->flags);
	printone("min_flt", data->min_flt);
	printone("cmin_flt", data->cmin_flt);
	printone("maj_flt", data->maj_flt);
	printone("cmaj_flt", data->cmaj_flt);
	printtime("utime", data->utime, data->ticks_per_sec);
	printtime("stime", data->stimev, data->ticks_per_sec);
	printtime("cutime", data->cutime, data->ticks_per_sec);
	printtime("cstime", data->cstime, data->ticks_per_sec);
	printone("priority", data->priority);
	printone("nice", data->nicev);
	printone("num_threads", data->num_threads);
	printtime("it_real_value", data->it_real_value, data->ticks_per_sec);
	//printtimediff("start_time", data->start_time, data->ticks_per_sec);
	printtime("start_time", data->start_time, data->ticks_per_sec);
	printone("vsize", data->vsize);
	printone("rss", data->rss);
	printone("rsslim", data->rsslim);
	printone("start_code", data->start_code);
	printone("end_code", data->end_code);
	printone("start_stack", data->start_stack);
	printone("esp", data->esp);
	printone("eip", data->eip);
	printonex("pending", data->pending);
	printonex("blocked", data->blocked);
	printonex("sigign", data->sigign);
	printonex("sigcatch", data->sigcatch);
	printone("wchan", data->wchan);
	printone("zero1", data->zero1);
	printone("zero2", data->zero2);
	printonex("exit_signal", data->exit_signal);
	printone("cpu", data->cpu);
	printone("rt_priority", data->rt_priority);
	printone("policy", data->policy);
}
