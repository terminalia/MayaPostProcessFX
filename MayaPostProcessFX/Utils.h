#pragma once

#include <maya/MString.h>
#include <Windows.h>

class Utils
{
public:
	static MString getPluginDirectory()
	{
        char path[256];
        HMODULE hm = NULL;
        
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&getPluginDirectory, &hm)) 
        {
            GetModuleFileNameA(hm, path, sizeof(path));
            MString fullPath(path);
            int lastSlash = fullPath.rindexW('\\');
            
            if (lastSlash != -1) 
            {
                MString dir = fullPath.substringW(0, lastSlash);
                dir.substitute("\\", "/");
                return dir;
            }
        }

        return "";
	}
};

