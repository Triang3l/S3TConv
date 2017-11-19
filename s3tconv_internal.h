/*
Part of S3TConv, a library for converting S3TC textures to other formats.
https://github.com/Triang3l/S3TConv

Copyright (c) 2017 Triang3l.
Contains portions of Compressonator:
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
Copyright (c) 2004-2006 ATI Technologies Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef S3TCONV_INTERNAL_H
#define S3TCONV_INTERNAL_H

#include "s3tconv.h"

#ifdef __cplusplus
extern "C" {
#endif

void S3TConv_Utility_Color565To888(uint16_t color565, uint8_t color888[3]);

static inline uint16_t S3TConv_Utility_Color565To555(uint16_t color565) {
	return (color565 & 0x001F) | ((color565 & 0xFFC0) >> 1);
}

#ifdef __cplusplus
}
#endif

#endif
