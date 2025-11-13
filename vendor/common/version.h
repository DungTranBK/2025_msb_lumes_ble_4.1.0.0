/********************************************************************************************************
 * @file	version.h
 *
 * @brief	for TLSR chips
 *
 * @author	telink
 * @date	Sep. 30, 2010
 *
 * @par     Copyright (c) 2017, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#pragma once

// can't use "enum" here, because cstartup.S not support

//#define   BUILD_VERSION  	"Revision: 45:49M"
//#define   BUILD_TIME  		"2012-07-24-21:37:43"

#include "../../proj/mcu/config.h"
#include "mesh_config.h"

#define VERSION_GET(low, high)      ((low)|(high << 8))
#define FW_VERSION_TELINK_RELEASE   (VERSION_GET(0x31, 0x30))

#define LM_SUB_VERSION               2

#define VER_NUM2CHAR(num)		((((num) >= 0)&&((num) <= 9)) ? ((num) + '0') : ((((num) >= 0x0a)&&((num) <= 0x0f)) ? ((num)-0x0a + 'a') : (num)))
#define U8_HIGH2CHAR(v_u8)		(VER_NUM2CHAR(((v_u8) >> 4) & 0x0f))
#define U8_LOW2CHAR(v_u8)		(VER_NUM2CHAR((v_u8) & 0x0f))
#define U8_2CHAR(v_u8)			((U8_HIGH2CHAR((v_u8)) << 8) + U8_LOW2CHAR((v_u8)))
#define VER_CHAR2NUM(c)		    ((((c) >= '0')&&((c) <= '9')) ? ((c) - '0') : ((((c) >= 'a')&&((c) <= 'f')) ? ((c)-'a' + 0x0a) : ((((c) >= 'A')&&((c) <= 'F')) ? ((c)-'A' + 0x0a) : (c))))

#if(CHIP_TYPE == CHIP_TYPE_8258)
#define PID_CHIP_TYPE			0 // set 0 for compatibility.
#elif(CHIP_TYPE == CHIP_TYPE_8278)
#define PID_CHIP_TYPE			1
#elif(CHIP_TYPE == CHIP_TYPE_8269)
#define PID_CHIP_TYPE			2
#elif(CHIP_TYPE == CHIP_TYPE_9518)
#define PID_CHIP_TYPE			3
#else
#error error mcu core type
#endif

/*
 * demo PID filed: 
 * {
 *     minor product type	: 8 // lowest 8 bit
 *     major product type	: 4
 *     MCU chip type	    : 4
 * }
*/

#define NEW_PRODUCT_ID              1

// ------ unknown ------

#define PID_UNKNOW              	(0x0000)

// ------ light ------

#define PID_CT				    	(0x0001)
#define PID_HSL				    	(0x0002)
#define PID_XYL				    	(0x0003)
#define PID_POWER			    	(0x0004)
#define PID_CT_HSL			   	 	(0x0005)
#define PID_DIM			        	(0x0006)   // only single PWM
#define PID_PANEL			    	(0x0007)   // only ONOFF model
#define PID_LPN_ONOFF_LEVEL     	(0x0008)   // only ONOFF , LEVEL model
#define PID_RGBWW_WIRE          	(0x0009)
#define PID_WW_WIRE             	(0x000A)

#define LM_DIMMER_ONE_CHANNEL   	(0x0051)    // Invert
#define LM_DIMMER_TWO_CHANNEL   	(0x0052)	// Invert
#define LM_DIMMER_THREE_CHANNEL 	(0x0053)	// Invert
#define LM_DIMMER_FOUR_CHANNEL  	(0x0054)	// Invert
#define LM_DIMMER_FIVE_CHANNEL  	(0x0055)	// Invert

#define LM_RGBWW_WIRE               (0x0060)	// Invert
#define LM_RGBWW_WIRE_INVERT        (0x0061)	// Invert

#if NEW_PRODUCT_ID
#define LM_CCT_DIM                  (0x0A00)    // DL
#define LM_CCT_POB                  (0x0A01)    // POB
#define LM_CCT_COB                  (0x0A02)    // COB
#define LM_CCT_RS                   (0x0A03)    // RS
#define LM_PANEL                    (0x0a04)    // LM_PANEL

#define LM_RAIL_6W_LIGHT            (0x0a08)
#define LM_RAIL_12W_LIGHT           (0x0a09)
#else
#define LM_CCT_DIM                  (0x0068)    // DL
#define LM_CCT_POB                  (0x0069)    // POB
#define LM_CCT_COB                  (0x006A)    // COB
#define LM_CCT_RS                   (0x006B)    // RS
#define LM_PANEL                    (0x006D)    // LM_PANEL
#endif

