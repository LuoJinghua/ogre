/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2014 Torus Knot Software Ltd

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
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "OgreGLES2PixelFormat.h"
#include "OgreRoot.h"
#include "OgreGLES2RenderSystem.h"
#include "OgreGLES2Support.h"
#include "OgreBitwise.h"

/* GL_IMG_texture_compression_pvrtc */
#ifndef GL_IMG_texture_compression_pvrtc
#define GL_IMG_texture_compression_pvrtc 1
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG                      0x8C00
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG                      0x8C01
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG                     0x8C02
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG                     0x8C03
#endif

/* GL_IMG_texture_compression_pvrtc2 */
#ifndef GL_IMG_texture_compression_pvrtc2
#define GL_IMG_texture_compression_pvrtc2 1
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG                     0x9137
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG                     0x9138
#endif

/* GL_AMD_compressed_ATC_texture */
#ifndef GL_AMD_compressed_ATC_texture
#define GL_AMD_compressed_ATC_texture 1
#define GL_ATC_RGB_AMD                                          0x8C92
#define GL_ATC_RGBA_EXPLICIT_ALPHA_AMD                          0x8C93
#define GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD                      0x87EE
#endif

#if 1 //OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
#ifndef GL_EXT_texture_compression_dxt1
#define GL_EXT_texture_compression_dxt1 1
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT                   0x83F0
#endif

#ifndef GL_EXT_texture_compression_s3tc
#define GL_EXT_texture_compression_s3tc 1
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT                  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT                  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT                  0x83F3
#endif

#ifndef GL_COMPRESSED_RGB8_ETC2
#define GL_COMPRESSED_RGB8_ETC2                           0x9274
#define GL_COMPRESSED_RGBA8_ETC2_EAC                      0x9278
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2       0x9276
#endif

#endif

namespace Ogre {
    map<PixelFormat, GLenum>::type GLES2PixelUtil::mGLOriginFormatMap;
    map<PixelFormat, GLenum>::type GLES2PixelUtil::mGLOriginDataTypeMap;
    map<PixelFormat, GLenum>::type GLES2PixelUtil::mGLInternalFormatMap;
    map<PixelFormat, GLenum>::type GLES2PixelUtil::mGLHwGammaInternalFormatMap;
    map<std::pair<GLenum, GLenum>, PixelFormat>::type GLES2PixelUtil::mClosestOgreFormat;

