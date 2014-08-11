/*
 * Copyright (c) 2013 Intel Corporation. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenFabrics.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "psmx.h"

void psmx_cntr_check_trigger(struct psmx_fid_cntr *cntr)
{
	struct psmx_trigger *trigger;

	/* TODO: protect the trigger list with mutex */

	if (!cntr->trigger)
		return;

	trigger = cntr->trigger;
	while (trigger) {
		if (cntr->counter < trigger->threshold)
			break;

		cntr->trigger = trigger->next;
		switch (trigger->op) {
		case PSMX_TRIGGERED_SEND:
			_psmx_sendto(trigger->send.ep,
				     trigger->send.buf,
				     trigger->send.len,
				     trigger->send.desc,
				     trigger->send.dest_addr,
				     trigger->send.context,
				     trigger->send.flags);
			break;
		case PSMX_TRIGGERED_RECV:
			_psmx_recvfrom(trigger->recv.ep,
				       trigger->recv.buf,
				       trigger->recv.len,
				       trigger->recv.desc,
				       trigger->recv.src_addr,
				       trigger->recv.context,
				       trigger->recv.flags,
				       trigger->recv.data);
			break;
		case PSMX_TRIGGERED_TSEND:
			_psmx_tagged_sendto(trigger->tsend.ep,
					    trigger->tsend.buf,
					    trigger->tsend.len,
					    trigger->tsend.desc,
					    trigger->tsend.dest_addr,
					    trigger->tsend.tag,
					    trigger->tsend.context,
					    trigger->tsend.flags);
			break;
		case PSMX_TRIGGERED_TRECV:
			_psmx_tagged_recvfrom(trigger->trecv.ep,
					      trigger->trecv.buf,
					      trigger->trecv.len,
					      trigger->trecv.desc,
					      trigger->trecv.src_addr,
					      trigger->trecv.tag,
					      trigger->trecv.ignore,
					      trigger->trecv.context,
					      trigger->trecv.flags);
			break;
		default:
			psmx_debug("%s: %d unsupported op\n", __func__, trigger->op);
			break;
		}

		free(trigger);
	}
}

void psmx_cntr_add_trigger(struct psmx_fid_cntr *cntr, struct psmx_trigger *trigger)
{
	struct psmx_trigger *p, *q;

	/* TODO: protect the trigger list with mutex */

	q = NULL;
	p = cntr->trigger;
	while (p && p->threshold <= trigger->threshold) {
		q = p;
		p = p->next;
	}
	if (q)
		q->next = trigger;
	else
		cntr->trigger = trigger;
	trigger->next = p;
	psmx_cntr_check_trigger(cntr);
}

static uint64_t psmx_cntr_read(struct fid_cntr *cntr)
{
	struct psmx_fid_cntr *fid_cntr;

	fid_cntr = container_of(cntr, struct psmx_fid_cntr, cntr);

	return fid_cntr->counter;
}

static int psmx_cntr_add(struct fid_cntr *cntr, uint64_t value)
{
	struct psmx_fid_cntr *fid_cntr;

	fid_cntr = container_of(cntr, struct psmx_fid_cntr, cntr);
	fid_cntr->counter += value;

	psmx_cntr_check_trigger(fid_cntr);

	if (fid_cntr->wait_obj == FI_WAIT_MUT_COND)
		pthread_cond_signal(&fid_cntr->cond);

	return 0;
}

static int psmx_cntr_set(struct fid_cntr *cntr, uint64_t value)
{
	struct psmx_fid_cntr *fid_cntr;

	fid_cntr = container_of(cntr, struct psmx_fid_cntr, cntr);
	fid_cntr->counter = value;

	psmx_cntr_check_trigger(fid_cntr);

	if (fid_cntr->wait_obj == FI_WAIT_MUT_COND)
		pthread_cond_signal(&fid_cntr->cond);

	return 0;
}

static int psmx_cntr_wait(struct fid_cntr *cntr, uint64_t threshold)
{
	struct psmx_fid_cntr *fid_cntr;

	fid_cntr = container_of(cntr, struct psmx_fid_cntr, cntr);

	switch (fid_cntr->wait_obj) {
	case FI_WAIT_NONE:
		while (fid_cntr->counter < threshold)
			psmx_eq_poll_mq(NULL, fid_cntr->domain);
		break;

	case FI_WAIT_MUT_COND:
		pthread_mutex_lock(&fid_cntr->mutex);
		while (fid_cntr->counter < threshold)
			pthread_cond_wait(&fid_cntr->cond, &fid_cntr->mutex);
		pthread_mutex_unlock(&fid_cntr->mutex);
		break;

	default:
		return -EBADF;
	}

	return 0;
}

