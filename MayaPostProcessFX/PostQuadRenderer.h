#pragma once
#include <maya/MViewport2Renderer.h>

/*
* The PostQuadRender class represents an individual post-processing pass within your custom Viewport 2.0 rendering pipeline. 
* It inherits from MHWRender::MQuadRender, a specialized Maya SDK class used to render a full-screen 2D quad (two triangles covering the entire screen).
* 
* Instead of rendering 3D geometry (vertices, normals, lighting), PostQuadRender is designed to take one or two existing textures ("inputTargetTexture", e.g. the previous pass's output texture), 
* apply a 2D fragment/pixel shader effect over the screen quad, and output the result to a new render target texture or directly to the final viewport.
*/
class PostQuadRenderer : public MHWRender::MQuadRender
{
public:
	PostQuadRenderer(const MString& name, const MString& fxFilePath, const MString& technique);
	~PostQuadRenderer() override;

    const MHWRender::MShaderInstance* shader() override;
    MHWRender::MClearOperation& clearOperation() override;
    MHWRender::MRenderTarget* const* targetOverrideList(unsigned int& listSize) override;

    void releaseCustomShader();
    void setInputTargetTexture(MHWRender::MRenderTarget* target);
    void setSecondaryInputTargetTexture(MHWRender::MRenderTarget* target);
    void setOutputTargetTexture(MHWRender::MRenderTarget* target);
    void setShaderFilePath(const MString& path);
    void setClearOverride(bool clear);

    void setBloomIntensity(float val);
    float getBloomIntensity() const;

    void setGlowRadius(float val);
    float getGlowRadius() const;

    void setThreshold(float val);
    float getThreshold() const;

    void setSoftKnee(float val);
    float getSoftKnee() const;

protected:
    MHWRender::MShaderInstance* shaderInstance;
    MString originalFxFilePath;
    MString fxFilePath;
    MString techniqueName;

    float bloomIntensity;
    float glowRadius;
    float threshold;
    float softKnee;

    bool shouldClear;

    //Stores the memory address of the primary input texture (render target) that the current shader pass needs to read from and process
    MHWRender::MRenderTarget* inputTargetTexture;
    
    //Acts as a supplementary data line used during the Upsampling (progressive blending) stages to feed a sharper, higher-resolution companion 
    //layer into the shader to preserve edge details and structure
    MHWRender::MRenderTarget* secondaryInputTargetTexture;

    //Specifies the destination texture buffer where the current shader pass should write its results
    //Tells Maya to intercept the rendering output of the full-screen quad and redirect it into an offscreen render target (like a lower-resolution texture) instead of drawing it directly onto the user's screen.
    MHWRender::MRenderTarget* outputTargetTexture;

    //Think of outputTargetArray as a compatibility wrapper. Even though your pass only handles a single output texture, you must wrap that texture pointer inside a C-style array 
    //to match the structural blueprint required by Maya's internal graphics pipeline
    MHWRender::MRenderTarget* outputTargetArray[1];
};

