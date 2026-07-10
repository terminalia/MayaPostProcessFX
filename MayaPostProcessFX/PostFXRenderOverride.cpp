#include "PostFXRenderOverride.h"
#include "PostQuadRenderer.h"
#include <maya/MShaderManager.h>
#include <maya/MGlobal.h>
#include "Utils.h"

//PostQuadRenderer render pass unique names
const MString PostFXRenderOverride::cDownsample1PassName = "PostFXRenderOverride_Down1";
const MString PostFXRenderOverride::cDownsample2PassName = "PostFXRenderOverride_Down2";
const MString PostFXRenderOverride::cDownsample3PassName = "PostFXRenderOverride_Down3";
const MString PostFXRenderOverride::cDownsample4PassName = "PostFXRenderOverride_Down4";
const MString PostFXRenderOverride::cDownsample5PassName = "PostFXRenderOverride_Down5";
const MString PostFXRenderOverride::cUpsample1PassName = "PostFXRenderOverride_Up1";
const MString PostFXRenderOverride::cUpsample2PassName = "PostFXRenderOverride_Up2";
const MString PostFXRenderOverride::cUpsample3PassName = "PostFXRenderOverride_Up3";
const MString PostFXRenderOverride::cUpsample4PassName = "PostFXRenderOverride_Up4";
const MString PostFXRenderOverride::cFinalCompositePassName = "PostFXRenderOverride_FinalComposite";


/*
* Sets up the sequence of rendering operations (MHWRender::MRenderOperation) that Maya will execute sequentially every frame:
*/
PostFXRenderOverride::PostFXRenderOverride(const MString& name)
    : MRenderOverride(name)
    , UIName("MultiPass Bloom Effects")
    , sceneRenderOp(NULL), targetFullScene(NULL)
    , targetHalf(NULL), targetQuarter(NULL), targetEighth(NULL)
    , targetSixteenth(NULL), targetThirtySecond(NULL)
    , targetSixteenthBlur(NULL), targetEighthBlur(NULL), targetQuarterBlur(NULL), targetHalfBlur(NULL)
{
    MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();

    if (!theRenderer)
        return;

    //Preserves the standard 3D Scene to let Maya perform its native scene geometry, material, lighting, and shadow passes normally.
    theRenderer->getStandardViewportOperations(mOperations);

    //Get the folder where the plugin (.mll) resides
    MString pluginDir = Utils::getPluginDirectory();
    //Choose what kind of shader it has to load based on the active API (.ogsfx -> OpenGL / .fx -> DirectX 11)
    MString ext = (theRenderer->drawAPI() == MHWRender::kOpenGLCoreProfile) ? ".ogsfx" : ".fx";
    MString shaderPath = pluginDir + "/shaders/MultiPassBloom" + ext;

    /*###########################################################################################################
    * INJECTING POST PROCESSING QUADS
    * * Allocates a series of PostQuadRenderer full-screen passes and appends them right after the native scene is drawn.
    * Each PostQuadRenderer pass is said to be a RenderOperation
    */

    //Downsample1: Takes the raw scene, extracts bright highlights based on a threshold, and shrinks the resolution to half (1/2).
    PostQuadRenderer* downSample1 = new PostQuadRenderer(cDownsample1PassName, shaderPath, "ThresholdDownsample");
    downSample1->setClearOverride(true);
    renderOperations.push_back(downSample1);
    mOperations.insertAfter(MHWRender::MRenderOperation::kStandardSceneName, downSample1);

    //Downsample2: Shrinks the highlights further to quarter resolution (1/4)
    PostQuadRenderer* downSample2 = new PostQuadRenderer(cDownsample2PassName, shaderPath, "StandardDownsample");
    downSample2->setClearOverride(true);
    renderOperations.push_back(downSample2);
    mOperations.insertAfter(cDownsample1PassName, downSample2);

    //Downsample3: Shrinks the highlights down to eighth resolution (1/8).
    PostQuadRenderer* downSample3 = new PostQuadRenderer(cDownsample3PassName, shaderPath, "StandardDownsample");
    downSample3->setClearOverride(true);
    renderOperations.push_back(downSample3);
    mOperations.insertAfter(cDownsample2PassName, downSample3);

    //Downsample4: Shrinks the highlights down to sixteenth resolution (1/16).
    PostQuadRenderer* downSample4 = new PostQuadRenderer(cDownsample4PassName, shaderPath, "StandardDownsample");
    downSample4->setClearOverride(true);
    renderOperations.push_back(downSample4);
    mOperations.insertAfter(cDownsample3PassName, downSample4);

    //Downsample5: Shrinks the highlights down to thirty-second resolution (1/32).
    PostQuadRenderer* downSample5 = new PostQuadRenderer(cDownsample5PassName, shaderPath, "StandardDownsample");
    downSample5->setClearOverride(true);
    renderOperations.push_back(downSample5);
    mOperations.insertAfter(cDownsample4PassName, downSample5);

    //Upsample1: Blends and blurs the 1/32nd resolution texture back up into a 1/16th resolution target.
    PostQuadRenderer* upSample1 = new PostQuadRenderer(cUpsample1PassName, shaderPath, "UpsampleWide");
    upSample1->setClearOverride(true);
    renderOperations.push_back(upSample1);
    mOperations.insertAfter(cDownsample5PassName, upSample1);

    //Upsample2: Blends and blurs the 1/16th resolution texture back up into a 1/8th resolution target.
    PostQuadRenderer* upSample2 = new PostQuadRenderer(cUpsample2PassName, shaderPath, "UpsampleWide");
    upSample2->setClearOverride(true);
    renderOperations.push_back(upSample2);
    mOperations.insertAfter(cUpsample1PassName, upSample2);

    //Upsample3: Blends and blurs the 1/8th resolution texture back up into a 1/4th resolution target.
    PostQuadRenderer* upSample3 = new PostQuadRenderer(cUpsample3PassName, shaderPath, "UpsampleMedium");
    upSample3->setClearOverride(true);
    renderOperations.push_back(upSample3);
    mOperations.insertAfter(cUpsample2PassName, upSample3);

    //Upsample4: Blends and blurs the 1/4th resolution texture back up into a 1/2th resolution target.
    PostQuadRenderer* upSample4 = new PostQuadRenderer(cUpsample4PassName, shaderPath, "UpsampleTight");
    upSample4->setClearOverride(true);
    renderOperations.push_back(upSample4);
    mOperations.insertAfter(cUpsample3PassName, upSample4);

    //FinalComposite: Adds the completely accumulated, smooth bloom blur back on top of your original full-resolution scene.
    PostQuadRenderer* finalComposite = new PostQuadRenderer(cFinalCompositePassName, shaderPath, "FinalComposite");
    renderOperations.push_back(finalComposite);
    mOperations.insertAfter(cUpsample4PassName, finalComposite);
}

