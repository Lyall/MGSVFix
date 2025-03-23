#include "stdafx.h"
#include "helper.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <inipp/inipp.h>
#include <safetyhook.hpp>

#define spdlog_confparse(var) spdlog::info("Config Parse: {}: {}", #var, var)

HMODULE exeModule = GetModuleHandle(NULL);
HMODULE thisModule;

// Fix details
std::string sFixName = "MGSVFix";
std::string sFixVersion = "0.0.1";
std::filesystem::path sFixPath;

// Ini
inipp::Ini<char> ini;
std::string sConfigFile = sFixName + ".ini";

// Logger
std::shared_ptr<spdlog::logger> logger;
std::string sLogFile = sFixName + ".log";
std::filesystem::path sExePath;
std::string sExeName;

// Aspect ratio / FOV / HUD
std::pair DesktopDimensions = { 0,0 };
const float fPi = 3.1415926535f;
const float fNativeAspect = 16.00f / 9.00f;
float fAspectRatio;
float fAspectMultiplier;
float fHUDWidth;
float fHUDWidthOffset;
float fHUDHeight;
float fHUDHeightOffset;

// Ini variables
bool bUnlockFPS;
bool bFixResolution;
bool bFixHUD;
bool bLODTweaks;
int iTerrainDistance;
float fModelDistance;
float fGrassDistance;
float fGameplayFOVMulti;

// Variables
int iCurrentResX;
int iCurrentResY;
bool bIsMoviePlaying;

// Game info
struct GameInfo
{
    std::string GameTitle;
    std::string ExeName;
};

enum class Game 
{
    Unknown,
    TPP,      // MGS V: The Phantom Pain
    GZ,       // MGS V: Ground Zeroes
};

const std::map<Game, GameInfo> kGames = {
    {Game::GZ, {"METAL GEAR SOLID V: GROUND ZEROES", "MgsGroundZeroes.exe"}},
    {Game::TPP, {"METAL GEAR SOLID V: THE PHANTOM PAIN", "mgsvtpp.exe"}},
};

const GameInfo* game = nullptr;
Game eGameType = Game::Unknown;

void CalculateAspectRatio(bool bLog)
{
    if (iCurrentResX <= 0 || iCurrentResY <= 0)
        return;

    // Calculate aspect ratio
    fAspectRatio = (float)iCurrentResX / (float)iCurrentResY;
    fAspectMultiplier = fAspectRatio / fNativeAspect;

    // HUD 
    fHUDWidth = (float)iCurrentResY * fNativeAspect;
    fHUDHeight = (float)iCurrentResY;
    fHUDWidthOffset = (float)(iCurrentResX - fHUDWidth) / 2.00f;
    fHUDHeightOffset = 0.00f;
    if (fAspectRatio < fNativeAspect) {
        fHUDWidth = (float)iCurrentResX;
        fHUDHeight = (float)iCurrentResX / fNativeAspect;
        fHUDWidthOffset = 0.00f;
        fHUDHeightOffset = (float)(iCurrentResY - fHUDHeight) / 2.00f;
    }

    // Log details about current resolution
    if (bLog) {
        spdlog::info("----------");
        spdlog::info("Current Resolution: Resolution: {:d}x{:d}", iCurrentResX, iCurrentResY);
        spdlog::info("Current Resolution: fAspectRatio: {}", fAspectRatio);
        spdlog::info("Current Resolution: fAspectMultiplier: {}", fAspectMultiplier);
        spdlog::info("Current Resolution: fHUDWidth: {}", fHUDWidth);
        spdlog::info("Current Resolution: fHUDHeight: {}", fHUDHeight);
        spdlog::info("Current Resolution: fHUDWidthOffset: {}", fHUDWidthOffset);
        spdlog::info("Current Resolution: fHUDHeightOffset: {}", fHUDHeightOffset);
        spdlog::info("----------");
    }
}

