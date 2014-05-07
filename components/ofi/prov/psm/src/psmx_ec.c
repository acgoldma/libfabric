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

void psmx_eq_enqueue_event(struct psmx_fid_eq *eq,
				struct psmx_event *event)
{
	if (eq->event_queue.tail) {
		eq->event_queue.tail->next = event;
		eq->event_queue.tail = event;
	}
	else {
		eq->event_queue.head = eq->event_queue.tail = event;
	}
}

static struct psmx_event *psmx_eq_dequeue_event(struct psmx_fid_eq *eq)
{
	struct psmx_event *event;

	if (!eq->event_queue.head)
		return NULL;

	event = eq->event_queue.head;
	eq->event_queue.head = event->next;
	if (!eq->event_queue.head)
		eq->event_queue.tail = NULL;

	event->next = NULL;
	return event;
}

struct psmx_event *psmx_eq_create_event(struct psmx_fid_eq *eq,
					void *op_context, void *buf,
					uint64_t flags, size_t len,
					uint64_t data, uint64_t tag,
					size_t olen, int err)
{
	struct psmx_event *event;

	event = calloc(1, sizeof(*event));
	if (!event) {
		fprintf(stderr, "%s: out of memory\n", __func__);
		return NULL;
	}

	if (err) {
		event->format = eq->err_format;
		eq->num_errors++;
	}
	else {
		event->format = eq->format;
		eq->num_events++;
	}

	switch (event->format) {
	case FI_EQ_FORMAT_CONTEXT:
		event->eqe.context.op_context = op_context;
		break;

	case FI_EQ_FORMAT_COMP:
		event->eqe.comp.op_context = op_context;
		event->eqe.comp.flags = flags;
		event->eqe.comp.len = len;
		break;

	case FI_EQ_FORMAT_DATA:
		event->eqe.data.op_context = op_context;
		event->eqe.data.buf = buf;
		event->eqe.data.flags = flags;
		event->eqe.data.len = len;
		event->eqe.data.data = data;
		break;

	case FI_EQ_FORMAT_TAGGED:
		event->eqe.tagged.op_context = op_context;
		event->eqe.tagged.buf = buf;
		event->eqe.tagged.flags = flags;
		event->eqe.tagged.len = len;
		event->eqe.tagged.data = data;
		event->eqe.tagged.tag = tag;
		break;

	case FI_EQ_FORMAT_ERR:
		event->eqe.err.op_context = op_context;
		event->eqe.err.err = err;
		event->eqe.err.prov_errno = 0;
		event->eqe.err.prov_data = NULL;
		break;

	case FI_EQ_FORMAT_TAGGED_ERR:
		if (err) {
			event->eqe.tagged_err.status = err;
			event->eqe.tagged_err.err.op_context = op_context;
			event->eqe.tagged_err.err.fid_context = eq->eq.fid.context;
			event->eqe.tagged_err.err.flags = flags;
			event->eqe.tagged_err.err.len = len;
			event->eqe.tagged_err.err.data = data;
			event->eqe.tagged_err.err.err = err;
			event->eqe.tagged_err.err.prov_errno = 0;
			event->eqe.tagged_err.err.prov_data = NULL;
		}
		else {
			event->eqe.tagged_err.status = 0;
			event->eqe.tagged_err.tagged.op_context = op_context;
			event->eqe.tagged_err.tagged.buf = buf;
			event->eqe.tagged_err.tagged.flags = flags;
			event->eqe.tagged_err.tagged.len = len;
			event->eqe.tagged_err.tagged.data = data;
			event->eqe.tagged_err.tagged.tag = tag;
		}
		break;

	case FI_EQ_FORMAT_CM:
	default:
		fprintf(stderr, "%s: unsupported EC format %d\n", __func__, event->format);
		return NULL;
	}

	return event;
}