    void GLES2PixelUtil::initialize()
    {
        if (!mGLOriginFormatMap.empty())
            return;

        mGLOriginFormatMap[PF_A8] = GL_ALPHA;
        mGLOriginFormatMap[PF_DEPTH] = GL_DEPTH_COMPONENT;

        mGLOriginFormatMap[PF_R5G6B5] = GL_RGB;
        mGLOriginFormatMap[PF_B5G6R5] = GL_RGB;
        mGLOriginFormatMap[PF_R8G8B8] = GL_RGB;
        mGLOriginFormatMap[PF_B8G8R8] = GL_RGB;
        mGLOriginFormatMap[PF_X8B8G8R8] = GL_RGB;
        mGLOriginFormatMap[PF_SHORT_RGB] = GL_RGB;


        mGLOriginFormatMap[PF_A4R4G4B4] = GL_RGBA;
        mGLOriginFormatMap[PF_A1R5G5B5] = GL_RGBA;
        mGLOriginFormatMap[PF_A2R10G10B10] = GL_RGBA;
        mGLOriginFormatMap[PF_A2B10G10R10] = GL_RGBA;
        mGLOriginFormatMap[PF_SHORT_RGBA] = GL_RGBA;
        if (getGLES2SupportRef()->checkExtension("GL_APPLE_texture_format_BGRA8888"))
        {
            mGLOriginFormatMap[PF_X8R8G8B8] = GL_BGRA_EXT;
            mGLOriginFormatMap[PF_A8R8G8B8] = GL_BGRA_EXT;
            mGLOriginFormatMap[PF_B8G8R8A8] = GL_BGRA_EXT;
            mGLOriginFormatMap[PF_A8B8G8R8] = GL_BGRA_EXT;
            mGLOriginFormatMap[PF_R8G8B8A8] = GL_BGRA_EXT;
        }
        else
        {
            mGLOriginFormatMap[PF_X8R8G8B8] = GL_RGBA;
            mGLOriginFormatMap[PF_A8R8G8B8] = GL_RGBA;
            mGLOriginFormatMap[PF_B8G8R8A8] = GL_RGBA;
            mGLOriginFormatMap[PF_A8B8G8R8] = GL_RGBA;
            mGLOriginFormatMap[PF_R8G8B8A8] = GL_RGBA;
        }

#if GL_OES_texture_half_float || GL_EXT_color_buffer_half_float || OGRE_NO_GLES3_SUPPORT == 0
        if (gleswIsSupported(3, 0) ||
            (getGLES2SupportRef()->checkExtension("GL_OES_texture_half_float") ||
             getGLES2SupportRef()->checkExtension("GL_EXT_color_buffer_half_float")))
        {
            mGLOriginFormatMap[PF_FLOAT16_RGB] = GL_RGB;
            mGLOriginFormatMap[PF_FLOAT32_RGB] = GL_RGB;
            mGLOriginFormatMap[PF_FLOAT16_RGBA] = GL_RGBA;
            mGLOriginFormatMap[PF_FLOAT32_RGBA] = GL_RGBA;
        }
#endif

#if OGRE_NO_GLES3_SUPPORT == 0
        if (gleswIsSupported(3, 0) ||
            getGLES2SupportRef()->checkExtension("GL_EXT_texture_rg"))
        {
            mGLOriginFormatMap[PF_R8] = GL_RED_EXT;
            mGLOriginFormatMap[PF_RG8] = GL_RG_EXT;
            mGLOriginFormatMap[PF_FLOAT16_R] = GL_RED_EXT;
            mGLOriginFormatMap[PF_FLOAT32_R] = GL_RED_EXT;
            mGLOriginFormatMap[PF_FLOAT16_GR] = GL_RG_EXT;
            mGLOriginFormatMap[PF_FLOAT32_GR] = GL_RG_EXT;
        }
#endif
#if GL_IMG_texture_compression_pvrtc
        // if (getGLES2SupportRef()->checkExtension("GL_IMG_texture_compression_pvrtc"))
        {
            mGLOriginFormatMap[PF_PVRTC_RGB2] = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
            mGLOriginFormatMap[PF_PVRTC_RGB4] = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
            mGLOriginFormatMap[PF_PVRTC_RGBA2] = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
            mGLOriginFormatMap[PF_PVRTC_RGBA4] = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
        }
#endif

#if GL_IMG_texture_compression_pvrtc2
        // if (getGLES2SupportRef()->checkExtension("GL_IMG_texture_compression_pvrtc2"))
        {
            mGLOriginFormatMap[PF_PVRTC2_2BPP] = GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG;
            mGLOriginFormatMap[PF_PVRTC2_4BPP] = GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
        }
#endif

#if OGRE_NO_ETC_CODEC == 0
#	ifdef GL_OES_compressed_ETC1_RGB8_texture
        //if (getGLES2SupportRef()->checkExtension("GL_OES_compressed_ETC1_RGB8_texture"))
        {
            mGLOriginFormatMap[PF_ETC1_RGB8] = GL_ETC1_RGB8_OES;
        }
#	endif
#endif

#ifdef GL_AMD_compressed_ATC_texture
        //if (getGLES2SupportRef()->checkExtension("GL_AMD_compressed_ATC_texture"))
        {
            mGLOriginFormatMap[PF_ATC_RGB] = GL_ATC_RGB_AMD;
            mGLOriginFormatMap[PF_ATC_RGBA_EXPLICIT_ALPHA] = GL_ATC_RGBA_EXPLICIT_ALPHA_AMD;
            mGLOriginFormatMap[PF_ATC_RGBA_INTERPOLATED_ALPHA] = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
            
        }
#endif
#ifdef GL_COMPRESSED_RGB8_ETC2
        // if (getGLES2SupportRef()->checkExtension("GL_COMPRESSED_RGB8_ETC2") ||
        //    gleswIsSupported(3, 0))
        {
            mGLOriginFormatMap[PF_ETC2_RGB8] = GL_COMPRESSED_RGB8_ETC2;
            mGLOriginFormatMap[PF_ETC2_RGBA8] = GL_COMPRESSED_RGBA8_ETC2_EAC;
            mGLOriginFormatMap[PF_ETC2_RGB8A1] = GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        }
#endif

#if GL_EXT_texture_compression_dxt1
        // if (getGLES2SupportRef()->checkExtension("GL_EXT_texture_compression_dxt1"))
        {
            mGLOriginFormatMap[PF_DXT1] = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        }
#endif
#if GL_EXT_texture_compression_s3tc
        // if (getGLES2SupportRef()->checkExtension("GL_EXT_texture_compression_s3tc"))
        {
            mGLOriginFormatMap[PF_DXT3] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            mGLOriginFormatMap[PF_DXT5] = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        }
#endif

#if OGRE_NO_GLES3_SUPPORT == 0
        if (gleswIsSupported(3, 0))
        {
            mGLOriginFormatMap[PF_BYTE_LA] = GL_RG;
            mGLOriginFormatMap[PF_L8] = GL_RED;
            mGLOriginFormatMap[PF_L16] = GL_RED;
            mGLOriginFormatMap[PF_R8_UINT] = GL_RED_INTEGER;
            mGLOriginFormatMap[PF_R16_UINT] = GL_RED_INTEGER;
            mGLOriginFormatMap[PF_R32_UINT] = GL_RED_INTEGER;
            mGLOriginFormatMap[PF_R8_SINT] = GL_RED_INTEGER;
            mGLOriginFormatMap[PF_R16_SINT] = GL_RED_INTEGER;
            mGLOriginFormatMap[PF_R32_SINT] = GL_RED_INTEGER;
            mGLOriginFormatMap[PF_R8G8_UINT] = GL_RG_INTEGER;
            mGLOriginFormatMap[PF_R16G16_UINT] = GL_RG_INTEGER;
            mGLOriginFormatMap[PF_R32G32_UINT] = GL_RG_INTEGER;
            mGLOriginFormatMap[PF_R8G8_SINT] = GL_RG_INTEGER;
            mGLOriginFormatMap[PF_R16G16_SINT] = GL_RG_INTEGER;
            mGLOriginFormatMap[PF_R32G32_SINT] = GL_RG_INTEGER;
            mGLOriginFormatMap[PF_R8G8B8_UINT] = GL_RGB_INTEGER;
            mGLOriginFormatMap[PF_R16G16B16_UINT] = GL_RGB_INTEGER;
            mGLOriginFormatMap[PF_R32G32B32_UINT] = GL_RGB_INTEGER;
            mGLOriginFormatMap[PF_R8G8B8_SINT] = GL_RGB_INTEGER;
            mGLOriginFormatMap[PF_R16G16B16_SINT] = GL_RGB_INTEGER;
            mGLOriginFormatMap[PF_R32G32B32_SINT] = GL_RGB_INTEGER;
            mGLOriginFormatMap[PF_R8G8B8A8_UINT] = GL_RGBA_INTEGER;
            mGLOriginFormatMap[PF_R16G16B16A16_UINT] = GL_RGBA_INTEGER;
            mGLOriginFormatMap[PF_R32G32B32A32_UINT] = GL_RGBA_INTEGER;
            mGLOriginFormatMap[PF_R8G8B8A8_SINT] = GL_RGBA_INTEGER;
            mGLOriginFormatMap[PF_R16G16B16A16_SINT] = GL_RGBA_INTEGER;
            mGLOriginFormatMap[PF_R32G32B32A32_SINT] = GL_RGBA_INTEGER;
            mGLOriginFormatMap[PF_R11G11B10_FLOAT] = GL_RGB;
            mGLOriginFormatMap[PF_R9G9B9E5_SHAREDEXP] = GL_RGB;
            mGLOriginFormatMap[PF_R8_SNORM] = GL_R8_SNORM;
            mGLOriginFormatMap[PF_R8G8_SNORM] = GL_RG8_SNORM;
            mGLOriginFormatMap[PF_R8G8B8_SNORM] = GL_RGB8_SNORM;
            mGLOriginFormatMap[PF_R8G8B8A8_SNORM] = GL_RGBA8_SNORM;
        }
        else
#endif
        {
            mGLOriginFormatMap[PF_L8] = GL_LUMINANCE;
            mGLOriginFormatMap[PF_L16] = GL_LUMINANCE;
            mGLOriginFormatMap[PF_BYTE_LA] = GL_LUMINANCE_ALPHA;
        }

        mGLOriginDataTypeMap[PF_DEPTH] = GL_UNSIGNED_INT;
        mGLOriginDataTypeMap[PF_A8] = GL_UNSIGNED_BYTE;
        mGLOriginDataTypeMap[PF_L8] = GL_UNSIGNED_BYTE;
        mGLOriginDataTypeMap[PF_L16] = GL_UNSIGNED_BYTE;
        mGLOriginDataTypeMap[PF_R8G8B8] = GL_UNSIGNED_BYTE;
        mGLOriginDataTypeMap[PF_R8G8B8] = GL_UNSIGNED_BYTE;
        mGLOriginDataTypeMap[PF_B8G8R8] = GL_UNSIGNED_BYTE;
        mGLOriginDataTypeMap[PF_BYTE_LA] = GL_UNSIGNED_BYTE;
        mGLOriginDataTypeMap[PF_R5G6B5] = GL_UNSIGNED_SHORT_5_6_5;
        mGLOriginDataTypeMap[PF_B5G6R5] = GL_UNSIGNED_SHORT_5_6_5;
        mGLOriginDataTypeMap[PF_A4R4G4B4] = GL_UNSIGNED_SHORT_4_4_4_4;
        mGLOriginDataTypeMap[PF_SHORT_RGB] = GL_UNSIGNED_SHORT_4_4_4_4;
        mGLOriginDataTypeMap[PF_SHORT_RGBA] = GL_UNSIGNED_SHORT_4_4_4_4;
        mGLOriginDataTypeMap[PF_A1R5G5B5] = GL_UNSIGNED_SHORT_5_5_5_1;

#if OGRE_ENDIAN == OGRE_ENDIAN_BIG
        mGLOriginDataTypeMap[PF_X8B8G8R8] = GL_UNSIGNED_INT_8_8_8_8_REV;
        mGLOriginDataTypeMap[PF_X8R8G8B8] = GL_UNSIGNED_INT_8_8_8_8_REV;
        mGLOriginDataTypeMap[PF_A8B8G8R8] = GL_UNSIGNED_INT_8_8_8_8_REV;
        mGLOriginDataTypeMap[PF_A8R8G8B8] = GL_UNSIGNED_INT_8_8_8_8_REV;
        mGLOriginDataTypeMap[PF_B8G8R8A8] = GL_UNSIGNED_BYTE;
        mGLOriginDataTypeMap[PF_R8G8B8A8] = GL_UNSIGNED_BYTE;
#else
        mGLOriginDataTypeMap[PF_X8B8G8R8] = GL_UNSIGNED_BYTE;
        mGLOriginDataTypeMap[PF_X8R8G8B8] = GL_UNSIGNED_BYTE;
        mGLOriginDataTypeMap[PF_A8B8G8R8] = GL_UNSIGNED_BYTE;
        mGLOriginDataTypeMap[PF_A8R8G8B8] = GL_UNSIGNED_BYTE;
        mGLOriginDataTypeMap[PF_B8G8R8A8] = GL_UNSIGNED_BYTE;
        mGLOriginDataTypeMap[PF_R8G8B8A8] = GL_UNSIGNED_BYTE;
#endif

#if GL_OES_texture_half_float || OGRE_NO_GLES3_SUPPORT == 0
        if (gleswIsSupported(3, 0) ||
            getGLES2SupportRef()->checkExtension("GL_OES_texture_half_float"))
        {
            mGLOriginDataTypeMap[PF_FLOAT16_R] = GL_HALF_FLOAT_OES;
            mGLOriginDataTypeMap[PF_FLOAT16_GR] = GL_HALF_FLOAT_OES;
            mGLOriginDataTypeMap[PF_FLOAT16_RGB] = GL_HALF_FLOAT_OES;
            mGLOriginDataTypeMap[PF_FLOAT16_RGBA] = GL_HALF_FLOAT_OES;

        }
#endif

#if GL_EXT_texture_rg || OGRE_NO_GLES3_SUPPORT == 0
        if (gleswIsSupported(3, 0) ||
            getGLES2SupportRef()->checkExtension("GL_EXT_texture_rg"))
        {
            mGLOriginDataTypeMap[PF_R8] = GL_UNSIGNED_BYTE;
            mGLOriginDataTypeMap[PF_RG8] = GL_UNSIGNED_BYTE;
        }
#endif
#if GL_OES_texture_float || OGRE_NO_GLES3_SUPPORT == 0
        if (gleswIsSupported(3, 0) ||
            getGLES2SupportRef()->checkExtension("GL_OES_texture_float"))
        {
            mGLOriginDataTypeMap[PF_FLOAT32_R] = GL_FLOAT;
            mGLOriginDataTypeMap[PF_FLOAT32_GR] = GL_FLOAT;
            mGLOriginDataTypeMap[PF_FLOAT32_RGB] = GL_FLOAT;
            mGLOriginDataTypeMap[PF_FLOAT32_RGBA] = GL_FLOAT;
        }
#endif

#if OGRE_NO_GLES3_SUPPORT == 0
        if (gleswIsSupported(3, 0))
        {
            mGLOriginDataTypeMap[PF_A2R10G10B10] = GL_UNSIGNED_INT_2_10_10_10_REV;
            mGLOriginDataTypeMap[PF_A2B10G10R10] = GL_UNSIGNED_INT_2_10_10_10_REV;
            mGLOriginDataTypeMap[PF_R8_SNORM] = GL_BYTE;
            mGLOriginDataTypeMap[PF_R8G8_SNORM] = GL_BYTE;
            mGLOriginDataTypeMap[PF_R8G8B8_SNORM] = GL_BYTE;
            mGLOriginDataTypeMap[PF_R8G8B8A8_SNORM] = GL_BYTE;
            mGLOriginDataTypeMap[PF_R16_SNORM] = GL_BYTE;
            mGLOriginDataTypeMap[PF_R16G16_SNORM] = GL_BYTE;
            mGLOriginDataTypeMap[PF_R16G16B16_SNORM] = GL_BYTE;
            mGLOriginDataTypeMap[PF_R16G16B16A16_SNORM] = GL_BYTE;
            mGLOriginDataTypeMap[PF_R8_SINT] = GL_BYTE;
            mGLOriginDataTypeMap[PF_R8G8_SINT] = GL_BYTE;
            mGLOriginDataTypeMap[PF_R8G8B8_SINT] = GL_BYTE;
            mGLOriginDataTypeMap[PF_R8G8B8_SINT] = GL_BYTE;
            mGLOriginDataTypeMap[PF_R8G8B8A8_SINT] = GL_BYTE;
            mGLOriginDataTypeMap[PF_R8_UINT] = GL_UNSIGNED_BYTE;
            mGLOriginDataTypeMap[PF_R8G8_UINT] = GL_UNSIGNED_BYTE;
            mGLOriginDataTypeMap[PF_R8G8B8_UINT] = GL_UNSIGNED_BYTE;
            mGLOriginDataTypeMap[PF_R8G8B8A8_UINT] = GL_UNSIGNED_BYTE;
            mGLOriginDataTypeMap[PF_R32_UINT] = GL_UNSIGNED_INT;
            mGLOriginDataTypeMap[PF_R32G32_UINT] = GL_UNSIGNED_INT;
            mGLOriginDataTypeMap[PF_R32G32B32_UINT] = GL_UNSIGNED_INT;
            mGLOriginDataTypeMap[PF_R32G32B32A32_UINT] = GL_UNSIGNED_INT;
            mGLOriginDataTypeMap[PF_R16_SINT] = GL_SHORT;
            mGLOriginDataTypeMap[PF_R16G16_SINT] = GL_SHORT;
            mGLOriginDataTypeMap[PF_R16G16B16_SINT] = GL_SHORT;
            mGLOriginDataTypeMap[PF_R16G16B16A16_SINT] = GL_SHORT;
            mGLOriginDataTypeMap[PF_R32G32B32_SINT] = GL_INT;
            mGLOriginDataTypeMap[PF_R32_SINT] = GL_INT;
            mGLOriginDataTypeMap[PF_R32G32_SINT] = GL_INT;
            mGLOriginDataTypeMap[PF_R32G32B32A32_SINT] = GL_INT;

            mGLOriginDataTypeMap[PF_R9G9B9E5_SHAREDEXP] = GL_UNSIGNED_INT_5_9_9_9_REV;
            mGLOriginDataTypeMap[PF_R11G11B10_FLOAT] = GL_UNSIGNED_INT_10F_11F_11F_REV;
        }
#endif

        mGLInternalFormatMap[PF_DEPTH] = GL_DEPTH_COMPONENT;

#if GL_IMG_texture_compression_pvrtc
        mGLInternalFormatMap[PF_PVRTC_RGB2] = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
        mGLInternalFormatMap[PF_PVRTC_RGB4] = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
        mGLInternalFormatMap[PF_PVRTC_RGBA2] = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
        mGLInternalFormatMap[PF_PVRTC_RGBA4] = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
#endif

#if GL_IMG_texture_compression_pvrtc2
        mGLInternalFormatMap[PF_PVRTC2_2BPP] = GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG;
        mGLInternalFormatMap[PF_PVRTC2_4BPP] = GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
#endif

#if OGRE_NO_ETC_CODEC == 0
#	ifdef GL_OES_compressed_ETC1_RGB8_texture
        mGLInternalFormatMap[PF_ETC1_RGB8] = GL_ETC1_RGB8_OES;
#	endif
#endif

#	ifdef GL_AMD_compressed_ATC_texture
        mGLInternalFormatMap[PF_ATC_RGB] = GL_ATC_RGB_AMD;
        mGLInternalFormatMap[PF_ATC_RGBA_EXPLICIT_ALPHA] = GL_ATC_RGBA_EXPLICIT_ALPHA_AMD;
        mGLInternalFormatMap[PF_ATC_RGBA_INTERPOLATED_ALPHA] = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
        
#	endif
        
#ifdef GL_COMPRESSED_RGB8_ETC2
        mGLInternalFormatMap[PF_ETC2_RGB8] = GL_COMPRESSED_RGB8_ETC2;
        mGLInternalFormatMap[PF_ETC2_RGBA8] = GL_COMPRESSED_RGBA8_ETC2_EAC;
        mGLInternalFormatMap[PF_ETC2_RGB8A1] = GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
#endif

#if OGRE_NO_GLES3_SUPPORT == 0
        if (gleswIsSupported(3, 0))
        {
            mGLInternalFormatMap[PF_A1R5G5B5] = GL_RGB5_A1;
            mGLInternalFormatMap[PF_R5G6B5] = GL_RGB565;
            mGLInternalFormatMap[PF_B5G6R5] = GL_RGB565;
            mGLInternalFormatMap[PF_A4R4G4B4] = GL_RGBA4;
            mGLInternalFormatMap[PF_R8G8B8] = GL_RGB8;
            mGLInternalFormatMap[PF_B8G8R8] = GL_RGB8;
            mGLHwGammaInternalFormatMap[PF_R8G8B8] = GL_SRGB8;
            mGLHwGammaInternalFormatMap[PF_B8G8R8] = GL_SRGB8;

            if (getGLES2SupportRef()->checkExtension("GL_APPLE_texture_format_BGRA8888"))
            {
                mGLInternalFormatMap[PF_X8R8G8B8] = GL_BGRA8_EXT;
                mGLInternalFormatMap[PF_A8R8G8B8] = GL_BGRA8_EXT;
                mGLInternalFormatMap[PF_B8G8R8A8] = GL_BGRA8_EXT;
                mGLInternalFormatMap[PF_A8B8G8R8] = GL_BGRA8_EXT;
                mGLInternalFormatMap[PF_R8G8B8A8] = GL_BGRA8_EXT;
            }
            else
            {
                mGLInternalFormatMap[PF_A8R8G8B8] = GL_RGBA8;
                mGLInternalFormatMap[PF_B8G8R8A8] = GL_RGBA8;
                mGLInternalFormatMap[PF_A8B8G8R8] = GL_RGBA8;
                mGLInternalFormatMap[PF_R8G8B8A8] = GL_RGBA8;
                mGLInternalFormatMap[PF_X8B8G8R8] = GL_RGBA8;
                mGLInternalFormatMap[PF_X8R8G8B8] = GL_RGBA8;
            }

            mGLHwGammaInternalFormatMap[PF_A8R8G8B8] = GL_SRGB8_ALPHA8;
            mGLHwGammaInternalFormatMap[PF_B8G8R8A8] = GL_SRGB8_ALPHA8;
            mGLHwGammaInternalFormatMap[PF_A8B8G8R8] = GL_SRGB8_ALPHA8;
            mGLHwGammaInternalFormatMap[PF_R8G8B8A8] = GL_SRGB8_ALPHA8;
            mGLHwGammaInternalFormatMap[PF_X8B8G8R8] = GL_SRGB8_ALPHA8;
            mGLHwGammaInternalFormatMap[PF_X8R8G8B8] = GL_SRGB8_ALPHA8;

            mGLInternalFormatMap[PF_A2R10G10B10] = GL_RGB10_A2UI;
            mGLInternalFormatMap[PF_A2B10G10R10] = GL_RGB10_A2UI;
            mGLInternalFormatMap[PF_FLOAT32_RGB] = GL_RGB32F;
            mGLInternalFormatMap[PF_FLOAT32_RGBA] = GL_RGBA32F;
            mGLInternalFormatMap[PF_FLOAT16_RGB] = GL_RGBA16F;
            mGLInternalFormatMap[PF_FLOAT16_RGBA] = GL_RGBA16F;
            mGLInternalFormatMap[PF_R11G11B10_FLOAT] = GL_R11F_G11F_B10F;
            mGLInternalFormatMap[PF_R8_UINT] = GL_R8UI;
            mGLInternalFormatMap[PF_R8G8_UINT] = GL_RG8UI;
            mGLInternalFormatMap[PF_R8G8B8_UINT] = GL_RGB8UI;
            mGLInternalFormatMap[PF_R8G8B8A8_UINT] = GL_RGBA8UI;
            mGLInternalFormatMap[PF_R16_UINT] = GL_R16UI;
            mGLInternalFormatMap[PF_R16G16_UINT] = GL_RG16UI;
            mGLInternalFormatMap[PF_R16G16B16_UINT] = GL_RGB16UI;
            mGLInternalFormatMap[PF_R16G16B16A16_UINT] = GL_RGBA16UI;
            mGLInternalFormatMap[PF_R32_UINT] = GL_R32UI;
            mGLInternalFormatMap[PF_R32G32_UINT] = GL_RG32UI;
            mGLInternalFormatMap[PF_R32G32B32_UINT] = GL_RGB32UI;
            mGLInternalFormatMap[PF_R32G32B32A32_UINT] = GL_RGBA32UI;
            mGLInternalFormatMap[PF_R8_SINT] = GL_R8I;
            mGLInternalFormatMap[PF_R8G8_SINT] = GL_RG8I;
            mGLInternalFormatMap[PF_R8G8B8_SINT] = GL_RG8I;
            mGLInternalFormatMap[PF_R8G8B8A8_SINT] = GL_RGB8I;
            mGLInternalFormatMap[PF_R16_SINT] = GL_R16I;
            mGLInternalFormatMap[PF_R16G16_SINT] = GL_RG16I;
            mGLInternalFormatMap[PF_R16G16B16_SINT] = GL_RGB16I;
            mGLInternalFormatMap[PF_R16G16B16A16_SINT] = GL_RGBA16I;
            mGLInternalFormatMap[PF_R32_SINT] = GL_R32I;
            mGLInternalFormatMap[PF_R32G32_SINT] = GL_RG32I;
            mGLInternalFormatMap[PF_R32G32B32_SINT] = GL_RGB32I;
            mGLInternalFormatMap[PF_R32G32B32A32_SINT] = GL_RGBA32I;
            mGLInternalFormatMap[PF_R9G9B9E5_SHAREDEXP] = GL_RGB9_E5;
            mGLInternalFormatMap[PF_R8_SNORM] = GL_R8_SNORM;
            mGLInternalFormatMap[PF_R8G8_SNORM] = GL_RG8_SNORM;
            mGLInternalFormatMap[PF_R8G8B8_SNORM] = GL_RGB8_SNORM;
            mGLInternalFormatMap[PF_R8G8B8A8_SNORM] = GL_RGBA8_SNORM;
            mGLInternalFormatMap[PF_L8] = GL_R8;
            mGLInternalFormatMap[PF_A8] = GL_R8;
            mGLInternalFormatMap[PF_L16] = GL_R16F_EXT;
            mGLInternalFormatMap[PF_BYTE_LA] = GL_RG8;
        }
        else
#endif
        {
            mGLInternalFormatMap[PF_L8] = GL_LUMINANCE;
            mGLInternalFormatMap[PF_L16] = GL_LUMINANCE;
            mGLInternalFormatMap[PF_A8] = GL_ALPHA;
            mGLInternalFormatMap[PF_BYTE_LA] = GL_LUMINANCE_ALPHA;
            mGLInternalFormatMap[PF_A8R8G8B8] = GL_RGBA;
            mGLInternalFormatMap[PF_A8B8G8R8] = GL_RGBA;
            mGLInternalFormatMap[PF_B8G8R8A8] = GL_RGBA;
            mGLInternalFormatMap[PF_A1R5G5B5] = GL_RGBA;
            mGLInternalFormatMap[PF_A4R4G4B4] = GL_RGBA;
            mGLInternalFormatMap[PF_X8B8G8R8] = GL_RGBA;
            mGLInternalFormatMap[PF_X8R8G8B8] = GL_RGBA;
            mGLInternalFormatMap[PF_SHORT_RGBA] = GL_RGBA;
            mGLInternalFormatMap[PF_FLOAT16_RGB] = GL_RGB;
            mGLInternalFormatMap[PF_FLOAT32_RGB] = GL_RGB;
            mGLInternalFormatMap[PF_R5G6B5] = GL_RGB;
            mGLInternalFormatMap[PF_B5G6R5] = GL_RGB;
            mGLInternalFormatMap[PF_R8G8B8] = GL_RGB;
            mGLInternalFormatMap[PF_B8G8R8] = GL_RGB;
            mGLInternalFormatMap[PF_SHORT_RGB] = GL_RGB;
        }

#if GL_EXT_texture_rg || OGRE_NO_GLES3_SUPPORT == 0
        if (gleswIsSupported(3, 0) ||
            getGLES2SupportRef()->checkExtension("GL_EXT_texture_rg"))
        {
            mGLInternalFormatMap[PF_FLOAT16_R] = GL_R16F_EXT;
            mGLInternalFormatMap[PF_FLOAT32_R] = GL_R32F_EXT;
            mGLInternalFormatMap[PF_R8] = GL_RED_EXT;
            mGLInternalFormatMap[PF_FLOAT16_GR] = GL_RG16F_EXT;
            mGLInternalFormatMap[PF_FLOAT32_GR] = GL_RG32F_EXT;
            mGLInternalFormatMap[PF_RG8] = GL_RG_EXT;
        }
#endif

#if GL_EXT_texture_compression_dxt1
        mGLInternalFormatMap[PF_DXT1] = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
#endif
#if GL_EXT_texture_compression_s3tc
        mGLInternalFormatMap[PF_DXT3] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        mGLInternalFormatMap[PF_DXT5] = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
#endif

        // GL format to OGRE format map
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_DEPTH_COMPONENT, 0)] = PF_DEPTH;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_DEPTH24_STENCIL8_OES, 0)] = PF_DEPTH;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_DEPTH_COMPONENT16, 0)] = PF_DEPTH;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_DEPTH_COMPONENT24_OES, 0)] = PF_DEPTH;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_DEPTH_COMPONENT32_OES, 0)] = PF_DEPTH;

        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_LUMINANCE, 0)] = PF_L8;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_ALPHA, 0)] = PF_A8;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_LUMINANCE_ALPHA, 0)] = PF_BYTE_LA;

        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB, GL_UNSIGNED_SHORT_5_6_5)] = PF_B5G6R5;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB, GL_HALF_FLOAT_OES)] = PF_FLOAT16_RGB;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB, GL_FLOAT)] = PF_FLOAT32_RGB;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB, 0)] = PF_R8G8B8;

        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB8_OES, GL_UNSIGNED_SHORT_5_6_5)] = PF_B5G6R5;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB8_OES, GL_HALF_FLOAT_OES)] = PF_FLOAT16_RGB;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB8_OES, GL_FLOAT)] = PF_FLOAT32_RGB;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB8_OES, 0)] = PF_R8G8B8;

        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1)] = PF_A1R5G5B5;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4)] = PF_A4R4G4B4;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA, GL_HALF_FLOAT_OES)] = PF_FLOAT16_RGBA;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA, GL_FLOAT)] = PF_FLOAT32_RGBA;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA, 0)] = PF_A8B8G8R8;

        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA8_OES, GL_UNSIGNED_SHORT_5_5_5_1)] = PF_A1R5G5B5;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA8_OES, GL_UNSIGNED_SHORT_4_4_4_4)] = PF_A4R4G4B4;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA8_OES, GL_HALF_FLOAT_OES)] = PF_FLOAT16_RGBA;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA8_OES, GL_FLOAT)] = PF_FLOAT32_RGBA;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA8_OES, 0)] = PF_A8B8G8R8;

        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_BGRA8_EXT, 0)] = PF_A8R8G8B8;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_BGRA_EXT, 0)] = PF_A8R8G8B8;

