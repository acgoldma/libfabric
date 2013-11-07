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

#ifndef _FI_CM_H_
#define _FI_CM_H_

#include <rdma/fi_socket.h>


#ifdef __cplusplus
extern "C" {
#endif


struct fi_ops_cm {
	size_t	size;
	int	(*getname)(fid_t fid, void *addr, size_t *addrlen);
	int	(*getpeer)(fid_t fid, void *addr, size_t *addrlen);
	int	(*connect)(fid_t fid, const void *param, size_t paramlen);
	int	(*listen)(fid_t fid);
	int	(*accept)(fid_t fid, const void *param, size_t paramlen);
	int	(*reject)(fid_t fid, struct fi_info *info,
			  const void *param, size_t paramlen);
	int	(*shutdown)(fid_t fid, uint64_t flags);
	int	(*join)(fid_t fid, void *addr, void **fi_addr, uint64_t flags);
	int	(*leave)(fid_t fid, void *addr, void *fi_addr, uint64_t flags);
};

static inline int fi_getsockname(fid_t fid, void *addr, size_t *addrlen)
{
	struct fid_socket *sock = container_of(fid, struct fid_socket, fid);
	FI_ASSERT_CLASS(fid, FID_CLASS_SOCKET);
	FI_ASSERT_OPS(fid, struct fid_socket, cm);
	FI_ASSERT_OP(sock->cm, struct fi_ops_cm, getname);
	return sock->cm->getname(fid, addr, addrlen);
}

static inline int fi_listen(fid_t fid)
{
	struct fid_socket *sock = container_of(fid, struct fid_socket, fid);
	FI_ASSERT_CLASS(fid, FID_CLASS_SOCKET);
	FI_ASSERT_OPS(fid, struct fid_socket, cm);
	FI_ASSERT_OP(sock->cm, struct fi_ops_cm, listen);
	return sock->cm->listen(fid);
}

static inline int fi_connect(fid_t fid, const void *param, size_t paramlen)
{
	struct fid_socket *sock = container_of(fid, struct fid_socket, fid);
	FI_ASSERT_CLASS(fid, FID_CLASS_SOCKET);
	FI_ASSERT_OPS(fid, struct fid_socket, cm);
	FI_ASSERT_OP(sock->cm, struct fi_ops_cm, connect);
	return sock->cm->connect(fid, param, paramlen);
}

static inline int fi_accept(fid_t fid, const void *param, size_t paramlen)
{
	struct fid_socket *sock = container_of(fid, struct fid_socket, fid);
	FI_ASSERT_CLASS(fid, FID_CLASS_SOCKET);
	FI_ASSERT_OPS(fid, struct fid_socket, cm);
	FI_ASSERT_OP(sock->cm, struct fi_ops_cm, accept);
	return sock->cm->accept(fid, param, paramlen);
}

static inline int fi_reject(fid_t fid, struct fi_info *info,
			    const void *param, size_t paramlen)
{
	struct fid_socket *sock = container_of(fid, struct fid_socket, fid);
	FI_ASSERT_CLASS(fid, FID_CLASS_SOCKET);
	FI_ASSERT_OPS(fid, struct fid_socket, cm);
	FI_ASSERT_OP(sock->cm, struct fi_ops_cm, reject);
	return sock->cm->reject(fid, info, param, paramlen);
}

static inline int fi_shutdown(fid_t fid, uint64_t flags)
{
	struct fid_socket *sock = container_of(fid, struct fid_socket, fid);
	FI_ASSERT_CLASS(fid, FID_CLASS_SOCKET);
	FI_ASSERT_OPS(fid, struct fid_socket, cm);
	FI_ASSERT_OP(sock->cm, struct fi_ops_cm, shutdown);
	return sock->cm->shutdown(fid, flags);
}


#ifdef __cplusplus
}
#endif

#endif /* _FI_CM_H_ */
