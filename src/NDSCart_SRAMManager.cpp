/*
    Copyright 2016-2020 Arisotura

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
#include <unistd.h>
#include <time.h>
#include "NDSCart_SRAMManager.h"
#include "Platform.h"

namespace NDSCart_SRAMManager
{
    Platform::Thread* FlushThread;
    bool FlushThreadRunning;
    Platform::Mutex* SecondaryBufferLock;

    char Path[1024];

    u8* Buffer;
    u32 Length;

    u8* SecondaryBuffer;
    u32 SecondaryBufferLength;
    
    time_t TimeAtLastFlushRequest;

    // We keep versions in case the user closes the application before
    // a flush cycle is finished.
    u32 PreviousFlushVersion;
    u32 FlushVersion;

    void FlushThreadFunc();
    void FlushSecondaryBufferToFile();

    bool Init()
    {
#ifndef __LIBRETRO__
        FlushThread = Platform::Thread_Create(FlushThreadFunc);
        FlushThreadRunning = true;

        SecondaryBufferLock = Platform::Mutex_Create();
#endif

        return true;
    }

    void DeInit()
    {
#ifndef __LIBRETRO__
        if (FlushThreadRunning)
        {
            FlushThreadRunning = false;
            Platform::Thread_Wait(FlushThread);
            Platform::Thread_Free(FlushThread);
            FlushSecondaryBufferToFile();
        }
#endif

        delete SecondaryBuffer;

        Platform::Mutex_Free(SecondaryBufferLock);
    }
    
    void Setup(const char* path, u8* buffer, u32 length)
    {
        // Flush SRAM in case there is unflushed data from previous state.
        FlushSecondaryBufferToFile();

#ifndef __LIBRETRO__
        Platform::Mutex_Lock(SecondaryBufferLock);
#endif

        strncpy(Path, path, 1023);
        Path[1023] = '\0';

        Buffer = buffer;
        Length = length;

        delete SecondaryBuffer; // Delete secondary buffer, there might be previous state.

        SecondaryBuffer = new u8[length];
        SecondaryBufferLength = length;

        FlushVersion = 0;
        PreviousFlushVersion = 0;
        TimeAtLastFlushRequest = 0;

#ifndef __LIBRETRO__
        Platform::Mutex_Unlock(SecondaryBufferLock);
#endif
    }

    void RequestFlush()
    {
#ifndef __LIBRETRO__
        Platform::Mutex_Lock(SecondaryBufferLock);
#endif
        printf("NDS SRAM: Flush requested\n");
        memcpy(SecondaryBuffer, Buffer, Length);
        FlushVersion++;
        TimeAtLastFlushRequest = time(NULL);
#ifndef __LIBRETRO__
        Platform::Mutex_Unlock(SecondaryBufferLock);
#endif
    }

#ifndef __LIBRETRO__
    void FlushThreadFunc()
    {
        for (;;)
        {
            Platform::Sleep(100 * 1000); // 100ms

            if (!FlushThreadRunning) return;
            
            // We debounce for two seconds after last flush request to ensure that writing has finished.
            if (TimeAtLastFlushRequest == 0 || difftime(time(NULL), TimeAtLastFlushRequest) < 2)
            {
                continue;
            }

            FlushSecondaryBufferToFile();
        }
    }
#else
    void Flush()
    {
        if (TimeAtLastFlushRequest != 0 && difftime(time(NULL), TimeAtLastFlushRequest) > 2)
        {
            FlushSecondaryBufferToFile();
        }
    }
#endif

    void FlushSecondaryBufferToFile()
    {
        if (FlushVersion == PreviousFlushVersion)
        {
            return;
        }

#ifndef __LIBRETRO__
        Platform::Mutex_Lock(SecondaryBufferLock);
#endif
        FILE* f = Platform::OpenFile(Path, "wb");
        if (f)
        {
            printf("NDS SRAM: Written\n");
            fwrite(SecondaryBuffer, SecondaryBufferLength, 1, f);
            fclose(f);
        }
        PreviousFlushVersion = FlushVersion;
        TimeAtLastFlushRequest = 0;
#ifndef __LIBRETRO__
        Platform::Mutex_Unlock(SecondaryBufferLock);
#endif
    }
}
