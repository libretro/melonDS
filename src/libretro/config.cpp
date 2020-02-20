/*
    Copyright 2016-2019 Arisotura

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Config.h"
#include "Platform.h"


namespace Config
{
    int _3DRenderer;
    int Threaded3D;

    int GL_ScaleFactor;
    int GL_Antialias;

#ifdef JIT_ENABLED
    bool JIT_Enable = true;
    int JIT_MaxBlockSize = 12;
    bool JIT_BrancheOptimisations = true;
    bool JIT_LiteralOptimisations = true;
#else
    // Needed for savestate
    bool JIT_Enable = false;
#endif

    ConfigEntry ConfigFile[] =
    {
        {"3DRenderer", 0, &_3DRenderer, 1, NULL, 0},
        {"Threaded3D", 0, &Threaded3D, 1, NULL, 0},

        {"GL_ScaleFactor", 0, &GL_ScaleFactor, 1, NULL, 0},
        {"GL_Antialias", 0, &GL_Antialias, 0, NULL, 0},

#ifdef JIT_ENABLED
        {"JIT_Enable", 0, &JIT_Enable, 0, NULL, 0},
        {"JIT_MaxBlockSize", 0, &JIT_MaxBlockSize, 10, NULL, 0},
        {"JIT_BrancheOptimisations", 0, &JIT_BrancheOptimisations, 1, NULL, 0},
        {"JIT_LiteralOptimisations", 0, &JIT_LiteralOptimisations, 1, NULL, 0},
#endif

        {"", -1, NULL, 0, NULL, 0}
    };

    extern ConfigEntry PlatformConfigFile[];


    void Load()
    {

    }

    void Save()
    {

    }
}
