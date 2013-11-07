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

#ifndef _FI_RDMA_H_
#define _FI_RDMA_H_

#include <rdma/fi_socket.h>


#ifdef __cplusplus
extern "C" {
#endif


struct fi_rdma_iov {
	uint64_t		addr;
	size_t			len;
	be64_t			key;
};

struct fi_msg_rdma {
	const void		*msg_iov;
	size_t			iov_count;
	const void		*addr;
	const struct fi_rdma_iov *rdma_iov;
	size_t			rdma_iov_count;
	void			*context;
	uint64_t		data;
};

struct fi_ops_rdma {
	size_t	size;
	int	(*read)(fid_t fid, void *buf, size_t len, uint64_t addr,
			be64_t key, void *context);
	int	(*readmem)(fid_t fid, void *buf, size_t len, uint64_t mem_desc,
			   uint64_t addr, be64_t key, void *context);
	int	(*readv)(fid_t fid, const void *iov, size_t count, uint64_t addr,
			 be64_t key, void *context);
	int	(*readfrom)(fid_t fid, void *buf, size_t len, const void *src_addr,
			    uint64_t addr, be64_t key, void *context);
	int	(*readmemfrom)(fid_t fid, void *buf, size_t len, uint64_t mem_desc,
			       const void *src_addr, uint64_t addr, be64_t key,
			       void *context);
	int	(*readmsg)(fid_t fid, const struct fi_msg_rdma *msg, uint64_t flags);
	int	(*write)(fid_t fid, const void *buf, size_t len, uint64_t addr,
			 be64_t key, void *context);
	int	(*writemem)(fid_t fid, const void *buf, size_t len, uint64_t mem_desc,
			    uint64_t addr, be64_t key, void *context);
	int	(*writev)(fid_t fid, const void *iov, size_t count, uint64_t addr,
			  be64_t key, void *context);
	int	(*writememto)(fid_t fid, const void *buf, size_t len, uint64_t mem_desc,
			      const void *dst_addr, uint64_t addr, be64_t key,
			      void *context);
	int	(*writeto)(fid_t fid, const void *buf, size_t len, const void *dst_addr,
			   uint64_t addr, be64_t key, void *context);
	int	(*writemsg)(fid_t fid, const struct fi_msg_rdma *msg, uint64_t flags);
};


#ifdef __cplusplus
}
#endif

#endif /* _FI_RDMA_H_ */
