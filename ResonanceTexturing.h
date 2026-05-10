#pragma once

// =============================================================
// L-PDS (Liny-On Procedural Detail Synthesis) Core Header
// =============================================================
/*

- Copyright (c) 2026 Rüzgar Taha Aslan
- Project developed under the fictional brand “Tarhy Interactive”
- All rights reserved.
- Developer: Rüzgar Taha Aslan
- License: MIT License
  */

#include “CoreMinimal.h”
#include “Engine/DataAsset.h”
#include “Components/ActorComponent.h”
#include “Subsystems/GameInstanceSubsystem.h”
#include “Materials/MaterialInstanceDynamic.h”
#include “ResonanceTexturing.generated.h”

// ─────────────────────────────────────────────
// L-PDS RUNTIME DEFINITIONS
// Project: Resonance Texturing (L-PDS)
// Studio: Tarhy Interactive
// Version: 1.7 “Stable Engine”
// Developer: Rüzgar Taha Aslan
// Date: 2026
// Description: Procedural detail injection & Thermal-aware LOD
// ─────────────────────────────────────────────

UENUM(BlueprintType)
enum class ESurfaceComplexity : uint8
{
Minimal    UMETA(DisplayName = “L-PDS: Thermal Safe”),   // Forced low-detail mode for thermal safety
Standard   UMETA(DisplayName = “L-PDS: Balanced”),
FullDetail UMETA(DisplayName = “L-PDS: Ultra Detail”)
};

USTRUCT(BlueprintType)
struct RESONANCETEXTURING_API FResonanceAgingParams
{
GENERATED_BODY()

```
UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=0.f, ClampMax=1.f))
float AgingFactor = 0.f; // 0: New/Mint, 1: Old/Ancient

UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=0.f, ClampMax=1.f))
float DirtAccumulation = 0.f; // Surface dirt level

UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=0.f, ClampMax=1.f))
float WearAmount = 0.f; // Physical wear and tear

UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=0.f, ClampMax=1.f))
float MoistureInfluence = 0.f; // Moisture/wetness effect
```

};

// ─────────────────────────────────────────────
// DATA CORE: L-PDS Surface Asset
// ─────────────────────────────────────────────

UCLASS(BlueprintType)
class RESONANCETEXTURING_API UResonanceSurfaceData : public UPrimaryDataAsset
{
GENERATED_BODY()

public:
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=“L-PDS|Base”)
TSoftObjectPtr<UTexture2D> BaseTexture;

```
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="L-PDS|Procedural", meta=(ClampMin=0.5f, ClampMax=32.f))
float GrainScale = 4.f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="L-PDS|Procedural", meta=(ClampMin=0.f, ClampMax=1.f))
float Turbulence = 0.35f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="L-PDS|Procedural")
ESurfaceComplexity ComplexityLevel = ESurfaceComplexity::Standard;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="L-PDS|Aging")
FResonanceAgingParams AgingParams;

// ── DISTANCE CULLING (L-PDS Optimization) ──
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="L-PDS|LOD", meta=(ClampMin=0.5f))
float LOD0_MaxDistance = 3.f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="L-PDS|LOD", meta=(ClampMin=1.f))
float LOD1_MaxDistance = 8.f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="L-PDS|LOD", meta=(ClampMin=2.f))
float LOD2_MaxDistance = 20.f;
```

};

// ─────────────────────────────────────────────
// CORE DRIVER: L-PDS Processor
// ─────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThermalThrottle, float, ThrottleLevel);

UCLASS(ClassGroup=(Resonance), meta=(BlueprintSpawnableComponent))
class RESONANCETEXTURING_API UResonanceTextureComponent : public UActorComponent
{
GENERATED_BODY()

public:
UResonanceTextureComponent();

```
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="L-PDS")
TObjectPtr<UResonanceSurfaceData> SurfaceData;

UPROPERTY(BlueprintAssignable, Category="L-PDS|Events")
FOnThermalThrottle OnThermalThrottle;

UFUNCTION(BlueprintCallable, Category="L-PDS")
void UpdateAgingParams(const FResonanceAgingParams& NewParams);

UFUNCTION(BlueprintCallable, Category="L-PDS")
void SetThermalThrottleLevel(float ThrottleLevel);

void RegisterToThermalManager();
```

protected:
virtual void BeginPlay() override;
virtual void EndPlay(const EEndPlayReason::Type Reason) override;
virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Fn) override;

private:
UPROPERTY()
TObjectPtr<UMaterialInstanceDynamic> CachedMID;

```
float CurrentLODBlend  = 0.f;
float TargetLODBlend   = 0.f;
float LastThermalLevel = 0.f;

// Update every N frames to reduce CPU overhead
static constexpr int32 UPDATE_FREQUENCY = 3;
int32 UpdateFrameCounter = 0;

float CalculateLODBlend() const;
void  FlushParametersToMaterial();
float GetComplexityMultiplier() const;
```

};

// ─────────────────────────────────────────────
// SUBSYSTEM: L-PDS Thermal Engine
// ─────────────────────────────────────────────

UCLASS()
class RESONANCETEXTURING_API UResonanceThermalManager : public UGameInstanceSubsystem
{
GENERATED_BODY()

public:
virtual void Initialize(FSubsystemCollectionBase& Collection) override;
virtual void Deinitialize() override;

```
UFUNCTION(BlueprintCallable, Category="L-PDS|Thermal")
float GetCurrentThermalLevel() const { return CurrentThermalLevel; }

void RegisterComponent(UResonanceTextureComponent* Comp);
void UnregisterComponent(UResonanceTextureComponent* Comp);
```

private:
float CurrentThermalLevel = 0.f;
FTimerHandle ThermalPollTimer;
TArray<TWeakObjectPtr<UResonanceTextureComponent>> ActiveComponents;

```
void PollThermalState();
void ApplyGlobalThrottle(float Level);
```

};