void Logging()
{
    // Get path to DLL
    WCHAR dllPath[_MAX_PATH] = {0};
    GetModuleFileNameW(thisModule, dllPath, MAX_PATH);
    sFixPath = dllPath;
    sFixPath = sFixPath.remove_filename();

    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = {0};
    GetModuleFileNameW(exeModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    sExePath = sExePath.remove_filename();

    // Spdlog initialisation
    try
    {
        logger = spdlog::basic_logger_st(sFixName, sExePath.string() + sLogFile, true);
        spdlog::set_default_logger(logger);
        spdlog::flush_on(spdlog::level::debug);

        spdlog::info("----------");
        spdlog::info("{:s} v{:s} loaded.", sFixName, sFixVersion);
        spdlog::info("----------");
        spdlog::info("Log file: {}", sFixPath.string() + sLogFile);
        spdlog::info("----------");
        spdlog::info("Module Name: {:s}", sExeName);
        spdlog::info("Module Path: {:s}", sExePath.string());
        spdlog::info("Module Address: 0x{:x}", (uintptr_t)exeModule);
        spdlog::info("Module Timestamp: {:d}", Memory::ModuleTimestamp(exeModule));
        spdlog::info("----------");
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        AllocConsole();
        FILE *dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "Log initialisation failed: " << ex.what() << std::endl;
        FreeLibraryAndExitThread(thisModule, 1);
    }
}

void Configuration()
{
    // Inipp initialisation
    std::ifstream iniFile(sFixPath / sConfigFile);
    if (!iniFile)
    {
        AllocConsole();
        FILE *dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "" << sFixName.c_str() << " v" << sFixVersion.c_str() << " loaded." << std::endl;
        std::cout << "ERROR: Could not locate config file." << std::endl;
        std::cout << "ERROR: Make sure " << sConfigFile.c_str() << " is located in " << sFixPath.string().c_str() << std::endl;
        spdlog::error("ERROR: Could not locate config file {}", sConfigFile);
        spdlog::shutdown();
        FreeLibraryAndExitThread(thisModule, 1);
    }
    else
    {
        spdlog::info("Config file: {}", sFixPath.string() + sConfigFile);
        ini.parse(iniFile);
    }

    // Parse config
    ini.strip_trailing_comments();
    spdlog::info("----------");

    // Load settings from ini
    inipp::get_value(ini.sections["Unlock Framerate"], "Enabled", bUnlockFPS);
    inipp::get_value(ini.sections["Fix Resolution"], "Enabled", bFixResolution);
    inipp::get_value(ini.sections["Fix HUD"], "Enabled", bFixHUD);
    inipp::get_value(ini.sections["LOD Tweaks"], "Enabled", bLODTweaks);
    inipp::get_value(ini.sections["LOD Tweaks"], "TerrainDistance", iTerrainDistance);
    inipp::get_value(ini.sections["LOD Tweaks"], "ModelDistance", fModelDistance);
    inipp::get_value(ini.sections["LOD Tweaks"], "GrassDistance", fGrassDistance);
    //inipp::get_value(ini.sections["Gameplay FOV"], "Multiplier", fGameplayFOVMulti);

    // Clamp settings
    //fGameplayFOVMulti = std::clamp(fGameplayFOVMulti, 0.10f, 3.00f);

    // Log ini parse
    spdlog_confparse(bUnlockFPS);
    spdlog_confparse(bFixResolution);
    spdlog_confparse(bFixHUD);
    spdlog_confparse(bLODTweaks);
    spdlog_confparse(iTerrainDistance);
    spdlog_confparse(fModelDistance);
    spdlog_confparse(fGrassDistance);
    //spdlog_confparse(fGameplayFOVMulti);

    spdlog::info("----------");
}

bool DetectGame()
{
    eGameType = Game::Unknown;

    for (const auto& [type, info] : kGames) {
        if (Util::string_cmp_caseless(info.ExeName, sExeName)) {
            spdlog::info("Detect Game: {} ({})", info.GameTitle, sExeName);
            eGameType = type;
            game = &info;
            return true;
        }
    }

    spdlog::error("Detect Game: Failed to detect supported game, {} isn't supported by MGSVFix.", sExeName);
    return false;
}

void CurrentResolution()
{
    if (eGameType == Game::GZ || eGameType == Game::TPP) {
        // GZ/TPP: Current resolution
        std::uint8_t* CurrentResolutionScanResult = Memory::PatternScan(exeModule, "48 89 ?? ?? 48 8B ?? ?? 48 ?? ?? ?? ?? ?? ?? ?? ?? B8 01 00 00 00 48 ?? ?? ??");
        if (CurrentResolutionScanResult) {
            spdlog::info("GZ/TPP: Current Resolution: Address is {:s}+{:x}", sExeName.c_str(), CurrentResolutionScanResult - (std::uint8_t*)exeModule);             
            static SafetyHookMid CurrentResolutionMidHook{};
            CurrentResolutionMidHook = safetyhook::create_mid(CurrentResolutionScanResult,
                [](SafetyHookContext& ctx) {
                    // Get current resolution
                    int iResX = static_cast<int>(ctx.rax & 0xFFFFFFFF);
                    int iResY = static_cast<int>((ctx.rax >> 32) & 0xFFFFFFFF);

                    // Log resolution
                    if (iResX != iCurrentResX || iResY != iCurrentResY) {
                        iCurrentResX = iResX;
                        iCurrentResY = iResY;
                        CalculateAspectRatio(true);
                    }
                });
        }
        else {
            spdlog::error("GZ/TPP: Current Resolution: Pattern scan failed.");
        }
    } 
}

void Resolution()
{
    if (bFixResolution) 
    {
        if (eGameType == Game::GZ || eGameType == Game::TPP) {
            // GZ/TPP: Unlock windowed/borderless resolutions
            std::uint8_t* WindowedResolutionsScanResult = Memory::PatternScan(exeModule, "72 ?? 0F ?? ?? 73 ?? 80 ?? ?? 00 74 ?? 0F ?? ?? 73 ?? F3 0F ?? ??");
            if (WindowedResolutionsScanResult) {
                spdlog::info("GZ/TPP: Unlock Resolutions: Windowed: Address is {:s}+{:x}", sExeName.c_str(), WindowedResolutionsScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(WindowedResolutionsScanResult, "\xEB\x24", 2); // jmp over resolution restrictions
                spdlog::info("GZ/TPP: Unlock Resolutions: Windowed: Patched instruction.");
            }
            else {
                spdlog::error("GZ/TPP: Unlock Resolutions: Pattern scan failed.");
            }
        }

        if (eGameType == Game::GZ) {
            // GZ: Remove HWND_TOPMOST flag for borderless mode
            std::uint8_t* BorderlessTopMostScanResult = Memory::PatternScan(exeModule, "48 ?? ?? FF 85 ?? 8B ?? ?? 74 ?? 44 ?? ?? ?? ?? 44 ?? ?? ?? ??");
            if (BorderlessTopMostScanResult) {
                spdlog::info("GZ: Borderless TopMost: Address is {:s}+{:x}", sExeName.c_str(), BorderlessTopMostScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(BorderlessTopMostScanResult + 0x2, "\xF2", 1); // or -> xor the HWND_TOPMOST flag
                spdlog::info("GZ: Borderless TopMost: Patched instruction.");
            }
            else {
                spdlog::error("GZ: Borderless TopMost: Pattern scan failed.");
            }

            // GZ: Unlock fullscreen resolutions
            std::uint8_t* FullscreenResolutionsScanResult = Memory::PatternScan(exeModule, "F3 0F ?? ?? F3 48 ?? ?? ?? 8B ?? 41 ?? ?? ?? ?? ?? ?? 0F ?? ?? 44 ?? ?? 41 ?? ?? ?? 41 ?? ?? 33 ??");
            if (FullscreenResolutionsScanResult) {
                spdlog::info("GZ: Unlock Resolutions: Fullscreen/Borderless: Address is {:s}+{:x}", sExeName.c_str(), FullscreenResolutionsScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(FullscreenResolutionsScanResult + 0x3, "\xD1", 1); // divss xmm2, xmm0 -> divss xmm2, xmm1 to divide by the actual aspect ratio
                spdlog::info("GZ: Unlock Resolutions: Fullscreen/Borderless: Patched instruction.");
            }
            else {
                spdlog::error("GZ: Unlock Resolutions: Pattern scan failed.");
            }
        }
        else if (eGameType == Game::TPP)
        {
            // TPP: Unlock fullscreen resolutions
            std::uint8_t* FullscreenResolutionsScanResult = Memory::PatternScan(exeModule, "F3 0F ?? ?? F3 48 ?? ?? ?? B8 ?? ?? ?? ?? 89 ?? 39 ?? 0F ?? ?? 89 ?? ?? ?? 39 ??");
            if (FullscreenResolutionsScanResult) { 
                spdlog::info("TPP: Unlock Resolutions: Fullscreen/Borderless: Address is {:s}+{:x}", sExeName.c_str(), FullscreenResolutionsScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(FullscreenResolutionsScanResult + 0x3, "\xD3", 1); // mulss xmm2, xmm0 -> mulss xmm2, xmm3 to multiply by the actual aspect ratio
                spdlog::info("TPP: Unlock Resolutions: Fullscreen/Borderless: Patched instruction.");
            }
            else {
                spdlog::error("TPP: Unlock Resolutions: Pattern scan failed.");
            }
        }
    } 
}

void HUD()
{
    if (bFixHUD) 
    {
        if (eGameType == Game::GZ || eGameType == Game::TPP) {
            // GZ/TPP: Throwable marker
            std::uint8_t* ThrowableMarkerScanResult = Memory::PatternScan(exeModule, "E8 ?? ?? ?? ?? F3 0F ?? ?? ?? ?? 66 0F ?? ?? 66 0F ?? ?? 41 ?? ?? ?? 4C ?? ?? ?? ?? BA 01 00 00 00");
            if (ThrowableMarkerScanResult) {
                spdlog::info("GZ/TPP: HUD: Throwable Marker: Address is {:s}+{:x}", sExeName.c_str(), ThrowableMarkerScanResult - (std::uint8_t*)exeModule);
                static SafetyHookMid ThrowableMarkerMidHook{};
                ThrowableMarkerMidHook = safetyhook::create_mid(ThrowableMarkerScanResult + 0x5,
                    [](SafetyHookContext& ctx) {
                        if (fAspectRatio > fNativeAspect)
                            ctx.xmm7.f32[0] = fAspectMultiplier;
                    });
            }
            else {
                spdlog::error("GZ/TPP: HUD: Throwable Marker: Pattern scan failed.");
            }

            // GZ/TPP: Fix lens effects (flares, dirt etc)
            std::uint8_t* LensEffectsScanResult = Memory::PatternScan(exeModule, "0F 28 ?? F3 ?? 0F ?? ?? ?? ?? ?? ?? F3 45 ?? ?? ?? ?? F3 45 ?? ?? ?? F3 44 ?? ?? ?? ?? E8 ?? ?? ?? ??");
            if (LensEffectsScanResult) {
                spdlog::info("GZ/TPP: HUD: Lens Effects: Address is {:s}+{:x}", sExeName.c_str(), LensEffectsScanResult - (std::uint8_t*)exeModule);
                static SafetyHookMid LensEffectsMidHook{};
                LensEffectsMidHook = safetyhook::create_mid(LensEffectsScanResult + 0x3,
                    [](SafetyHookContext& ctx) {
                        if (fAspectRatio > fNativeAspect) {
                            ctx.xmm13.f32[0] = fNativeAspect;
                            ctx.xmm9.f32[0] /= fAspectMultiplier;
                        }
                    });
            }
            else {
                spdlog::error("GZ/TPP: HUD: Lens Effects: Pattern scan failed.");
            }

            // GZ/TPP: Span backgrounds
            std::uint8_t* HUDBackgroundsScanResult = nullptr;
            if (eGameType == Game::GZ)
                HUDBackgroundsScanResult = Memory::PatternScan(exeModule, "41 0F ?? ?? 8B ?? ?? F6 ?? ?? 0F 84 ?? ?? ?? ?? 44 ?? ?? 41 ?? ?? ?? 41 ?? ?? ?? 74 ??");
            else if (eGameType == Game::TPP)
                HUDBackgroundsScanResult = Memory::PatternScan(exeModule, "F6 41 ?? 01 74 ?? 0F ?? ?? ?? 0F ?? ?? ?? 44 0F ?? ?? ?? 41 ?? ?? ??");
                
            if (HUDBackgroundsScanResult) {
                spdlog::info("GZ/TPP: HUD: Backgrounds: Address is {:s}+{:x}", sExeName.c_str(), HUDBackgroundsScanResult - (std::uint8_t*)exeModule);
                static SafetyHookMid HUDBackgroundsMidHook{};
                HUDBackgroundsMidHook = safetyhook::create_mid(HUDBackgroundsScanResult,
                    [](SafetyHookContext& ctx) {
                        if (!ctx.rcx)
                            return;

                        float Width = *reinterpret_cast<float*>(ctx.rcx + 0x30);
                        float Height = *reinterpret_cast<float*>(ctx.rcx + 0x34);

                        // Different offsets for TPP
                        if (eGameType == Game::TPP) {
                            Width = *reinterpret_cast<float*>(ctx.rcx + 0x40);
                            Height = *reinterpret_cast<float*>(ctx.rcx + 0x44);
                        }

                        // Resize HUD to counteract viewport scaling when a movie plays
                        if (bIsMoviePlaying) {
                            if (fAspectRatio > fNativeAspect && Width > 1.00f) {
                                ctx.xmm0.f32[0] *= fAspectMultiplier;
                            }
                        }

                        if (fAspectRatio > fNativeAspect) {
                            // TPP/GZ: ui_sys_cmn_bg
                            if (Width == 2048.00f && Height == 1152.00f)
                                ctx.xmm0.f32[0] *= fAspectMultiplier;

                            // TPP/GZ: Cutscene skip BG
                            if (Width == 2000.00f && Height == 1125.00f)
                                ctx.xmm0.f32[0] *= fAspectMultiplier;

                            // TPP: Loadout BG
                            if (Width == 1400.00f && Height == 1400.00f)
                                ctx.xmm0.f32[0] *= fAspectMultiplier;

                            // TPP/GZ: Mission failed BGs
                            if (Width == 1500.00f && Height == 1500.00f)
                                ctx.xmm0.f32[0] *= fAspectMultiplier;
                            if (Width == 2000.00f && Height == 2000.00f)
                                ctx.xmm0.f32[0] *= fAspectMultiplier;
                            if ((Width > 1882.00f && Width < 1884.00f) && (Height > 1059.00f && Height < 1061.00f))
                                ctx.xmm0.f32[0] *= fAspectMultiplier;
                            if ((Width > 1770.00f && Width < 1772.00f) && (Height > 995.00f && Height < 997.00f))
                                ctx.xmm0.f32[0] *= fAspectMultiplier;

                            // TPP/GZ: Scope fade
                            if (Width == 1400.00f && Height == 1280.00f)
                                ctx.xmm0.f32[0] *= fAspectMultiplier;

                            // GZ: Scope frame
                            if (Width == 600.00f && (Height > 1230.00f && Height < 1231.00f))
                                ctx.xmm0.f32[0] *= fAspectMultiplier;

                            // TPP: Scope frame
                            if (Width == 1500.00f && Height == 1000.00f)
                                *reinterpret_cast<float*>(ctx.rcx + 0x740) = fAspectRatio / 2.00f; // Set the overall width scale
                        }
                        else {
                            // TPP: Scope frame
                            if (Width == 1500.00f && Height == 1000.00f)
                                *reinterpret_cast<float*>(ctx.rcx + 0x740) = 1.00f; // Reset in-case the resolution has changed.
                        }
                    });
            }
            else {
                spdlog::error("GZ/TPP: HUD: Backgrounds: Pattern scan failed.");
            }
        }

        if (eGameType == Game::TPP) {
            // TPP: Fix incorrectly positioned markers
            std::uint8_t* MarkersScanResult = Memory::PatternScan(exeModule, "48 81 ?? ?? ?? ?? ?? E9 ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ??");
            if (MarkersScanResult) {
                spdlog::info("TPP: HUD: Markers: Address is {:s}+{:x}", sExeName.c_str(), MarkersScanResult - (std::uint8_t*)exeModule);
                static SafetyHookMid MarkersMidHook{};
                MarkersMidHook = safetyhook::create_mid(MarkersScanResult,
                    [](SafetyHookContext& ctx) {
                        if (fAspectRatio > fNativeAspect) {
                            *reinterpret_cast<float*>(ctx.rdx + 0x120) = 64.00f * fAspectMultiplier;
                            ctx.xmm1.f32[0] = 64.00f;
                        }
                        else {
                            *reinterpret_cast<float*>(ctx.rdx + 0x120) = 64.00f;
                            ctx.xmm1.f32[0] = 64.00f;
                        }
                    });
            }
            else {
                spdlog::error("TPP: HUD: Markers: Pattern scan failed.");
            }

            // TPP: Fix various overlays
            std::vector<std::uint8_t*> OverlayScanResult = Memory::PatternScanAll(exeModule, "F3 0F ?? ?? ?? ?? ?? ?? C7 44 ?? ?? 00 00 80 BF C7 44 ?? ?? 00 00 80 3F");
            if (!OverlayScanResult.empty() && OverlayScanResult.size() == 3) {
                spdlog::info("TPP: HUD: Overlays: 1: Address is {:s}+{:x}", sExeName.c_str(), OverlayScanResult[0] - (std::uint8_t*)exeModule);
                static SafetyHookMid Overlay1MidHook{};
                Overlay1MidHook = safetyhook::create_mid(OverlayScanResult[0],
                    [](SafetyHookContext& ctx) {
                        if (fAspectRatio > fNativeAspect)
                            ctx.xmm5.f32[0] *= fAspectMultiplier;
                    });

                spdlog::info("TPP: HUD: Overlays: 2: Address is {:s}+{:x}", sExeName.c_str(), OverlayScanResult[1] - (std::uint8_t*)exeModule);
                static SafetyHookMid Overlay2MidHook{};
                Overlay2MidHook = safetyhook::create_mid(OverlayScanResult[1],
                    [](SafetyHookContext& ctx) {
                        if (fAspectRatio > fNativeAspect)
                            ctx.xmm5.f32[0] *= fAspectMultiplier;
                    });

                spdlog::info("TPP: HUD: Overlays: 3: Address is {:s}+{:x}", sExeName.c_str(), OverlayScanResult[2] - (std::uint8_t*)exeModule);
                static SafetyHookMid Overlay3MidHook{};
                Overlay3MidHook = safetyhook::create_mid(OverlayScanResult[2],
                    [](SafetyHookContext& ctx) {
                        if (fAspectRatio > fNativeAspect)
                            ctx.xmm5.f32[0] *= fAspectMultiplier;
                    });
            }
            else {
                spdlog::error("TPP: HUD: Overlays: Pattern scan failed.");
            }

            // TPP: Fix sonar markers
            std::uint8_t* ViewportScanResult = Memory::PatternScan(exeModule, "F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? 48 83 ?? ??");
            if (ViewportScanResult) {
                spdlog::info("TPP: HUD: Sonar Markers: Address is {:s}+{:x}", sExeName.c_str(), ViewportScanResult - (std::uint8_t*)exeModule);
                static SafetyHookMid ViewportMidHook{};
                ViewportMidHook = safetyhook::create_mid(ViewportScanResult,
                    [](SafetyHookContext& ctx) {
                        if (fAspectRatio > fNativeAspect)
                            ctx.xmm0.f32[0] *= fAspectMultiplier;
                    });
            }
            else {
                spdlog::error("TPP: HUD: Sonar Markers: Pattern scan failed.");
            }
        }
    }
}

void Movies()
{
    if (bFixHUD) 
    {
        if (eGameType == Game::TPP) {
            // TPP: Adjust movie frame
            std::uint8_t* MovieFrameScanResult = Memory::PatternScan(exeModule, "72 ?? 44 0F ?? ?? 72 ?? 41 0F ?? ?? F3 41 ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 0F ?? ?? 76 ??");
            if (MovieFrameScanResult) {
                spdlog::info("TPP: HUD: Movie Frame: Address is {:s}+{:x}", sExeName.c_str(), MovieFrameScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(MovieFrameScanResult, "\xEB", 1);
            }
            else {
                spdlog::error("TPP: HUD: Movie Frame: Pattern scan failed.");
            }

            // TPP: Movie status
            std::uint8_t* MovieStatusScanResult = Memory::PatternScan(exeModule, "8B ?? ?? ?? ?? ?? FF ?? 0F 84 ?? ?? ?? ?? FF ?? 0F 84 ?? ?? ?? ?? FF ?? 74 ?? 48 8D ?? ?? ?? ?? ?? 33 ??");
            if (MovieStatusScanResult) {
                spdlog::info("TPP: HUD: Movie Status: Address is {:s}+{:x}", sExeName.c_str(), MovieStatusScanResult - (std::uint8_t*)exeModule);
                static SafetyHookMid MovieStatusMidHook{};
                MovieStatusMidHook = safetyhook::create_mid(MovieStatusScanResult,
                    [](SafetyHookContext& ctx) {
                        // Playing/paused
                        if (ctx.rax == 1 || ctx.rax == 2)
                            bIsMoviePlaying = true;
                        else
                            bIsMoviePlaying = false;
                    });
            }
            else {
                spdlog::error("TPP: HUD: Movie Status: Pattern scan failed.");
            }

            // TPP: Viewport
            std::uint8_t* ViewportScanResult = Memory::PatternScan(exeModule, "F3 0F ?? ?? F3 0F ?? ?? 0F ?? ?? 73 ?? 41 0F ?? ?? 41 ?? ?? 44 ?? ?? F3 0F ?? ?? F3 0F ?? ??");
            if (ViewportScanResult) {
                spdlog::info("TPP: HUD: Viewport: Address is {:s}+{:x}", sExeName.c_str(), ViewportScanResult - (std::uint8_t*)exeModule);
                static SafetyHookMid ViewportMidHook{};
                ViewportMidHook = safetyhook::create_mid(ViewportScanResult,
                    [](SafetyHookContext& ctx) {
                        if (bIsMoviePlaying) {
                            if (fAspectRatio > fNativeAspect)
                                ctx.xmm1.f32[0] = fHUDWidth;
                        }
                    });
            }
            else {
                spdlog::error("TPP: HUD: Viewport: Pattern scan failed.");
            }
        }
    }  
}

void Framerate()
{
    if (bUnlockFPS)
    {
        if (eGameType == Game::GZ || eGameType == Game::TPP) {
            // GZ/TPP: Force "variable" framerate setting
            std::uint8_t* FramerateSettingScanResult = Memory::PatternScan(exeModule, "48 33 ?? ?? ?? ?? ?? 49 85 ?? 48 0F ?? ?? ?? ?? ?? ?? 48 89 ?? ?? ?? ??");
            std::uint8_t* FramerateTargetScanResult = Memory::PatternScan(exeModule, "49 85 ?? 75 ?? F2 0F 10 0D ?? ?? ?? ??");
            if (FramerateSettingScanResult && FramerateTargetScanResult) { 
                spdlog::info("GZ/TPP: Variable Framerate: Setting: Address is {:s}+{:x}", sExeName.c_str(), FramerateSettingScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(FramerateSettingScanResult, "\x48\x31\xC0\x90\x90\x90\x90", 7); // xor rax, rax
                spdlog::info("GZ/TPP: Variable Framerate: Setting: Patched instruction.");

                spdlog::info("GZ/TPP: Variable Framerate: Target: Address is {:s}+{:x}", sExeName.c_str(), FramerateTargetScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(FramerateTargetScanResult + 0x3, "\xEB", 1); // jmp
                spdlog::info("GZ/TPP: Variable Framerate: Target: Patched instruction.");
            }
            else {
                spdlog::error("GZ/TPP: Variable Framerate: Pattern scan(s) failed.");
            }
        }

        if (eGameType == Game::GZ) {
            // GZ: Fix freezing bug with throwables when using variable framerate
            std::uint8_t* ThrowableBugScanResult = Memory::PatternScan(exeModule, "F2 0F 59 ?? ?? ?? ?? ?? 66 0F ?? ?? F7 ?? ?? ?? ?? ?? 00 01 00 00 74 ??");
            if (ThrowableBugScanResult) { 
                spdlog::info("GZ: Throwable Framerate Bug: Address is {:s}+{:x}", sExeName.c_str(), ThrowableBugScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(ThrowableBugScanResult, "\xF2\x0F\x59\x40\x30\x90\x90\x90", 8); // mulsd xmm0,[7FF677CA9C00] (fixed 60fps frametime) -> mulsd xmm0, [rax+30] (current frametime)
                spdlog::info("GZ: Throwable Framerate Bug: Patched instruction.");
            }
            else {
                spdlog::error("GZ: Throwable Framerate Bug: Pattern scan failed.");
            }
        }
    }
}

void Graphics()
{
    if (bLODTweaks)
    {
        if (eGameType == Game::TPP) {
            // TPP: LOD factor resolution
            std::uint8_t* LODFactorResolutionScanResult = Memory::PatternScan(exeModule, "8B ?? ?? ?? ?? ?? 4C 8B ?? ?? ?? ?? ?? 85 ?? 75 ?? 8B ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ??");
            if (LODFactorResolutionScanResult) { 
                spdlog::info("TPP: Graphics: LOD: LOD Factor Resolution: Address is {:s}+{:x}", sExeName.c_str(), LODFactorResolutionScanResult - (std::uint8_t*)exeModule);
                std::uint8_t* LODFactorResolution = Memory::GetAbsolute(LODFactorResolutionScanResult + 0x2);
                Memory::Write(LODFactorResolution, iTerrainDistance);
            }
            else {
                spdlog::error("TPP: Graphics: LOD: LOD Factor Resolution: Pattern scan failed.");
            }
        }
        else if (eGameType == Game::GZ) {
            // GZ: LOD factor resolution
            std::uint8_t* LODFactorResolutionScanResult = Memory::PatternScan(exeModule, "66 0F ?? ?? ?? ?? ?? ?? 0F 29 ?? ?? 0F 28 ?? F3 0F ?? ?? ?? ?? ?? ?? 0F 5B ??");
            if (LODFactorResolutionScanResult) { 
                spdlog::info("GZ: Graphics: LOD: LOD Factor Resolution: Address is {:s}+{:x}", sExeName.c_str(), LODFactorResolutionScanResult - (std::uint8_t*)exeModule);
                static SafetyHookMid LODFactorResolutionMidHook{};
                LODFactorResolutionMidHook = safetyhook::create_mid(LODFactorResolutionScanResult + 0x8,
                    [](SafetyHookContext& ctx) {
                        ctx.xmm3.u16[0] = iTerrainDistance;
                    });
            }
            else {
                spdlog::error("GZ: Graphics: LOD: LOD Factor Resolution: Pattern scan failed.");
            }
        }

        if (eGameType == Game::GZ || eGameType == Game::TPP) {
            // GZ/TPP: Model quality
            std::uint8_t* ModelQualityScanResult = Memory::PatternScan(exeModule, "89 ?? 64 B0 01 C3 8B ?? ?? C6 ?? ?? 00 89 ?? ?? B0 01 C3");
            if (ModelQualityScanResult) { 
                spdlog::info("GZ/TPP: Graphics: LOD: Model/Grass LOD Distance: Address is {:s}+{:x}", sExeName.c_str(), ModelQualityScanResult - (std::uint8_t*)exeModule);
                static SafetyHookMid ModelQualityMidHook{};
                ModelQualityMidHook = safetyhook::create_mid(ModelQualityScanResult,
                    [](SafetyHookContext& ctx) {
                        if (ctx.rbx == 9)
                            ctx.rax = *(uint32_t*)&fGrassDistance;
                        else
                            ctx.rax = *(uint32_t*)&fModelDistance;
                    });
            }
            else {
                spdlog::error("GZ/TPP: Graphics: LOD: Model/Grass LOD Distance: Pattern scan failed.");
            }
        }
    }   
}

void FOV()
{
    if (fGameplayFOVMulti != 1.00f)
    {
        if (eGameType == Game::GZ) {
            // GZ: Gameplay FOV
            std::uint8_t* GameplayFOVScanResult = Memory::PatternScan(exeModule, "89 ?? ?? ?? ?? ?? F3 0F ?? ?? ?? F3 0F ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 0F 28 ?? 0F 28 ??");
            if (GameplayFOVScanResult) { 
                spdlog::info("GZ: Gameplay FOV: Address is {:s}+{:x}", sExeName.c_str(), GameplayFOVScanResult - (std::uint8_t*)exeModule);
                static SafetyHookMid GameplayFOVMidHook{};
                GameplayFOVMidHook = safetyhook::create_mid(GameplayFOVScanResult,
                    [](SafetyHookContext& ctx) {   
                        float fov = *reinterpret_cast<float*>(&ctx.rax);
                        
                        // Don't change scope FOV
                        if (fov != 36.00f)
                            fov /= fGameplayFOVMulti;
    
                        ctx.rax = *(uint32_t*)&fov;
                    });
            }
            else {
                spdlog::error("GZ: Gameplay FOV: Pattern scan failed.");
            }
        }
        else if (eGameType == Game::TPP) {
            // TPP: Gameplay FOV
            std::uint8_t* GameplayFOVScanResult = Memory::PatternScan(exeModule, "F3 0F ?? ?? ?? ?? ?? ?? 44 0F ?? ?? ?? ?? ?? ?? ?? 44 0F ?? ?? ?? ?? ?? ?? ?? 44 0F ?? ?? ?? ?? ?? ?? ?? 41 0F ?? ??");
            if (GameplayFOVScanResult) { 
                spdlog::info("TPP: Gameplay FOV: Address is {:s}+{:x}", sExeName.c_str(), GameplayFOVScanResult - (std::uint8_t*)exeModule);
                static SafetyHookMid GameplayFOVMidHook{};
                GameplayFOVMidHook = safetyhook::create_mid(GameplayFOVScanResult + 0x8,
                    [](SafetyHookContext& ctx) {
                        float fov = ctx.xmm1.f32[0];
                        
                        // Don't change scope/ADS FOV
                        if (fov != 36.00f && fov != 29.00f)
                            fov /= fGameplayFOVMulti;
    
                        ctx.xmm1.f32[0] = fov;
                    });
            }
            else {
                spdlog::error("TPP: Gameplay FOV: Pattern scan failed.");
            }
        }
    }   
}

DWORD __stdcall Main(void*)
{
    Logging();
    Configuration();
    if (DetectGame())
    {
        CurrentResolution();
        Resolution();
        HUD();
        Movies();
        Framerate();
        Graphics();
        //FOV();
    }
    return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        thisModule = hModule;

        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);
        if (mainHandle)
        {
            SetThreadPriority(mainHandle, THREAD_PRIORITY_HIGHEST);
            CloseHandle(mainHandle);
        }
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
