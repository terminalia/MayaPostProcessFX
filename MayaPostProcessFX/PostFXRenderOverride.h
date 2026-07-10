#pragma once
#include <maya/MViewport2Renderer.h> //  Maya 2026 Unified Viewport 2.0 Architecture header
#include <maya/MString.h>
#include <maya/MRenderTarget.h>
#include <vector>

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

    // PostQuadRenderer render pass unique names
    static const MString cDownsample1PassName;
    static const MString cDownsample2PassName;
    static const MString cDownsample3PassName;
    static const MString cDownsample4PassName;
    static const MString cDownsample5PassName;
    static const MString cDownsample6PassName; // 1/64
    static const MString cDownsample7PassName; // 1/128
    static const MString cUpsample1PassName;
    static const MString cUpsample2PassName;
    static const MString cUpsample3PassName;
    static const MString cUpsample4PassName;
    static const MString cUpsample5PassName;
    static const MString cUpsample6PassName;
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
    MHWRender::MRenderTarget* targetSixtyFourth;       // Native 1/64th texture target
    MHWRender::MRenderTarget* targetOneTwentyEighth;   // Native 1/128th texture target

    // UP SAMPLING PHASE
    MHWRender::MRenderTarget* targetSixtyFourthBlur;
    MHWRender::MRenderTarget* targetThirtySecondBlur;
    MHWRender::MRenderTarget* targetSixteenthBlur;
    MHWRender::MRenderTarget* targetEighthBlur;
    MHWRender::MRenderTarget* targetQuarterBlur;
    MHWRender::MRenderTarget* targetHalfBlur;
};