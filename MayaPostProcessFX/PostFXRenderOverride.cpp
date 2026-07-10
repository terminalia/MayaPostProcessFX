#include "PostFXRenderOverride.h"
#include "PostQuadRenderer.h"
#include <maya/MShaderManager.h>
#include <maya/MGlobal.h>
#include "Utils.h"

// PostQuadRenderer render pass unique names
const MString PostFXRenderOverride::cDownsample1PassName = "PostFXRenderOverride_Down1";
const MString PostFXRenderOverride::cDownsample2PassName = "PostFXRenderOverride_Down2";
const MString PostFXRenderOverride::cDownsample3PassName = "PostFXRenderOverride_Down3";
const MString PostFXRenderOverride::cDownsample4PassName = "PostFXRenderOverride_Down4";
const MString PostFXRenderOverride::cDownsample5PassName = "PostFXRenderOverride_Down5";
const MString PostFXRenderOverride::cDownsample6PassName = "PostFXRenderOverride_Down6"; //  1/64
const MString PostFXRenderOverride::cDownsample7PassName = "PostFXRenderOverride_Down7"; //  1/128
const MString PostFXRenderOverride::cUpsample1PassName = "PostFXRenderOverride_Up1";
const MString PostFXRenderOverride::cUpsample2PassName = "PostFXRenderOverride_Up2";
const MString PostFXRenderOverride::cUpsample3PassName = "PostFXRenderOverride_Up3";
const MString PostFXRenderOverride::cUpsample4PassName = "PostFXRenderOverride_Up4";
const MString PostFXRenderOverride::cUpsample5PassName = "PostFXRenderOverride_Up5";    // 
const MString PostFXRenderOverride::cUpsample6PassName = "PostFXRenderOverride_Up6";    // 
const MString PostFXRenderOverride::cFinalCompositePassName = "PostFXRenderOverride_FinalComposite";

