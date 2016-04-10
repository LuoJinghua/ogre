/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

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
#ifndef __GLES2VERTEXDECLARATION_H__
#define __GLES2VERTEXDECLARATION_H__

#include "OgreGLES2Prerequisites.h"
#include "OgreHardwareVertexBuffer.h"
#include "OgreGLES2ManagedResource.h"

namespace Ogre { 

    class GLES2VertexArrayObjectBinding
    {
    public:
        GLES2VertexArrayObjectBinding()
        {
            clear();
        }

        void clear()
        {
            mUsedAttribs = 0;
            memset(mBoundBuffers, 0, sizeof(mBoundBuffers));
            mElementBuffer = 0;
        }

        uint32 getUsedAttribs() const { return mUsedAttribs; }
        GLuint getBufferId(GLuint attrib) const
        {
            assert(attrib < sizeof(uint32) * 8);
            return mBoundBuffers[attrib].bufferId;
        }

        void setBufferId(GLuint attrib, GLuint bufferId, void* offset)
        {
            assert(attrib < sizeof(uint32) * 8);
            if (bufferId)
                mUsedAttribs |= 1u << attrib;
            else
                mUsedAttribs &= ~(1u << attrib);
            mBoundBuffers[attrib].bufferId = bufferId;
            mBoundBuffers[attrib].bufferOffset = offset;
        }

        bool isSameBuffer(GLuint attrib, GLuint bufferId, void* offset) const
        {
            assert(attrib < sizeof(uint32) * 8);
            return mBoundBuffers[attrib].bufferId == bufferId &&
                mBoundBuffers[attrib].bufferOffset == offset;
        }

        GLuint getElementBufferId() const
        {
            return mElementBuffer;
        }

        void setElementBufferId(GLuint bufferId)
        {
            mElementBuffer = bufferId;
        }

        bool operator== (const GLES2VertexArrayObjectBinding& rhs) const
        {
            return mUsedAttribs == rhs.mUsedAttribs &&
                memcmp(mBoundBuffers, rhs.mBoundBuffers, sizeof(mBoundBuffers)) == 0 &&
                mElementBuffer == rhs.mElementBuffer;
        }

        bool operator!= (const GLES2VertexArrayObjectBinding& rhs) const
        {
            return mUsedAttribs != rhs.mUsedAttribs ||
                memcmp(mBoundBuffers, rhs.mBoundBuffers, sizeof(mBoundBuffers)) != 0 ||
                mElementBuffer != rhs.mElementBuffer;
        }

    private:
        uint32 mUsedAttribs;
        struct BufferInfo
        {
            GLuint bufferId;
            void* bufferOffset;
        };
        BufferInfo mBoundBuffers[sizeof(uint32) * 8];
        GLuint mElementBuffer;
    };

	/** Specialisation of VertexDeclaration for OpenGL ES 2 Vertex Array Object usage */
	class GLES2VertexDeclaration : public VertexDeclaration MANAGED_RESOURCE
	{
	protected:
        /// OpenGL id for the vertex array object
        GLuint mVAO;
        GLES2VertexArrayObjectBinding mBinding;

	public:
		GLES2VertexDeclaration();
		~GLES2VertexDeclaration();

        void _createInternalResource();
        void _destroyInernalResource();
        void _load();

        GLuint getGLBufferId() const { return mVAO; }
        void bind(void);
        GLES2VertexArrayObjectBinding& getBinding() { return mBinding; }

#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
        /** See AndroidResource. */
        virtual void notifyOnContextLost();

        /** See AndroidResource. */
        virtual void notifyOnContextReset();
#endif
	};

}

#endif
