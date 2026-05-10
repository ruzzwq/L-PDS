# Resonance Hybrid Texturing — L-PDS Core

**Unreal Engine 5.7 Plugin**  
Developer: Rüzgar Taha Aslan | Studio: Tarhy Interactive

-----

## What does it do?

L-PDS (Low-Poly Detail System) — A procedural texture engine built for VRAM-constrained rendering environments.

- Thermal load tracking (automatic LOD based on GPU temperature)
- Distance-based detail levels (LOD 0 → LOD 2)
- Surface aging system (dirt, wear, moisture)
- HLSL shader integration (`/ResonanceShaders/`)

-----

## Folder Structure

Make sure the files are organized as follows before installing:

```
ResonanceTexturing/
├── ResonanceTexturing.uplugin
├── Source/
│   └── ResonanceTexturing/
│       ├── ResonanceTexturing.Build.cs
│       ├── Public/
│       │   └── ResonanceTexturing.h
│       └── Private/
│           └── ResonanceTexturing.cpp
└── Shaders/
    └── ResonanceWoodGrain.usf
```

-----

## Installation

### 1. Place the files

Copy the entire `ResonanceTexturing` folder into your project’s `Plugins/` directory:

```
YourProject/
└── Plugins/
    └── ResonanceTexturing/   ← here
```

> If the `Plugins/` folder does not exist, create it manually.

### 2. Open your project

Open your project with Unreal Engine 5.7. You will see the following prompt:

> **“The following modules are missing or built with a different engine version. Would you like to rebuild them now?”**

Click **Yes**. Unreal will automatically compile the plugin.

### 3. Enable the plugin

Once the build is complete:

```
Edit → Plugins → Search "Resonance" → Enable
```

### 4. Restart the editor

Unreal will ask you to restart the editor. Click **Restart Now** and you’re done.

-----

## Usage

Add `UResonanceTextureComponent` to any Actor.  
Assign a `UResonanceSurfaceData` asset to the `SurfaceData` field.  
The thermal management and LOD system will activate automatically.

-----

## Requirements

- Unreal Engine 5.7
- C++ project (Blueprint-only projects cannot compile this plugin)
- Visual Studio 2022 or JetBrains Rider

-----

## License

MIT License — © 2026 Rüzgar Taha Aslan, Tarhy Interactive  
See the `LICENSE` file for details.