static struct psmx_event *psmx_eq_create_event_from_status(
				struct psmx_fid_eq *eq,
				psm_mq_status_t *psm_status)
{
	struct psmx_event *event;
	struct fi_context *fi_context = psm_status->context;
	int err;

	event = calloc(1, sizeof(*event));
	if (!event) {
		fprintf(stderr, "%s: out of memory\n", __func__);
		return NULL;
	}

	if (psm_status->error_code) {
		event->format = eq->err_format;
		eq->num_errors++;
	}
	else {
		event->format = eq->format;
		eq->num_events++;
	}

	switch (event->format) {
	case FI_EQ_FORMAT_CONTEXT:
		event->eqe.context.op_context = PSMX_CTXT_USER(fi_context);
		break;

	case FI_EQ_FORMAT_COMP:
		event->eqe.comp.op_context = PSMX_CTXT_USER(fi_context);
		//event->eqe.comp.flags = 0; /* FIXME */
		event->eqe.comp.len = psm_status->nbytes;
		break;

	case FI_EQ_FORMAT_DATA:
		event->eqe.data.op_context = PSMX_CTXT_USER(fi_context);
		//event->eqe.data.buf = NULL; /* FIXME */
		//event->eqe.data.flags = 0; /* FIXME */
		event->eqe.data.len = psm_status->nbytes;
		//event->eqe.data.data = 0; /* FIXME */
		break;

	case FI_EQ_FORMAT_TAGGED:
		event->eqe.tagged.op_context = PSMX_CTXT_USER(fi_context);
		//event->eqe.tagged.buf = NULL; /* FIXME */
		//event->eqe.tagged.flags = 0; /* FIXME */
		event->eqe.tagged.len = psm_status->nbytes;
		//event->eqe.tagged.data = 0; /* FIXME */
		event->eqe.tagged.tag = psm_status->msg_tag;
		break;

	case FI_EQ_FORMAT_ERR:
		event->eqe.err.op_context = PSMX_CTXT_USER(fi_context);
		event->eqe.err.err = psmx_errno(psm_status->error_code);
		event->eqe.err.prov_errno = psm_status->error_code;
		event->eqe.err.olen = psm_status->msg_length - psm_status->nbytes;
		//event->eqe.err.prov_data = NULL; /* FIXME */
		break;

	case FI_EQ_FORMAT_TAGGED_ERR:
		err = psmx_errno(psm_status->error_code);
		if (err) {
			event->eqe.tagged_err.status = err;
			event->eqe.tagged_err.err.op_context = PSMX_CTXT_USER(fi_context);
			event->eqe.tagged_err.err.fid_context = eq->eq.fid.context;
			//event->eqe.tagged_err.err.flags = 0; /* FIXME */
			event->eqe.tagged_err.err.len = psm_status->nbytes;
			event->eqe.tagged_err.err.olen = psm_status->msg_length -
							 psm_status->nbytes;
			//event->eqe.tagged_err.err.data = 0; /* FIXME */
			event->eqe.tagged_err.err.err = err;
			event->eqe.tagged_err.err.prov_errno = psm_status->error_code;
			//event->eqe.tagged_err.err.prov_data = NULL; /* FIXME */
		}
		else {
			event->eqe.tagged_err.status = 0;
			event->eqe.tagged_err.tagged.op_context = PSMX_CTXT_USER(fi_context);
			//event->eqe.tagged_err.tagged.buf = NULL; /* FIXME */
			//event->eqe.tagged_err.tagged.flags = 0; /* FIXME */
			event->eqe.tagged_err.tagged.len = psm_status->nbytes;
			//event->eqe.tagged_err.tagged.data = 0; /* FIXME */
			event->eqe.tagged_err.tagged.tag = psm_status->msg_tag;
		}
		break;

	case FI_EQ_FORMAT_CM:
	default:
		fprintf(stderr, "%s: unsupported EC format %d %d %d\n", __func__, event->format, eq->format, eq->err_format);
		return NULL;
	}

	event->source = psm_status->msg_tag;

	return event;
}

static int psmx_eq_get_event_src_addr(struct psmx_fid_eq *fid_eq,
					struct psmx_event *event,
					void *src_addr, size_t *addrlen)
{
	int err;

	if (!src_addr)
		return 0;

	if ((fid_eq->domain->reserved_tag_bits & PSMX_MSG_BIT) &&
		(event->source & PSMX_MSG_BIT)) {
		err = psmx_epid_to_epaddr(
			fid_eq->domain->psm_ep,
			event->source & ~PSMX_MSG_BIT,
			src_addr);
		*addrlen = sizeof(psm_epaddr_t);
	}

	return 0;
}

