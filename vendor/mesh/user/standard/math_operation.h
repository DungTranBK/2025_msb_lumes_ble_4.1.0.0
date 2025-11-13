/*
 * math_operation.h
 *
 *  Created on: Oct 5, 2023
 *      Author: DungTranBK
 */

#ifndef MATH_OPERATION_H_
#define MATH_OPERATION_H_
/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/
#define M_PI       (3.1415927f)
#define M_PI_2     (M_PI/2.0f)
#define M_PI_M_2   (M_PI*2.0f)

/******************************************************************************/
/*                             EXPORT FUNCTIONS                               */
/******************************************************************************/
int compare_float(double f1, double f2);
double cos(double x);
double sin(double x);

#endif /* MATH_OPERATION_H_ */
