#pragma once

#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace cb3
{

struct WorldConfig
{
    static constexpr std::uint32_t MinWidth = 1;
    static constexpr std::uint32_t MinHeight = 1;
    static constexpr std::uint32_t MaxAxisSize = 65535;
    static constexpr std::uint32_t DefaultWidth = 12 * 16 * 30;
    static constexpr std::uint32_t DefaultHeight = 133;

    std::uint32_t width = DefaultWidth;
    std::uint32_t height = DefaultHeight;
    std::uint32_t maxWorkerThreads = 0;

    bool Validate(std::wstring* error = nullptr) const
    {
        auto fail = [&](const wchar_t* message)
        {
            if (error)
            {
                *error = message;
            }
            return false;
        };

        if (width < MinWidth || width > MaxAxisSize)
        {
            return fail(L"World width must be between 1 and 65535.");
        }

        if (height < MinHeight || height > MaxAxisSize)
        {
            return fail(L"World height must be between 1 and 65535.");
        }

        if (maxWorkerThreads > MaxAxisSize)
        {
            return fail(L"Worker thread limit must be between 0 and 65535.");
        }

        const std::uint64_t cellCount = static_cast<std::uint64_t>(width) * height;
        constexpr std::uint64_t estimatedBytesPerCell = 64;

        if (cellCount > (std::numeric_limits<std::size_t>::max)() / estimatedBytesPerCell)
        {
            return fail(L"World dimensions overflow the addressable memory size.");
        }

        const std::uint64_t estimatedBytes = cellCount * estimatedBytesPerCell;
        MEMORYSTATUSEX memory = {};
        memory.dwLength = sizeof(memory);

        if (GlobalMemoryStatusEx(&memory))
        {
            const std::uint64_t availableCommit = memory.ullAvailPhys + memory.ullAvailPageFile;
            if (estimatedBytes > memory.ullAvailVirtual || estimatedBytes > availableCommit)
            {
                return fail(L"Not enough available memory to create this world.");
            }
        }

        return true;
    }
};

}
