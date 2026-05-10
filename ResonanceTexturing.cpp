#include “ResonanceTexturing.h”
#include “Kismet/GameplayStatics.h”
#include “Misc/Paths.h”
#include “ShaderCore.h”
#include “Interfaces/IPluginManager.h”
#include “TimerManager.h”

// ============================================================
// Project: Resonance Texturing (L-PDS)
// Studio: Tarhy Interactive
// Version: 1.8 “Stable Engine”
// Module: Runtime Implementation
// Developer: Rüzgar Taha Aslan
// Date: 2026
// Description: Procedural detail injection & Thermal-aware LOD
// ============================================================
/*

- Copyright (c) 2026 Rüzgar Taha Aslan
- Project developed under the fictional brand “Tarhy Interactive”
- All rights reserved.
- Developer: Rüzgar Taha Aslan
- License: MIT License
  */

// ============================================================
//  MODULE SETUP (Shader Mapping)
// ============================================================

class FResonanceModule : public IModuleInterface
{
public:
virtual void StartupModule() override
{
TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT(“ResonanceTexturing”));
if (!Plugin.IsValid()) return;

```
    FString ShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
    if (FPaths::DirectoryExists(ShaderDir))
    {
        // .usf files are now accessible via the "/ResonanceShaders/" prefix
        AddShaderSourceDirectoryMapping(TEXT("/ResonanceShaders"), ShaderDir);
    }
}

virtual void ShutdownModule() override {}
```

};

IMPLEMENT_MODULE(FResonanceModule, ResonanceTexturing)

// ============================================================
//  UResonanceTextureComponent - Texture Injection Engine
// ============================================================

UResonanceTextureComponent::UResonanceTextureComponent()
{
PrimaryComponentTick.bCanEverTick = true;
PrimaryComponentTick.TickInterval = 0.016f; // Targeting 60fps
UpdateFrameCounter = 0;
}

void UResonanceTextureComponent::BeginPlay()
{
Super::BeginPlay();

```
// 1. Find mesh and create Dynamic Material Instance (MID)
if (UStaticMeshComponent* Mesh = GetOwner()->FindComponentByClass<UStaticMeshComponent>())
{
    CachedMID = Mesh->CreateAndSetMaterialInstanceDynamic(0);
}

// 2. Load Base Texture (VRAM-friendly, 1K limit)
if (SurfaceData && !SurfaceData->BaseTexture.IsNull())
{
    UTexture2D* LoadedTex = SurfaceData->BaseTexture.LoadSynchronous();
    if (CachedMID && LoadedTex)
    {
        CachedMID->SetTextureParameterValue(TEXT("BaseTexture"), LoadedTex);
    }
}

RegisterToThermalManager();
FlushParametersToMaterial();
```

}

void UResonanceTextureComponent::EndPlay(const EEndPlayReason::Type Reason)
{
Super::EndPlay(Reason);

```
// Unregister to prevent dangling pointer in ThermalManager
if (UGameInstance* GI = GetWorld()->GetGameInstance())
{
    if (UResonanceThermalManager* TM = GI->GetSubsystem<UResonanceThermalManager>())
    {
        TM->UnregisterComponent(this);
    }
}
```

}

void UResonanceTextureComponent::RegisterToThermalManager()
{
if (UGameInstance* GI = GetWorld()->GetGameInstance())
{
if (UResonanceThermalManager* TM = GI->GetSubsystem<UResonanceThermalManager>())
{
TM->RegisterComponent(this);
}
}
}

void UResonanceTextureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Fn)
{
Super::TickComponent(DeltaTime, TickType, Fn);

```
// Update every 3 frames to reduce unnecessary CPU load
if (++UpdateFrameCounter < UPDATE_FREQUENCY) return;
UpdateFrameCounter = 0;

if (!CachedMID || !SurfaceData) return;

TargetLODBlend = CalculateLODBlend();
CurrentLODBlend = FMath::FInterpTo(CurrentLODBlend, TargetLODBlend, DeltaTime * 5.f, 2.0f);

FlushParametersToMaterial();
```

}

float UResonanceTextureComponent::CalculateLODBlend() const
{
if (!SurfaceData) return 1.0f;

```
APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
if (!Cam) return 1.0f;

const float Dist = FVector::Dist(Cam->GetCameraLocation(), GetOwner()->GetActorLocation());

// Blend detail out based on distance (LOD 0 -> LOD 2)
float BaseLOD = FMath::GetMappedRangeValueClamped(
    FVector2D(SurfaceData->LOD0_MaxDistance, SurfaceData->LOD2_MaxDistance),
    FVector2D(0.0f, 1.0f),
    Dist);

// Thermal influence: high temperature reduces detail earlier
float ThermalPush = FMath::GetMappedRangeValueClamped(
    FVector2D(0.5f, 1.0f),
    FVector2D(0.0f, 0.6f),
    LastThermalLevel);

return FMath::Clamp(BaseLOD + ThermalPush, 0.0f, 1.0f);
```

}

float UResonanceTextureComponent::GetComplexityMultiplier() const
{
if (!SurfaceData) return 0.f;
switch (SurfaceData->ComplexityLevel)
{
case ESurfaceComplexity::Minimal:    return 0.f;
case ESurfaceComplexity::Standard:   return 0.5f;
case ESurfaceComplexity::FullDetail: return 1.f;
default:                             return 0.5f;
}
}

