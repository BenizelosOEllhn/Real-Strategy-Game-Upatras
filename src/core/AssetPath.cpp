#include "AssetPath.h"
#include <filesystem>
#include <vector>
#include <climits>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace
{
namespace fs = std::filesystem;

bool IsAssetsDir(const fs::path& path)
{
    std::error_code ec;
    return fs::exists(path, ec) && fs::is_directory(path, ec);
}

fs::path FindAssetsUpwards(fs::path start, int maxDepth = 8)
{
    if (start.empty())
        return {};

    std::error_code ec;
    start = fs::weakly_canonical(start, ec);
    if (ec)
        ec.clear();
    if (fs::is_regular_file(start, ec))
        start = start.parent_path();
    if (ec)
        ec.clear();

    fs::path cur = start;
    for (int i = 0; i < maxDepth; ++i)
    {
        fs::path candidate = cur / "assets";
        if (IsAssetsDir(candidate))
            return candidate;

        fs::path parent = cur.parent_path();
        if (parent.empty() || parent == cur)
            break;
        cur = parent;
    }
    return {};
}

std::string ComputeAssetBasePath()
{
    fs::path exeDir;
#ifdef __APPLE__
    char buffer[PATH_MAX];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0)
    {
        exeDir = fs::path(buffer).parent_path();
    }
#elif defined(_WIN32)
    wchar_t wpath[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, wpath, MAX_PATH);
    if (len > 0)
    {
        exeDir = fs::path(wpath).parent_path();
    }
#endif
    if (exeDir.empty())
        exeDir = fs::current_path();

    std::vector<fs::path> searchRoots = {
        exeDir,
        fs::current_path()
    };

    for (const fs::path& root : searchRoots)
    {
        fs::path found = FindAssetsUpwards(root);
        if (!found.empty())
            return found.string() + "/";
    }

    fs::path candidate = exeDir / "assets";
    return candidate.string() + "/";
}
}

const std::string& GetAssetBasePath()
{
    static const std::string base = ComputeAssetBasePath();
    return base;
}
