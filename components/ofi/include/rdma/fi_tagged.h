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

#ifndef _FI_TAGGED_H_
#define _FI_TAGGED_H_

#include <assert.h>
#include <rdma/fabric.h>
#include <rdma/fi_endpoint.h>


#ifdef __cplusplus
extern "C" {
#endif

#define FI_CLAIM		(1ULL << 0)

struct fi_msg_tagged {
	const struct iovec	*msg_iov;
	void			*desc;
	size_t			iov_count;
	const void		*addr;
	uint64_t		tag;
	uint64_t		ignore;
	void			*context;
	uint64_t		data;
	int			flow;
};

struct fi_ops_tagged {
	size_t	size;
	ssize_t (*recv)(struct fid_ep *ep, void *buf, size_t len, void *desc,
			uint64_t tag, uint64_t ignore, void *context);
	ssize_t (*recvv)(struct fid_ep *ep, const struct iovec *iov, void *desc,
			size_t count, uint64_t tag, uint64_t ignore, void *context);
	ssize_t (*recvfrom)(struct fid_ep *ep, void *buf, size_t len, void *desc,
			const void *src_addr,
			uint64_t tag, uint64_t ignore, void *context);
	ssize_t (*recvmsg)(struct fid_ep *ep, const struct fi_msg_tagged *msg,
			uint64_t flags);
	ssize_t (*send)(struct fid_ep *ep, const void *buf, size_t len, void *desc,
			uint64_t tag, void *context);
	ssize_t (*sendv)(struct fid_ep *ep, const struct iovec *iov, void *desc,
			size_t count, uint64_t tag, void *context);
	ssize_t (*sendto)(struct fid_ep *ep, const void *buf, size_t len, void *desc,
			const void *dest_addr, uint64_t tag, void *context);
	ssize_t (*sendmsg)(struct fid_ep *ep, const struct fi_msg_tagged *msg,
			uint64_t flags);
	ssize_t (*search)(struct fid_ep *ep, uint64_t *tag, uint64_t ignore,
			uint64_t flags, void *src_addr, size_t *src_addrlen,
			size_t *len, void *context);
};


#ifndef FABRIC_DIRECT

static inline ssize_t
fi_tsendto(struct fid_ep *ep, const void *buf, size_t len, void *desc,
	   const void *dest_addr, uint64_t tag, void *context)
{
	return ep->tagged->sendto(ep, buf, len, desc, dest_addr, tag, context);
}

static inline ssize_t
fi_trecvfrom(struct fid_ep *ep, void *buf, size_t len, void *desc,
	     const void *src_addr, uint64_t tag, uint64_t ignore, void *context)
{
	return ep->tagged->recvfrom(ep, buf, len, desc, src_addr, tag, ignore,
				    context);
}

static inline ssize_t
fi_tsearch(struct fid_ep *ep, uint64_t *tag, uint64_t ignore, uint64_t flags,
	   void *src_addr, size_t *src_addrlen, size_t *len, void *context)
{
	return ep->tagged->search(ep, tag, ignore, flags, src_addr, src_addrlen,
				  len, context);
}


#else // FABRIC_DIRECT
#include <rdma/fi_direct_tagged.h>
#endif

#ifdef __cplusplus
}
#endif

#endif /* _FI_TAGGED_H_ */
