/*
 * Copyright (c) 2023, smartmx - smartmx@qq.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#ifndef _PRINTF_CONFIG_H_
#define _PRINTF_CONFIG_H_

#include "debug.h"

__forceinline void putchar_(char c)
{
#if(DEBUG == DEBUG_UART1)
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    USART1->DATAR = c;
#elif(DEBUG == DEBUG_UART2)
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
    USART2->DATAR = c;
#elif(DEBUG == DEBUG_UART3)
    while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
    USART3->DATAR = c;
#endif
}

/* Support for the decimal notation floating point conversion specifiers (%f, %F), 0 or 1 */
#define PRINTF_SUPPORT_DECIMAL_SPECIFIERS               0

/* Support for the exponential notation floating point conversion specifiers (%e, %g, %E, %G), 0 or 1 */
#define PRINTF_SUPPORT_EXPONENTIAL_SPECIFIERS           0

/* Support for the length write-back specifier (%n), 0 or 1 */
#define PRINTF_SUPPORT_WRITEBACK_SPECIFIER              1

/*
 * Support for the long long integral types (with the ll, z and t length modifiers for specifiers
 * %d,%i,%o,%x,%X,%u, and with the %p specifier).
 * Note: 'L' (long double) is not supported, 0 or 1.
 */
#define PRINTF_SUPPORT_LONG_LONG                        1

/*
 * 'ntoa' conversion buffer size, this must be big enough to hold one converted
 * numeric number including padded zeros (dynamically created on stack)
 */
#define PRINTF_INTEGER_BUFFER_SIZE                      32

/*
 * size of the fixed (on-stack) buffer for printing individual decimal numbers.
 * this must be big enough to hold one converted floating-point value including
 * padded zeros.
*/
#define PRINTF_DECIMAL_BUFFER_SIZE                      32

/* Default precision for the floating point conversion specifiers (the C standard sets this at 6) */
#define PRINTF_DEFAULT_FLOAT_PRECISION                  6

/*
 * According to the C languages standard, printf() and related functions must be able to print any
 * integral number in floating-point notation, regardless of length, when using the %f specifier -
 * possibly hundreds of characters, potentially overflowing your buffers. In this implementation,
 * all values beyond this threshold are switched to exponential notation.
 */
#define PRINTF_MAX_INTEGRAL_DIGITS_FOR_DECIMAL          9

/*
 * The number of terms in a Taylor series expansion of log_10(x) to
 * use for approximation - including the power-zero term (i.e. the
 * value at the point of expansion).
*/
#define PRINTF_LOG10_TAYLOR_TERMS                       4

/*
 * Be extra-safe, and don't assume format specifiers are completed correctly
 * before the format string end.
*/
#define PRINTF_CHECK_FOR_NUL_IN_FORMAT_SPECIFIER        1

/*
 * Support for msvc style integer specifiers (%I), comment it to disable.
*/
//#define PRINTF_SUPPORT_MSVC_STYLE_INTEGER_SPECIFIERS

#endif /* _PRINTF_CONFIG_H_ */