static int psmx_eq_poll_mq(struct psmx_fid_eq *eq)
{
	psm_mq_req_t psm_req;
	psm_mq_status_t psm_status;
	struct fi_context *fi_context;
	struct psmx_fid_eq *tmp_ec;
	struct psmx_event *event;
	int err;

	while (1) {
		err = psm_mq_ipeek(eq->domain->psm_mq, &psm_req, NULL);

		if (err == PSM_OK) {
			err = psm_mq_test(&psm_req, &psm_status);

			fi_context = psm_status.context;

			if (!fi_context) /* only possible with FI_SYNC set */
				continue;

			if (PSMX_CTXT_TYPE(fi_context) == PSMX_NOCOMP_CONTEXT)
				continue;

			tmp_ec = PSMX_CTXT_EC(fi_context);
			event = psmx_eq_create_event_from_status(tmp_ec, &psm_status);
			if (!event)
				return -ENOMEM;

			psmx_eq_enqueue_event(tmp_ec, event);

			if (tmp_ec == eq)
				return 1;
		}
		else if (err == PSM_MQ_NO_COMPLETIONS) {
			return 0;
		}
		else {
			return psmx_errno(err);
		}
	}
}

static ssize_t psmx_eq_readfrom(struct fid_eq *eq, void *buf, size_t len,
				void *src_addr, size_t *addrlen)
{
	struct psmx_fid_eq *fid_eq;
	struct psmx_event *event;

	fid_eq = container_of(eq, struct psmx_fid_eq, eq);
	assert(fid_eq->domain);

	if (len < fid_eq->entry_size)
		return -FI_ETOOSMALL;

	psmx_eq_poll_mq(fid_eq);

	if (fid_eq->pending_error)
		return -FI_EAVAIL;

	event = psmx_eq_dequeue_event(fid_eq);
	if (event) {
		if (event->format == fid_eq->format) {
			memcpy(buf, (void *)&event->eqe, fid_eq->entry_size);
			psmx_eq_get_event_src_addr(fid_eq, event, src_addr, addrlen);
			free(event);
			return fid_eq->entry_size;
		}
		else {
			fid_eq->pending_error = event;
			return -FI_EAVAIL;
		}
	}

	return 0;
}

static ssize_t psmx_eq_read(struct fid_eq *eq, void *buf, size_t len)
{
	return psmx_eq_readfrom(eq, buf, len, NULL, NULL);
}

static ssize_t psmx_eq_readerr(struct fid_eq *eq, void *buf, size_t len, uint64_t flags)
{
	struct psmx_fid_eq *fid_eq;

	fid_eq = container_of(eq, struct psmx_fid_eq, eq);

	if (len < fid_eq->err_entry_size)
		return -FI_ETOOSMALL;

	if (fid_eq->pending_error) {
		memcpy(buf, &fid_eq->pending_error->eqe, fid_eq->err_entry_size);
		free(fid_eq->pending_error);
		fid_eq->pending_error = NULL;
		return fid_eq->err_entry_size;
	}

	return 0;
}

static ssize_t psmx_eq_write(struct fid_eq *eq, const void *buf, size_t len)
{
	return -ENOSYS;
}

static int psmx_eq_reset(struct fid_eq *eq, const void *cond)
{
	return -ENOSYS;
}

static ssize_t psmx_eq_condread(struct fid_eq *eq, void *buf, size_t len, const void *cond)
{
	return -ENOSYS;
}

static ssize_t psmx_eq_condreadfrom(struct fid_eq *eq, void *buf, size_t len,
				    void *src_addr, size_t *addrlen, const void *cond)
{
	return -ENOSYS;
}

static const char *psmx_eq_strerror(struct fid_eq *eq, int prov_errno, const void *prov_data,
				    void *buf, size_t len)
{
	return psm_error_get_string(prov_errno);
}

static int psmx_eq_close(fid_t fid)
{
	struct psmx_fid_eq *fid_eq;

	fid_eq = container_of(fid, struct psmx_fid_eq, eq.fid);
	free(fid_eq);

	return 0;
}

static int psmx_eq_bind(fid_t fid, struct fi_resource *fids, int nfids)
{
	struct fi_resource ress;
	int err;
	int i;

	for (i=0; i<nfids; i++) {
		if (!fids[i].fid)
			return -EINVAL;
		switch (fids[i].fid->fclass) {
		case FID_CLASS_EP:
		case FID_CLASS_MR:
			if (!fids[i].fid->ops || !fids[i].fid->ops->bind)
				return -EINVAL;
			ress.fid = fid;
			ress.flags = fids[i].flags;
			err = fids[i].fid->ops->bind(fids[i].fid, &ress, 1);
			if (err)
				return err;
			break;

		default:
			return -ENOSYS;
		}
	}
	return 0;
}

static int psmx_eq_sync(fid_t fid, uint64_t flags, void *context)
{
	return -ENOSYS;
}

static int psmx_eq_control(fid_t fid, int command, void *arg)
{
	return -ENOSYS;
}

