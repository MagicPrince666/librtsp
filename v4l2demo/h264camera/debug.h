/**
 * @file debug.h
 * @author 黄李全 (846863428@qq.com)
 * @brief 
 * @version 0.1
 * @date 2023-07-18
 * @copyright 个人版权所有 Copyright (c) 2023
 */
#ifndef __DEBUG_H__
#define __DEBUG_H__


int Dbg_Param = 0x1f;
#define TestAp_Printf(flag, msg...) ((Dbg_Param & flag)?printf(msg):flag)

#define TESTAP_DBG_USAGE	(1 << 0)
#define TESTAP_DBG_ERR		(1 << 1)
#define TESTAP_DBG_FLOW		(1 << 2)
#define TESTAP_DBG_FRAME	(1 << 3)
#define TESTAP_DBG_BW	    (1 << 4)

#endif
