/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2009 Torus Knot Software Ltd
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
#include "OgreShaderExDualQuaternionSkinning.h"
#ifdef RTSHADER_SYSTEM_BUILD_EXT_SHADERS
#include "OgreShaderFFPRenderState.h"
#include "OgreShaderProgram.h"
#include "OgreShaderParameter.h"
#include "OgreShaderProgramSet.h"
#include "OgreEntity.h"
#include "OgreSubEntity.h"
#include "OgreMaterial.h"
#include "OgreSubMesh.h"
#include "OgreShaderGenerator.h"

#define HS_DATA_BIND_NAME "HS_SRS_DATA"


namespace Ogre {

namespace RTShader {

/************************************************************************/
/*                                                                      */
/************************************************************************/
DualQuaternionSkinning::DualQuaternionSkinning() : HardwareSkinningTechnique()
{
}

//-----------------------------------------------------------------------
bool DualQuaternionSkinning::preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass)
{
	bool isValid = true;
	Technique* pFirstTech = srcPass->getParent()->getParent()->getTechnique(0);
	const Any& hsAny = pFirstTech->getUserObjectBindings().getUserAny(HS_DATA_BIND_NAME);
	if (hsAny.isEmpty() == false)
	{
		HardwareSkinningTechnique::SkinningData pData =
			(any_cast<HardwareSkinningTechnique::SkinningData>(hsAny));
		isValid = pData.isValid;
		mBoneCount = pData.maxBoneCount;
		mWeightCount = pData.maxWeightCount;
	}
	mDoBoneCalculations =  isValid &&
		(mBoneCount != 0) && (mBoneCount <= 256) &&
		(mWeightCount != 0) && (mWeightCount <= 4) &&
		((mCreator == NULL) || (mBoneCount <= mCreator->getMaxCalculableBoneCount()));

	if ((mDoBoneCalculations) && (mCreator))
	{
		//update the receiver and caster materials
		if (dstPass->getParent()->getShadowCasterMaterial().isNull())
		{
			dstPass->getParent()->setShadowCasterMaterial(
				mCreator->getCustomShadowCasterMaterial(mWeightCount - 1));
		}

		if (dstPass->getParent()->getShadowReceiverMaterial().isNull())
		{
			dstPass->getParent()->setShadowReceiverMaterial(
				mCreator->getCustomShadowReceiverMaterial(mWeightCount - 1));
		}
	}

	return true;
}

//-----------------------------------------------------------------------
bool DualQuaternionSkinning::resolveParameters(ProgramSet* programSet)
{
	Program* vsProgram = programSet->getCpuVertexProgram();
	Function* vsMain = vsProgram->getEntryPointFunction();

	//if needed mark this vertex program as hardware skinned
	if (mDoBoneCalculations == true)
	{
		vsProgram->setSkeletalAnimationIncluded(true);
	}

	//
	// get the parameters we need whether we are doing bone calculations or not
	//
	// Note: in order to be consistent we will always output position, normal,
	// tangent and binormal in both object and world space. And output position
	// in projective space to cover the responsibility of the transform stage

	//input param
	mParamInPosition = vsMain->resolveInputParameter(Parameter::SPS_POSITION, 0, Parameter::SPC_POSITION_OBJECT_SPACE, GCT_FLOAT4);
	mParamInNormal = vsMain->resolveInputParameter(Parameter::SPS_NORMAL, 0, Parameter::SPC_NORMAL_OBJECT_SPACE, GCT_FLOAT3);
	mParamInBiNormal = vsMain->resolveInputParameter(Parameter::SPS_BINORMAL, 0, Parameter::SPC_BINORMAL_OBJECT_SPACE, GCT_FLOAT3);
	mParamInTangent = vsMain->resolveInputParameter(Parameter::SPS_TANGENT, 0, Parameter::SPC_TANGENT_OBJECT_SPACE, GCT_FLOAT3);

	//local param
	mParamLocalPositionWorld = vsMain->resolveLocalParameter(Parameter::SPS_POSITION, 0, Parameter::SPC_POSITION_WORLD_SPACE, GCT_FLOAT4);
	mParamLocalNormalWorld = vsMain->resolveLocalParameter(Parameter::SPS_NORMAL, 0, Parameter::SPC_NORMAL_WORLD_SPACE, GCT_FLOAT3);
	mParamLocalTangentWorld = vsMain->resolveLocalParameter(Parameter::SPS_TANGENT, 0, Parameter::SPC_TANGENT_WORLD_SPACE, GCT_FLOAT3);
	mParamLocalBinormalWorld = vsMain->resolveLocalParameter(Parameter::SPS_BINORMAL, 0, Parameter::SPC_BINORMAL_WORLD_SPACE, GCT_FLOAT3);

	//output param
	mParamOutPositionProj = vsMain->resolveOutputParameter(Parameter::SPS_POSITION, 0, Parameter::SPC_POSITION_PROJECTIVE_SPACE, GCT_FLOAT4);

	//check if parameter retrival went well
	bool isValid =
		(mParamInPosition.get() != NULL) &&
		(mParamInNormal.get() != NULL) &&
		(mParamInBiNormal.get() != NULL) &&
		(mParamInTangent.get() != NULL) &&
		(mParamLocalPositionWorld.get() != NULL) &&
		(mParamLocalNormalWorld.get() != NULL) &&
		(mParamLocalTangentWorld.get() != NULL) &&
		(mParamLocalBinormalWorld.get() != NULL) &&
		(mParamOutPositionProj.get() != NULL);


	if (mDoBoneCalculations == true)
	{
		GpuProgramParameters::AutoConstantType worldMatrixType = GpuProgramParameters::ACT_WORLD_DUALQUATERNION_ARRAY_2x4;
		//if (ShaderGenerator::getSingleton().getTargetLanguage() == "hlsl")
		//{
			//given that hlsl shaders use column major matrices which are not compatible with the cg
			//and glsl method of row major matrices, we will use a full matrix instead.
		//	worldMatrixType = GpuProgramParameters::ACT_WORLD_MATRIX_ARRAY;
		//}

		//input parameters
		mParamInNormal = vsMain->resolveInputParameter(Parameter::SPS_NORMAL, 0, Parameter::SPC_NORMAL_OBJECT_SPACE, GCT_FLOAT3);
		mParamInBiNormal = vsMain->resolveInputParameter(Parameter::SPS_BINORMAL, 0, Parameter::SPC_BINORMAL_OBJECT_SPACE, GCT_FLOAT3);
		mParamInTangent = vsMain->resolveInputParameter(Parameter::SPS_TANGENT, 0, Parameter::SPC_TANGENT_OBJECT_SPACE, GCT_FLOAT3);
		mParamInIndices = vsMain->resolveInputParameter(Parameter::SPS_BLEND_INDICES, 0, Parameter::SPC_UNKNOWN, GCT_FLOAT4);
		mParamInWeights = vsMain->resolveInputParameter(Parameter::SPS_BLEND_WEIGHTS, 0, Parameter::SPC_UNKNOWN, GCT_FLOAT4);
		mParamInWorldMatrices = vsProgram->resolveAutoParameterInt(worldMatrixType, 0, mBoneCount);
		mParamInInvWorldMatrix = vsProgram->resolveAutoParameterInt(GpuProgramParameters::ACT_INVERSE_WORLD_MATRIX, 0);
		mParamInViewProjMatrix = vsProgram->resolveAutoParameterInt(GpuProgramParameters::ACT_VIEWPROJ_MATRIX, 0);
		
		mParamBlendDQ = vsMain->resolveLocalParameter(Parameter::SPS_UNKNOWN, -1, "blendDQ", GCT_MATRIX_2X4);
				
		if(mScalingShearingSupport)
		{
			mParamInScaleShearMatrices = vsProgram->resolveAutoParameterInt(GpuProgramParameters::ACT_WORLD_SCALE_SHEAR_MATRIX_ARRAY_3x4, 0, mBoneCount);
			mParamBlendS = vsMain->resolveLocalParameter(Parameter::SPS_UNKNOWN, -1, "blendS", GCT_MATRIX_3X4);
			mParamTempFloat3x3 = vsMain->resolveLocalParameter(Parameter::SPS_UNKNOWN, -1, "TempVal3x3", GCT_MATRIX_3X3);
			mParamTempFloat3x4 = vsMain->resolveLocalParameter(Parameter::SPS_UNKNOWN, -1, "TempVal3x4", GCT_MATRIX_3X4);
		}
		
		mParamTempFloat2x4 = vsMain->resolveLocalParameter(Parameter::SPS_UNKNOWN, -1, "TempVal2x4", GCT_MATRIX_2X4);
		mParamTempFloat4 = vsMain->resolveLocalParameter(Parameter::SPS_UNKNOWN, -1, "TempVal4", GCT_FLOAT4);
		mParamTempFloat3 = vsMain->resolveLocalParameter(Parameter::SPS_UNKNOWN, -1, "TempVal3", GCT_FLOAT3);

		//check if parameter retrival went well
		isValid &=
			(mParamInIndices.get() != NULL) &&
			(mParamInWeights.get() != NULL) &&
			(mParamInWorldMatrices.get() != NULL) &&
			(mParamInViewProjMatrix.get() != NULL) &&
			(mParamInInvWorldMatrix.get() != NULL) &&
			(mParamBlendDQ.get() != NULL) &&
						
			(!mScalingShearingSupport || (mScalingShearingSupport &&
			(mParamInScaleShearMatrices.get() != NULL &&
			mParamBlendS.get() != NULL &&
			mParamTempFloat3x3.get() != NULL &&
			mParamTempFloat3x4.get() != NULL))) &&

			(mParamTempFloat2x4.get() != NULL) &&
			(mParamTempFloat4.get() != NULL) &&
			(mParamTempFloat3.get() != NULL);
	}
	else
	{
		mParamInWorldMatrix = vsProgram->resolveAutoParameterInt(GpuProgramParameters::ACT_WORLD_MATRIX, 0);
		mParamInWorldViewProjMatrix = vsProgram->resolveAutoParameterInt(GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX, 0);

		//check if parameter retrival went well
		isValid &=
			(mParamInWorldMatrix.get() != NULL) &&
			(mParamInWorldViewProjMatrix.get() != NULL);
	}
	return isValid;
}

//-----------------------------------------------------------------------
bool DualQuaternionSkinning::resolveDependencies(ProgramSet* programSet)
{
	Program* vsProgram = programSet->getCpuVertexProgram();
	vsProgram->addDependency(FFP_LIB_COMMON);
	vsProgram->addDependency(FFP_LIB_TRANSFORM);
	vsProgram->addDependency(SGX_LIB_DUAL_QUATERNION);

	return true;
}

//-----------------------------------------------------------------------
bool DualQuaternionSkinning::addFunctionInvocations(ProgramSet* programSet)
{
	Program* vsProgram = programSet->getCpuVertexProgram();
	Function* vsMain = vsProgram->getEntryPointFunction();
	int internalCounter = 0;

	//add functions to calculate position data in world, object and projective space
	addPositionCalculations(vsMain, internalCounter);

	//add functions to calculate normal and normal related data in world and object space
	addNormalRelatedCalculations(vsMain, mParamInNormal, mParamLocalNormalWorld, internalCounter);
	addNormalRelatedCalculations(vsMain, mParamInTangent, mParamLocalTangentWorld, internalCounter);
	addNormalRelatedCalculations(vsMain, mParamInBiNormal, mParamLocalBinormalWorld, internalCounter);
	return true;
}

//-----------------------------------------------------------------------
void DualQuaternionSkinning::addPositionCalculations(Function* vsMain, int& funcCounter)
{
	FunctionInvocation* curFuncInvocation = NULL;

	if (mDoBoneCalculations == true)
	{
		if(mScalingShearingSupport)
		{
			//Construct a scaling and shearing matrix based on the blend weights
			for(int i = 0 ; i < getWeightCount() ; ++i)
			{
				//First phase - applies scaling and shearing:
				addIndexedPositionWeight(vsMain, i, mParamInScaleShearMatrices, mParamTempFloat3x4, mParamBlendS, funcCounter);
			}

			//Transform the position based by the scaling and shearing matrix
			curFuncInvocation = OGRE_NEW FunctionInvocation(FFP_FUNC_TRANSFORM, FFP_VS_TRANSFORM, funcCounter++);
			curFuncInvocation->pushOperand(mParamBlendS, Operand::OPS_IN);
			curFuncInvocation->pushOperand(mParamInPosition, Operand::OPS_IN);
			curFuncInvocation->pushOperand(mParamInPosition, Operand::OPS_OUT, Operand::OPM_X | Operand::OPM_Y | Operand::OPM_Z);
			vsMain->addAtomInstance(curFuncInvocation);
		}
		
		//set functions to calculate world position
		for(int i = 0 ; i < getWeightCount() ; ++i)
		{
			if(mCorrectAntipodalityHandling)
			{
				adjustForCorrectAntipodality(vsMain, i, funcCounter);
			}
			
			addIndexedPositionWeight(vsMain, i, mParamInWorldMatrices, mParamTempFloat2x4, mParamBlendDQ, funcCounter);
		}

		curFuncInvocation = OGRE_NEW FunctionInvocation(SGX_FUNC_NORMALIZE_DUAL_QUATERNION, FFP_VS_TRANSFORM, funcCounter++);
		curFuncInvocation->pushOperand(mParamBlendDQ, Operand::OPS_INOUT);
		vsMain->addAtomInstance(curFuncInvocation);

		//update the projective position thereby filling the transform stage role
		curFuncInvocation = OGRE_NEW FunctionInvocation(SGX_FUNC_CALCULATE_BLEND_POSITION, FFP_VS_TRANSFORM, funcCounter++);
		curFuncInvocation->pushOperand(mParamInPosition, Operand::OPS_IN);
		curFuncInvocation->pushOperand(mParamBlendDQ, Operand::OPS_IN);
		curFuncInvocation->pushOperand(mParamTempFloat4, Operand::OPS_OUT);
		vsMain->addAtomInstance(curFuncInvocation);

		//update from object to projective space
		curFuncInvocation = OGRE_NEW FunctionInvocation(FFP_FUNC_TRANSFORM, FFP_VS_TRANSFORM, funcCounter++);
		curFuncInvocation->pushOperand(mParamInViewProjMatrix, Operand::OPS_IN);
		curFuncInvocation->pushOperand(mParamTempFloat4, Operand::OPS_IN);
		curFuncInvocation->pushOperand(mParamOutPositionProj, Operand::OPS_OUT);
		vsMain->addAtomInstance(curFuncInvocation);
	}
	else
	{
		//update from object to world space
		curFuncInvocation = OGRE_NEW FunctionInvocation(FFP_FUNC_TRANSFORM, FFP_VS_TRANSFORM, funcCounter++);
		curFuncInvocation->pushOperand(mParamInWorldMatrix, Operand::OPS_IN);
		curFuncInvocation->pushOperand(mParamInPosition, Operand::OPS_IN);
		curFuncInvocation->pushOperand(mParamLocalPositionWorld, Operand::OPS_OUT);
		vsMain->addAtomInstance(curFuncInvocation);

		//update from object to projective space
		curFuncInvocation = OGRE_NEW FunctionInvocation(FFP_FUNC_TRANSFORM, FFP_VS_TRANSFORM, funcCounter++);
		curFuncInvocation->pushOperand(mParamInWorldViewProjMatrix, Operand::OPS_IN);
		curFuncInvocation->pushOperand(mParamInPosition, Operand::OPS_IN);
		curFuncInvocation->pushOperand(mParamOutPositionProj, Operand::OPS_OUT);
		vsMain->addAtomInstance(curFuncInvocation);
	}
}

//-----------------------------------------------------------------------
void DualQuaternionSkinning::addNormalRelatedCalculations(Function* vsMain,
								ParameterPtr& pNormalRelatedParam,
								ParameterPtr& pNormalWorldRelatedParam,
								int& funcCounter)
{
	FunctionInvocation* curFuncInvocation;

	if (mDoBoneCalculations == true)
	{
		if(mScalingShearingSupport)
		{
			//curFuncInvocation = OGRE_NEW FunctionInvocation(SGX_FUNC_CALCULATE_BLEND_NORMAL, FFP_VS_TRANSFORM, funcCounter++);
			//curFuncInvocation->pushOperand(pNormalRelatedParam, Operand::OPS_IN);
			//curFuncInvocation->pushOperand(mParamBlendDQ, Operand::OPS_IN);
			//curFuncInvocation->pushOperand(mP, Operand::OPS_OUT);
			//vsMain->addAtomInstance(curFuncInvocation);
		}
		
		//update back the original position relative to the object
		curFuncInvocation = OGRE_NEW FunctionInvocation(SGX_FUNC_CALCULATE_BLEND_NORMAL, FFP_VS_TRANSFORM, funcCounter++);
		curFuncInvocation->pushOperand(pNormalRelatedParam, Operand::OPS_IN);
		curFuncInvocation->pushOperand(mParamBlendDQ, Operand::OPS_IN);
		curFuncInvocation->pushOperand(pNormalWorldRelatedParam, Operand::OPS_OUT);
		vsMain->addAtomInstance(curFuncInvocation);

		//update back the original position relative to the object
		curFuncInvocation = OGRE_NEW FunctionInvocation(FFP_FUNC_TRANSFORM, FFP_VS_TRANSFORM, funcCounter++);
		curFuncInvocation->pushOperand(mParamInInvWorldMatrix, Operand::OPS_IN);
		curFuncInvocation->pushOperand(pNormalWorldRelatedParam, Operand::OPS_IN);
		curFuncInvocation->pushOperand(pNormalRelatedParam, Operand::OPS_OUT);
		vsMain->addAtomInstance(curFuncInvocation);	
	}
	else
	{
		//update from object to world space
		curFuncInvocation = OGRE_NEW FunctionInvocation(FFP_FUNC_TRANSFORM, FFP_VS_TRANSFORM, funcCounter++);
		curFuncInvocation->pushOperand(mParamInWorldMatrix, Operand::OPS_IN);
		curFuncInvocation->pushOperand(pNormalRelatedParam, Operand::OPS_IN);
		curFuncInvocation->pushOperand(pNormalWorldRelatedParam, Operand::OPS_OUT);
		vsMain->addAtomInstance(curFuncInvocation);
	}
}

//-----------------------------------------------------------------------
void DualQuaternionSkinning::adjustForCorrectAntipodality(Function* vsMain,
								int index, int& funcCounter)
{
	Operand::OpMask indexMask = indexToMask(index);
	FunctionInvocation* curFuncInvocation;
	
	curFuncInvocation = OGRE_NEW FunctionInvocation(SGX_FUNC_ANTIPODALITY_ADJUSTMENT, FFP_VS_TRANSFORM, funcCounter++);
	curFuncInvocation->pushOperand(mParamInWorldMatrices, Operand::OPS_IN);
	//This is the base dual quaternion dq0, which the antipodality calculations are based on
	curFuncInvocation->pushOperand(mParamInIndices, Operand::OPS_IN, indexToMask(0), 1);
	curFuncInvocation->pushOperand(mParamInWorldMatrices, Operand::OPS_INOUT);
	curFuncInvocation->pushOperand(mParamInIndices, Operand::OPS_IN,  indexMask, 1);
	vsMain->addAtomInstance(curFuncInvocation);
}

//-----------------------------------------------------------------------
void DualQuaternionSkinning::addIndexedPositionWeight(Function* vsMain,
								int index, UniformParameterPtr& pPositionRelatedParam, ParameterPtr& pPositionTempParameter,
								ParameterPtr& pPositionRelatedOutputParam, int& funcCounter)
{
	Operand::OpMask indexMask = indexToMask(index);
	FunctionInvocation* curFuncInvocation;

	//multiply position with world matrix and put into temporary param
	curFuncInvocation = OGRE_NEW FunctionInvocation(SGX_FUNC_BLEND_WEIGHT, FFP_VS_TRANSFORM, funcCounter++);
	curFuncInvocation->pushOperand(mParamInWeights, Operand::OPS_IN, indexMask);
	curFuncInvocation->pushOperand(pPositionRelatedParam, Operand::OPS_IN);
	curFuncInvocation->pushOperand(mParamInIndices, Operand::OPS_IN,  indexMask, 1);
	curFuncInvocation->pushOperand(pPositionTempParameter, Operand::OPS_OUT);
	vsMain->addAtomInstance(curFuncInvocation);

	//check if on first iteration
	if (index == 0)
	{
		//set the local param as the value of the world param
		curFuncInvocation = OGRE_NEW FunctionInvocation(FFP_FUNC_ASSIGN, FFP_VS_TRANSFORM, funcCounter++);
		curFuncInvocation->pushOperand(pPositionTempParameter, Operand::OPS_IN);
		curFuncInvocation->pushOperand(pPositionRelatedOutputParam, Operand::OPS_OUT);
		vsMain->addAtomInstance(curFuncInvocation);
	}
	else
	{
		//add the local param as the value of the world param
		curFuncInvocation = OGRE_NEW FunctionInvocation(FFP_FUNC_ADD, FFP_VS_TRANSFORM, funcCounter++);
		curFuncInvocation->pushOperand(pPositionTempParameter, Operand::OPS_IN);
		curFuncInvocation->pushOperand(pPositionRelatedOutputParam, Operand::OPS_IN);
		curFuncInvocation->pushOperand(pPositionRelatedOutputParam, Operand::OPS_OUT);
		vsMain->addAtomInstance(curFuncInvocation);
	}
}

}
}

#endif



