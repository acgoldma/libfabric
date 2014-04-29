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

static int psmx_getinfo(const char *node, const char *service,
			struct fi_info *hints, struct fi_info **info)
{
	struct fi_info *psmx_info;
	uint64_t flags = 0;
	uint32_t cnt = 0;
	void *dest_addr = NULL;
	void *uuid;
	char *s;

	if (psm_ep_num_devunits(&cnt) || !cnt)
		return -FI_ENODATA;

	uuid = calloc(1, sizeof(psm_uuid_t));
	if (!uuid) 
		return -ENOMEM;

	s = getenv("SFI_PSM_UUID");
	if (s)
		psmx_string_to_uuid(s, uuid);

	if (node)
		dest_addr = psmx_resolve_name(node, uuid);

	if (service) {
		/* FIXME: check service */
	}

	if (hints) {
		switch (hints->type) {
		case FID_UNSPEC:
		case FID_RDM:
			break;
		default:
			*info = NULL;
			return -ENODATA;
		}

		switch (hints->protocol) {
		case FI_PROTO_UNSPEC:
			  if ((hints->protocol_cap & PSMX_PROTO_CAPS) == hints->protocol_cap)
				  break;
		/* fall through */
		default:
			*info = NULL;
			return -ENODATA;
		}

		flags = hints->flags;
		if ((flags & PSMX_SUPPORTED_FLAGS) != flags) {
			*info = NULL;
			return -ENODATA;
		}

		if (hints->domain_name && strncmp(hints->domain_name, "psm", 3)) {
			*info = NULL;
			return -ENODATA;
		}

		/* FIXME: check other fields of hints */
	}

	psmx_info = calloc(1, sizeof *psmx_info);
	if (!psmx_info) {
		free(uuid);
		return -ENOMEM;
	}

	psmx_info->next = NULL;
	psmx_info->size = sizeof(*psmx_info);
	psmx_info->flags = flags | PSMX_DEFAULT_FLAGS;
	psmx_info->type = FID_RDM;
	psmx_info->protocol = PSMX_OUI_INTEL << FI_OUI_SHIFT | PSMX_PROTOCOL;
	if (hints->protocol_cap)
		psmx_info->protocol_cap = hints->protocol_cap & PSMX_PROTO_CAPS;
	else
		psmx_info->protocol_cap = FI_PROTO_CAP_TAGGED;
	psmx_info->iov_format = FI_IOV;
	psmx_info->addr_format = FI_ADDR; 
	psmx_info->info_addr_format = FI_ADDR;
	psmx_info->src_addrlen = 0;
	psmx_info->dest_addrlen = sizeof(psm_epid_t);
	psmx_info->src_addr = NULL;
	psmx_info->dest_addr = dest_addr;
	psmx_info->auth_keylen = sizeof(psm_uuid_t);
	psmx_info->auth_key = uuid;
	psmx_info->shared_fd = -1;
	psmx_info->domain_name = strdup("psm");
	psmx_info->datalen = 0;
	psmx_info->data = NULL;

	*info = psmx_info;

	return 0;
}

static struct fi_ops_prov psmx_ops = {
	.size = sizeof(struct fi_ops_prov),
	.getinfo = psmx_getinfo,
	.freeinfo = NULL,
	.open = NULL,
	.domain = psmx_domain_open,
	.endpoint = psmx_ep_open,
};

void psmx_ini(void)
{
	int major, minor;
	int err;

        psm_error_register_handler(NULL, PSM_ERRHANDLER_NO_HANDLER);

	major = PSM_VERNO_MAJOR;
	minor = PSM_VERNO_MINOR;

        err = psm_init(&major, &minor);
	if (err != PSM_OK) {
		fprintf(stderr, "%s: psm_init failed: %s\n", __func__,
			psm_error_get_string(err));
		return;
	}

	if (major > PSM_VERNO_MAJOR) {
		fprintf(stderr, "%s: PSM loaded an unexpected/unsupported version %d.%d\n",
			__func__, major, minor);
		return;
	}

	fi_register(&psmx_ops);
}

void psmx_fini(void)
{
	psm_finalize();
}

