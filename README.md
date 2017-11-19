# S3TConv
**A library for load-time conversion of S3 Texture Compression textures to other formats.**

Currently, the library only supports conversion to **ATI Texture Compression (ATITC or ATC)** used on Qualcomm Adreno.

S3TConv is designed for load-time conversion, primarily for mobile ports of PC games. It doesn't re-compress images or blocks, instead, it performs a fixed set of checks to re-use the existing colors and color indices from S3TC blocks in the most accurate way.

The ATITC conversion method is inspired by "[A Method for Load-Time Conversion of DXTC Assets to ATC](http://www.guildsoftware.com/papers/2012.Converting.DXTC.to.ATC.pdf)" by Ray Ratelis and John Bergman of [Guild Software](http://www.guildsoftware.com), but expanded to support both DXT1 modes and to give a higher bit depth to brighter colors, which matches the way ATITC encodes colors by design.

As various S3TC compressors provide generally better results than the ATITC encoder in Compressonator, S3TConv can also be used offline to prepare compressed textures for Adreno, in some cases creating images more accurate than Compressonator.

![Conversion demo](/demo.png?raw=true)

Usage
-----
Add the C code and header files to your project and `#include "s3tconv.h"`.

The library performs compression on block level, so you'll generally need to call the conversion functions in a loop through 8-byte DXT1 or 16-byte DXT3/DXT5 blocks.

Some functions have `remainingWidth` and `remainingHeight` parameters. They are used to skip padding colors if the size of the image is not a multiple of 4 (or it's one of the smallest mipmaps). You need to pass the number of pixels left in the row/column starting from the leftmost/topmost pixel of the block. For mid-image blocks, they must be 4 or more, for right and bottom edges, they may be 4, 3, 2 or 1.

The conversion functions may also take the `asDXT1` parameter, which should be:
* 1 for DXT1.
* 0 for DXT3 and DXT5, but in certain cases, may depend on how the textures were originally compressed:
    * NVIDIA GeForce 6xxx and 7xxx (2004–2006) graphics cards violate the S3TC specification by decoding DXT3 and DXT5 the same way as DXT1 — supporting both modes. This has behavior been corrected in newer models. According to the specification, DXT3 and DXT5 should use only one mode, regardless of whether the first or the second color is greater.
    * Compressors usually take this into account, using only one mode and make the first color greater than the second, so both 0 and 1 should be fine.
    * If that's not the case, 0 should be passed for specification-conforming textures, and 1 for textures targeting these GPUs.

### DXT1 to ATITC
DXT1 has two ways of decoding — with and without punch-through alpha. ATITC doesn't support punch-through alpha, so it needs to be converted to explicit or to interpolated alpha if used.

ATITC textures with alpha require twice as large storage as RGB-only textures. If you don't know in advance whether the texture has a punch-through alpha, you can check if a block has alpha using `S3TConv_DXT1_BlockHasPunchthroughPixels`.

To convert to `ATC_RGB_AMD` (8 bytes per block), call `S3TConv_ATITC_RGBBlockFromDXT` for every 8-byte DXT1 block.

To convert to `ATC_RGBA_EXPLICIT_ALPHA_AMD` or `ATC_RGBA_INTERPOLATED_ALPHA_AMD` (16 bytes per block), use `S3TConv_DXT1_PunchthroughToExplicitAlpha` or `S3TConv_DXT1_PunchthroughToInterpolatedAlpha` on the DXT1 block to write the first 8 bytes of the ATITC block, and `S3TConv_ATITC_RGBBlockFromDXT` to write the second 8 bytes.

### DXT3 to `ATC_RGBA_EXPLICIT_ALPHA_AMD` or DXT5 to `ATC_RGBA_INTERPOLATED_ALPHA_AMD`
S3TC and ATITC use the same methods of encoding explicit and interpolated alpha, so for every 16-byte block, simply copy the first 8 bytes and run `S3TConv_ATITC_RGBBlockFromDXT` on the second 8 bytes.
