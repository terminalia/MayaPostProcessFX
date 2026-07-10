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
class PostQuadRenderer;

class PostFXRenderOverride : public MHWRender::MRenderOverride
{
public:
    PostFXRenderOverride(const MString& name);
    ~PostFXRenderOverride() override;

    MHWRender::DrawAPI supportedDrawAPIs() const override;
    MStatus setup(const MString& destination) override;
    MStatus cleanup() override;
    MString uiName() const override { return UIName; }

    void triggerShaderReload();
    MHWRender::MRenderTarget* getTargetHalfBlur() const;

    std::vector<MHWRender::MRenderOperation*> renderOperations;

    //PostQuadRenderer render pass unique names
    static const MString cDownsample1PassName;
    static const MString cDownsample2PassName;
    static const MString cDownsample3PassName;
    static const MString cDownsample4PassName; 
    static const MString cDownsample5PassName; 
    static const MString cUpsample1PassName;
    static const MString cUpsample2PassName;
    static const MString cUpsample3PassName;   
    static const MString cUpsample4PassName;   
    static const MString cFinalCompositePassName;

private:
    void releaseTargets();

    MString UIName;
    MHWRender::MRenderOperation* sceneRenderOp;

    // Extended Offscreen Hardware Cache Buffers
    MHWRender::MRenderTarget* targetFullScene;
    MHWRender::MRenderTarget* targetHalf;
    MHWRender::MRenderTarget* targetQuarter;
    MHWRender::MRenderTarget* targetEighth;
    MHWRender::MRenderTarget* targetSixteenth;     
    MHWRender::MRenderTarget* targetThirtySecond;  

    //UP SAMPLING PHASE
    //These copy the dimensions of your previous pyramid levels. They serve as the destination containers for your upsampling passes, blending the super-blurry 
    //small layers back into the sharper large layers before your FinalComposite pass merges them onto the screen.
    MHWRender::MRenderTarget* targetSixteenthBlur; 
    MHWRender::MRenderTarget* targetEighthBlur; 
    MHWRender::MRenderTarget* targetQuarterBlur;
    MHWRender::MRenderTarget* targetHalfBlur;
};