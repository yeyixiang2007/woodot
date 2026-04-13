# Woodot Engine

Woodot is a Godot-based engine branch focused on turning AI into a first-class engine capability instead of a loose external tool.

## Overview

The current work is centered on an in-engine AI module under `modules/woodot_ai`, with build-time feature flags, vendored `llama.cpp`, and a staged plan for runtime inference, editor collaboration, and asset-pipeline integration.

## Current Status

- `modules/woodot_ai` has been scaffolded and wired into the Godot module build flow.
- `llama.cpp` is vendored under `modules/woodot_ai/thirdparty/llama.cpp`.
- Desktop CPU-only integration is the first supported path.
- Android build rules are defined for `platform=android arch=arm64`, but cross-build validation still depends on a configured Android toolchain.
- The module remains opt-in through `module_woodot_ai_enabled=yes`.

## Documentation

- [AI system overview](dev-doc/woodot-ai/00-system-overview.md)
- [AI build and dependencies](dev-doc/woodot-ai/01-build-and-dependencies.md)
- [AI runtime inference plan](dev-doc/woodot-ai/02-runtime-inference.md)
- [Project architecture](dev-doc/ARCHITECTURE.md)
- [Project roadmap](dev-doc/ROADMAP.md)

## Build Notes

- The module-level kill switch is `ai_module_enabled=yes/no`.
- Backend and platform flags are registered in `modules/woodot_ai/config.py`.
- The first validated link target is a desktop editor build with the AI module linked in but no runtime feature surface enabled yet.

## License

Woodot follows the same licensing terms defined in [LICENSE.txt](LICENSE.txt).
