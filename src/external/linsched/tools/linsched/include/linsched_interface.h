/* LinSched -- The Linux Scheduler Simulator
 * Copyright (C) 2008  John M. Calandrino
 * E-mail: jmc@cs.unc.edu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see COPYING); if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef LINSCHED_INTERFACE_H
#define LINSCHED_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

struct task_sched_info {
    int task_id;
    double run_time;
    double run_delay;
    int pcount;
    int nvcsw;
    int nivcsw;
};

void linsched_setup(int);
void linsched_reset(int numcpus);
int linsched_create_task(double run, double sleep, int core, int niceval);
void linsched_sim(double time);
void linsched_task_info(int task, struct task_sched_info *info);
void linsched_print_info(void);

#ifdef __cplusplus
}
#endif

#endif /* LINSCHED_H */
