/****************************************************************************
*
*     Copyright (c) 2009 Broadcom Corporation
*
*   Unless you and Broadcom execute a separate written software license
*   agreement governing use of this software, this software is licensed to you
*   under the terms of the GNU General Public License version 2, available
*    at http://www.gnu.org/licenses/old-licenses/gpl-2.0.html (the "GPL").
*
*  Notwithstanding the above, under no circumstances may you combine this
*  software in any way with any other Broadcom software provided under a license
*  other than the GPL, without Broadcom's express prior written consent.
*
****************************************************************************/
/**
*
*   @file   ipc_stubs.h
*
*   @brief  This file includes stubs of functions from the Nucleus AP build
*           required for IPC.
*
*
****************************************************************************/

#include <linux/broadcom/csl_types.h>

/******************************************************************************/
/**
*   Retrieve the current value of the system timer tick counter
*
*	@return		Number of clock ticks since system startup.
*
*******************************************************************************/
UInt32 TIMER_GetValue(void);

void IPC_AlignTime(void);