PostFXRenderOverride::~PostFXRenderOverride()
{
    releaseTargets();

    // FIXED FOR MAYA 2026: Do NOT manually delete the operations here.
    // mOperations has full ownership and handles the destruction automatically.
    renderOperations.clear();
}

MHWRender::DrawAPI PostFXRenderOverride::supportedDrawAPIs() const
{
    return (MHWRender::kDirectX11 | MHWRender::kOpenGLCoreProfile);
}

/*
* Every single frame, Maya calls the setup() method before rendering to see if any hardware settings or viewport boundaries have changed.
*/
MStatus PostFXRenderOverride::setup(const MString& destination)
{
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();

    if (!renderer)
        return MStatus::kFailure;

    const MHWRender::MRenderTargetManager* targetMgr = renderer->getRenderTargetManager();

    if (!targetMgr)
        return MStatus::kFailure;

    //Viewport Responsiveness: It queries Maya for the active viewport dimensions via renderer->outputTargetSize(w, h); 
    //If the artist resizes the panel or enters full-screen mode, this class dynamically detects it.
    unsigned int w = 1920, h = 1080;
    renderer->outputTargetSize(w, h);

    //When the viewport resizes or the plugin unloads, it triggers releaseTargets(), which safely commands Maya's target manager to free up memory
    //on the graphics card and prevent dangerous VRAM leaks.
    releaseTargets();

    /*###########################################################################################################
    * ALLOCATING OFFSCREEN TEXTURE BUFFERS (RENDER TARGETS)
    *
    * Instead of processing the bloom at full screen resolution (which is slow and makes wide blurs difficult),
    * this code sets up a series of progressively smaller textures to efficiently downsample, blur, and
    * upsample the bright areas of your scene.
    */

    //DOWN SAMPLING PHASE
    //This line instantiates a description object (targetDescriptor) which acts as a configuration state for the texture you want the GPU to create. 
    //The parameters are defined as follows:
    //  -"_bloom_half": The internal debug name for the texture asset.
    //  -w / 2, h / 2: The dimensions of the texture. It takes the current viewport width (w) and height (h) and cuts them in half.
    //  -1: The number of mipmap levels to generate (1 means only the base level, no automatic mipmaps).
    //  -MHWRender::kR16G16B16A16_FLOAT: The pixel data format. Floating-point textures allow colors to exceed 1.0, preserving high-dynamic range (HDR) super-bright highlights
    //  -0: Multi-sampling (MSAA) count. Post-processing textures do not need anti-aliasing directly inside them, so this is set to 0 to save VRAM.
    //  -false: Specifies that this texture will not require a matching automatic depth/stencil buffer.
    MHWRender::MRenderTargetDescription targetDescriptor("_bloom_half", w / 2, h / 2, 1, MHWRender::kR16G16B16A16_FLOAT, 0, false);

    //This line passes the targetDescriptor to Maya’s MRenderTargetManager. 
    //Maya either allocates a fresh texture block in GPU memory that matches your description or reuses an existing cached texture, returning a pointer to it (targetHalf).
    targetHalf = targetMgr->acquireRenderTarget(targetDescriptor);

    //Quarter Resolution (1/4): Modifies the name and drops the size to a quarter of the screen. This target catches the next stage of downsampling.
    targetDescriptor.setName("_bloom_quarter");
    targetDescriptor.setWidth(w / 4);
    targetDescriptor.setHeight(h / 4);
    targetQuarter = targetMgr->acquireRenderTarget(targetDescriptor);

    //Eighth Resolution (1/8): Drops the size down to an eighth of the screen. By the time your image is shrunk to 1/8th resolution, a tiny 3x3 pixel blur shader 
    //covers a massive visual area relative to the original scene, allowing for a huge, soft glow trail without killing frame rates.
    targetDescriptor.setName("_bloom_eighth");
    targetDescriptor.setWidth(w / 8);
    targetDescriptor.setHeight(h / 8);
    targetEighth = targetMgr->acquireRenderTarget(targetDescriptor);

    //Sixteenth Resolution (1/16): Pushes the downsampling chain deeper to expand maximum visual bleed coverage.
    targetDescriptor.setName("_bloom_sixteenth");
    targetDescriptor.setWidth(w / 16);
    targetDescriptor.setHeight(h / 16);
    targetSixteenth = targetMgr->acquireRenderTarget(targetDescriptor);

    //Thirty-Second Resolution (1/32): The deepest level of the pyramid to swallow enormous screen coordinates safely.
    targetDescriptor.setName("_bloom_thirtysecond");
    targetDescriptor.setWidth(w / 32);
    targetDescriptor.setHeight(h / 32);
    targetThirtySecond = targetMgr->acquireRenderTarget(targetDescriptor);

    //UP SAMPLING PHASE
    //These copy the dimensions of your previous pyramid levels. They serve as the destination containers for your upsampling passes, blending the super-blurry 
    //small layers back into the sharper large layers before your FinalComposite pass merges them onto the screen.
    targetDescriptor.setName("_bloom_sixteenth_blur");
    targetDescriptor.setWidth(w / 16);
    targetDescriptor.setHeight(h / 16);
    targetSixteenthBlur = targetMgr->acquireRenderTarget(targetDescriptor);

    targetDescriptor.setName("_bloom_eighth_blur");
    targetDescriptor.setWidth(w / 8);
    targetDescriptor.setHeight(h / 8);
    targetEighthBlur = targetMgr->acquireRenderTarget(targetDescriptor);

    targetDescriptor.setName("_bloom_quarter_blur");
    targetDescriptor.setWidth(w / 4);
    targetDescriptor.setHeight(h / 4);
    targetQuarterBlur = targetMgr->acquireRenderTarget(targetDescriptor);

    targetDescriptor.setName("_bloom_half_blur");
    targetDescriptor.setWidth(w / 2);
    targetDescriptor.setHeight(h / 2);
    targetHalfBlur = targetMgr->acquireRenderTarget(targetDescriptor);

    /*###########################################################################################################
    * SETUP THE ROUTING LOGIC (NODE CONNECTIONS) BETWEEN THE VARIOUS RENDER PASSES FOR MULTI-PASS BLOOM PYRAMID
    *
    * By passing texture pointers from one pass into the next, you are chaining a series of full-screen quad rendering operations together.
    * This configuration explicitly dictates how data flows step-by-step through the downsampling, blurring, upsampling, and final compositing phases.
    */

    //DOWN SAMPLING CHAIN
    //The first three operations shrink the image layer-by-layer to efficiently build a wide blur radius.

    //PASS 1
    //It runs a shader to extract only the super-bright pixels (the threshold highlights) and writes that filtered image out at half resolution into targetHalf.
    //"downSamplePass1->setInputTargetTexture(NULL)" means to capture Maya's native full-resolution viewport scene buffer.
    PostQuadRenderer* downSamplePass1 = (PostQuadRenderer*)renderOperations[0];
    downSamplePass1->setInputTargetTexture(NULL);
    downSamplePass1->setOutputTargetTexture(targetHalf);

    //PASS 2
    //It blurs and downscales them further into a quarter-resolution buffer (targetQuarter).
    PostQuadRenderer* downSamplePass2 = (PostQuadRenderer*)renderOperations[1];
    downSamplePass2->setInputTargetTexture(targetHalf);
    downSamplePass2->setOutputTargetTexture(targetQuarter);

    //PASS 3
    //Shrinks the highlights to their smallest state: eighth resolution (targetEighth).
    PostQuadRenderer* downSamplePass3 = (PostQuadRenderer*)renderOperations[2];
    downSamplePass3->setInputTargetTexture(targetQuarter);
    downSamplePass3->setOutputTargetTexture(targetEighth);

    //PASS 4
    // Connect 1/8th resolution texture down into the 1/16th resolution texture buffer.
    PostQuadRenderer* downSamplePass4 = (PostQuadRenderer*)renderOperations[3];
    downSamplePass4->setInputTargetTexture(targetEighth);
    downSamplePass4->setOutputTargetTexture(targetSixteenth);

    //PASS 5
    // Connect 1/16th resolution texture down into the 1/32nd resolution texture buffer.
    PostQuadRenderer* downSamplePass5 = (PostQuadRenderer*)renderOperations[4];
    downSamplePass5->setInputTargetTexture(targetSixteenth);
    downSamplePass5->setOutputTargetTexture(targetThirtySecond);

    //UP SAMPLING CHAIN
    //Instead of stretching a tiny 1/8th resolution texture back to full screen—which would look muddy and blocky—the upsampling stages progressively 
    //upscale the blur while injecting sharper structural data cached from the downsampling phase.
    //The up sampling chain runs in reverse order, it means it takes the last output texture (targetEight), combines with the previous (targetQuarter) 
    //to get an upscaled texture

    //PASS 6 (Upsample Pass 1)
    // Input = 1/32nd texture, Secondary Reference = 1/16th texture -> Writes to 1/16th Blur Target.
    PostQuadRenderer* upSamplePass1 = (PostQuadRenderer*)renderOperations[5];
    upSamplePass1->setInputTargetTexture(targetThirtySecond);
    upSamplePass1->setSecondaryInputTargetTexture(targetSixteenth);
    upSamplePass1->setOutputTargetTexture(targetSixteenthBlur);

    //PASS 7 (Upsample Pass 2)
    // Input = 1/16th Blur, Secondary Reference = 1/8th texture -> Writes to 1/8th Blur Target.
    PostQuadRenderer* upSamplePass2 = (PostQuadRenderer*)renderOperations[6];
    upSamplePass2->setInputTargetTexture(targetSixteenthBlur);
    upSamplePass2->setSecondaryInputTargetTexture(targetEighth);
    upSamplePass2->setOutputTargetTexture(targetEighthBlur);

    // PASS 8 (Upsample Pass 3)
    // Input = 1/8th Blur, Secondary Reference = 1/4th texture -> Writes to 1/4th Blur Target.
    PostQuadRenderer* upSamplePass3 = (PostQuadRenderer*)renderOperations[7];
    upSamplePass3->setInputTargetTexture(targetEighthBlur);
    upSamplePass3->setSecondaryInputTargetTexture(targetQuarter);
    upSamplePass3->setOutputTargetTexture(targetQuarterBlur);

    // PASS 9 (Upsample Pass 4)
    // Input = 1/4th Blur, Secondary Reference = 1/2th texture -> Writes to 1/2th Blur Target.
    PostQuadRenderer* upSamplePass4 = (PostQuadRenderer*)renderOperations[8];
    upSamplePass4->setInputTargetTexture(targetQuarterBlur);
    upSamplePass4->setSecondaryInputTargetTexture(targetHalf);
    upSamplePass4->setOutputTargetTexture(targetHalfBlur);

    //FINAL COMPOSITE
    //Its primary job is to take the fully accumulated, up-sampled output texture(targetHalfBlur) and blend it cleanly back on top of your original, razor-sharp 3D viewport scene
    PostQuadRenderer* comp = (PostQuadRenderer*)renderOperations[9];
    comp->setInputTargetTexture(targetHalfBlur);
    comp->setSecondaryInputTargetTexture(NULL);

    return MRenderOverride::setup(destination);
}

