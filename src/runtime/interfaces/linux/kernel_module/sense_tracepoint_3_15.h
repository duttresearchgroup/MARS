/*
 * sense_tracepoint.c
 *
 * Copyright (C) 2018 David Juhasz <david.juhasz@tuwien.ac.at>
 *
 * Derived from lttng-tracepoints.c:
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


#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/notifier.h>
#include <linux/tracepoint.h>
#include <linux/slab.h>
#include <linux/jhash.h>
#include <linux/module.h>

#include "sense_tracepoint.h"

/*
 * Protect the tracepoint table. sense_tracepoint_mutex nests within
 * kernel/tracepoint.c tp_modlist_mutex. kernel/tracepoint.c
 * tracepoint_mutex nests within sense_tracepoint_mutex.
 */
static
DEFINE_MUTEX(sense_tracepoint_mutex);

#define TRACEPOINT_HASH_BITS 6
#define TRACEPOINT_TABLE_SIZE (1 << TRACEPOINT_HASH_BITS)
static
struct hlist_head tracepoint_table[TRACEPOINT_TABLE_SIZE];

/*
 * The tracepoint entry is the node contained within the hash table. It
 * is a mapping from the "string" key to the struct tracepoint pointer.
 */
struct tracepoint_entry {
	struct hlist_node hlist;
	struct tracepoint *tp;
	int refcount;
	struct list_head probes;
	char name[0];
};

struct sense_tp_probe {
	struct tracepoint_func tp_func;
	struct list_head list;
};

static
int add_probe(struct tracepoint_entry *e, void *probe, void *data)
{
	struct sense_tp_probe *p;
	int found = 0;

	list_for_each_entry(p, &e->probes, list) {
		if (p->tp_func.func == probe && p->tp_func.data == data) {
			found = 1;
			break;
		}
	}
	if (found)
		return -EEXIST;
	p = kmalloc(sizeof(struct sense_tp_probe), GFP_KERNEL);
	if (!p)
		return -ENOMEM;
	p->tp_func.func = probe;
	p->tp_func.data = data;
	list_add(&p->list, &e->probes);
	return 0;
}

static
int remove_probe(struct tracepoint_entry *e, void *probe, void *data)
{
	struct sense_tp_probe *p;
	int found = 0;

	list_for_each_entry(p, &e->probes, list) {
		if (p->tp_func.func == probe && p->tp_func.data == data) {
			found = 1;
			break;
		}
	}
	if (found) {
		list_del(&p->list);
		kfree(p);
		return 0;
	} else {
		WARN_ON(1);
		return -ENOENT;
	}
}

/*
 * Get tracepoint if the tracepoint is present in the tracepoint hash table.
 * Must be called with sense_tracepoint_mutex held.
 * Returns NULL if not present.
 */
static
struct tracepoint_entry *get_tracepoint(const char *name)
{
	struct hlist_head *head;
	struct tracepoint_entry *e;
	u32 hash = jhash(name, strlen(name), 0);

	head = &tracepoint_table[hash & (TRACEPOINT_TABLE_SIZE - 1)];
	hlist_for_each_entry(e, head, hlist) {
		if (!strcmp(name, e->name))
			return e;
	}
	return NULL;
}

/*
 * Add the tracepoint to the tracepoint hash table. Must be called with
 * sense_tracepoint_mutex held.
 */
static
struct tracepoint_entry *add_tracepoint(const char *name)
{
	struct hlist_head *head;
	struct tracepoint_entry *e;
	size_t name_len = strlen(name) + 1;
	u32 hash = jhash(name, name_len - 1, 0);

	head = &tracepoint_table[hash & (TRACEPOINT_TABLE_SIZE - 1)];
	hlist_for_each_entry(e, head, hlist) {
		if (!strcmp(name, e->name)) {
			return ERR_PTR(-EEXIST);        /* Already there */
		}
	}
	/*
	 * Using kmalloc here to allocate a variable length element. Could
	 * cause some memory fragmentation if overused.
	 */
	e = kmalloc(sizeof(struct tracepoint_entry) + name_len, GFP_KERNEL);
	if (!e)
		return ERR_PTR(-ENOMEM);
	memcpy(&e->name[0], name, name_len);
	e->tp = NULL;
	e->refcount = 0;
	INIT_LIST_HEAD(&e->probes);
	hlist_add_head(&e->hlist, head);
	return e;
}