#if GL_IMG_texture_compression_pvrtc
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG, 0)] = PF_PVRTC_RGB2;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG, 0)] = PF_PVRTC_RGBA2;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG, 0)] = PF_PVRTC_RGB4;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, 0)] = PF_PVRTC_RGBA4;
#endif

#if GL_IMG_texture_compression_pvrtc2
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG, 0)] = PF_PVRTC2_2BPP;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG, 0)] = PF_PVRTC2_4BPP;
#endif
        
#if OGRE_NO_ETC_CODEC == 0
#	ifdef GL_OES_compressed_ETC1_RGB8_texture
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_ETC1_RGB8_OES, 0)] = PF_ETC1_RGB8;
#	endif
#endif
        
#ifdef GL_AMD_compressed_ATC_texture
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_ATC_RGB_AMD, 0)] = PF_ATC_RGB;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_ATC_RGBA_EXPLICIT_ALPHA_AMD, 0)] = PF_ATC_RGBA_EXPLICIT_ALPHA;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD, 0)] = PF_ATC_RGBA_INTERPOLATED_ALPHA;
#	endif
#ifdef GL_COMPRESSED_RGB8_ETC2
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_COMPRESSED_RGB8_ETC2, 0)] = PF_ETC2_RGB8;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_COMPRESSED_RGBA8_ETC2_EAC, 0)] = PF_ETC2_RGBA8;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, 0)] = PF_ETC2_RGB8A1;
#endif
        
