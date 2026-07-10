#include "PostFXCommand.h"
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MSyntax.h>
#include <maya/MPxCommand.h>
#include <maya/MViewport2Renderer.h>
#include "PostFXRenderOverride.h"
#include "PostQuadRenderer.h"

const char* PostFXCommand::cReloadLongFlag = "-reload";
const char* PostFXCommand::cBloomIntensityLongFlag = "-bloomIntensity";
const char* PostFXCommand::cGlowRadiusLongFlag = "-glowRadius";
const char* PostFXCommand::cBloomActiveLongFlag = "-bloom";

PostFXCommand::PostFXCommand()
{ }

PostFXCommand::~PostFXCommand()
{ }

MStatus PostFXCommand::doIt(const MArgList & args)
{
    MStatus status;

    MArgDatabase argData(syntax(), args, &status);

    if (!status) 
    {
        MGlobal::displayError("Failed to parse command line arguments.");
        return status;
    }

    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();

    if (!renderer) 
        return MStatus::kFailure;

    const MHWRender::MRenderOverride* baseOverride = renderer->findRenderOverride("ColorPostProcessOverride");

    if (!baseOverride) 
    {
        MGlobal::displayError("ColorPostProcessOverride pipeline is not active.");
        return MStatus::kFailure;
    }

    PostFXRenderOverride* ourOverride = const_cast<PostFXRenderOverride*>(dynamic_cast<const PostFXRenderOverride*>(baseOverride));

    if (!ourOverride) 
        return MStatus::kFailure;

    // Grab a pointer to the first custom post pass to extract values during queries
    PostQuadRenderer* firstOp = nullptr;
    if (!ourOverride->renderOperations.empty()) 
    {
        firstOp = dynamic_cast<PostQuadRenderer*>(ourOverride->renderOperations[0]);
    }

    // --- 1. HANDLE QUERY MODE (postColor -q ...) ---
    if (argData.isQuery())
    {
        if (!firstOp) 
            return MStatus::kFailure;

        if (argData.isFlagSet(cBloomIntensityLongFlag)) 
        {
            setResult(firstOp->getBloomIntensity());
            return MStatus::kSuccess;
        }

        if (argData.isFlagSet(cGlowRadiusLongFlag)) 
        {
            setResult(firstOp->getGlowRadius());
            return MStatus::kSuccess;
        }

        if (argData.isFlagSet(cBloomActiveLongFlag)) 
        {
            // Check if the bloom operation pass is enabled in Viewport 2.0 graph
            setResult(firstOp->enabled());
            return MStatus::kSuccess;
        }

        MGlobal::displayError("Invalid query flag specified.");
        return MStatus::kFailure;
    }

    // --- 2. HANDLE EDIT / COMMAND INVOCATIONS ---
    if (argData.isFlagSet(cReloadLongFlag))
    {
        ourOverride->triggerShaderReload();
        return MStatus::kSuccess;
    }

    bool explicitUpdate = false;

    // Loop through operations to apply edit updates cleanly
    for (auto op : ourOverride->renderOperations) 
    {
        PostQuadRenderer* quadOp = dynamic_cast<PostQuadRenderer*>(op);
        
        if (!quadOp) 
            continue;

        if (argData.isFlagSet(cBloomIntensityLongFlag)) 
        {
            double val;
            argData.getFlagArgument(cBloomIntensityLongFlag, 0, val);
            quadOp->setBloomIntensity((float)val);
            explicitUpdate = true;
        }

        if (argData.isFlagSet(cGlowRadiusLongFlag)) 
        {
            double val;
            argData.getFlagArgument(cGlowRadiusLongFlag, 0, val);
            quadOp->setGlowRadius((float)val);
            explicitUpdate = true;
        }
        
        if (argData.isFlagSet(cBloomActiveLongFlag)) 
        {
            int state = 1; 
            argData.getFlagArgument(cBloomActiveLongFlag, 0, state);

            if (quadOp->name() != PostFXRenderOverride::cFinalCompositePassName) 
            {
                quadOp->setEnabled(state != 0);
            }
            explicitUpdate = true;
        }
    }

    if (explicitUpdate) 
    {
        MGlobal::executeCommand("refresh -f");
    }

    return MStatus::kSuccess;
}

MSyntax PostFXCommand::newSyntax()
{
    MSyntax syntax;

    // Enable global query and edit switches natively in the command syntax core
    syntax.enableQuery(true);
    syntax.enableEdit(false); // We handle edits through clean direct flags

    syntax.addFlag("", cReloadLongFlag, MSyntax::kNoArg);
    syntax.addFlag("", cBloomIntensityLongFlag, MSyntax::kDouble);
    syntax.addFlag("", cGlowRadiusLongFlag, MSyntax::kDouble);
    syntax.addFlag("", cBloomActiveLongFlag, MSyntax::kLong);

    return syntax;
}

void* PostFXCommand::creator()
{
    return new PostFXCommand();
}