PostFXRenderOverride::PostFXRenderOverride(const MString& name)
    : MRenderOverride(name)
    , UIName("MultiPass Bloom Effects")
    , sceneRenderOp(NULL), targetFullScene(NULL)
    , targetHalf(NULL), targetQuarter(NULL), targetEighth(NULL)
    , targetSixteenth(NULL), targetThirtySecond(NULL)
    , targetSixtyFourth(NULL), targetOneTwentyEighth(NULL) // 
    , targetSixtyFourthBlur(NULL), targetThirtySecondBlur(NULL) // 
    , targetSixteenthBlur(NULL), targetEighthBlur(NULL), targetQuarterBlur(NULL), targetHalfBlur(NULL)
{
    MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
    if (!theRenderer) return;

    theRenderer->getStandardViewportOperations(mOperations);

    MString pluginDir = Utils::getPluginDirectory();
    MString ext = (theRenderer->drawAPI() == MHWRender::kOpenGLCoreProfile) ? ".ogsfx" : ".fx";
    MString shaderPath = pluginDir + "/shaders/MultiPassBloom" + ext;

    // --- 7-STAGE DOWNSAMPLING CHAIN ---
    PostQuadRenderer* downSample1 = new PostQuadRenderer(cDownsample1PassName, shaderPath, "ThresholdDownsample");
    downSample1->setClearOverride(true);
    renderOperations.push_back(downSample1);
    mOperations.insertAfter(MHWRender::MRenderOperation::kStandardSceneName, downSample1);

    PostQuadRenderer* downSample2 = new PostQuadRenderer(cDownsample2PassName, shaderPath, "StandardDownsample");
    downSample2->setClearOverride(true);
    renderOperations.push_back(downSample2);
    mOperations.insertAfter(cDownsample1PassName, downSample2);

    PostQuadRenderer* downSample3 = new PostQuadRenderer(cDownsample3PassName, shaderPath, "StandardDownsample");
    downSample3->setClearOverride(true);
    renderOperations.push_back(downSample3);
    mOperations.insertAfter(cDownsample2PassName, downSample3);

    PostQuadRenderer* downSample4 = new PostQuadRenderer(cDownsample4PassName, shaderPath, "StandardDownsample");
    downSample4->setClearOverride(true);
    renderOperations.push_back(downSample4);
    mOperations.insertAfter(cDownsample3PassName, downSample4);

    PostQuadRenderer* downSample5 = new PostQuadRenderer(cDownsample5PassName, shaderPath, "StandardDownsample");
    downSample5->setClearOverride(true);
    renderOperations.push_back(downSample5);
    mOperations.insertAfter(cDownsample4PassName, downSample5);

    //  Pass 6: Shrinks down to 1/64
    PostQuadRenderer* downSample6 = new PostQuadRenderer(cDownsample6PassName, shaderPath, "StandardDownsample");
    downSample6->setClearOverride(true);
    renderOperations.push_back(downSample6);
    mOperations.insertAfter(cDownsample5PassName, downSample6);

    //  Pass 7: Shrinks down to 1/128 for total ambient frame dispersion
    PostQuadRenderer* downSample7 = new PostQuadRenderer(cDownsample7PassName, shaderPath, "StandardDownsample");
    downSample7->setClearOverride(true);
    renderOperations.push_back(downSample7);
    mOperations.insertAfter(cDownsample6PassName, downSample7);

    // --- 6-STAGE UPSAMPLING PROGRESSION ---
    //  Up1: 1/128 -> 1/64 Blur
    PostQuadRenderer* upSample1 = new PostQuadRenderer(cUpsample1PassName, shaderPath, "UpsampleWide");
    upSample1->setClearOverride(true);
    renderOperations.push_back(upSample1);
    mOperations.insertAfter(cDownsample7PassName, upSample1);

    //  Up2: 1/64 Blur -> 1/32 Blur
    PostQuadRenderer* upSample2 = new PostQuadRenderer(cUpsample2PassName, shaderPath, "UpsampleWide");
    upSample2->setClearOverride(true);
    renderOperations.push_back(upSample2);
    mOperations.insertAfter(cUpsample1PassName, upSample2);

    // Up3: 1/32 Blur -> 1/16 Blur
    PostQuadRenderer* upSample3 = new PostQuadRenderer(cUpsample3PassName, shaderPath, "UpsampleWide");
    upSample3->setClearOverride(true);
    renderOperations.push_back(upSample3);
    mOperations.insertAfter(cUpsample2PassName, upSample3);

    // Up4: 1/16 Blur -> 1/8 Blur
    PostQuadRenderer* upSample4 = new PostQuadRenderer(cUpsample4PassName, shaderPath, "UpsampleMedium");
    upSample4->setClearOverride(true);
    renderOperations.push_back(upSample4);
    mOperations.insertAfter(cUpsample3PassName, upSample4);

    // Up5: 1/8 Blur -> 1/4 Blur
    PostQuadRenderer* upSample5 = new PostQuadRenderer(cUpsample5PassName, shaderPath, "UpsampleMedium");
    upSample5->setClearOverride(true);
    renderOperations.push_back(upSample5);
    mOperations.insertAfter(cUpsample4PassName, upSample5);

    // Up6: 1/4 Blur -> 1/2 Blur
    PostQuadRenderer* upSample6 = new PostQuadRenderer(cUpsample6PassName, shaderPath, "UpsampleTight");
    upSample6->setClearOverride(true);
    renderOperations.push_back(upSample6);
    mOperations.insertAfter(cUpsample5PassName, upSample6);

    // Composite
    PostQuadRenderer* finalComposite = new PostQuadRenderer(cFinalCompositePassName, shaderPath, "FinalComposite");
    renderOperations.push_back(finalComposite);
    mOperations.insertAfter(cUpsample6PassName, finalComposite);
}

PostFXRenderOverride::~PostFXRenderOverride()
{
    releaseTargets();
    renderOperations.clear();
}

