/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef __PROC_STAT_H
#define __PROC_STAT_H

struct proc_stat_data {
	typedef long long int num;

	num pid;
	char tcomm[PATH_MAX];
	char state;

	num ppid;
	num pgid;
	num sid;
	num tty_nr;
	num tty_pgrp;

	num flags;
	num min_flt;
	num cmin_flt;
	num maj_flt;
	num cmaj_flt;
	num utime;
	num stimev;

	num cutime;
	num cstime;
	num priority;
	num nicev;
	num num_threads;
	num it_real_value;

	unsigned long long start_time;

	num vsize;
	num rss;
	num rsslim;
	num start_code;
	num end_code;
	num start_stack;
	num esp;
	num eip;

	num pending;
	num blocked;
	num sigign;
	num sigcatch;
	num wchan;
	num zero1;
	num zero2;
	num exit_signal;
	num cpu;
	num rt_priority;
	num policy;

	long ticks_per_sec;
};

void read_proc_pid_stats(long int pid, long int tid, proc_stat_data *data);

void print_proc_pid_stats(proc_stat_data *data);

#endif