MStatus PostFXRenderOverride::cleanup()
{
    return MRenderOverride::cleanup();
}

MHWRender::MRenderTarget* PostFXRenderOverride::getTargetHalfBlur() const
{
    return targetHalfBlur;
}

/*
* Its primary purpose is to enable live shader hot-reloading within Maya's viewport without forcing the developer to restart
* Maya every time they modify a shader file (.fx or .ogsfx) on disk.
*/
void PostFXRenderOverride::triggerShaderReload()
{
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();

    if (!renderer)
        return;

    MString originalPath = Utils::getPluginDirectory() + "/shaders/MultiPassBloom";
    MString ext = (renderer->drawAPI() == MHWRender::kOpenGLCoreProfile) ? ".ogsfx" : ".fx";
    originalPath += ext;

    MString finalPathToLoad = originalPath;
    MString lowerPath = originalPath.toLowerCase();

    bool endsWithFx = (lowerPath.rindexW(".fx") == (int)(lowerPath.length() - 3));
    bool endsWithOgsfx = (lowerPath.rindexW(".ogsfx") == (int)(lowerPath.length() - 6));

    if (endsWithFx || endsWithOgsfx)
    {
        MString tempPath = originalPath.substringW(0, originalPath.length() - ext.length());
        tempPath += "_temp_" + MString(std::to_string(GetTickCount()).c_str()) + ext;

        if (CopyFileA(originalPath.asChar(), tempPath.asChar(), FALSE)) {
            finalPathToLoad = tempPath;
        }
    }

    for (size_t i = 0; i < renderOperations.size(); ++i)
    {
        PostQuadRenderer* quadOp = dynamic_cast<PostQuadRenderer*>(renderOperations[i]);
        if (quadOp)
        {
            quadOp->releaseCustomShader();
            quadOp->setShaderFilePath(finalPathToLoad);
        }
    }

    MGlobal::executeCommand("refresh -f");
}