void UResonanceTextureComponent::FlushParametersToMaterial()
{
if (!CachedMID || !SurfaceData) return;

```
const FResonanceAgingParams& AP = SurfaceData->AgingParams;

// Send all parameters to HLSL shader in one pass
CachedMID->SetScalarParameterValue(TEXT("LOD_Blend"),         CurrentLODBlend);
CachedMID->SetScalarParameterValue(TEXT("ComplexityMult"),     GetComplexityMultiplier());
CachedMID->SetScalarParameterValue(TEXT("GrainScale"),         SurfaceData->GrainScale);
CachedMID->SetScalarParameterValue(TEXT("Turbulence"),         SurfaceData->Turbulence);
CachedMID->SetScalarParameterValue(TEXT("Thermal_Throttle"),   LastThermalLevel);
CachedMID->SetScalarParameterValue(TEXT("AgingFactor"),        AP.AgingFactor);
CachedMID->SetScalarParameterValue(TEXT("DirtAccumulation"),   AP.DirtAccumulation);
CachedMID->SetScalarParameterValue(TEXT("WearAmount"),         AP.WearAmount);
CachedMID->SetScalarParameterValue(TEXT("MoistureInfluence"),  AP.MoistureInfluence);
```

}

void UResonanceTextureComponent::UpdateAgingParams(const FResonanceAgingParams& NewParams)
{
if (!SurfaceData) return;
SurfaceData->AgingParams = NewParams;
FlushParametersToMaterial();
}

void UResonanceTextureComponent::SetThermalThrottleLevel(float ThrottleLevel)
{
LastThermalLevel = FMath::Clamp(ThrottleLevel, 0.f, 1.f);

```
if (ThrottleLevel > 0.7f) // Fire event if critical threshold (70%) is exceeded
{
    OnThermalThrottle.Broadcast(ThrottleLevel);
}
```

}

// ============================================================
//  UResonanceThermalManager - Hardware Watchdog
// ============================================================

void UResonanceThermalManager::Initialize(FSubsystemCollectionBase& Collection)
{
Super::Initialize(Collection);

```
// Poll hardware state every 2 seconds
GetGameInstance()->GetTimerManager().SetTimer(
    ThermalPollTimer,
    this,
    &UResonanceThermalManager::PollThermalState,
    2.0f, true);
```

}

void UResonanceThermalManager::Deinitialize()
{
GetGameInstance()->GetTimerManager().ClearTimer(ThermalPollTimer);
ActiveComponents.Empty();
Super::Deinitialize();
}

void UResonanceThermalManager::PollThermalState()
{
// ––––––––––––––––––––––––––––––––
// Thermal estimation strategy:
//
// Windows/Desktop (PLATFORM_WINDOWS):
//   UE5 does not expose a direct GPU temperature API on desktop.
//   Instead, we use GGPUFrameTime as a proxy for thermal load.
//   A frame time above ~16ms (60fps) suggests the GPU is under
//   pressure; above ~33ms (~30fps) we treat it as critical.
//   This is intentional — it’s a rendering-pressure heuristic,
//   not a temperature sensor, and it compiles and runs correctly.
//
// Mobile/Console (other platforms):
//   UE5 exposes FPlatformMisc::GetDeviceThermalState() on iOS,
//   Android, and some consoles. On unsupported platforms this
//   returns EDeviceThermalState::Unknown and we fall back to 0.
// ––––––––––––––––––––––––––––––––

#if PLATFORM_WINDOWS
const float GPUTimeMs = static_cast<float>(GGPUFrameTime) * FPlatformTime::GetSecondsPerCycle() * 1000.0f;

```
CurrentThermalLevel = FMath::GetMappedRangeValueClamped(
    FVector2D(16.0f, 33.0f),
    FVector2D(0.0f, 1.0f),
    GPUTimeMs);
```

#elif PLATFORM_IOS || PLATFORM_ANDROID
// Native thermal state available on mobile platforms
const EDeviceThermalState ThermalState = FPlatformMisc::GetDeviceThermalState();
switch (ThermalState)
{
case EDeviceThermalState::Nominal:  CurrentThermalLevel = 0.0f;  break;
case EDeviceThermalState::Fair:     CurrentThermalLevel = 0.25f; break;
case EDeviceThermalState::Serious:  CurrentThermalLevel = 0.6f;  break;
case EDeviceThermalState::Critical: CurrentThermalLevel = 1.0f;  break;
default:                            CurrentThermalLevel = 0.0f;  break;
}

#else
// Unsupported platform — thermal throttling disabled
CurrentThermalLevel = 0.0f;
#endif

```
ApplyGlobalThrottle(CurrentThermalLevel);
```

}

void UResonanceThermalManager::ApplyGlobalThrottle(float Level)
{
ActiveComponents.RemoveAll([](const TWeakObjectPtr<UResonanceTextureComponent>& C) { return !C.IsValid(); });

```
for (auto& WeakComp : ActiveComponents)
{
    if (WeakComp.IsValid())
    {
        WeakComp->SetThermalThrottleLevel(Level);
    }
}
```

}

void UResonanceThermalManager::RegisterComponent(UResonanceTextureComponent* Comp)
{
if (Comp)
{
ActiveComponents.AddUnique(Comp);
}
}

void UResonanceThermalManager::UnregisterComponent(UResonanceTextureComponent* Comp)
{
ActiveComponents.RemoveAll([Comp](const TWeakObjectPtr<UResonanceTextureComponent>& C)
{ return !C.IsValid() || C.Get() == Comp; });
}
