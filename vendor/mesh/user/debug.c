/*
 * debug.c
 *
 *  Created on: Sep 30, 2019
 *      Author: DungTran BK
 */

#include "proj/tl_common.h"
#include "proj/drivers/uart.h"
#include "./printf/my_printf.h"
#include "debug.h"

/**
 * @func    Int2String
 * @brief
 * @param
 * @retval  None
 */
static void Int2String(u16 i, s8 *s)	// Convert Integer to String
{
	u8 len;
	s8 *p;
	len = 0;
	p = s;
	do {
		*s = (i % 10) + '0';
		s++;
		len++;
		i /= 10;
	} while (i != 0);
	for (i = 0; i < len / 2; i++) {
		p[len] = p[i];
		p[i] = p[len - 1 - i];
		p[len - 1 - i] = p[len];
	}
	p[len] = 0;
}
/**
 * @func    Dbg_sendString
 * @brief
 * @param
 * @retval  None
 */
void Dbg_sendString(s8 *s)
{
	while(*s){
		#if MY_PRINTF_DEBUG_EN
		my_printf_send_bytes((u8*)s++, 1);
		#endif
	}
}
/**
 * @func    Int2String
 * @brief
 * @param
 * @retval  None
 */
static void Dword2String(u32 i, s8 *s)	// Convert Integer to String
{
	u8 len;
	s8 *p;
	len = 0;
	p = s;
	do {
		*s = (i % 10) + '0';
		s++;
		len++;
		i /= 10;
	} while (i != 0);
	for (i = 0; i < len / 2; i++) {
		p[len] = p[i];
		p[i] = p[len - 1 - i];
		p[len - 1 - i] = p[len];
	}
	p[len] = 0;
}
/**
 * @func    Dbg_sendInt
 * @brief
 * @param
 * @retval  None
 */
void Dbg_sendDword(u32 data)
{
	s8 s[11];
	Dword2String(data, &s[0]);
	Dbg_sendString(&s[0]);
}
/**
 * @func    Dbg_sendInt32
 * @brief
 * @param
 * @retval  None
 */
void Dbg_sendInt(u16 data)
{
	s8 s[7];
	Int2String(data, &s[0]);
	Dbg_sendString(&s[0]);
}

/**
 * @func   hex2Char
 * @brief  None
 * @param  None
 * @retval
 */
char Dbg_hex2Char(char byHex)
{
	char byChar;

    if (byHex < 10) byChar = byHex + 0x30;
    else byChar = byHex + 55;

    return byChar;
}
/**
 * @func    Dbg_sendHex
 * @brief
 * @param
 * @retval  None
 */
void Dbg_sendHex(u16 data)
{
	Dbg_sendString((s8*)"0x");

    char str[7];

    str[0] = Dbg_hex2Char((data&0xF000) >> 12);
    str[1] = Dbg_hex2Char((data&0x0F00) >> 8);
    str[2] = Dbg_hex2Char((data&0x00F0) >> 4);
    str[3] = Dbg_hex2Char(data&0x000F);
    str[4] = 0;

	Dbg_sendString((s8*)str);
}

/**
 * @func    Dbg_sendHex
 * @brief
 * @param
 * @retval  None
 */
void Dbg_sendHex32(u32 data)
{
	Dbg_sendString((s8*)"0x");
    char str[10];

    str[0] = Dbg_hex2Char((data&0xF0000000) >> 28);
    str[1] = Dbg_hex2Char((data&0xF000000) >> 24);
    str[2] = Dbg_hex2Char((data&0xF00000) >> 20);
    str[3] = Dbg_hex2Char((data&0xF0000) >> 16);

    str[4] = Dbg_hex2Char((data&0xF000) >> 12);
    str[5] = Dbg_hex2Char((data&0x0F00) >> 8);
    str[6] = Dbg_hex2Char((data&0x00F0) >> 4);
    str[7] = Dbg_hex2Char(data&0x000F);
    str[8] = 0;

	Dbg_sendString((s8*)str);
}
/**
 * @func    Dbg_sendHexOneByte
 * @brief
 * @param
 * @retval  None
 */
void Dbg_sendHexOneByte(u16 data)
{
    char str[4];
    str[0] = Dbg_hex2Char((data&0x00F0) >> 4);
    str[1] = Dbg_hex2Char(data&0x000F);
    str[2] = 0;
	Dbg_sendString((s8*)str);
}

/**
 * @func    Dbg_sendHex
 * @brief
 * @param
 * @retval  None
 */
void Dbg_sendOneByteHex(s8 data)
{
    char str[7];
    str[0] = Dbg_hex2Char((data&0x00F0) >> 4);
    str[1] = Dbg_hex2Char(data&0x000F);
    str[2] = 0;
	Dbg_sendString((s8*)str);
}
/**
 * @func    Dbg_sendByte
 * @brief
 * @param
 * @retval  None
 */
void Dbg_sendByte(u8 data)
{
	#if MY_PRINTF_DEBUG_EN
	my_printf_send_bytes(&data, 1);
	#endif
}
