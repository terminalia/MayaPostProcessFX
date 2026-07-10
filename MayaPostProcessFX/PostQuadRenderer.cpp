#include "PostQuadRenderer.h"
#include <maya/MShaderManager.h>
#include <Windows.h>

PostQuadRenderer::PostQuadRenderer(const MString& name, const MString& fxFilePath, const MString& technique)
	: MHWRender::MQuadRender(name)
    , shaderInstance(NULL), originalFxFilePath(fxFilePath), fxFilePath(fxFilePath)
    , techniqueName(technique), bloomIntensity(1.5f), glowRadius(1.0f), shouldClear(false)
    , threshold(0.22f), softKnee(0.6f)
    , inputTargetTexture(NULL), secondaryInputTargetTexture(NULL), outputTargetTexture(NULL)
{
    outputTargetArray[0] = NULL;
}

PostQuadRenderer::~PostQuadRenderer()
{
    if (shaderInstance) 
    {
        MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();

        if (renderer && renderer->getShaderManager()) 
            renderer->getShaderManager()->releaseShader(shaderInstance);
    }

    if (fxFilePath != originalFxFilePath && fxFilePath.indexW("_temp_") != -1) 
    {
        DeleteFileA(fxFilePath.asChar());
    }
}

const MHWRender::MShaderInstance* PostQuadRenderer::shader()
{
    if (shaderInstance == NULL) 
    {
        MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();

        if (renderer && renderer->getShaderManager()) 
        {
            shaderInstance = renderer->getShaderManager()->getEffectsFileShader(fxFilePath.asChar(), techniqueName.asChar(), NULL);
        }
    }

    if (shaderInstance) 
    {
        MHWRender::MRenderTargetAssignment assignment;
        
        //Takes the input target texture and binds it to the uniform texture sampler "gInputTex" inside the shader
        if (inputTargetTexture) 
        {
            assignment.target = inputTargetTexture;
        }
        else 
        {
            //FALLBACK: binds the native viewport render here in case inputTargetTexture is not present
            assignment.target = getInputTarget(MHWRender::MRenderOperation::kColorTargetName);
        }
        
        //Effectively binds the input target texture to the shader uniform sampler
        shaderInstance->setParameter("gInputTex", assignment);

        if (techniqueName == "FinalComposite") 
        {
            MHWRender::MRenderTargetAssignment sceneAssignment;
            sceneAssignment.target = getInputTarget(MHWRender::MRenderOperation::kColorTargetName);
            shaderInstance->setParameter("gSourceMipTex", sceneAssignment);
        }
        else if (secondaryInputTargetTexture) 
        {
            //Bind the secondary input target texture to help upscaling
            MHWRender::MRenderTargetAssignment secAssignment;
            secAssignment.target = secondaryInputTargetTexture;
            shaderInstance->setParameter("gSourceMipTex", secAssignment);
        }

        shaderInstance->setParameter("gBloomIntensity", bloomIntensity);
        shaderInstance->setParameter("gGlowRadius", glowRadius);
        shaderInstance->setParameter("gThreshold", threshold);
        shaderInstance->setParameter("gSoftKnee", softKnee);
    }
    return shaderInstance;
}

MHWRender::MClearOperation& PostQuadRenderer::clearOperation()
{
    mClearOperation.setClearGradient(false);
    mClearOperation.setMask(shouldClear ? (unsigned int)MHWRender::MClearOperation::kClearColor : (unsigned int)MHWRender::MClearOperation::kClearNone);
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    mClearOperation.setClearColor(clearColor);

    return mClearOperation;
}

/*
* By default, any standard quad operation in Maya draws directly into the active viewport backbuffer (the screen). 
* To override this behavior, PostQuadRenderer leverages Maya's native targetOverrideList() method
*/
MHWRender::MRenderTarget* const* PostQuadRenderer::targetOverrideList(unsigned int& listSize)
{
    if (outputTargetTexture) 
    {
        listSize = 1;
        //Returns an array containing out target textures
        return outputTargetArray;
    }

    listSize = 0;
    
    //Fallback to the default viewport output
    return NULL;
}

void PostQuadRenderer::releaseCustomShader()
{
    if (shaderInstance) 
    {
        MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();

        if (renderer && renderer->getShaderManager()) 
            renderer->getShaderManager()->releaseShader(shaderInstance);
        
        shaderInstance = NULL;
    }
}

void PostQuadRenderer::setInputTargetTexture(MHWRender::MRenderTarget* target) 
{ 
    inputTargetTexture = target; 
}

void PostQuadRenderer::setSecondaryInputTargetTexture(MHWRender::MRenderTarget* target)
{
    secondaryInputTargetTexture = target;
}

void PostQuadRenderer::setOutputTargetTexture(MHWRender::MRenderTarget* target)
{
    outputTargetTexture = target; 
    outputTargetArray[0] = target;
}

void PostQuadRenderer::setShaderFilePath(const MString& path)
{
    fxFilePath = path;
}

void PostQuadRenderer::setClearOverride(bool clear)
{
    shouldClear = clear;
}

void PostQuadRenderer::setBloomIntensity(float val)
{
    bloomIntensity = val;
}

float PostQuadRenderer::getBloomIntensity() const
{
    return bloomIntensity;
}

void PostQuadRenderer::setGlowRadius(float val)
{
    glowRadius = val;
}

float PostQuadRenderer::getGlowRadius() const
{
    return glowRadius;
}

void PostQuadRenderer::setThreshold(float val)
{
    threshold = val;
}

float PostQuadRenderer::getThreshold() const
{
    return threshold;
}

void PostQuadRenderer::setSoftKnee(float val)
{
    softKnee = val;
}

float PostQuadRenderer::getSoftKnee() const
{
    return softKnee;
}


