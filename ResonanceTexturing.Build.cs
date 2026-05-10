// ============================================================
// Project: Resonance Texturing (L-PDS)
// Studio: Tarhy Interactive
// Version: 1.2 "Stable Engine"
// Module: Build Configuration
// Developer: Rüzgar Taha Aslan
// ============================================================
/*
 * Copyright (c) 2026 Rüzgar Taha Aslan
 * Project developed under the fictional brand "Tarhy Interactive"
 * All rights reserved.
 * Developer: Rüzgar Taha Aslan
 * License: MIT License
 */

using UnrealBuildTool;
using System.IO;

public class ResonanceTexturing : ModuleRules
{
    public ResonanceTexturing(ReadOnlyTargetRules Target) : base(Target)
    {
        // IWYU: Reduces compile time and CPU pressure
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Core engine modules
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Projects"
        });

        // Rendering and shader modules
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "RenderCore",
            "RHI",
            "Renderer"
        });

        // Virtual Shader Path: enables #include "/ResonanceShaders/..." in .usf files
        string PluginShaderDir = Path.Combine(ModuleDirectory, "../../Shaders");

        if (Directory.Exists(PluginShaderDir))
        {
            ShaderIncludePathMappings.Add("/ResonanceShaders", PluginShaderDir);
        }

        // Build quality settings
        bLegacyPublicIncludePaths = false;
        ShadowVariableWarningLevel = WarningLevel.Error;
    }
}