static struct fi_ops psmx_fi_ops = {
	.size = sizeof(struct fi_ops),
	.close = psmx_eq_close,
	.bind = psmx_eq_bind,
	.sync = psmx_eq_sync,
	.control = psmx_eq_control,
};

static struct fi_ops_eq psmx_eq_ops = {
	.size = sizeof(struct fi_ops_eq),
	.read = psmx_eq_read,
	.readfrom = psmx_eq_readfrom,
	.readerr = psmx_eq_readerr,
	.write = psmx_eq_write,
	.reset = psmx_eq_reset,
	.condread = psmx_eq_condread,
	.condreadfrom = psmx_eq_condreadfrom,
	.strerror = psmx_eq_strerror,
};

int psmx_eq_open(struct fid_domain *domain, struct fi_eq_attr *attr,
		struct fid_eq **eq, void *context)
{
	struct psmx_fid_domain *fid_domain;
	struct psmx_fid_eq *fid_eq;
	int format, err_format;
	int entry_size, err_entry_size;

	switch (attr->domain) {
	case FI_EQ_DOMAIN_GENERAL:
	case FI_EQ_DOMAIN_COMP:
		break;

	default:
		psmx_debug("%s: attr->domain=%d, supported=%d,%d\n", __func__, attr->domain,
				FI_EQ_DOMAIN_GENERAL, FI_EQ_DOMAIN_COMP);
		return -ENOSYS;
	}

	switch (attr->format) {
	case FI_EQ_FORMAT_UNSPEC:
		format = FI_EQ_FORMAT_TAGGED;
		err_format = FI_EQ_FORMAT_ERR;
		entry_size = sizeof(struct fi_eq_tagged_entry);
		err_entry_size = sizeof(struct fi_eq_err_entry);
		break;

	case FI_EQ_FORMAT_CONTEXT:
		format = attr->format;
		err_format = FI_EQ_FORMAT_ERR;
		entry_size = sizeof(struct fi_eq_entry);
		err_entry_size = sizeof(struct fi_eq_err_entry);
		break;

	case FI_EQ_FORMAT_COMP:
		format = attr->format;
		err_format = FI_EQ_FORMAT_ERR;
		entry_size = sizeof(struct fi_eq_comp_entry);
		err_entry_size = sizeof(struct fi_eq_err_entry);
		break;

	case FI_EQ_FORMAT_DATA:
		format = attr->format;
		err_format = FI_EQ_FORMAT_ERR;
		entry_size = sizeof(struct fi_eq_data_entry);
		err_entry_size = sizeof(struct fi_eq_err_entry);
		break;

	case FI_EQ_FORMAT_TAGGED:
		format = attr->format;
		err_format = FI_EQ_FORMAT_ERR;
		entry_size = sizeof(struct fi_eq_tagged_entry);
		err_entry_size = sizeof(struct fi_eq_err_entry);
		break;

	case FI_EQ_FORMAT_ERR:
		format = err_format = attr->format;
		entry_size = err_entry_size = sizeof(struct fi_eq_err_entry);
		break;

	case FI_EQ_FORMAT_TAGGED_ERR:
		format = err_format = attr->format;
		entry_size = err_entry_size = sizeof(struct fi_eq_tagged_err_entry);
		break;

	case FI_EQ_FORMAT_CM:
		format = err_format = attr->format;
		entry_size = err_entry_size = sizeof(struct fi_eq_cm_entry);
		break;

	default:
		psmx_debug("%s: attr->format=%d, supported=%d...%d\n", __func__, attr->format,
				FI_EQ_FORMAT_UNSPEC, FI_EQ_FORMAT_CM);
		return -EINVAL;
	}

	fid_domain = container_of(domain, struct psmx_fid_domain, domain);
	fid_eq = (struct psmx_fid_eq *) calloc(1, sizeof *fid_eq);
	if (!fid_eq)
		return -ENOMEM;

	fid_eq->domain = fid_domain;
	fid_eq->format = format;
	fid_eq->err_format = err_format;
	fid_eq->entry_size = entry_size;
	fid_eq->err_entry_size = err_entry_size;
	fid_eq->eq.fid.size = sizeof(struct fid_eq);
	fid_eq->eq.fid.fclass = FID_CLASS_EQ;
	fid_eq->eq.fid.context = context;
	fid_eq->eq.fid.ops = &psmx_fi_ops;
	fid_eq->eq.ops = &psmx_eq_ops;

	*eq = &fid_eq->eq;
	return 0;
}

