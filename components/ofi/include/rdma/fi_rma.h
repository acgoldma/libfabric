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

#ifndef _FI_RMA_H_
#define _FI_RMA_H_

#include <assert.h>
#include <rdma/fabric.h>
#include <rdma/fi_endpoint.h>


#ifdef __cplusplus
extern "C" {
#endif


struct fi_rma_iov {
	uint64_t		addr;
	size_t			len;
	uint64_t		key;
};

struct fi_msg_rma {
	const struct iovec	*msg_iov;
	void			*desc;
	size_t			iov_count;
	const void		*addr;
	const struct fi_rma_iov *rma_iov;
	size_t			rma_iov_count;
	void			*context;
	uint64_t		data;
	int			flow;
};

struct fi_ops_rma {
	size_t	size;
	int	(*read)(fid_t fid, void *buf, size_t len, void *desc,
			uint64_t addr, uint64_t key, void *context);
	int	(*readv)(fid_t fid, const struct iovec *iov, void *desc,
			size_t count, uint64_t addr, uint64_t key, void *context);
	int	(*readfrom)(fid_t fid, void *buf, size_t len, void *desc,
			const void *src_addr, uint64_t addr, uint64_t key,
			void *context);
	int	(*readmsg)(fid_t fid, const struct fi_msg_rma *msg, uint64_t flags);
	int	(*write)(fid_t fid, const void *buf, size_t len, void *desc,
			uint64_t addr, uint64_t key, void *context);
	int	(*writev)(fid_t fid, const struct iovec *iov, void *desc,
			size_t count, uint64_t addr, uint64_t key, void *context);
	int	(*writeto)(fid_t fid, const void *buf, size_t len, void *desc,
			const void *dest_addr, uint64_t addr, uint64_t key,
			void *context);
	int	(*writemsg)(fid_t fid, const struct fi_msg_rma *msg, uint64_t flags);
};


#ifndef FABRIC_DIRECT

static inline ssize_t
fi_read(fid_t fid, void *buf, size_t len, void *desc,
	uint64_t addr, uint64_t key, void *context)
{
	struct fid_ep *ep = container_of(fid, struct fid_ep, fid);
	FI_ASSERT_CLASS(fid, FID_CLASS_EP);
	FI_ASSERT_OPS(fid, struct fid_ep, rma);
	FI_ASSERT_OP(ep->rma, struct fi_ops_rma, read);
	return ep->rma->read(fid, buf, len, desc, addr, key, context);
}

static inline ssize_t
fi_write(fid_t fid, const void *buf, size_t len, void *desc,
	 uint64_t addr, uint64_t key, void *context)
{
	struct fid_ep *ep = container_of(fid, struct fid_ep, fid);
	FI_ASSERT_CLASS(fid, FID_CLASS_EP);
	FI_ASSERT_OPS(fid, struct fid_ep, rma);
	FI_ASSERT_OP(ep->rma, struct fi_ops_rma, write);
	return ep->rma->write(fid, buf, len, desc, addr, key, context);
}

#else // FABRIC_DIRECT
#include <rdma/fi_direct_rma.h>
#endif

#ifdef __cplusplus
}
#endif

#endif /* _FI_RMA_H_ */
