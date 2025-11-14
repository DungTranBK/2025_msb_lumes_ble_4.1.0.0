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

static inline void set_buff_with_check(char s[], int index, int len_max, char value)
{
	if(index < len_max){
		s[index] = value;
	}
}


#if 1
void itoa_one_char(int n, char s[], int radix)
{
	if(n < 0){
		n = -n;	// use abs value.
	}

	s[0] = n % radix + '0';
}

//
// Tim Hirzel
// tim@growdown.com
// March 2008
// float to string
// @param  places: Number of decimal points
// @retval len of outsrt
// If you don't save this as a .h, you will want to remove the default arguments
//     uncomment this first line, and swap it for the next.  I don't think keyword arguments compile in .pde files
//char * floatToString(char * outstr, float value, int places, int minwidth=0, bool rightjustify=false) {
int floatToString_ll(char * outstr, int len_max, float value, int places, int minwidth, bool rightjustify)
{
    // this is used to write a float value to string, outstr.  oustr is also the return value.
    int digit;
    float tens = (float)0.1;
    int tenscount = 0;
    int i;
    float tempfloat = value;
    int c = 0;
    int charcount = 1;
    int extra = 0;
    // make sure we round properly. this could use pow from <math.h>, but doesn't seem worth the import
    // if this rounding step isn't here, the value  54.321 prints as 54.3209

    // calculate rounding term d:   0.5/pow(10,places)
    float d = 0.5;
    if (value < 0)
        d *= -1.0;
    // divide by ten for each decimal place
    for (i = 0; i < places; i++)
        d/= 10.0;
    // this small addition, combined with truncation will round our values properly
    tempfloat +=  d;

    // first get value tens to be the large power of ten less than value
    if (value < 0)
        tempfloat *= -1.0;
    while ((tens * 10.0) <= tempfloat) {
        tens *= 10.0;
        tenscount += 1;
    }

    if (tenscount > 0)
        charcount += tenscount;
    else
        charcount += 1;

    if (value < 0)
        charcount += 1;
    charcount += 1 + places;

    minwidth += 1; // both count the null final character
    if (minwidth > charcount){
        extra = minwidth - charcount;
        charcount = minwidth;
    }

    if (extra > 0 && rightjustify) {
        for (int i = 0; i< extra; i++) {
        	set_buff_with_check(outstr, c++, len_max, ' ');
        }
    }

    // write out the negative if needed
    if (value < 0){
    	set_buff_with_check(outstr, c++, len_max, '-');
    }

    if (tenscount == 0){
		set_buff_with_check(outstr, c++, len_max, '0');
    }

    for (i=0; i< tenscount; i++) {
        digit = (int) (tempfloat/tens);
        if(c < len_max){
        	itoa_one_char(digit, &outstr[c++], 10);
        }else{
        	return c;
        }
        tempfloat = tempfloat - ((float)digit * tens);
        tens /= 10.0;
    }

    // if no places after decimal, stop now and return

    // otherwise, write the point and continue on
    if (places > 0){
    	set_buff_with_check(outstr, c++, len_max, '.');
    }


    // now write out each decimal place by shifting digits one by one into the ones place and writing the truncated value
    for (i = 0; i < places; i++) {
        tempfloat *= 10.0;
        digit = (int) tempfloat;
        if(c < len_max){
        	itoa_one_char(digit, &outstr[c++], 10);
        }else{
        	return c;
        }
        // once written, subtract off that digit
        tempfloat = tempfloat - (float) digit;
    }
    if (extra > 0 && !rightjustify) {
        for (int i = 0; i< extra; i++) {
			set_buff_with_check(outstr, c++, len_max, ' ');
        }
    }

	set_buff_with_check(outstr, c++, len_max, '\0');

    return c;
}

// @param  places: Number of decimal points
// @retval len of outsrt
int floatToString(char * outstr, int len_max, float value)
{
	#define DEFAULT_VALID_NUMBER	(0)
	int places = DEFAULT_VALID_NUMBER;
	float value_temp = value;
	if(value_temp < 0){
		value_temp = -value_temp;
	}

	for(unsigned int i = 0; i < (45 - DEFAULT_VALID_NUMBER); ++i){	// max 45 valid number
		if(value_temp > 1000000){ // only keep 7 valid number is enough
			break;
		}
		value_temp *= 10;
		places++;
	}

	int count = floatToString_ll(outstr, len_max, value, places, 0, 0);	// 6.5 valid number.
	if(count >= len_max){
		if(len_max > 0){
			outstr[len_max - 1] = '\0';
			if(len_max > 1){
				outstr[len_max - 2] = 'E';	// error flag
			}
		}
	}
	return count;
}

/**
 * @func   Dbg_sendFloat
 * @brief  None
 * @param  None
 * @retval
 */
void Dbg_sendFloat(float data)
{
	char temp[64];
	floatToString(temp, sizeof(temp), data);
	Dbg_sendString((s8*)temp);
}
#endif

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
