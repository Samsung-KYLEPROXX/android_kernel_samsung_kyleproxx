/*
	�2007 Broadcom Corporation

	Unless you and Broadcom execute a separate written software license
	agreement governing use of this software, this software is licensed to you
	under the terms of the GNU General Public License version 2, available
	at http://www.gnu.org/licenses/old-licenses/gpl-2.0.html (the "GPL").

   Notwithstanding the above, under no circumstances may you combine this
   software in any way with any other Broadcom software provided under a license
   other than the GPL, without Broadcom's express prior written consent.
*/

//============================================================
// IPC_SmBuffers.h
//
// Definitions for the Shared memory used for Apps - Comms communication
//============================================================
#ifndef IPC_SmBuffers_h
#define IPC_SmBuffers_h

#ifdef UNDER_LINUX
#include <mach/comms/platform_mconfig.h>
#else
#include "nandsdram_memmap.h"
#endif

#define IPC_SmSize IPC_SIZE

extern volatile unsigned char SmBuffer[IPC_SmSize];

#endif