MHWRender::DrawAPI PostFXRenderOverride::supportedDrawAPIs() const
{
    return (MHWRender::kDirectX11 | MHWRender::kOpenGLCoreProfile);
}

MStatus PostFXRenderOverride::setup(const MString& destination)
{
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) return MStatus::kFailure;

    const MHWRender::MRenderTargetManager* targetMgr = renderer->getRenderTargetManager();
    if (!targetMgr) return MStatus::kFailure;

    unsigned int w = 1920, h = 1080;
    renderer->outputTargetSize(w, h);

    releaseTargets();

    // Downsampling Buffers
    MHWRender::MRenderTargetDescription targetDescriptor("_bloom_half", w / 2, h / 2, 1, MHWRender::kR16G16B16A16_FLOAT, 0, false);
    targetHalf = targetMgr->acquireRenderTarget(targetDescriptor);

    targetDescriptor.setName("_bloom_quarter"); targetDescriptor.setWidth(w / 4); targetDescriptor.setHeight(h / 4);
    targetQuarter = targetMgr->acquireRenderTarget(targetDescriptor);

    targetDescriptor.setName("_bloom_eighth"); targetDescriptor.setWidth(w / 8); targetDescriptor.setHeight(h / 8);
    targetEighth = targetMgr->acquireRenderTarget(targetDescriptor);

    targetDescriptor.setName("_bloom_sixteenth"); targetDescriptor.setWidth(w / 16); targetDescriptor.setHeight(h / 16);
    targetSixteenth = targetMgr->acquireRenderTarget(targetDescriptor);

    targetDescriptor.setName("_bloom_thirtysecond"); targetDescriptor.setWidth(w / 32); targetDescriptor.setHeight(h / 32);
    targetThirtySecond = targetMgr->acquireRenderTarget(targetDescriptor);

    //  1/64 Target
    targetDescriptor.setName("_bloom_sixtyfourth"); targetDescriptor.setWidth(w / 64); targetDescriptor.setHeight(h / 64);
    targetSixtyFourth = targetMgr->acquireRenderTarget(targetDescriptor);

    //  1/128 Target
    targetDescriptor.setName("_bloom_onetwentyeighth"); targetDescriptor.setWidth(w / 128); targetDescriptor.setHeight(h / 128);
    targetOneTwentyEighth = targetMgr->acquireRenderTarget(targetDescriptor);

    // Upsampling Buffers
    //  1/64 Blur Container
    targetDescriptor.setName("_bloom_sixtyfourth_blur"); targetDescriptor.setWidth(w / 64); targetDescriptor.setHeight(h / 64);
    targetSixtyFourthBlur = targetMgr->acquireRenderTarget(targetDescriptor);

    //  1/32 Blur Container
    targetDescriptor.setName("_bloom_thirtysecond_blur"); targetDescriptor.setWidth(w / 32); targetDescriptor.setHeight(h / 32);
    targetThirtySecondBlur = targetMgr->acquireRenderTarget(targetDescriptor);

    targetDescriptor.setName("_bloom_sixteenth_blur"); targetDescriptor.setWidth(w / 16); targetDescriptor.setHeight(h / 16);
    targetSixteenthBlur = targetMgr->acquireRenderTarget(targetDescriptor);

    targetDescriptor.setName("_bloom_eighth_blur"); targetDescriptor.setWidth(w / 8); targetDescriptor.setHeight(h / 8);
    targetEighthBlur = targetMgr->acquireRenderTarget(targetDescriptor);

    targetDescriptor.setName("_bloom_quarter_blur"); targetDescriptor.setWidth(w / 4); targetDescriptor.setHeight(h / 4);
    targetQuarterBlur = targetMgr->acquireRenderTarget(targetDescriptor);

    targetDescriptor.setName("_bloom_half_blur"); targetDescriptor.setWidth(w / 2); targetDescriptor.setHeight(h / 2);
    targetHalfBlur = targetMgr->acquireRenderTarget(targetDescriptor);

    // --- PIPELINE ROUTING CONNECTIONS ---
    PostQuadRenderer* down1 = (PostQuadRenderer*)renderOperations[0];
    down1->setInputTargetTexture(NULL); down1->setOutputTargetTexture(targetHalf);

    PostQuadRenderer* down2 = (PostQuadRenderer*)renderOperations[1];
    down2->setInputTargetTexture(targetHalf); down2->setOutputTargetTexture(targetQuarter);

    PostQuadRenderer* down3 = (PostQuadRenderer*)renderOperations[2];
    down3->setInputTargetTexture(targetQuarter); down3->setOutputTargetTexture(targetEighth);

    PostQuadRenderer* down4 = (PostQuadRenderer*)renderOperations[3];
    down4->setInputTargetTexture(targetEighth); down4->setOutputTargetTexture(targetSixteenth);

    PostQuadRenderer* down5 = (PostQuadRenderer*)renderOperations[4];
    down5->setInputTargetTexture(targetSixteenth); down5->setOutputTargetTexture(targetThirtySecond);

    //  Route 1/32 into 1/64
    PostQuadRenderer* down6 = (PostQuadRenderer*)renderOperations[5];
    down6->setInputTargetTexture(targetThirtySecond); down6->setOutputTargetTexture(targetSixtyFourth);

    //  Route 1/64 into 1/128
    PostQuadRenderer* down7 = (PostQuadRenderer*)renderOperations[6];
    down7->setInputTargetTexture(targetSixtyFourth); down7->setOutputTargetTexture(targetOneTwentyEighth);

    // RECONSTRUCTION CHAIN REVERSE ROUTING
    //  Up1: Input = 1/128, Ref = 1/64 -> Output = 1/64 Blur
    PostQuadRenderer* up1 = (PostQuadRenderer*)renderOperations[7];
    up1->setInputTargetTexture(targetOneTwentyEighth); up1->setSecondaryInputTargetTexture(targetSixtyFourth); up1->setOutputTargetTexture(targetSixtyFourthBlur);

    //  Up2: Input = 1/64 Blur, Ref = 1/32 -> Output = 1/32 Blur
    PostQuadRenderer* up2 = (PostQuadRenderer*)renderOperations[8];
    up2->setInputTargetTexture(targetSixtyFourthBlur); up2->setSecondaryInputTargetTexture(targetThirtySecond); up2->setOutputTargetTexture(targetThirtySecondBlur);

    // Up3: Input = 1/32 Blur, Ref = 1/16 -> Output = 1/16 Blur
    PostQuadRenderer* up3 = (PostQuadRenderer*)renderOperations[9];
    up3->setInputTargetTexture(targetThirtySecondBlur); up3->setSecondaryInputTargetTexture(targetSixteenth); up3->setOutputTargetTexture(targetSixteenthBlur);

    // Up4: Input = 1/16 Blur, Ref = 1/8 -> Output = 1/8 Blur
    PostQuadRenderer* up4 = (PostQuadRenderer*)renderOperations[10];
    up4->setInputTargetTexture(targetSixteenthBlur); up4->setSecondaryInputTargetTexture(targetEighth); up4->setOutputTargetTexture(targetEighthBlur);

    // Up5: Input = 1/8 Blur, Ref = 1/4 -> Output = 1/4 Blur
    PostQuadRenderer* up5 = (PostQuadRenderer*)renderOperations[11];
    up5->setInputTargetTexture(targetEighthBlur); up5->setSecondaryInputTargetTexture(targetQuarter); up5->setOutputTargetTexture(targetQuarterBlur);

    // Up6: Input = 1/4 Blur, Ref = 1/2 -> Output = 1/2 Blur
    PostQuadRenderer* up6 = (PostQuadRenderer*)renderOperations[12];
    up6->setInputTargetTexture(targetQuarterBlur); up6->setSecondaryInputTargetTexture(targetHalf); up6->setOutputTargetTexture(targetHalfBlur);

    // Composite
    PostQuadRenderer* comp = (PostQuadRenderer*)renderOperations[13];
    comp->setInputTargetTexture(targetHalfBlur); comp->setSecondaryInputTargetTexture(NULL);

    return MRenderOverride::setup(destination);
}