#if GL_EXT_texture_compression_dxt1
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 0)] = PF_DXT1;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 0)] = PF_DXT1;
#endif
#if GL_EXT_texture_compression_s3tc
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 0)] = PF_DXT3;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 0)] = PF_DXT5;
#endif
#if GL_EXT_texture_rg || OGRE_NO_GLES3_SUPPORT == 0
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_R8_EXT, 0)] = PF_R8;
        mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RG8_EXT, 0)] = PF_RG8;
#endif
#if OGRE_NO_GLES3_SUPPORT == 0
        if (gleswIsSupported(3, 0))
        {
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB10_A2, 0)] = PF_A2R10G10B10;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB10_A2UI, 0)] = PF_A2R10G10B10;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_R11F_G11F_B10F, 0)] = PF_R11G11B10_FLOAT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_R8UI, 0)] = PF_R8_UINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RG8UI, 0)] = PF_R8G8_UINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB8UI, 0)] = PF_R8G8B8_UINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA8UI, 0)] = PF_R8G8B8A8_UINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_R16UI, 0)] = PF_R16_UINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RG16UI, 0)] = PF_R16G16_UINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB16UI, 0)] = PF_R16G16B16_UINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA16UI, 0)] = PF_R16G16B16A16_UINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_R32UI, 0)] = PF_R32_UINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RG32UI, 0)] = PF_R32G32_UINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB32UI, 0)] = PF_R32G32B32_UINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA32UI, 0)] = PF_R32G32B32A32_UINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_R8I, 0)] = PF_R8_SINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RG8I, 0)] = PF_R8G8_SINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB8I, 0)] = PF_R8G8B8_SINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA8I, 0)] = PF_R8G8B8A8_SINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_R16I, 0)] = PF_R16_SINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RG16I, 0)] = PF_R16G16_SINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB16I, 0)] = PF_R16G16B16_SINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA16I, 0)] = PF_R16G16B16A16_SINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_R32I, 0)] = PF_R32_SINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RG32I, 0)] = PF_R32G32_SINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB32I, 0)] = PF_R32G32B32_SINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA32I, 0)] = PF_R32G32B32A32_SINT;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB9_E5, 0)] = PF_R9G9B9E5_SHAREDEXP;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_R8_SNORM, 0)] = PF_R8_SNORM;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RG8_SNORM, 0)] = PF_R8G8_SNORM;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB8_SNORM, 0)] = PF_R8G8B8_SNORM;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA8_SNORM, 0)] = PF_R8G8B8A8_SNORM;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_SRGB8, 0)] = PF_X8R8G8B8;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_SRGB8_ALPHA8, 0)] = PF_A8R8G8B8;
        }
