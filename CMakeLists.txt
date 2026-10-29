cmake_minimum_required(VERSION 3.16)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "Vita toolchain")
endif()
project(spire_android_probe C ASM)
include("${VITASDK}/share/vita.cmake" REQUIRED)
# All game-derived build inputs are generated locally outside Git.
set(SPIRE_PRIVATE_DIR "" CACHE PATH "Output of prepare_private_resources.py")
if(NOT SPIRE_PRIVATE_DIR)
  message(FATAL_ERROR "Set SPIRE_PRIVATE_DIR; see BUILDING.md")
endif()
set(SPIRE_DIRECT_INDEX "${SPIRE_PRIVATE_DIR}/direct-index.bin")
set(SPIRE_DIRECT_HEADER "${SPIRE_PRIVATE_DIR}/direct_cache_identity.h")
find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_build_inputs.py" "${SPIRE_PRIVATE_DIR}"
  RESULT_VARIABLE SPIRE_INPUT_CHECK)
if(NOT SPIRE_INPUT_CHECK EQUAL 0)
  message(FATAL_ERROR "Private resources are incomplete or inconsistent; regenerate together")
endif()
configure_file("${SPIRE_DIRECT_HEADER}" "${CMAKE_CURRENT_BINARY_DIR}/generated/direct_cache_identity.h" COPYONLY)
set(VITA_MKSFOEX_FLAGS "-d ATTRIBUTE2=12")
add_executable(spire_android_probe src/main.c src/audio_ima.c src/audio_wave.c src/audio_vorbis.c src/audio_mix.c src/audio_sources.c src/audio_runtime.c src/audio_output.c src/audio_hooks.c src/description_strings.c src/launcher.c src/launcher_background.c src/launcher_splash.c src/launcher_settings.c src/direct_cache.c src/direct_formats.c src/lazy_character.c src/load_profile.c src/card_profile.c src/gameplay_profile.c src/grid_card_observer.c src/grid_batch_contract.c src/grid_objects.c src/grid_commands.c src/grid_uniforms.c src/grid_scalar_state.c src/graphics_guard.c src/graphics_guard_trampolines.S src/gameplay_trampolines.S src/map_legend.c src/map_cull.c src/map_trampoline.S src/glyph_fast.c src/text_fast.c src/text_submit.c src/buffer_pointer.c src/buffer_pointer_trampoline.S src/text_memo.c src/text_trampoline.S src/performance_clocks.c src/psvshell_read.S src/loading_screen.c src/boot_display.c src/scene_metrics.c src/game_hooks.c src/game_trampolines.S src/keyboard.c src/native_controller.c src/vita_keyboard.c src/vita_frame.c src/frame_input.c src/android_frame_loop.c src/android_input.c src/android_dlfcn.c src/android_utf8.c src/android_files.c src/android_proc.c src/preview_audio.c src/preview_music.c src/preview_offline.c src/image_view.c src/android_platform.c src/graphics_bridge.c src/etc2_decode.c src/etc2_transcode.c src/texture_cache.c src/android_log.c src/android_time.c src/android_assets.c src/bionic_cxa.c src/gc_probe.c src/platform_probe.c src/android_jump.S src/android_jump_mask.c src/bionic_modules.c src/elf_probe.c src/elf_image.c src/elf_init.c src/elf_reloc.c src/android_tls.c src/tls_selftest.c src/compat_libc.c src/compat_test.c src/bionic_sync.c src/sync_test.c src/bionic_threads.c src/bionic_local.c vendor/debugscreen/debugScreen.c
 vendor/kubridge-stubs/kubridge_kubridge_kuKernelMemProtect.S
 vendor/kubridge-stubs/kubridge_kubridge_kuKernelFlushCaches.S)
target_link_options(spire_android_probe PRIVATE "-Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/src/vita-segment-gap.ld")
target_compile_definitions(spire_android_probe PRIVATE GEN_WEAK_EXPORTS=0)
target_include_directories(spire_android_probe PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated" src vendor/debugscreen)
target_compile_options(spire_android_probe PRIVATE -Wall -Wextra -O2)
target_link_libraries(spire_android_probe pthread vitaGL stdc++ mathneon vitashark SceShaccCgExt taihen_stub
 SceShaccCg_stub SceGxm_stub SceDisplay_stub ScePower_stub SceCtrl_stub SceTouch_stub SceAppMgr_stub
 SceAudio_stub SceCommonDialog_stub SceIme_stub SceAppUtil_stub SceSysmodule_stub SceKernelDmacMgr_stub SceKernelModulemgr_stub m)
vita_create_self(eboot.bin spire_android_probe UNSAFE)
vita_create_vpk(spire-vita-sourceprep-0.72.vpk SPAV00001 eboot.bin VERSION 00.72 NAME "Spire Vita Source Prep"
 FILE "${SPIRE_PRIVATE_DIR}/launcher-splashes.bin" "launcher-splashes.bin"
 FILE "${SPIRE_PRIVATE_DIR}/sce_sys/icon0.png" "sce_sys/icon0.png"
 FILE "${SPIRE_PRIVATE_DIR}/sce_sys/livearea/contents/bg0.png" "sce_sys/livearea/contents/bg0.png"
 FILE "${SPIRE_PRIVATE_DIR}/sce_sys/livearea/contents/startup.png" "sce_sys/livearea/contents/startup.png"
 FILE "${CMAKE_CURRENT_SOURCE_DIR}/assets/sce_sys/livearea/contents/template.xml" "sce_sys/livearea/contents/template.xml"
 FILE "${SPIRE_PRIVATE_DIR}/loading-pack.bin" "loading-pack.bin"
 FILE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/graphics-licenses/pthread-COPYING.txt" "licenses/pthread-COPYING.txt"
 FILE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/graphics-licenses/pthread-COPYING.LIB.txt" "licenses/pthread-COPYING.LIB.txt"
 FILE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/graphics-licenses/pthread-COPYING.vita.txt" "licenses/pthread-COPYING.vita.txt"
 FILE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/debugscreen/PSPSDK-LICENSE" "licenses/PSPSDK-font.txt"
 FILE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/debugscreen/CC0-1.0.txt" "licenses/CC0-1.0.txt"
 FILE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" "licenses/original-additions-MIT.txt"
 FILE "${SPIRE_DIRECT_INDEX}" "direct-index.bin"
 FILE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/detex/LICENSE.txt" "licenses/detex.txt"
 FILE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb/LICENSE-MIT.txt" "licenses/stb-vorbis-MIT.txt"
 FILE "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY.txt" "THIRD_PARTY.txt"
 FILE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/graphics-licenses/vitaGL-LGPL.txt" "licenses/vitaGL-LGPL.txt"
 FILE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/graphics-licenses/vitaGL-GPL.txt" "licenses/vitaGL-GPL.txt"
 FILE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/graphics-licenses/vitaShaRK.txt" "licenses/vitaShaRK.txt"
 FILE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/graphics-licenses/SceShaccCgExt.txt" "licenses/SceShaccCgExt.txt"
 FILE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/graphics-licenses/math-neon.txt" "licenses/math-neon.txt"
 FILE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/graphics-licenses/taiHEN.txt" "licenses/taiHEN.txt")