/*
 * Remove the tracepoint from the tracepoint hash table. Must be called
 * with sense_tracepoint_mutex held.
 */
static
void remove_tracepoint(struct tracepoint_entry *e)
{
	hlist_del(&e->hlist);
	kfree(e);
}

int sense_tracepoint_probe_register(const char *name, void *probe, void *data)
{
	struct tracepoint_entry *e;
	int ret = 0;

	mutex_lock(&sense_tracepoint_mutex);
	e = get_tracepoint(name);
	if (!e) {
		e = add_tracepoint(name);
		if (IS_ERR(e)) {
			ret = PTR_ERR(e);
			goto end;
		}
	}
	/* add (probe, data) to entry */
	ret = add_probe(e, probe, data);
	if (ret)
		goto end;
	e->refcount++;
	if (e->tp) {
		ret = tracepoint_probe_register(e->tp, probe, data);
		WARN_ON_ONCE(ret);
		ret = 0;
	}
end:
	mutex_unlock(&sense_tracepoint_mutex);
	return ret;
}

int sense_tracepoint_probe_unregister(const char *name, void *probe, void *data)
{
	struct tracepoint_entry *e;
	int ret = 0;

	mutex_lock(&sense_tracepoint_mutex);
	e = get_tracepoint(name);
	if (!e) {
		ret = -ENOENT;
		goto end;
	}
	/* remove (probe, data) from entry */
	ret = remove_probe(e, probe, data);
	if (ret)
		goto end;
	if (e->tp) {
		ret = tracepoint_probe_unregister(e->tp, probe, data);
		WARN_ON_ONCE(ret);
		ret = 0;
	}
	if (!--e->refcount)
		remove_tracepoint(e);
end:
	mutex_unlock(&sense_tracepoint_mutex);
	return ret;
}

#ifdef CONFIG_MODULES

static
int sense_tracepoint_coming(struct tp_module *tp_mod)
{
	int i;

	mutex_lock(&sense_tracepoint_mutex);
	for (i = 0; i < tp_mod->mod->num_tracepoints; i++) {
		struct tracepoint *tp;
		struct tracepoint_entry *e;
		struct sense_tp_probe *p;

		tp = tp_mod->mod->tracepoints_ptrs[i];
		e = get_tracepoint(tp->name);
		if (!e) {
			e = add_tracepoint(tp->name);
			if (IS_ERR(e)) {
				pr_warn("LTTng: error (%ld) adding tracepoint\n",
					PTR_ERR(e));
				continue;
			}
		}
		/* If already enabled, just check consistency */
		if (e->tp) {
			WARN_ON(e->tp != tp);
			continue;
		}
		e->tp = tp;
		e->refcount++;
		/* register each (probe, data) */
		list_for_each_entry(p, &e->probes, list) {
			int ret;

			ret = tracepoint_probe_register(e->tp,
					p->tp_func.func, p->tp_func.data);
			WARN_ON_ONCE(ret);
		}
	}
	mutex_unlock(&sense_tracepoint_mutex);
	return 0;
}

static
int sense_tracepoint_going(struct tp_module *tp_mod)
{
	int i;

	mutex_lock(&sense_tracepoint_mutex);
	for (i = 0; i < tp_mod->mod->num_tracepoints; i++) {
		struct tracepoint *tp;
		struct tracepoint_entry *e;
		struct sense_tp_probe *p;

		tp = tp_mod->mod->tracepoints_ptrs[i];
		e = get_tracepoint(tp->name);
		if (!e || !e->tp)
			continue;
		/* unregister each (probe, data) */
		list_for_each_entry(p, &e->probes, list) {
			int ret;

			ret = tracepoint_probe_unregister(e->tp,
					p->tp_func.func, p->tp_func.data);
			WARN_ON_ONCE(ret);
		}
		e->tp = NULL;
		if (!--e->refcount)
			remove_tracepoint(e);
	}
	mutex_unlock(&sense_tracepoint_mutex);
	return 0;
}

