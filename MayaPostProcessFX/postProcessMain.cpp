#include <maya/MIOStream.h>
#include <cstdlib>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h> 
#include <maya/MViewport2Renderer.h>
#include <string>
#include "PostFXRenderOverride.h"
#include "PostFXCommand.h"

//PLUGIN MAIN ENTRY POINT
/*
* In any Maya C++ plugin, two specific global functions must be exposed so that Maya can load and unload the compiled library: 
* -initializePlugin() 
* -uninitializePlugin().
*/

const std::string cAuthor = "Mistwork Studio";
const std::string cVersion = "1.0";
const std::string cCompatibility = "Any";
const std::string commandName = "postFX";

/*
This function is called by Maya the moment a user checks the Loaded box in Maya's Plug-in Manager. 
Its purpose is to register the custom viewport rendering override and its associated command line controls.
*/
MStatus initializePlugin(MObject obj)
{
	MFnPlugin plugin(obj, cAuthor.c_str(), cVersion.c_str(), cCompatibility.c_str());
	MStatus status = MStatus::kSuccess;

	MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();

	if (renderer)
	{
		MString apiName = "Unknown API";
		switch (renderer->drawAPI())
		{
		case MHWRender::kDirectX11:
			apiName = "DirectX 11";
			break;

		case MHWRender::kOpenGLCoreProfile:
			apiName = "OpenGL Core Profile (Strict)";
			break;
		
		default:
			apiName = "Other/Unsupported Profile";
			break;
		}

		MGlobal::displayInfo("[MistworkPostFX] MistworkPostFX Loaded successfully. Viewport 2.0 Active API: " + apiName);

		PostFXRenderOverride* overridePtr = new PostFXRenderOverride("PostFXRenderOverride");
		if (overridePtr)
		{
			renderer->registerOverride(overridePtr);
		}

		plugin.registerCommand(commandName.c_str(),
			PostFXCommand::creator,
			PostFXCommand::newSyntax);
	}

	

	return status;
}

MStatus uninitializePlugin(MObject obj)
{
	MStatus status = MStatus::kSuccess;
	MFnPlugin plugin(obj);
	
	MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
	
	if (renderer)
	{
		const MHWRender::MRenderOverride* overridePtr = renderer->findRenderOverride("PostFXRenderOverride");
		if (overridePtr)
		{
			renderer->deregisterOverride(overridePtr);
			delete overridePtr;
		}
		plugin.deregisterCommand(commandName.c_str());
		MGlobal::displayInfo("[MistworkPostFX] Unloaded successfully.");
	}

	return status;
}