MStatus PostFXRenderOverride::cleanup() { return MRenderOverride::cleanup(); }
MHWRender::MRenderTarget* PostFXRenderOverride::getTargetHalfBlur() const { return targetHalfBlur; }

void PostFXRenderOverride::triggerShaderReload()
{
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) return;
    MString originalPath = Utils::getPluginDirectory() + "/shaders/MultiPassBloom";
    MString ext = (renderer->drawAPI() == MHWRender::kOpenGLCoreProfile) ? ".ogsfx" : ".fx";
    originalPath += ext;
    MString finalPathToLoad = originalPath;
    MString lowerPath = originalPath.toLowerCase();
    if (lowerPath.rindexW(".fx") == (int)(lowerPath.length() - 3) || lowerPath.rindexW(".ogsfx") == (int)(lowerPath.length() - 6)) {
        MString tempPath = originalPath.substringW(0, originalPath.length() - ext.length()) + "_temp_" + MString(std::to_string(GetTickCount()).c_str()) + ext;
        if (CopyFileA(originalPath.asChar(), tempPath.asChar(), FALSE)) finalPathToLoad = tempPath;
    }
    for (size_t i = 0; i < renderOperations.size(); ++i) {
        PostQuadRenderer* quadOp = dynamic_cast<PostQuadRenderer*>(renderOperations[i]);
        if (quadOp) { quadOp->releaseCustomShader(); quadOp->setShaderFilePath(finalPathToLoad); }
    }
    MGlobal::executeCommand("refresh -f");
}