static
int sense_tracepoint_notify(struct notifier_block *self,
		unsigned long val, void *data)
{
	struct tp_module *tp_mod = data;
	int ret = 0;

	switch (val) {
	case MODULE_STATE_COMING:
		ret = sense_tracepoint_coming(tp_mod);
		break;
	case MODULE_STATE_GOING:
		ret = sense_tracepoint_going(tp_mod);
		break;
	default:
		break;
	}
	return ret;
}

static
struct notifier_block sense_tracepoint_notifier = {
	.notifier_call = sense_tracepoint_notify,
	.priority = 0,
};

static
int sense_tracepoint_module_init(void)
{
	return register_tracepoint_module_notifier(&sense_tracepoint_notifier);
}

static
void sense_tracepoint_module_exit(void)
{
	(void)unregister_tracepoint_module_notifier(&sense_tracepoint_notifier);
}

#else /* #ifdef CONFIG_MODULES */

static
int sense_tracepoint_module_init(void)
{
	return 0;
}

static
void sense_tracepoint_module_exit(void)
{
}

#endif /* #else #ifdef CONFIG_MODULES */

static
void sense_kernel_tracepoint_add(struct tracepoint *tp, void *priv)
{
	struct tracepoint_entry *e;
	struct sense_tp_probe *p;
	int *ret = priv;

	mutex_lock(&sense_tracepoint_mutex);
	e = get_tracepoint(tp->name);
	if (!e) {
		e = add_tracepoint(tp->name);
		if (IS_ERR(e)) {
			pr_warn("LTTng: error (%ld) adding tracepoint\n",
				PTR_ERR(e));
			*ret = (int) PTR_ERR(e);
			goto end;
		}
	}
	/* If already enabled, just check consistency */
	if (e->tp) {
		WARN_ON(e->tp != tp);
		goto end;
	}
	e->tp = tp;
	e->refcount++;
	/* register each (probe, data) */
	list_for_each_entry(p, &e->probes, list) {
		int ret;

		ret = tracepoint_probe_register(e->tp,
				p->tp_func.func, p->tp_func.data);
		WARN_ON_ONCE(ret);
	}
end:
	mutex_unlock(&sense_tracepoint_mutex);
}

static
void sense_kernel_tracepoint_remove(struct tracepoint *tp, void *priv)
{
	struct tracepoint_entry *e;
	int *ret = priv;

	mutex_lock(&sense_tracepoint_mutex);
	e = get_tracepoint(tp->name);
	if (!e || e->refcount != 1 || !list_empty(&e->probes)) {
		*ret = -EINVAL;
		goto end;
	}
	remove_tracepoint(e);
end:
	mutex_unlock(&sense_tracepoint_mutex);
}

int sense_tracepoint_init(void)
{
	int error_ret = 0;
	int ret = 0;

	for_each_kernel_tracepoint(sense_kernel_tracepoint_add, &ret);
	if (!ret) {
		ret = sense_tracepoint_module_init();
		if (ret) {
			for_each_kernel_tracepoint(sense_kernel_tracepoint_remove, &error_ret);
			WARN_ON(error_ret);
		}
	}
	return ret;
}

void sense_tracepoint_exit(void)
{
	int i, ret = 0;

	sense_tracepoint_module_exit();
	for_each_kernel_tracepoint(sense_kernel_tracepoint_remove, &ret);
	WARN_ON(ret);
	mutex_lock(&sense_tracepoint_mutex);
	for (i = 0; i < TRACEPOINT_TABLE_SIZE; i++) {
		struct hlist_head *head = &tracepoint_table[i];

		/* All tracepoints should be removed */
		WARN_ON(!hlist_empty(head));
	}
	mutex_unlock(&sense_tracepoint_mutex);
}