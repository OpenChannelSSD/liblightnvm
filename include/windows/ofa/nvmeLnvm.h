/*
 * OFA NVMe Driver IOCTL extension for Open-Channel SSDs
 *
 * Copyright (C) 2017 Simon A. F. Lund <slund@cnexlabs.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
* File: nvmeLnvm.h
*/

#ifndef __NVME_LNVM_H__
#define __NVME_LNVM_H__

#define LNVM_NADDRS_MAX			32
#define LNVM_OOB_NBYTES_MAX		64
#define LNVM_SECTOR_NBYTES_MAX		4096

#define LNVM_S12_OPC_IDF		0xE2
#define LNVM_S12_OPC_SET_BBT		0xF1
#define LNVM_S12_OPC_GET_BBT		0xF2
#define LNVM_S12_OPC_ERASE		0x90
#define LNVM_S12_OPC_WRITE		0x91
#define LNVM_S12_OPC_READ		0x92

#define NVME_LNVM_IOCTL_PAYLOAD_SZ LNVM_NADDRS_MAX * (LNVM_SECTOR_NBYTES_MAX + LNVM_OOB_NBYTES_MAX)

typedef struct _LNVM_COMMAND {
	UCHAR		OPC;			/* CDW0*/
	UCHAR		flags;
	USHORT		CID;

	ULONG		NSID;		/* CWD1*/
	ULONG		CDW2;
	ULONG		CDW3;
	ULONGLONG	MPTR;		/* CWD4-5*/
	ULONGLONG	PRP1;		/* CWD6-7*/
	ULONGLONG	PRP2;		/* CWD8-9*/

	ULONGLONG	ppas;		/* CWD10-11*/

	USHORT		nppas;		/* CWD12 */
	USHORT		control;

	ULONG		CDW13;
	ULONG		CDW14;
	ULONG		CDW15;
} NVMe_LNVM_COMMAND, *PNVMe_LNVM_COMMAND;

#pragma pack(1)
/******************************************************************************
* NVMe Open-Channel SSD Pass Through IOCTL data structure.
*
* This structure contains WDK defined SRB_IO_CONTROL structure, 64-byte NVMe
* command entry and 16-byte completion entry, and other important fields that
* driver needs to reference when processing the requests.
*
* User applications need to allocate proper size of buffer(s) and populate the
* fields to ensure the requests are being processed correctly after issuing.
******************************************************************************/
typedef struct _NVME_LNVM_IOCTL
{
	/* WDK defined SRB_IO_CONTROL structure */
	SRB_IO_CONTROL SrbIoCtrl;

	/* 64-byte submission entry defined in NVMe Specification */
	ULONG NVMeCmd[NVME_IOCTL_CMD_DW_SIZE];

	/* DW[0..3] of completion entry */
	ULONG CplEntry[NVME_IOCTL_COMPLETE_DW_SIZE];

	/* Number of addresses in LightNVM CMD */
	ULONG NumAddrs;

	/* Address list */
	ULONGLONG SrcAddrs[LNVM_NADDRS_MAX];

	/* Address list */
	ULONGLONG DstAddrs[LNVM_NADDRS_MAX];

	/* # bytes of DATA in Payload */
	ULONG DataLength;

	/* # bytes of META/OOB in Payload */
	ULONG MetaLength;

	UCHAR Payload[NVME_LNVM_IOCTL_PAYLOAD_SZ];
} NVME_LNVM_IOCTL, *PNVME_LNVM_IOCTL;
#pragma pack()

#define NVME_LNVM_IOCTL_NBYTES sizeof(NVME_LNVM_IOCTL)
/*
BOOLEAN NVMeLnvmProcessIoctl(
	PNVME_DEVICE_EXTENSION pDevExt,
#if (NTDDI_VERSION > NTDDI_WIN7)
	PSTORAGE_REQUEST_BLOCK pSrb
#else
	PSCSI_REQUEST_BLOCK pSrb
#endif
);*/

#endif /* __NVME_LNVM_H__ */
