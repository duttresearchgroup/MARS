/* sense_tracepoint.c
 *
 * Copyright (C) 2018 David Juhasz <david.juhasz@tuwien.ac.at>
 *
 * Derived from lttng-tracepoints.h: *
 * LTTng adaptation layer for Linux kernel 3.15+ tracepoints.
 *
 * Copyright (C) 2014 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; only
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _SENSE_TRACEPOINT_H
#define _SENSE_TRACEPOINT_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,15,0))

int sense_tracepoint_probe_register(const char *name, void *probe, void *data);
int sense_tracepoint_probe_unregister(const char *name, void *probe, void *data);
int sense_tracepoint_init(void);
void sense_tracepoint_exit(void);

#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3,15,0)) */

#endif /* _SENSE_TRACEPOINT_H */