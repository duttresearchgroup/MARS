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

#include "../linux-module/helpers.h"


#include <linux/cpufreq.h>

#include "../linux-module/core.h"


bool vitamins_change_cpu(struct task_struct *kern_tsk, int next_cpu){
    cpumask_t mask;

    cpus_clear(mask);
    cpu_set(next_cpu, mask);

    return set_cpus_allowed_ptr(kern_tsk,&mask);
}

int kern_cpu_get_freq_mhz(int core)
{
    struct cpufreq_policy *policy;
    int freq;

    policy = cpufreq_cpu_get(core);

    if (!policy) return 0;

    freq = policy->cur;
    cpufreq_cpu_put(policy);

    return freq / 1000;
}


bool kern_cpu_set_freq_mhz(int core, int freq)
{
    struct cpufreq_policy *policy;

    policy = cpufreq_cpu_get(core);
    if (!policy) return false;

    policy->governor->store_setspeed(policy, freq * 1000);

    cpufreq_cpu_put(policy);

    return true;
}

bool kern_cpu_freq_isuserspace(int core)
{
    struct cpufreq_policy *policy;
    bool userspace = true;
    policy = cpufreq_cpu_get(core);
    if (!policy) {
        pinfo("DS: ERROR!! cannot get cpu policy dsUS\n");
        return false;
    }

    pinfo("Core %d governor is %s \n",core,policy->governor->name);

    cpufreq_cpu_put(policy);

    return userspace;
}


int circbuf_push(circbuf_t *c, circbuf_data_t data)
{
    int next;

    spin_lock(c->lock);

    next = c->head + 1;
    if (next >= c->maxLen)
        next = 0;

    // Cicular buffer is full
    if (next == c->tail){
    	spin_unlock(c->lock);
        return -1;  // quit with an error
    }

    c->buffer[c->head] = data;
    c->head = next;

    spin_unlock(c->lock);

    //notify readers
    up(c->rd_sem);

    return 0;
}

int circbuf_pop(circbuf_t *c, circbuf_data_t *data)
{
	int next;

	//wait for data
	down(c->rd_sem);

	spin_lock(c->lock);

	// if the head isn't ahead of the tail, we don't have any characters, which is an invalid state here
    BUG_ON(c->head == c->tail);

    *data = c->buffer[c->tail];
    c->buffer[c->tail] = 0;  // clear the data (optional)

    next = c->tail + 1;
    if(next >= c->maxLen)
        next = 0;

    c->tail = next;

    spin_unlock(c->lock);

    return 0;
}
