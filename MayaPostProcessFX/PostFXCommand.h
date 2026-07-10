#pragma once
#include <maya/MPxCommand.h>

class PostFXCommand : public MPxCommand
{
public:
	PostFXCommand();
	~PostFXCommand() override;

	MStatus doIt(const MArgList& args) override;
	static MSyntax newSyntax();
	static void* creator();

private:
	static const char* cReloadLongFlag;
	static const char* cBloomIntensityLongFlag;
	static const char* cGlowRadiusLongFlag;
	static const char* cBloomActiveLongFlag;
};

