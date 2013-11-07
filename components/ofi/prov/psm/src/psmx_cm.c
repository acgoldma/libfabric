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

static int psmx_cm_getname(fid_t fid, void *addr, size_t *addrlen)
{
	struct psmx_fid_socket *fid_socket;

	fid_socket = container_of(fid, struct psmx_fid_socket, socket.fid);
	if (!fid_socket->domain)
		return -EBADF;

	if (*addrlen < sizeof(psm_epid_t))
		return -FI_ETOOSMALL;

	*(psm_epid_t *)addr = fid_socket->domain->psm_epid;
	*addrlen = sizeof(psm_epid_t);

	return 0;
}

static int psmx_cm_getpeer(fid_t fid, void *addr, size_t *addrlen)
{
	return -ENOSYS;
}

static int psmx_cm_connect(fid_t fid, const void *param, size_t paramlen)
{
	return -ENOSYS;
}

static int psmx_cm_listen(fid_t fid)
{
	return -ENOSYS;
}

static int psmx_cm_accept(fid_t fid, const void *param, size_t paramlen)
{
	return -ENOSYS;
}

static int psmx_cm_reject(fid_t fid, struct fi_info *info, const void *param,
			size_t paramlen)
{
	return -ENOSYS;
}

static int psmx_cm_shutdown(fid_t fid, uint64_t flags)
{
	return -ENOSYS;
}

static int psmx_cm_join(fid_t fid, void *addr, void **fi_addr, uint64_t flags)
{
	return -ENOSYS;
}

static int psmx_cm_leave(fid_t fid, void *addr, void *fi_addr, uint64_t flags)
{
	return -ENOSYS;
}

struct fi_ops_cm psmx_cm_ops = {
	.size = sizeof(struct fi_ops_cm),
	.getname = psmx_cm_getname,
	.getpeer = psmx_cm_getpeer,
	.connect = psmx_cm_connect,
	.listen = psmx_cm_listen,
	.accept = psmx_cm_accept,
	.reject = psmx_cm_reject,
	.shutdown = psmx_cm_shutdown,
	.join = psmx_cm_join,
	.leave = psmx_cm_leave,
};

