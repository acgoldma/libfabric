/*
 * Copyright (c) 2013-2014 Intel Corporation. All rights reserved.
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

	if ((event->error = !!err)) {
		event->eqe.err.op_context = op_context;
		event->eqe.err.err = -err;
		event->eqe.err.data = data;
		event->eqe.err.tag = tag;
		event->eqe.err.olen = olen;
		event->eqe.err.prov_errno = 0;
		event->eqe.err.prov_data = NULL;
		goto out;
	}

	switch (eq->format) {
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

	case FI_EQ_FORMAT_CM:
	default:
		fprintf(stderr, "%s: unsupported EC format %d\n", __func__, eq->format);
		return NULL;
	}

out:
	return event;
}

static struct psmx_event *psmx_eq_create_event_from_status(
				struct psmx_fid_eq *eq,
				psm_mq_status_t *psm_status)
{
	struct psmx_event *event;
	struct psmx_multi_recv *req;
	struct fi_context *fi_context = psm_status->context;
	void *op_context, *buf;
	int is_recv = 0;

	event = calloc(1, sizeof(*event));
	if (!event) {
		fprintf(stderr, "%s: out of memory\n", __func__);
		return NULL;
	}

	switch(PSMX_CTXT_TYPE(fi_context)) {
	case PSMX_SEND_CONTEXT:
		op_context = fi_context;
		buf = PSMX_CTXT_USER(fi_context);
		break;
	case PSMX_RECV_CONTEXT:
		op_context = fi_context;
		buf = PSMX_CTXT_USER(fi_context);
		is_recv = 1;
		break;
	case PSMX_MULTI_RECV_CONTEXT:
		op_context = fi_context;
		req = PSMX_CTXT_USER(fi_context);
		buf = req->buf + req->offset;
		is_recv = 1;
		break;
	default:
		op_context = PSMX_CTXT_USER(fi_context);
		buf = NULL;
		break;
	}

	if ((event->error = !!psm_status->error_code)) {
		event->eqe.err.op_context = op_context;
		event->eqe.err.err = -psmx_errno(psm_status->error_code);
		event->eqe.err.prov_errno = psm_status->error_code;
		event->eqe.err.tag = psm_status->msg_tag;
		event->eqe.err.olen = psm_status->msg_length - psm_status->nbytes;
		//event->eqe.err.prov_data = NULL; /* FIXME */
		goto out;
	}

	switch (eq->format) {
	case FI_EQ_FORMAT_CONTEXT:
		event->eqe.context.op_context = op_context;
		break;

	case FI_EQ_FORMAT_COMP:
		event->eqe.comp.op_context = op_context;
		//event->eqe.comp.flags = 0; /* FIXME */
		event->eqe.comp.len = psm_status->nbytes;
		break;

	case FI_EQ_FORMAT_DATA:
		event->eqe.data.op_context = op_context;
		event->eqe.data.buf = buf;
		//event->eqe.data.flags = 0; /* FIXME */
		event->eqe.data.len = psm_status->nbytes;
		//event->eqe.data.data = 0; /* FIXME */
		break;

	case FI_EQ_FORMAT_TAGGED:
		event->eqe.tagged.op_context = op_context;
		event->eqe.tagged.buf = buf;
		//event->eqe.tagged.flags = 0; /* FIXME */
		event->eqe.tagged.len = psm_status->nbytes;
		//event->eqe.tagged.data = 0; /* FIXME */
		event->eqe.tagged.tag = psm_status->msg_tag;
		break;

	case FI_EQ_FORMAT_CM:
	default:
		fprintf(stderr, "%s: unsupported EQ format %d\n", __func__, eq->format);
		return NULL;
	}

out:
	if (is_recv)
		event->source = psm_status->msg_tag;

	return event;
}

static int psmx_eq_get_event_src_addr(struct psmx_fid_eq *fid_eq,
					struct psmx_event *event,
					fi_addr_t *src_addr)
{
	int err;

	if (!src_addr)
		return 0;

	if ((fid_eq->domain->reserved_tag_bits & PSMX_MSG_BIT) &&
		(event->source & PSMX_MSG_BIT)) {
		err = psmx_epid_to_epaddr(fid_eq->domain,
					  event->source & ~PSMX_MSG_BIT,
					  (psm_epaddr_t *) src_addr);
		if (err)
			return err;

		return 0;
	}

	return -ENODATA;
}

