#include "ResonanceTexturing.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "Interfaces/IPluginManager.h"
#include "TimerManager.h"

// ============================================================
// Project: Resonance Texturing (L-PDS)
// Studio: tarhy games
// Version: 1.7 "Stable Engine"
// Module: Runtime Implementation
// Developer: Taha
// Date: 2026
// Description: Procedural detail injection & Thermal-aware LOD
// ============================================================

// ============================================================
//  MODULE SETUP (Shader Mapping)
// ============================================================
/*
 * Copyright (c) 2026 Rüzgar Taha Aslan
 * Project developed under the fictional brand "Tarhy Interactive"
 * All rights reserved.
 * * Developer: Rüzgar Taha Aslan
 * License: MIT License
 */


class FResonanceModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("ResonanceTexturing"));
        if (!Plugin.IsValid()) return;

        FString ShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
        if (FPaths::DirectoryExists(ShaderDir))
        {
            // Artık .usf dosyaları "/ResonanceShaders/" prefix'iyle çağrılabilir
            AddShaderSourceDirectoryMapping(TEXT("/ResonanceShaders"), ShaderDir);
        }
    }

    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FResonanceModule, ResonanceTexturing)

// ============================================================
//  UResonanceTextureComponent - Doku Enjeksiyon Motoru
// ============================================================

UResonanceTextureComponent::UResonanceTextureComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.016f; // 60fps hedefli çalışma
    UpdateFrameCounter = 0;
}

void UResonanceTextureComponent::BeginPlay()
{
    Super::BeginPlay();

    // 1. Mesh bul ve Dinamik Materyal (MID) oluştur
    if (UStaticMeshComponent* Mesh = GetOwner()->FindComponentByClass<UStaticMeshComponent>())
    {
        CachedMID = Mesh->CreateAndSetMaterialInstanceDynamic(0);
    }

    // 2. Base Texture yükle (VRAM dostu 1K limitli)
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
}

void UResonanceTextureComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    Super::EndPlay(Reason);

    // Bellek sızıntısını önlemek için kaydı sil (Dangling Pointer koruması)
    if (UGameInstance* GI = GetWorld()->GetGameInstance())
    {
        if (UResonanceThermalManager* TM = GI->GetSubsystem<UResonanceThermalManager>())
        {
            TM->UnregisterComponent(this);
        }
    }
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

    // Her 3 frame'de bir güncelleme (Gereksiz işlemci yükünü keser)
    if (++UpdateFrameCounter < UPDATE_FREQUENCY) return;
    UpdateFrameCounter = 0;

    if (!CachedMID || !SurfaceData) return;

    TargetLODBlend = CalculateLODBlend();
    CurrentLODBlend = FMath::FInterpTo(CurrentLODBlend, TargetLODBlend, DeltaTime * 5.f, 2.0f);

    FlushParametersToMaterial();
}

float UResonanceTextureComponent::CalculateLODBlend() const
{
    if (!SurfaceData) return 1.0f;

    APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
    if (!Cam) return 1.0f;

    const float Dist = FVector::Dist(Cam->GetCameraLocation(), GetOwner()->GetActorLocation());

    // Mesafeye göre detay kısma (LOD 0 -> LOD 2 geçişi)
    float BaseLOD = FMath::GetMappedRangeValueClamped(
        FVector2D(SurfaceData->LOD0_MaxDistance, SurfaceData->LOD2_MaxDistance),
        FVector2D(0.0f, 1.0f),
        Dist);

    // Termal etki: Isınma artarsa detaylar erkenden kapanır
    float ThermalPush = FMath::GetMappedRangeValueClamped(
        FVector2D(0.5f, 1.0f),
        FVector2D(0.0f, 0.6f),
        LastThermalLevel);

    return FMath::Clamp(BaseLOD + ThermalPush, 0.0f, 1.0f);
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

    const FResonanceAgingParams& AP = SurfaceData->AgingParams;

    // HLSL Shader'a tüm verileri tek seferde gönder
    CachedMID->SetScalarParameterValue(TEXT("LOD_Blend"),         CurrentLODBlend);
    CachedMID->SetScalarParameterValue(TEXT("ComplexityMult"),     GetComplexityMultiplier());
    CachedMID->SetScalarParameterValue(TEXT("GrainScale"),         SurfaceData->GrainScale);
    CachedMID->SetScalarParameterValue(TEXT("Turbulence"),         SurfaceData->Turbulence);
    CachedMID->SetScalarParameterValue(TEXT("Thermal_Throttle"),   LastThermalLevel);
    CachedMID->SetScalarParameterValue(TEXT("AgingFactor"),        AP.AgingFactor);
    CachedMID->SetScalarParameterValue(TEXT("DirtAccumulation"),   AP.DirtAccumulation);
    CachedMID->SetScalarParameterValue(TEXT("WearAmount"),         AP.WearAmount);
    CachedMID->SetScalarParameterValue(TEXT("MoistureInfluence"),  AP.MoistureInfluence);
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

    if (ThrottleLevel > 0.7f) // %70 kritik eşik aşılırsa event fırlat
    {
        OnThermalThrottle.Broadcast(ThrottleLevel);
    }
}

// ============================================================
//  UResonanceThermalManager - Donanım Bekçisi
// ============================================================

void UResonanceThermalManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Her 2 saniyede bir donanım durumunu kontrol et
    GetGameInstance()->GetTimerManager().SetTimer(
        ThermalPollTimer,
        this,
        &UResonanceThermalManager::PollThermalState,
        2.0f, true);
}

void UResonanceThermalManager::Deinitialize()
{
    GetGameInstance()->GetTimerManager().ClearTimer(ThermalPollTimer);
    ActiveComponents.Empty();
    Super::Deinitialize();
}

void UResonanceThermalManager::PollThermalState()
{
#if PLATFORM_WINDOWS
    // GPU Frame süresini ısı tahmini için kullanıyoruz
    const float GPUTimeMs = static_cast<float>(GGPUFrameTime) * FPlatformTime::GetSecondsPerCycle() * 1000.0f;

    CurrentThermalLevel = FMath::GetMappedRangeValueClamped(
        FVector2D(16.0f, 33.0f),
        FVector2D(0.0f, 1.0f),
        GPUTimeMs);
#else
    CurrentThermalLevel = static_cast<float>(FPlatformMisc::GetDeviceThermalState()) / static_cast<float>(EDeviceThermalState::Critical);
#endif

    ApplyGlobalThrottle(CurrentThermalLevel);
}

void UResonanceThermalManager::ApplyGlobalThrottle(float Level)
{
    ActiveComponents.RemoveAll([](const TWeakObjectPtr<UResonanceTextureComponent>& C) { return !C.IsValid(); });

    for (auto& WeakComp : ActiveComponents)
    {
        if (WeakComp.IsValid())
        {
            WeakComp->SetThermalThrottleLevel(Level);
        }
    }
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