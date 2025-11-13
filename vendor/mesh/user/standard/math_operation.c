/*
 * math_operation.c
 *
 *  Created on: Oct 5, 2023
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "tl_common.h"
#include "vendor/common/light.h"
#include "vendor/common/system_time.h"
#include "proj_lib/ble/blt_config.h"
#include "math_operation.h"

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/


/******************************************************************************/
/*                       PRIVATE FUNCTION DECLERATION                         */
/******************************************************************************/


/******************************************************************************/
/*                            EXPORT FUNCTION                                 */
/******************************************************************************/

/**
 * @func    compare_float
 * @brief
 * @param   None
 * @retval  None
 */
int compare_float(double f1, double f2)
{
	double precision = 0.00000000000000000001;
	if ((f1 - precision) < f2)
	{
		return -1;
	}
	else if ((f1 + precision) > f2)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/**
 * @func    cos
 * @brief
 * @param   None
 * @retval  None
 */
double cos(double x)
{
	if( x < 0.0f ) x = -x;

	if (0 <= compare_float(x,M_PI_M_2))
	{
		do {
			x -= M_PI_M_2;
		}while(0 <= compare_float(x,M_PI_M_2));
	}

	if ((0 <= compare_float(x, M_PI)) && (-1 == compare_float(x, M_PI_M_2)))
	{
		x -= M_PI;
		return ((-1)*(1.0f - (x*x/2.0f)*( 1.0f - (x*x/12.0f) * ( 1.0f - (x*x/30.0f) * (1.0f - (x*x/56.0f )*(1.0f - (x*x/90.0f)*(1.0f - (x*x/132.0f)*(1.0f - (x*x/182.0f)))))))));
	}
	return 1.0f - (x*x/2.0f)*( 1.0f - (x*x/12.0f) * ( 1.0f - (x*x/30.0f) * (1.0f - (x*x/56.0f )*(1.0f - (x*x/90.0f)*(1.0f - (x*x/132.0f)*(1.0f - (x*x/182.0f)))))));
}

/**
 * @func    sin
 * @brief
 * @param   None
 * @retval  None
 */
double sin(double x)
{
	return cos(x-M_PI_2);
}
// End file