#define LM_WW                       (0x006C)    // 1 channel

#define LM_RGB_AND_WW_WIRE          (0x0078)    // 1 channel RGB, 1 channel WW

#define LM_2WW_1DIMMER      		(0x0080)	// Invert
#define LM_1WW_3DIMMER 				(0x0081)	// Invert

#if NEW_PRODUCT_ID
#define LM_2_CCT                    (0x0A20)    // Wall light
#else
#define LM_2_CCT                    (0x0082)    // Wall light
#endif

// ------ gateway ------
#define PID_GATEWAY             	(0x0101)
// ------ LPN ------
#define PID_LPN                 	(0x0201)
// ------ SWITCH ------
#define PID_SWITCH              	(0x0301)
// ------ SPIRIT_LPN ------
#define PID_SPIRIT_LPN          	(0x0401)

// One Wire
#define PID_LM_ONE_WIRE_SW1_SN2     (0x08B1)
#define PID_LM_ONE_WIRE_SW2_SN2     (0x08B2)
#define PID_LM_ONE_WIRE_SW3_SN2     (0x08B3)

/*
MESH_PID_SEL : PID is product ID,
MESH_VID: VID is software version ID,

PID and VID are used in composition data: model_sig_cfg_s_cps.
format: please refer to spec "4.2.1.1 Composition Data Page 0"
user can be allowed to redefined PID and VID if needed.
*/

#if (WIN32)
#define MESH_PID_SEL		(PID_LIGHT)
#define MESH_VID		    FW_VERSION_TELINK_RELEASE       // user can redefine
#elif (__PROJECT_MESH_PRO__ || __PROJECT_MESH_GW_NODE__)  // must define in TC32_CC_Assembler ->General , too. because cstartup.s can't read predefine value in TC32_compiler-->symbols
#define MESH_PID_SEL		(PID_GATEWAY)
#define MESH_VID		    FW_VERSION_TELINK_RELEASE       // user can redefine
#elif (__PROJECT_MESH_LPN__)  // must define in TC32_CC_Assembler ->General , too. because cstartup.s can't read predefine value in TC32_compiler-->symbols
#define MESH_PID_SEL		(PID_LPN)
#define MESH_VID		    FW_VERSION_TELINK_RELEASE       // user can redefine
#elif (__PROJECT_MESH_SWITCH__)  // must define in TC32_CC_Assembler ->General , too. because cstartup.s can't read predefine value in TC32_compiler-->symbols
#define MESH_PID_SEL		(PID_SWITCH)
#define MESH_VID		    FW_VERSION_TELINK_RELEASE       // user can redefine
#elif (__PROJECT_SPIRIT_LPN__)  // must define in TC32_CC_Assembler ->General , too. because cstartup.s can't read predefine value in TC32_compiler-->symbols
#define MESH_PID_SEL		(PID_SPIRIT_LPN)
#define MESH_VID		    FW_VERSION_TELINK_RELEASE       // user can redefine
#elif (__PROJECT_8267_MASTER_KMA_DONGLE__)
#define MESH_PID_SEL		(PID_UNKNOW)
#define MESH_VID		    FW_VERSION_TELINK_RELEASE       // user can redefine
#elif (__PROJECT_MESH__)   // light

#if LIGHT_TYPE_SEL == G_TYPE_SWITCH_BUTTON
    #if _BTN_NUMBER_ == 3
    #define MESH_PID_SEL   PID_LM_ONE_WIRE_SW1_SN2
    #elif _BTN_NUMBER_ == 4
    #define MESH_PID_SEL   PID_LM_ONE_WIRE_SW2_SN2
    #elif _BTN_NUMBER_ == 5
    #define MESH_PID_SEL   PID_LM_ONE_WIRE_SW3_SN2
    #endif
