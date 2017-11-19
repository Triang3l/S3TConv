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

static inline unsigned int S3TConv_ATITC_GetLuminance(const uint8_t color888[3]) {
	// From Compressonator, matches the hardware calculation.
	return (19 * (unsigned int) color888[0] +
			38 * (unsigned int) color888[1] +
			7 * (unsigned int) color888[2]) >> 6;
}

static void S3TConv_ATITC_ConvertBlackTrickDiscardingLowOrHigh(
		uint16_t colorLow565, uint16_t colorHigh565, unsigned int lumaLow, unsigned int lumaHigh,
		uint32_t dxtIndices, unsigned int indexCountLow, unsigned int indexCountHigh,
		uint16_t *atitcColorLow555, uint16_t *atitcColorHigh565, uint32_t *atitcIndices) {
	uint16_t colorMed565;
	uint8_t colorMed888[3];
	unsigned int lumaMed;
	uint32_t colorIndexMask;

	colorMed565 = (((colorLow565 & 0x001F) + (colorHigh565 & 0x001F)) >> 1) |
			((((colorLow565 & 0x07E0) + (colorHigh565 & 0x07E0)) >> 1) & 0x07E0) |
			((((colorLow565 & 0xF800) + (colorHigh565 & 0xF800)) >> 1) & 0xF800);
	S3TConv_Utility_Color565To888(colorMed565, colorMed888);
	lumaMed = S3TConv_ATITC_GetLuminance(colorMed888);

	colorIndexMask = dxtIndices & 0x55555555 & ((dxtIndices & 0xAAAAAAAA) >> 1);
	colorIndexMask = ~(colorIndexMask | (colorIndexMask << 1));

	if (indexCountLow > indexCountHigh) { // Not >= because low gets less green bits than high.
		if (lumaMed >= lumaLow) {
			*atitcColorLow555 = 0x8000 | S3TConv_Utility_Color565To555(colorLow565);
			*atitcColorHigh565 = colorMed565;
			// 0 1 2 3 -> 2 3 3 0.
			*atitcIndices = (dxtIndices | 0xAAAAAAAA | ((dxtIndices & 0xAAAAAAAA) >> 1)) & colorIndexMask;
		} else {
			// Generally shouldn't happen without hue variation.
			*atitcColorLow555 = 0x8000 | S3TConv_Utility_Color565To555(colorMed565);
			*atitcColorHigh565 = colorLow565;
			// 0 1 2 3 -> 3 2 2 0.
			*atitcIndices = ((dxtIndices | 0xAAAAAAAA) ^ ((~dxtIndices & 0xAAAAAAAA) >> 1)) & colorIndexMask;
		}
	} else {
		if (lumaMed <= lumaHigh) {
			*atitcColorLow555 = 0x8000 | S3TConv_Utility_Color565To555(colorMed565);
			*atitcColorHigh565 = colorHigh565;
			// 0 1 2 3 -> 2 3 2 0.
			*atitcIndices = (0xAAAAAAAA | dxtIndices) & colorIndexMask;
		} else {
			// Generally shouldn't happen without hue variation.
			*atitcColorLow555 = 0x8000 | S3TConv_Utility_Color565To555(colorHigh565);
			*atitcColorHigh565 = colorMed565;
			// 0 1 2 3 -> 3 2 3 0.
			*atitcIndices = ((dxtIndices | 0xAAAAAAAA) ^ 0x55555555) & colorIndexMask;
		}
	}
}

