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

#ifndef S3TCONV_H
#define S3TCONV_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// Target-independent functions.
//

/**
 * Returns whether a DXT1 block contains transparent pixels.
 *
 * This can be used to choose the right destination format if the
 * target doesn't support encoding punch-through transparency
 * directly in color blocks (such as ATITC).
 *
 * @param rgbBlock DXT1 block data.
 * @return 1 if the block contains punch-through transparency,
 *         0 if it's fully opaque.
 * @param remainingWidth How many pixels left in the row starting
 *                       from the leftmost pixel of this block,
 *                       to skip padding (4 or more for full block).
 * @param remainingHeight How many pixels left in the column starting
 *                        from the topmost pixel of this block,
 *                        to skip padding (4 or more for full block).
 */
int S3TConv_DXT1_BlockHasPunchthroughPixels(const uint8_t rgbBlock[8],
		unsigned int remainingWidth, unsigned int remainingHeight);

/**
 * Extracts punch-through transparency from a DXT1 block to a DXT3
 * explicit alpha block, which can also be used for ATITC alpha.
 *
 * As an ATITC texture with alpha is twice as large as RGB-only one,
 * it's recommended to only use an RGBA destination format and
 * convert punch-through pixels only if there are any. This can be
 * checked using {@link S3TConv_DXT1_BlockHasPunchthroughPixels}.
 *
 * @param rgbBlock Source DXT1 block data.
 * @param alphaBlock Target DXT3 alpha block data.
 * @see S3TConv_DXT1_PunchthroughToInterpolatedAlpha
 */
void S3TConv_DXT1_PunchthroughToExplicitAlpha(const uint8_t rgbBlock[8], uint8_t alphaBlock[8]);

/**
 * Extracts punch-through transparency from a DXT1 block to a DXT5
 * interpolated alpha block, which can also be used for ATITC alpha.
 *
 * As an ATITC texture with alpha is twice as large as RGB-only one,
 * it's recommended to only use an RGBA destination format and
 * convert punch-through pixels only if there are any. This can be
 * checked using {@link S3TConv_DXT1_BlockHasPunchthroughPixels}.
 *
 * @param rgbBlock Source DXT1 block data.
 * @param alphaBlock Target DXT5 alpha block data.
 * @see S3TConv_DXT1_PunchthroughToExplicitAlpha
 */
void S3TConv_DXT1_PunchthroughToInterpolatedAlpha(const uint8_t rgbBlock[8], uint8_t alphaBlock[8]);

//
// ATI Texture Compression (Qualcomm Adreno) conversion.
//

/**
 * Converts a DXT RGB block to ATITC.
 *
 * Alpha blocks can be copied directly for DXT3 (explicit alpha)
 * and DXT5 (interpolated alpha), and for RGBA DXT1, punch-through
 * transparency can be converted using either
 * {@link S3TConv_DXT1_PunchthroughToExplicitAlpha} or
 * {@link S3TConv_DXT1_PunchthroughToInterpolatedAlpha}.
 *
 * @param dxtBlock Source DXT RGB block data.
 * @param asDXT1 1 or other non-zero value if the texture is DXT1,
 *               or if it's DXT3/DXT5 targeting GeForce 6xxx/7xxx,
 *               0 if it's specification-conforming DXT3/DXT5.
 * @param atitcBlock Target ATITC RGB block data.
 * @param remainingWidth How many pixels left in the row starting
 *                       from the leftmost pixel of this block,
 *                       to skip padding (4 or more for full block).
 * @param remainingHeight How many pixels left in the column starting
 *                        from the topmost pixel of this block,
 *                        to skip padding (4 or more for full block).
 */
void S3TConv_ATITC_RGBBlockFromDXT(const uint8_t dxtBlock[8], int asDXT1, uint8_t atitcBlock[8],
		unsigned int remainingWidth, unsigned int remainingHeight);

#ifdef __cplusplus
}
#endif

#endif