int psmx_eq_poll_mq(struct psmx_fid_eq *eq, struct psmx_fid_domain *domain_if_null_eq)
{
	psm_mq_req_t psm_req;
	psm_mq_status_t psm_status;
	struct fi_context *fi_context;
	struct psmx_fid_domain *domain;
	struct psmx_fid_ep *tmp_ep;
	struct psmx_fid_eq *tmp_eq;
	struct psmx_fid_cntr *tmp_cntr;
	struct psmx_event *event;
	int multi_recv;
	int err;

	if (eq)
		domain = eq->domain;
	else
		domain = domain_if_null_eq;

	while (1) {
		err = psm_mq_ipeek(domain->psm_mq, &psm_req, NULL);

		if (err == PSM_OK) {
			err = psm_mq_test(&psm_req, &psm_status);

			fi_context = psm_status.context;

			tmp_ep = PSMX_CTXT_EP(fi_context);
			tmp_eq = NULL;
			tmp_cntr = NULL;
			multi_recv = 0;

			switch (PSMX_CTXT_TYPE(fi_context)) {
			case PSMX_NOCOMP_SEND_CONTEXT:
				tmp_ep->pending_sends--;
				if (!tmp_ep->send_cntr_event_flag)
					tmp_cntr = tmp_ep->send_cntr;
				break;

			case PSMX_NOCOMP_RECV_CONTEXT:
				if (!tmp_ep->recv_cntr_event_flag)
					tmp_cntr = tmp_ep->recv_cntr;
				break;

			case PSMX_NOCOMP_WRITE_CONTEXT:
				tmp_ep->pending_writes--;
				if (!tmp_ep->write_cntr_event_flag)
					tmp_cntr = tmp_ep->write_cntr;
				break;

			case PSMX_NOCOMP_READ_CONTEXT:
				tmp_ep->pending_reads--;
				if (!tmp_ep->read_cntr_event_flag)
					tmp_cntr = tmp_ep->read_cntr;
				break;

			case PSMX_INJECT_CONTEXT:
				tmp_ep->pending_sends--;
				if (!tmp_ep->send_cntr_event_flag)
					tmp_cntr = tmp_ep->send_cntr;
				free(fi_context);
				break;

			case PSMX_INJECT_WRITE_CONTEXT:
				tmp_ep->pending_writes--;
				if (!tmp_ep->write_cntr_event_flag)
					tmp_cntr = tmp_ep->write_cntr;
				free(fi_context);
				break;

			case PSMX_SEND_CONTEXT:
				tmp_ep->pending_sends--;
				tmp_eq = tmp_ep->send_eq;
				tmp_cntr = tmp_ep->send_cntr;
				break;

			case PSMX_RECV_CONTEXT:
				tmp_eq = tmp_ep->recv_eq;
				tmp_cntr = tmp_ep->recv_cntr;
				break;

			case PSMX_MULTI_RECV_CONTEXT:
				multi_recv = 1;
				tmp_eq = tmp_ep->recv_eq;
				tmp_cntr = tmp_ep->recv_cntr;
				break;

			case PSMX_READ_CONTEXT:
				tmp_ep->pending_reads--;
				tmp_eq = tmp_ep->send_eq;
				tmp_cntr = tmp_ep->read_cntr;
				break;

			case PSMX_WRITE_CONTEXT:
				tmp_ep->pending_writes--;
				tmp_eq = tmp_ep->send_eq;
				tmp_cntr = tmp_ep->write_cntr;
				break;

#if PSMX_USE_AM
			case PSMX_REMOTE_WRITE_CONTEXT:
			case PSMX_REMOTE_READ_CONTEXT:
				{
				  struct fi_context *fi_context = psm_status.context;
				  struct psmx_fid_mr *mr;
				  mr = PSMX_CTXT_USER(fi_context);
				  if (mr->eq) {
					event = psmx_eq_create_event_from_status(
							mr->eq, &psm_status);
					if (!event)
						return -ENOMEM;
					psmx_eq_enqueue_event(mr->eq, event);
				  }
				  if (mr->cntr)
					mr->cntr->cntr.ops->add(&tmp_cntr->cntr, 1);
				  if (!eq || mr->eq == eq)
					return 1;
				  continue;
				}
#endif
			}

			if (tmp_eq) {
				event = psmx_eq_create_event_from_status(tmp_eq, &psm_status);
				if (!event)
					return -ENOMEM;

				psmx_eq_enqueue_event(tmp_eq, event);
			}

			if (tmp_cntr)
				tmp_cntr->cntr.ops->add(&tmp_cntr->cntr, 1);

			if (multi_recv) {
				struct psmx_multi_recv *req;
				psm_mq_req_t psm_req;

				req = PSMX_CTXT_USER(fi_context);
				req->offset += psm_status.nbytes;
				if (req->offset + req->min_buf_size <= req->len) {
					err = psm_mq_irecv(tmp_ep->domain->psm_mq,
							   req->tag, req->tagsel, req->flag,
							   req->buf + req->offset, 
							   req->len - req->offset,
							   (void *)fi_context, &psm_req);
					if (err != PSM_OK)
						return psmx_errno(err);

					PSMX_CTXT_REQ(fi_context) = psm_req;
				}
				else {
					if (tmp_eq) {
						event = psmx_eq_create_event(
								tmp_eq,
								req->context,
								req->buf,
								FI_MULTI_RECV,
								req->len,
								req->len - req->offset, /* data */
								0,	/* tag */
								0,	/* olen */
								0);	/* err */
						if (!event)
							return -ENOMEM;

						psmx_eq_enqueue_event(tmp_eq, event);
					}

					free(req);
				}
			}

			if (!eq || tmp_eq == eq)
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
				fi_addr_t *src_addr)
{
	struct psmx_fid_eq *fid_eq;
	struct psmx_event *event;

	fid_eq = container_of(eq, struct psmx_fid_eq, eq);
	assert(fid_eq->domain);

#if PSMX_USE_AM
	fid_eq->poll_am_before_mq = !fid_eq->poll_am_before_mq;
	if (fid_eq->poll_am_before_mq)
		psmx_am_progress(fid_eq->domain);
#endif

	psmx_eq_poll_mq(fid_eq, fid_eq->domain);

#if PSMX_USE_AM
	if (!fid_eq->poll_am_before_mq)
		psmx_am_progress(fid_eq->domain);
#endif

	if (fid_eq->pending_error)
		return -FI_EAVAIL;

	if (len < fid_eq->entry_size)
		return -FI_ETOOSMALL;

	if (!buf)
		return -EINVAL;

	event = psmx_eq_dequeue_event(fid_eq);
	if (event) {
		if (!event->error) {
			memcpy(buf, (void *)&event->eqe, fid_eq->entry_size);
			if (psmx_eq_get_event_src_addr(fid_eq, event, src_addr))
				*src_addr = FI_ADDR_UNSPEC;
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
	return psmx_eq_readfrom(eq, buf, len, NULL);
}

static ssize_t psmx_eq_readerr(struct fid_eq *eq, struct fi_eq_err_entry *buf,
				size_t len, uint64_t flags)
{
	struct psmx_fid_eq *fid_eq;

	fid_eq = container_of(eq, struct psmx_fid_eq, eq);

	if (len < sizeof *buf)
		return -FI_ETOOSMALL;

	if (fid_eq->pending_error) {
		memcpy(buf, &fid_eq->pending_error->eqe, sizeof *buf);
		free(fid_eq->pending_error);
		fid_eq->pending_error = NULL;
		return sizeof *buf;
	}

	return 0;
}

static ssize_t psmx_eq_write(struct fid_eq *eq, const void *buf, size_t len)
{
	return -ENOSYS;
}

static ssize_t psmx_eq_condreadfrom(struct fid_eq *eq, void *buf, size_t len,
				    fi_addr_t *src_addr, const void *cond)
{
	return -ENOSYS;
}

static ssize_t psmx_eq_condread(struct fid_eq *eq, void *buf, size_t len, const void *cond)
{
	return psmx_eq_condreadfrom(eq, buf, len, NULL, cond);
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

static int psmx_eq_bind(struct fid *fid, struct fid *bfid, uint64_t flags)
{
	return -ENOSYS;
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
	.condread = psmx_eq_condread,
	.condreadfrom = psmx_eq_condreadfrom,
	.strerror = psmx_eq_strerror,
};

int psmx_eq_open(struct fid_domain *domain, struct fi_eq_attr *attr,
		struct fid_eq **eq, void *context)
{
	struct psmx_fid_domain *fid_domain;
	struct psmx_fid_eq *fid_eq;
	int format;
	int entry_size;

	if ((attr->wait_cond != FI_EQ_COND_NONE) ||
	    (attr->flags & FI_WRITE))
		return -ENOSYS;

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
		entry_size = sizeof(struct fi_eq_tagged_entry);
		break;

	case FI_EQ_FORMAT_CONTEXT:
		format = attr->format;
		entry_size = sizeof(struct fi_eq_entry);
		break;

	case FI_EQ_FORMAT_COMP:
		format = attr->format;
		entry_size = sizeof(struct fi_eq_comp_entry);
		break;

	case FI_EQ_FORMAT_DATA:
		format = attr->format;
		entry_size = sizeof(struct fi_eq_data_entry);
		break;

	case FI_EQ_FORMAT_TAGGED:
		format = attr->format;
		entry_size = sizeof(struct fi_eq_tagged_entry);
		break;

	case FI_EQ_FORMAT_CM:
		format = attr->format;
		entry_size = sizeof(struct fi_eq_cm_entry);
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
	fid_eq->entry_size = entry_size;
	fid_eq->eq.fid.fclass = FID_CLASS_EQ;
	fid_eq->eq.fid.context = context;
	fid_eq->eq.fid.ops = &psmx_fi_ops;
	fid_eq->eq.ops = &psmx_eq_ops;

	*eq = &fid_eq->eq;
	return 0;
}

