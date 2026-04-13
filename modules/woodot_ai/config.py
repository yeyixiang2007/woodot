def can_build(env, platform):
    return True


def get_opts(platform):
    from SCons.Variables import BoolVariable

    return [
        BoolVariable(
            "ai_module_enabled",
            "Enable Woodot AI module sources and feature wiring",
            True,
        ),
        BoolVariable(
            "woodot_ai_cpu_enabled",
            "Enable CPU backend sources for Woodot AI",
            True,
        ),
        BoolVariable(
            "woodot_ai_llama_enabled",
            "Enable llama.cpp-based backend sources for Woodot AI",
            True,
        ),
        BoolVariable(
            "woodot_ai_desktop_enabled",
            "Enable desktop platform sources for Woodot AI",
            True,
        ),
        BoolVariable(
            "woodot_ai_windows_enabled",
            "Enable Windows-specific Woodot AI sources",
            True,
        ),
        BoolVariable(
            "woodot_ai_linuxbsd_enabled",
            "Enable Linux/BSD-specific Woodot AI sources",
            True,
        ),
        BoolVariable(
            "woodot_ai_macos_enabled",
            "Enable macOS-specific Woodot AI sources",
            True,
        ),
        BoolVariable(
            "woodot_ai_vulkan_enabled",
            "Enable Vulkan backend sources for Woodot AI",
            False,
        ),
        BoolVariable(
            "woodot_ai_cuda_enabled",
            "Enable CUDA backend sources for Woodot AI",
            False,
        ),
        BoolVariable(
            "woodot_ai_metal_enabled",
            "Enable Metal backend sources for Woodot AI",
            False,
        ),
        BoolVariable(
            "woodot_ai_android_enabled",
            "Enable Android-specific Woodot AI sources",
            True,
        ),
    ]


def configure(env):
    pass


def is_enabled():
    # Keep the module opt-in until the runtime/backend pieces are in place.
    # Enable with module_woodot_ai_enabled=yes.
    return False
