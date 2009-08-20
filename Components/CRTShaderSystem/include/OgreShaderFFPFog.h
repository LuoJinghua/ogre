/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2006 Torus Knot Software Ltd
Also see acknowledgements in Readme.html

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.

You may alternatively use this source under the terms of a specific version of
the OGRE Unrestricted License provided you have obtained such a license from
Torus Knot Software Ltd.
-----------------------------------------------------------------------------
*/
#ifndef _ShaderFFPFog_
#define _ShaderFFPFog_

#include "OgrePrerequisites.h"
#include "OgreShaderSubRenderState.h"
#include "OgreVector4.h"

namespace Ogre {
namespace CRTShader {

class Parameter;
class Function;
/************************************************************************/
/*                                                                      */
/************************************************************************/
class FFPFog : public SubRenderState
{
public:
	enum CalcMode
	{
		CM_PER_VERTEX	= 1,
		CM_PER_PIXEL	= 2
	};

// Interface.
public:
	FFPFog();
	virtual ~FFPFog();

	virtual const String&	getType					() const;
	virtual int				getExecutionOrder		() const;
	virtual uint32			getHashCode				();
	virtual void			updateGpuProgramsParams	(Renderable* rend, Pass* pass, const AutoParamDataSource* source, const LightList* pLightList);
	virtual void			copyFrom				(const SubRenderState& rhs);
	


	void			setFogProperties				(FogMode fogMode, 
													const ColourValue& fogColour, 
													float fogStart, 
													float fogEnd, 
													float fogDensity);
	void			setCalcMode						(CalcMode calcMode) { mCalcMode = calcMode; }
	CalcMode		getCalcMode						() const { return mCalcMode; }

	static String Type;

// Protected methods
protected:
	virtual bool			resolveParameters		(ProgramSet* programSet);
	virtual bool			resolveDependencies		(ProgramSet* programSet);
	virtual bool			addFunctionInvocations	(ProgramSet* programSet);

// Attributes.
protected:	
	CalcMode			mCalcMode;
	FogMode				mFogMode;
	ColourValue			mFogColourValue;
	Vector4				mFogParamsValue;

	Parameter*			mWorldViewProjMatrix;		
	Parameter*			mFogColour;
	Parameter*			mFogParams;	
	Parameter*			mVSInPos;
	Parameter*			mVSOutFogFactor;
	Parameter*			mPSInFogFactor;
	Parameter*			mVSOutDepth;
	Parameter*			mPSInDepth;
	Parameter*			mPSOutDiffuse;	
};


/************************************************************************/
/*                                                                      */
/************************************************************************/
class FFPFogFactory : public SubRenderStateFactory
{
public:
	virtual const String&	getType				() const;
	
protected:
	virtual SubRenderState*	createInstanceImpl	();


};

}
}

#endif