#elif LIGHT_TYPE_SEL == LIGHT_TYPE_CT
		#ifdef _RGBWW_INDEPENDENT_
		#define MESH_PID_SEL	PID_WW_WIRE
		#else
	        #if defined _POB_LIGHT_
			    #define MESH_PID_SEL    LM_CCT_POB
			#elif defined _COB_LIGHT_
				#define MESH_PID_SEL    LM_CCT_COB
			#elif defined _LM_RS_
                #define MESH_PID_SEL    LM_CCT_RS
			#elif defined _LM_DOWNLIGHT_
                #define MESH_PID_SEL    LM_CCT_DIM
			#elif defined _LM_PANEL_
				#define MESH_PID_SEL    LM_PANEL
			#elif defined _LM_RAIL_
			#define  MESH_PID_SEL       LM_RAIL_6W_LIGHT
			#else
				#define MESH_PID_SEL    LM_CCT_DIM
			#endif
		#endif
	#elif LIGHT_TYPE_SEL == LIGHT_TYPE_HSL
		#define MESH_PID_SEL    PID_HSL
	#elif LIGHT_TYPE_SEL == LIGHT_TYPE_CT_HSL
		#ifdef _RGBWW_INDEPENDENT_
		#ifdef _INVERT_OUTPUT_
			#define MESH_PID_SEL	LM_RGBWW_WIRE_INVERT
		#else
			#define MESH_PID_SEL	LM_RGBWW_WIRE
		#endif
		#else
		#define MESH_PID_SEL    PID_CT_HSL
		#endif
	#elif LIGHT_TYPE_SEL == LIGHT_TYPE_DIM
		#ifdef _DIM_TYPE_COB_
			#define MESH_PID_SEL    PID_DIM
		#else
			#if   _DIMMER_CHANNEL_NUMBER_ == 1
				#define MESH_PID_SEL    LM_DIMMER_ONE_CHANNEL
			#elif _DIMMER_CHANNEL_NUMBER_ == 2
				#define MESH_PID_SEL    LM_DIMMER_TWO_CHANNEL
			#elif _DIMMER_CHANNEL_NUMBER_ == 3
				#define MESH_PID_SEL    LM_DIMMER_THREE_CHANNEL
			#elif _DIMMER_CHANNEL_NUMBER_ == 4
				#define MESH_PID_SEL    LM_DIMMER_FOUR_CHANNEL
			#elif _DIMMER_CHANNEL_NUMBER_ == 5
				#define MESH_PID_SEL    LM_DIMMER_FIVE_CHANNEL
			#endif
		#endif
    #elif LIGHT_TYPE_SEL == LIGHT_TYPE_2WW_1DIM
        #define  MESH_PID_SEL       LM_2WW_1DIMMER
    #elif LIGHT_TYPE_SEL == LIGHT_TYPE_1WW_3DIM
        #define  MESH_PID_SEL       LM_1WW_3DIMMER
    #elif LIGHT_TYPE_SEL == LIGHT_TYPE_RGB_AND_WW
        #define  MESH_PID_SEL       LM_RGB_AND_WW_WIRE
    #elif LIGHT_TYPE_SEL == LIGHT_TYPE_2_CCT
        #define  MESH_PID_SEL       LM_2_CCT
	#endif
   #define MESH_VID		    FW_VERSION_TELINK_RELEASE       // user can redefine

#elif (__PROJECT_MESH_GW_NODE_HK__)   // light
#define MESH_PID_SEL		(PID_GW_NODE_HK)
#define MESH_VID		    FW_VERSION_TELINK_RELEASE       // user can redefine
#elif (__PROJECT_BOOTLOADER__)   // light
#define MESH_PID_SEL		(PID_LIGHT)						// 
#define MESH_VID		    FW_VERSION_TELINK_RELEASE       // user can redefine
#else
//#error: must define PID, VID, no default value.
#endif

	#if DU_ENABLE
#define BUILD_VERSION		DU_PID	// if value change, must make clean. same sequence with cps
	#else
#define BUILD_VERSION		(MESH_PID_SEL|(MESH_VID << 16))	// if value change, must make clean. same sequence with cps
	#endif

// -- 
#define RUN_128K_IN_20000_EN 			0 // be enable to run 128K in 0x20000(default is 124k), disable to save RAM code.

#if __PROJECT_BOOTLOADER__
#define MCU_RUN_SRAM_WITH_CACHE_EN      1   // must with cache, because read flash by pointer 
#else
#define MCU_RUN_SRAM_WITH_CACHE_EN      0   // 
#endif
#define MCU_RUN_SRAM_EN                 0   // manual set data address in boot.link.can't read flash by pointer 

#if (MI_SWITCH_LPN_EN || __PROJECT_MESH_LPN__ || __PROJECT_SPIRIT_LPN__ || __PROJECT_MESH_SWITCH__)
#define MCU_STACK_INIT_EN               (0) // because stack may be in retention RAM HW, and also to decrease time cost of wake up.
#else
#define MCU_STACK_INIT_EN               (1)
#endif

// must set ram size according to the chip type
#ifndef RAM_SIZE_MAX
#define RAM_SIZE_MAX            		(64*1024)
#endif

#ifndef __IRQ_STACK_SIZE__
	#if EXTENDED_ADV_ENABLE
#define __IRQ_STACK_SIZE__            	(0x280)	// cost about 0x1D0 for demo SDK. // because call irq_mesh_sec_msg_check_cache in irq state.
	#else
#define __IRQ_STACK_SIZE__            	(0x180)
	#endif
#endif

