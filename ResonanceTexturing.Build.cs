// ============================================================
// Project: Resonance Texturing (L-PDS)
// Studio: tarhy games
// Version: 1.1 "Stable Engine"
// Module: Build Configuration
// Developer: Taha
// ============================================================
/*
 * Copyright (c) 2026 Rüzgar Taha Aslan
 * Project developed under the fictional brand "Tarhy Interactive"
 * All rights reserved.
 * * Developer: Rüzgar Taha Aslan
 * License: MIT License
 */

using UnrealBuildTool;
using System.IO;

public class ResonanceTexturing : ModuleRules
{
    public ResonanceTexturing(ReadOnlyTargetRules Target) : base(Target)
    {
        // IWYU (Include What You Use): Derleme süresini kısaltır ve CPU'yu rahatlatır.
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Projenin ana çekirdek modülleri
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", 
            "CoreUObject", 
            "Engine",
            "InputCore",
            "Projects"
        });

        // Grafik ve Shader motoruyla doğrudan konuşan özel modüller
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "RenderCore",   
            "RHI",          
            "Renderer"      
        });

        // Virtual Shader Path: /ResonanceShaders/ üzerinden HLSL erişimi sağlar.
        string PluginShaderDir = Path.Combine(ModuleDirectory, "../../Shaders");
        
        if (Directory.Exists(PluginShaderDir))
        {
            ShaderIncludePathMappings.Add("/ResonanceShaders", PluginShaderDir);
        }

        // Derleme hızlandırma ve hata yakalama ayarları
        bLegacyPublicIncludePaths = false;
        ShadowVariableWarningLevel = WarningLevel.Error;
    }
}