void PostFXRenderOverride::releaseTargets()
{
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();

    if (!renderer)
        return;

    const MHWRender::MRenderTargetManager* targetMgr = renderer->getRenderTargetManager();

    if (!targetMgr)
        return;

    if (targetHalf)
    {
        targetMgr->releaseRenderTarget(targetHalf);
        targetHalf = NULL;
    }

    if (targetQuarter)
    {
        targetMgr->releaseRenderTarget(targetQuarter);
        targetQuarter = NULL;
    }

    if (targetEighth)
    {
        targetMgr->releaseRenderTarget(targetEighth);
        targetEighth = NULL;
    }

    if (targetSixteenth)
    {
        targetMgr->releaseRenderTarget(targetSixteenth);
        targetSixteenth = NULL;
    }

    if (targetThirtySecond)
    {
        targetMgr->releaseRenderTarget(targetThirtySecond);
        targetThirtySecond = NULL;
    }

    if (targetSixteenthBlur)
    {
        targetMgr->releaseRenderTarget(targetSixteenthBlur);
        targetSixteenthBlur = NULL;
    }

    if (targetEighthBlur)
    {
        targetMgr->releaseRenderTarget(targetEighthBlur);
        targetEighthBlur = NULL;
    }

    if (targetQuarterBlur)
    {
        targetMgr->releaseRenderTarget(targetQuarterBlur);
        targetQuarterBlur = NULL;
    }

    if (targetHalfBlur)
    {
        targetMgr->releaseRenderTarget(targetHalfBlur);
        targetHalfBlur = NULL;
    }
}