void S3TConv_ATITC_RGBBlockFromDXT(const uint8_t dxtBlock[8], int asDXT1, uint8_t atitcBlock[8],
		unsigned int remainingWidth, unsigned int remainingHeight) {
	// Source block data.
	uint16_t dxtColor0565, dxtColor1565;
	uint8_t dxtColor0888[3], dxtColor1888[3];
	unsigned int dxtLuma0, dxtLuma1;
	uint32_t dxtSourceIndices;

	// Resulting block data.
	uint16_t atitcColorLow555, atitcColorHigh565;
	uint32_t atitcIndices;

	// Extracting data from the DXT block bytes.
	dxtColor0565 = (uint16_t) dxtBlock[0] | ((uint16_t) dxtBlock[1] << 8);
	dxtColor1565 = (uint16_t) dxtBlock[2] | ((uint16_t) dxtBlock[3] << 8);
	dxtSourceIndices = (uint32_t) dxtBlock[4] | ((uint32_t) dxtBlock[5] << 8) |
			((uint32_t) dxtBlock[6] << 16) | ((uint32_t) dxtBlock[7] << 24);
	S3TConv_Utility_Color565To888(dxtColor0565, dxtColor0888);
	dxtLuma0 = S3TConv_ATITC_GetLuminance(dxtColor0888);
	S3TConv_Utility_Color565To888(dxtColor1565, dxtColor1888);
	dxtLuma1 = S3TConv_ATITC_GetLuminance(dxtColor1888);

	// Handling 2 main modes.
	if (!asDXT1 || dxtColor0565 > dxtColor1565) {
		// The RGB0, RGB1, (2*RGB0+RGB1)/3, (RGB0+2*RGB1)/3 mode.
		// Simply reordering the indices. DXT may be implemented as 1/3 and 2/3 or 3/8 and 5/8. ATITC is 3/8 and 5/8.
		// 0 1 2 3 -> 0 3 1 2, if color 0 is the lower one (which is unlikely though).
		atitcIndices = dxtSourceIndices ^ ((dxtSourceIndices & 0xAAAAAAAA) >> 1);
		atitcIndices ^= (atitcIndices & 0x55555555) << 1;
		if (dxtLuma0 >= dxtLuma1) {
			atitcColorLow555 = S3TConv_Utility_Color565To555(dxtColor1565);
			atitcColorHigh565 = dxtColor0565;
			atitcIndices = ~atitcIndices;
		} else {
			atitcColorLow555 = S3TConv_Utility_Color565To555(dxtColor0565);
			atitcColorHigh565 = dxtColor1565;
		}
	} else {
		// The RGB0, RGB1, (RGB0+RGB1)/2, BLACK mode. In general, can't be represented exactly by ATITC.

		uint16_t dxtColorLow565, dxtColorHigh565;
		const uint8_t *dxtColorLow888, *dxtColorHigh888;
		unsigned int dxtLumaLow, dxtLumaHigh;
		uint32_t dxtIndices = dxtSourceIndices;

		unsigned int dxtIndexCount[4] = { 0 }, dxtLowHighIndexCounts[4][2] = { 0 };
		unsigned int pixelIndex;

		// Sort the DXT colors by luminance, so it's easier to interpolate and to match the way ATITC sorts shades.
		if (dxtLuma0 <= dxtLuma1) {
			dxtColorLow565 = dxtColor0565;
			dxtColorHigh565 = dxtColor1565;
			dxtColorLow888 = dxtColor0888;
			dxtColorHigh888 = dxtColor1888;
			dxtLumaLow = dxtLuma0;
			dxtLumaHigh = dxtLuma1;
		} else {
			dxtColorLow565 = dxtColor1565;
			dxtColorHigh565 = dxtColor0565;
			dxtColorLow888 = dxtColor1888;
			dxtColorHigh888 = dxtColor0888;
			dxtLumaLow = dxtLuma1;
			dxtLumaHigh = dxtLuma0;
			dxtIndices ^= (~dxtIndices & 0xAAAAAAAA) >> 1; // Swap RGB0 and RGB1.
		}

		// Count all indices in the whole block, and also low and high colors in 2x2 corners, so less common ones may be neglected.
		for (pixelIndex = 0; pixelIndex < 16; ++pixelIndex) {
			if ((pixelIndex & 3) >= remainingWidth || (pixelIndex >> 2) >= remainingHeight) {
				continue;
			}
			unsigned int dxtIndex = (dxtIndices >> (pixelIndex << 1)) & 3;
			++dxtIndexCount[dxtIndex];
			// Count low and high colors in 2x2 corners in case they will be used to replace the medium color with close interpolations.
			if ((dxtIndex & 2) == 0) {
				++dxtLowHighIndexCounts[(pixelIndex >> 2) ^ (((pixelIndex >> 1) ^ (pixelIndex >> 2)) & 1)][dxtIndex];
			}
		}

		// Try various conversions, from the most accurate ones in specific cases to the general ones.
		if (dxtIndexCount[2] == 0) {
			// If at least one shade is not used, the remaining two can be converted exactly.
			// The medium color isn't used.
			atitcColorLow555 = 0x8000 | S3TConv_Utility_Color565To555(dxtColorLow565);
			atitcColorHigh565 = dxtColorHigh565;
			// 0 1 3 -> 2 3 0.
			atitcIndices = (dxtIndices ^ 0xAAAAAAAA) & ~((dxtIndices & 0xAAAAAAAA) >> 1);
		} else if (dxtIndexCount[0] == 0 || dxtIndexCount[1] == 0) {
			// Similar case, but either low or high isn't used.
			S3TConv_ATITC_ConvertBlackTrickDiscardingLowOrHigh(
					dxtColorLow565, dxtColorHigh565, dxtLumaLow, dxtLumaHigh,
					dxtIndices, dxtIndexCount[0], dxtIndexCount[1],
					&atitcColorLow555, &atitcColorHigh565, &atitcIndices);
		} else if (dxtIndexCount[3] == 0) {
			// If black isn't used, approximate 1/2 with 3/8 or 5/8, depending on whether low or high is more common in each 2x2 corner.
			uint32_t medIndexMask;
			int lowIsMoreCommon = (dxtIndexCount[0] > dxtIndexCount[1]); // Not >= because high has more green bits.
			unsigned int subBlockIndex;
			atitcColorLow555 = S3TConv_Utility_Color565To555(dxtColorLow565);
			atitcColorHigh565 = dxtColorHigh565;
			// 0 1 2 -> 0 3 2, or 0 2 1 -> 0 3 1 if the lower approximation is the closer one.
			atitcIndices = dxtIndices | ((dxtIndices & 0x55555555) << 1);
			medIndexMask = dxtIndices & 0xAAAAAAAA;
			medIndexMask |= medIndexMask >> 1;
			for (subBlockIndex = 0; subBlockIndex < 4; ++subBlockIndex) {
				unsigned int countLow = dxtLowHighIndexCounts[subBlockIndex][0], countHigh = dxtLowHighIndexCounts[subBlockIndex][1];
				if (countLow > countHigh || (lowIsMoreCommon && countLow == countHigh)) {
					atitcIndices ^= (0x00000F0F << (((subBlockIndex & 2) << 3) | ((subBlockIndex & 1) << 2))) & medIndexMask;
				}
			}
		} else {
			// 3 shades will be used if the resulting approximation is near-perfect (if medium is the most common),
			// or if it's at least somewhere between the original colors (if low or high is more common than medium).
			// In other cases, 2 shades will be picked, depending on which ones are the most common.

			unsigned int colorBlackTrickMedHigh888[3];
			unsigned int blackTrickBoundScaleLow, blackTrickBoundScaleHigh;
			unsigned int blackTrickBoundLow[3], blackTrickBoundHigh[3];

			colorBlackTrickMedHigh888[0] = dxtColorLow888[0] + (dxtColorHigh888[0] >> 2);
			colorBlackTrickMedHigh888[1] = dxtColorLow888[1] + (dxtColorHigh888[1] >> 2);
			colorBlackTrickMedHigh888[2] = dxtColorLow888[2] + (dxtColorHigh888[2] >> 2);

			blackTrickBoundScaleLow = ((dxtIndexCount[2] >= dxtIndexCount[0] && dxtIndexCount[2] >= dxtIndexCount[1]) ? 3 : 0);
			blackTrickBoundScaleHigh = 8 - blackTrickBoundScaleLow;

			blackTrickBoundLow[0] = (blackTrickBoundScaleHigh * dxtColorLow888[0] + blackTrickBoundScaleLow * dxtColorHigh888[0]) >> 3;
			blackTrickBoundLow[1] = (blackTrickBoundScaleHigh * dxtColorLow888[1] + blackTrickBoundScaleLow * dxtColorHigh888[1]) >> 3;
			blackTrickBoundLow[2] = (blackTrickBoundScaleHigh * dxtColorLow888[2] + blackTrickBoundScaleLow * dxtColorHigh888[2]) >> 3;
			blackTrickBoundHigh[0] = (blackTrickBoundScaleLow * dxtColorLow888[0] + blackTrickBoundScaleHigh * dxtColorHigh888[0]) >> 3;
			blackTrickBoundHigh[1] = (blackTrickBoundScaleLow * dxtColorLow888[1] + blackTrickBoundScaleHigh * dxtColorHigh888[1]) >> 3;
			blackTrickBoundHigh[2] = (blackTrickBoundScaleLow * dxtColorLow888[2] + blackTrickBoundScaleHigh * dxtColorHigh888[2]) >> 3;

			// Will also ensure there's no overflow in each component.
			if (colorBlackTrickMedHigh888[0] >= blackTrickBoundLow[0] && colorBlackTrickMedHigh888[0] <= blackTrickBoundHigh[0] &&
					colorBlackTrickMedHigh888[1] >= blackTrickBoundLow[1] && colorBlackTrickMedHigh888[1] <= blackTrickBoundHigh[1] &&
					colorBlackTrickMedHigh888[2] >= blackTrickBoundLow[2] && colorBlackTrickMedHigh888[2] <= blackTrickBoundHigh[2]) {
				atitcColorLow555 = (uint16_t) (0x8000 | ((colorBlackTrickMedHigh888[0] >> 3) << 10) |
						((colorBlackTrickMedHigh888[1] >> 3) << 5) | (colorBlackTrickMedHigh888[2] >> 3));
				atitcColorHigh565 = dxtColorHigh565;
				// 0 1 2 3 -> 1 3 2 0.
				atitcIndices = ~dxtIndices;
				atitcIndices ^= (atitcIndices & 0x55555555) << 1;
				atitcIndices ^= (atitcIndices & 0xAAAAAAAA) >> 1;
			} else {
				// Can't use the black trick to represent all 3 shades, so discard one of them.
				if (dxtIndexCount[2] <= dxtIndexCount[0] && dxtIndexCount[2] <= dxtIndexCount[1]) {
					// Discard the medium shade.
					uint32_t colorIndexMask;
					atitcColorLow555 = 0x8000 | S3TConv_Utility_Color565To555(dxtColorLow565);
					atitcColorHigh565 = dxtColorHigh565;
					colorIndexMask = dxtIndices & 0x55555555 & ((dxtIndices & 0xAAAAAAAA) >> 1);
					colorIndexMask = ~(colorIndexMask | (colorIndexMask << 1));
					if (dxtIndexCount[0] > dxtIndexCount[1]) {
						// 0 1 2 3 -> 2 3 2 0.
						atitcIndices = (0xAAAAAAAA | dxtIndices) & colorIndexMask;
					} else {
						// 0 1 2 3 -> 2 3 3 0.
						atitcIndices = (dxtIndices | 0xAAAAAAAA | ((dxtIndices & 0xAAAAAAAA) >> 1)) & colorIndexMask;
					}
				} else {
					// Discard the low or the high shade.
					S3TConv_ATITC_ConvertBlackTrickDiscardingLowOrHigh(
							dxtColorLow565, dxtColorHigh565, dxtLumaLow, dxtLumaHigh,
							dxtIndices, dxtIndexCount[0], dxtIndexCount[1],
							&atitcColorLow555, &atitcColorHigh565, &atitcIndices);
				}
			}
		}
	}

	// Writing the ATITC block.
	atitcBlock[0] = (uint8_t) atitcColorLow555;
	atitcBlock[1] = (uint8_t) (atitcColorLow555 >> 8);
	atitcBlock[2] = (uint8_t) atitcColorHigh565;
	atitcBlock[3] = (uint8_t) (atitcColorHigh565 >> 8);
	atitcBlock[4] = (uint8_t) atitcIndices;
	atitcBlock[5] = (uint8_t) (atitcIndices >> 8);
	atitcBlock[6] = (uint8_t) (atitcIndices >> 16);
	atitcBlock[7] = (uint8_t) (atitcIndices >> 24);
}