static int psmx_cntr_close(fid_t fid)
{
	struct psmx_fid_cntr *fid_cntr;

	fid_cntr = container_of(fid, struct psmx_fid_cntr, cntr.fid);
	free(fid_cntr);

	return 0;
}

static int psmx_cntr_bind(struct fid *fid, struct fid *bfid, uint64_t flags)
{
	return -ENOSYS;
}

static int psmx_cntr_sync(fid_t fid, uint64_t flags, void *context)
{
	return -ENOSYS;
}

static int psmx_cntr_control(fid_t fid, int command, void *arg)
{
	struct psmx_fid_cntr *fid_cntr;

	fid_cntr = container_of(fid, struct psmx_fid_cntr, cntr.fid);

	switch (command) {
	case FI_SETOPSFLAG:
		fid_cntr->flags = *(uint64_t *)arg;
		break;

	case FI_GETOPSFLAG:
		if (!arg)
			return -EINVAL;
		*(uint64_t *)arg = fid_cntr->flags;
		break;

	case FI_GETWAIT:
		if (!arg)
			return -EINVAL;
		((void **)arg)[0] = &fid_cntr->mutex;
		((void **)arg)[1] = &fid_cntr->cond;
		break;

	default:
		return -ENOSYS;
	}

	return 0;
}

static struct fi_ops psmx_fi_ops = {
	.close = psmx_cntr_close,
	.bind = psmx_cntr_bind,
	.sync = psmx_cntr_sync,
	.control = psmx_cntr_control,
};

static struct fi_ops_cntr psmx_cntr_ops = {
	.read = psmx_cntr_read,
	.add = psmx_cntr_add,
	.set = psmx_cntr_set,
	.wait = psmx_cntr_wait,
};

int psmx_cntr_open(struct fid_domain *domain, struct fi_cntr_attr *attr,
			struct fid_cntr **cntr, void *context)
{
	struct psmx_fid_domain *fid_domain;
	struct psmx_fid_cntr *fid_cntr;
	int events , wait_obj;
	uint64_t flags;

	events = FI_CNTR_EVENTS_COMP;
	wait_obj = FI_WAIT_NONE;
	flags = 0;

	if (attr->mask & FI_CNTR_ATTR_EVENTS) {
		switch (attr->events) {
		case FI_CNTR_EVENTS_COMP:
			events = attr->events;
			break;

		default:
			psmx_debug("%s: attr->events=%d, supported=%d\n", __func__,
					attr->events, FI_CNTR_EVENTS_COMP);
			return -EINVAL;
		}
	}

	if (attr->mask & FI_CNTR_ATTR_WAIT_OBJ) {
		switch (attr->wait_obj) {
		case FI_WAIT_NONE:
		case FI_WAIT_MUT_COND:
			wait_obj = attr->wait_obj;
			break;

		default:
			psmx_debug("%s: attr->wait_obj=%d, supported=%d,%d\n", __func__,
					attr->wait_obj, FI_WAIT_NONE, FI_WAIT_MUT_COND);
			return -EINVAL;
		}
	}

	fid_domain = container_of(domain, struct psmx_fid_domain, domain);
	fid_cntr = (struct psmx_fid_cntr *) calloc(1, sizeof *fid_cntr);
	if (!fid_cntr)
		return -ENOMEM;

	fid_cntr->domain = fid_domain;
	fid_cntr->events = events;
	fid_cntr->wait_obj = wait_obj;
	fid_cntr->flags = flags;
	fid_cntr->cntr.fid.fclass = FID_CLASS_CNTR;
	fid_cntr->cntr.fid.context = context;
	fid_cntr->cntr.fid.ops = &psmx_fi_ops;
	fid_cntr->cntr.ops = &psmx_cntr_ops;

	if (wait_obj == FI_WAIT_MUT_COND) {
		pthread_mutex_init(&fid_cntr->mutex, NULL);
		pthread_cond_init(&fid_cntr->cond, NULL);
	}

	*cntr = &fid_cntr->cntr;
	return 0;
}