void PostFXRenderOverride::releaseTargets()
{
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer(); if (!renderer) return;
    const MHWRender::MRenderTargetManager* targetMgr = renderer->getRenderTargetManager(); if (!targetMgr) return;

    if (targetHalf) { targetMgr->releaseRenderTarget(targetHalf); targetHalf = NULL; }
    if (targetQuarter) { targetMgr->releaseRenderTarget(targetQuarter); targetQuarter = NULL; }
    if (targetEighth) { targetMgr->releaseRenderTarget(targetEighth); targetEighth = NULL; }
    if (targetSixteenth) { targetMgr->releaseRenderTarget(targetSixteenth); targetSixteenth = NULL; }
    if (targetThirtySecond) { targetMgr->releaseRenderTarget(targetThirtySecond); targetThirtySecond = NULL; }
    if (targetSixtyFourth) { targetMgr->releaseRenderTarget(targetSixtyFourth); targetSixtyFourth = NULL; } // 
    if (targetOneTwentyEighth) { targetMgr->releaseRenderTarget(targetOneTwentyEighth); targetOneTwentyEighth = NULL; } // 
    if (targetSixtyFourthBlur) { targetMgr->releaseRenderTarget(targetSixtyFourthBlur); targetSixtyFourthBlur = NULL; } // 
    if (targetThirtySecondBlur) { targetMgr->releaseRenderTarget(targetThirtySecondBlur); targetThirtySecondBlur = NULL; } // 
    if (targetSixteenthBlur) { targetMgr->releaseRenderTarget(targetSixteenthBlur); targetSixteenthBlur = NULL; }
    if (targetEighthBlur) { targetMgr->releaseRenderTarget(targetEighthBlur); targetEighthBlur = NULL; }
    if (targetQuarterBlur) { targetMgr->releaseRenderTarget(targetQuarterBlur); targetQuarterBlur = NULL; }
    if (targetHalfBlur) { targetMgr->releaseRenderTarget(targetHalfBlur); targetHalfBlur = NULL; }
}