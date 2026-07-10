#pragma once
#include <maya/MViewport2Renderer.h>
#include <vector>

/*
* The PostFXRenderOverride class is the central manager and pipeline builder for your custom viewport post-processing effect. 
* It inherits from MHWRender::MRenderOverride, which is Maya's Viewport 2.0 API used to completely hijack or enhance the viewport rendering loop.
* 
* Instead of letting Maya draw the 3D scene and push it straight to your screen, this class orchestrates a multi-pass image processing chain (specifically 
* a bloom pyramid—by) creating intermediate textures, setting up shaders, and chaining a series of full-screen quad rendering steps together.
*/
class PostFXRenderOverride : public MHWRender::MRenderOverride
{
public:
    static const MString cDownsample1PassName;
    static const MString cDownsample2PassName;
    static const MString cDownsample3PassName;
    static const MString cUpsample1PassName;
    static const MString cUpsample2PassName;
    static const MString cFinalCompositePassName;

protected:
    MString UIName;
    std::vector<MHWRender::MRenderOperation*> renderOperations;

    MHWRender::MRenderOperation* sceneRenderOp;
    MHWRender::MRenderTarget* targetFullScene;

    MHWRender::MRenderTarget* targetHalf;
    MHWRender::MRenderTarget* targetQuarter;
    MHWRender::MRenderTarget* targetEighth;
    MHWRender::MRenderTarget* targetQuarterBlur;
    MHWRender::MRenderTarget* targetHalfBlur;

    friend class PostFXCommand;

public:
    PostFXRenderOverride(const MString& name);
    ~PostFXRenderOverride() override;

    MHWRender::DrawAPI supportedDrawAPIs() const override;

    MStatus setup(const MString& destination) override;
    MStatus cleanup() override;

    MString uiName() const override { return UIName; }
    MHWRender::MRenderTarget* getTargetHalfBlur() const;
    void triggerShaderReload();

protected:
    void releaseTargets();
};