#endif
        
#if GL_EXT_color_buffer_half_float || (OGRE_NO_GLES3_SUPPORT == 0)
        if (gleswIsSupported(3, 0) ||
            getGLES2SupportRef()->checkExtension("GL_EXT_color_buffer_half_float"))
        {
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA16F_EXT, 0)] = PF_FLOAT16_RGBA;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB16F_EXT, 0)] = PF_FLOAT16_RGB;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RG16F_EXT, 0)] = PF_FLOAT16_GR;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_R16F_EXT, 0)] = PF_FLOAT16_R;
        }
#endif
        
#if GL_EXT_color_buffer_float || (OGRE_NO_GLES3_SUPPORT == 0)
        if (gleswIsSupported(3, 0) ||
            getGLES2SupportRef()->checkExtension("GL_EXT_color_buffer_float"))
        {
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGBA32F_EXT, 0)] = PF_FLOAT32_RGBA;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RGB32F_EXT, 0)] = PF_FLOAT32_RGB;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_RG32F_EXT, 0)] = PF_FLOAT32_GR;
            mClosestOgreFormat[std::pair<GLenum, GLenum>(GL_R32F_EXT, 0)] = PF_FLOAT32_R;
        }
#endif
    }

    void GLES2PixelUtil::reset()
    {
        mGLOriginFormatMap.clear();
        mGLOriginDataTypeMap.clear();
        mGLInternalFormatMap.clear();
        mClosestOgreFormat.clear();
    }

	//-----------------------------------------------------------------------------
    GLenum GLES2PixelUtil::getGLOriginFormat(PixelFormat mFormat)
    {
        initialize();

        map<PixelFormat, GLenum>::type::iterator it = mGLOriginFormatMap.find(mFormat);
        if (it == mGLOriginFormatMap.end())
            return 0;
        return it->second;
    }
	//-----------------------------------------------------------------------------
    GLenum GLES2PixelUtil::getGLOriginDataType(PixelFormat mFormat)
    {
        initialize();

        map<PixelFormat, GLenum>::type::iterator it = mGLOriginDataTypeMap.find(mFormat);
        if (it == mGLOriginDataTypeMap.end())
            return 0;
        return it->second;
    }

	//-----------------------------------------------------------------------------
    GLenum GLES2PixelUtil::getGLInternalFormat(PixelFormat fmt, bool hwGamma)
    {
        initialize();

        map<PixelFormat, GLenum>::type::iterator it;
        if (hwGamma)
        {
            it = mGLHwGammaInternalFormatMap.find(fmt);
            if (it != mGLHwGammaInternalFormatMap.end())
                return it->second;
        }
        it = mGLInternalFormatMap.find(fmt);
        if (it != mGLInternalFormatMap.end())
            return it->second;
        return GL_NONE;
    }
	//-----------------------------------------------------------------------------
    GLenum GLES2PixelUtil::getClosestGLInternalFormat(PixelFormat mFormat,
                                                   bool hwGamma)
    {
        initialize();

        GLenum format = getGLInternalFormat(mFormat, hwGamma);
        if (format == GL_NONE)
        {
#if OGRE_NO_GLES3_SUPPORT == 0
            if (gleswIsSupported(3, 0))
            {
                if (hwGamma)
                    return GL_SRGB8;
                else
                    return GL_RGBA8;
            }
#endif
            if (hwGamma)
            {
                // TODO not supported
                return 0;
            }
            else
            {
                return GL_RGBA;
            }
        }
        else
        {
            return format;
        }
    }
	//-----------------------------------------------------------------------------
    PixelFormat GLES2PixelUtil::getClosestOGREFormat(GLenum fmt, GLenum dataType)
    {
        initialize();

        map<std::pair<GLenum, GLenum>, PixelFormat>::type::iterator it =
            mClosestOgreFormat.find(std::pair<GLenum, GLenum>(fmt, dataType));
        if (it == mClosestOgreFormat.end())
            it = mClosestOgreFormat.find(std::pair<GLenum, GLenum>(fmt, 0));
        if (it == mClosestOgreFormat.end())
        {
            LogManager::getSingleton().logMessage("Unhandled Pixel format: " + StringConverter::toString(fmt));
            return PF_A8B8G8R8;
        }
        return it->second;
    }
	//-----------------------------------------------------------------------------
    size_t GLES2PixelUtil::getMaxMipmaps(uint32 width, uint32 height, uint32 depth,
                                      PixelFormat format)
    {
		size_t count = 0;
        if((width > 0) && (height > 0) && (depth > 0))
        {
            do {
                if(width > 1)
                    width = width / 2;
                if(height > 1)
                    height = height / 2;
                if(depth > 1)
                    depth = depth / 2;
                /*
                 NOT needed, compressed formats will have mipmaps up to 1x1
                 if(PixelUtil::isValidExtent(width, height, depth, format))
                 count ++;
                 else
                 break;
                 */
                
                count++;
            } while(!(width == 1 && height == 1 && depth == 1));
        }		
		return count;
    }
	//-----------------------------------------------------------------------------
    // TODO: Remove
    uint32 GLES2PixelUtil::optionalPO2(uint32 value)
    {
        const RenderSystemCapabilities *caps =
            Root::getSingleton().getRenderSystem()->getCapabilities();

        if (caps->hasCapability(RSC_NON_POWER_OF_2_TEXTURES))
        {
            return value;
        }
        else
        {
            return Bitwise::firstPO2From(value);
        }
    }
}
