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

#include "s3tconv_internal.h"

void S3TConv_Utility_Color565To888(uint16_t color565, uint8_t color888[3]) {
	// From Compressonator.
	color888[0] = (uint8_t) (((color565 & 0xF800) >> 8) | ((color565 & 0xE000) >> 13));
	color888[1] = (uint8_t) (((color565 & 0x07E0) >> 3) | ((color565 & 0x0600) >> 9));
	color888[2] = (uint8_t) (((color565 & 0x001F) << 3) | ((color565 & 0x001C) >> 2));
}

int S3TConv_DXT1_BlockHasPunchthroughPixels(const uint8_t rgbBlock[8],
		unsigned int remainingWidth, unsigned int remainingHeight) {
	uint32_t indices;
	if (((uint16_t) rgbBlock[0] | ((uint16_t) rgbBlock[1] << 8)) >
			((uint16_t) rgbBlock[2] | ((uint16_t) rgbBlock[3] << 8))) {
		return 0;
	}
	indices = (uint32_t) rgbBlock[4] | ((uint32_t) rgbBlock[5] << 8) |
			((uint32_t) rgbBlock[6] << 16) | ((uint32_t) rgbBlock[7] << 24);
	if (remainingWidth < 4) {
		const uint32_t remainingWidthMask[] = { 0, 0x03030303, 0x0F0F0F0F, 0x3F3F3F3F };
		indices &= remainingWidthMask[remainingWidth];
	}
	if (remainingHeight < 4) {
		indices &= (1 << (remainingHeight << 3)) - 1;
	}
	return (indices & 0x55555555 & ((indices >> 1) & 0x55555555)) != 0;
}

void S3TConv_DXT1_PunchthroughToExplicitAlpha(const uint8_t rgbBlock[8], uint8_t alphaBlock[8]) {
	uint32_t colorIndexMask;
	unsigned int alphaByteIndex;

	if (((uint16_t) rgbBlock[0] | ((uint16_t) rgbBlock[1] << 8)) >
			((uint16_t) rgbBlock[2] | ((uint16_t) rgbBlock[3] << 8))) {
		alphaBlock[0] = alphaBlock[1] = alphaBlock[2] = alphaBlock[3] =
				alphaBlock[4] = alphaBlock[5] = alphaBlock[6] = alphaBlock[7] = 0;
		return;
	}

	colorIndexMask = (uint32_t) rgbBlock[4] | ((uint32_t) rgbBlock[5] << 8) |
			((uint32_t) rgbBlock[6] << 16) | ((uint32_t) rgbBlock[7] << 24);
	colorIndexMask = colorIndexMask & 0x55555555 & ((colorIndexMask & 0xAAAAAAAA) >> 1);
	colorIndexMask = ~(colorIndexMask | (colorIndexMask << 1));

	for (alphaByteIndex = 0; alphaByteIndex < 8; ++alphaByteIndex) {
		alphaBlock[alphaByteIndex] = (uint8_t) ((colorIndexMask & 0x3) | ((colorIndexMask & 0x3) << 2) |
				((colorIndexMask & 0xC) << 2) | ((colorIndexMask & 0xC) << 4));
		colorIndexMask >>= 4;
	}
}

void S3TConv_DXT1_PunchthroughToInterpolatedAlpha(const uint8_t rgbBlock[8], uint8_t alphaBlock[8]) {
	uint32_t blackIndexBits;
	uint32_t alphaCodes;
	unsigned int pixelIndex;

	if (((uint16_t) rgbBlock[0] | ((uint16_t) rgbBlock[1] << 8)) >
			((uint16_t) rgbBlock[2] | ((uint16_t) rgbBlock[3] << 8))) {
		alphaBlock[0] = alphaBlock[1] = 0xFF;
		alphaBlock[2] = alphaBlock[3] = alphaBlock[4] =
				alphaBlock[5] = alphaBlock[6] = alphaBlock[7] = 0;
		return;
	}

	blackIndexBits = (uint32_t) rgbBlock[4] | ((uint32_t) rgbBlock[5] << 8) |
			((uint32_t) rgbBlock[6] << 16) | ((uint32_t) rgbBlock[7] << 24);
	blackIndexBits = blackIndexBits & 0x55555555 & ((blackIndexBits & 0xAAAAAAAA) >> 1);

	alphaBlock[0] = 0xFF;
	alphaBlock[1] = 0;
	alphaCodes = 0;
	for (pixelIndex = 0; pixelIndex < 8; ++pixelIndex) {
		alphaCodes |= ((blackIndexBits >> (pixelIndex << 1)) & 1) << (pixelIndex * 3);
	}
	alphaBlock[2] = (uint8_t) alphaCodes;
	alphaBlock[3] = (uint8_t) (alphaCodes >> 8);
	alphaBlock[4] = (uint8_t) (alphaCodes >> 16);
	blackIndexBits >>= 16;
	alphaCodes = 0;
	for (pixelIndex = 0; pixelIndex < 8; ++pixelIndex) {
		alphaCodes |= ((blackIndexBits >> (pixelIndex << 1)) & 1) << (pixelIndex * 3);
	}
	alphaBlock[5] = (uint8_t) alphaCodes;
	alphaBlock[6] = (uint8_t) (alphaCodes >> 8);
	alphaBlock[7] = (uint8_t) (alphaCodes >> 16);
}
