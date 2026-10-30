# These isolation tests use GNU ELF --wrap and Itanium C++ symbol names.
# Compiler brand is not the capability: Windows Clang uses COFF/lld-link and
# silently ignores --wrap, while Apple uses a different linker/object format.
set(OPENCMR2_TEST_ELF_WRAP OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
 set(OPENCMR2_TEST_ELF_WRAP ON)
endif()

# Portable fixed-point helpers vs the original inline assembly. Needs a
# 32-bit x86 build: clang compiles the MSVC-style assembly with -fasm-blocks.
if(CMAKE_SIZEOF_VOID_P EQUAL 4 AND CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]86|x86")
    add_executable(fixedpoint_equiv fixedpoint_equiv.cpp)
    target_include_directories(fixedpoint_equiv PRIVATE ${PROJECT_SOURCE_DIR}/game ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_options(fixedpoint_equiv PRIVATE
        -include ${PROJECT_SOURCE_DIR}/src/port/types.h -fms-extensions -fasm-blocks -Wno-everything)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # The original assembly uses absolute addresses (offset g_atanTable).
        # Ubuntu's Clang defaults to PIE, which cannot encode that operand.
        target_compile_options(fixedpoint_equiv PRIVATE -fno-pic -fno-pie)
        target_link_options(fixedpoint_equiv PRIVATE -no-pie)
    endif()
    add_test(NAME fixedpoint_equiv COMMAND fixedpoint_equiv)
endif()

add_executable(net_session_test net_session.cpp)
target_link_libraries(net_session_test PRIVATE opencmr2_net)
target_compile_options(net_session_test PRIVATE "SHELL:-include ${PROJECT_SOURCE_DIR}/src/port/types.h")
add_test(NAME net_session COMMAND net_session_test)
set_tests_properties(net_session PROPERTIES TIMEOUT 60)

add_executable(bink_decoder_test bink_decoder.cpp)
target_link_libraries(bink_decoder_test PRIVATE opencmr2_bink)
add_test(NAME bink_decoder COMMAND bink_decoder_test)

add_executable(movie_playback_test movie_playback.cpp ${PROJECT_SOURCE_DIR}/src/video/movie.cpp)
target_link_libraries(movie_playback_test PRIVATE opencmr2_bink)
add_test(NAME movie_playback COMMAND movie_playback_test)

add_executable(movie_render_state_test movie_render_state.cpp
    ${PROJECT_SOURCE_DIR}/src/diagnostics/observer.cpp
    ${PROJECT_SOURCE_DIR}/src/video/movie.cpp ${PROJECT_SOURCE_DIR}/src/render/gfx_core.cpp)
target_link_libraries(movie_render_state_test PRIVATE opencmr2_bink SDL3::SDL3)
add_test(NAME movie_render_state COMMAND movie_render_state_test)

# FTOI vs a real x87 FISTP (the /QIfist conversion of the original).
if(CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]86|x86|AMD64|amd64")
    add_executable(ftoi_fistp_test ftoi_fistp.cpp)
    target_include_directories(ftoi_fistp_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
    target_compile_options(ftoi_fistp_test PRIVATE "SHELL:-include ${PROJECT_SOURCE_DIR}/src/port/types.h")
    add_test(NAME ftoi_fistp COMMAND ftoi_fistp_test)
endif()

# Tests that run game code, on every target.
add_executable(math_tables_test math_tables.cpp)
target_link_libraries(math_tables_test PRIVATE opencmr2_platform)
add_test(NAME math_tables COMMAND math_tables_test
    ${CMAKE_CURRENT_SOURCE_DIR}/reference/x87-single/math-tables.bin)
add_executable(game_data_setup_test game_data_setup.cpp)
target_link_libraries(game_data_setup_test PRIVATE opencmr2_platform)
add_test(NAME game_data_setup COMMAND game_data_setup_test)
set_tests_properties(game_data_setup PROPERTIES TIMEOUT 30)
add_executable(car_scheduler_test car_scheduler.cpp)
target_link_libraries(car_scheduler_test PRIVATE opencmr2_platform)
add_test(NAME car_scheduler COMMAND car_scheduler_test)
add_executable(stage_probe stage_probe.cpp)
target_link_libraries(stage_probe PRIVATE opencmr2_platform)
# The monkey test's watchdog uses POSIX alarm().
if(NOT WIN32)
    add_executable(monkey_probe monkey_probe.cpp)
    target_link_libraries(monkey_probe PRIVATE opencmr2_platform)
endif()
add_executable(preview_cleanup_test preview_cleanup.cpp)
target_include_directories(preview_cleanup_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
target_link_libraries(preview_cleanup_test PRIVATE opencmr2_platform)
add_test(NAME preview_cleanup COMMAND preview_cleanup_test)
add_executable(service_park_weather_test service_park_weather.cpp)
target_link_libraries(service_park_weather_test PRIVATE opencmr2_platform)
add_test(NAME service_park_weather COMMAND service_park_weather_test)
add_executable(gamepad_layout_test gamepad_layout.cpp)
target_include_directories(gamepad_layout_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
target_link_libraries(gamepad_layout_test PRIVATE opencmr2_platform)
add_test(NAME gamepad_layout COMMAND gamepad_layout_test)

add_executable(frontend_language_test frontend_language.cpp)
target_link_libraries(frontend_language_test PRIVATE opencmr2_platform)
add_test(NAME frontend_language COMMAND frontend_language_test)
set_tests_properties(frontend_language PROPERTIES TIMEOUT 30)

add_executable(graphics_options_test graphics_options.cpp)
target_include_directories(graphics_options_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
add_test(NAME graphics_options COMMAND graphics_options_test)

add_executable(tree_break_test tree_break.cpp)
target_include_directories(tree_break_test PRIVATE ${PROJECT_SOURCE_DIR}/game ${PROJECT_SOURCE_DIR}/src)
add_test(NAME tree_break COMMAND tree_break_test)

add_executable(camera_effects_test camera_effects.cpp)
target_include_directories(camera_effects_test PRIVATE ${PROJECT_SOURCE_DIR}/game ${PROJECT_SOURCE_DIR}/src)
add_test(NAME camera_effects COMMAND camera_effects_test)

add_executable(animated_scenery_test animated_scenery.cpp)
target_include_directories(animated_scenery_test PRIVATE ${PROJECT_SOURCE_DIR}/game ${PROJECT_SOURCE_DIR}/src)
add_test(NAME animated_scenery COMMAND animated_scenery_test)
add_executable(tree_wind_cache_test tree_wind_cache.cpp)
target_include_directories(tree_wind_cache_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
add_test(NAME tree_wind_cache COMMAND tree_wind_cache_test)

add_executable(motion_blur_test motion_blur.cpp)
target_include_directories(motion_blur_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
add_test(NAME motion_blur COMMAND motion_blur_test)

add_executable(bloom_test bloom.cpp)
target_include_directories(bloom_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
add_test(NAME bloom COMMAND bloom_test)

add_executable(shadows_test shadows.cpp)
target_include_directories(shadows_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
add_test(NAME shadows COMMAND shadows_test)

add_executable(render_stats_test render_stats.cpp
    ${PROJECT_SOURCE_DIR}/src/render/gfx_core.cpp
    ${PROJECT_SOURCE_DIR}/src/diagnostics/observer.cpp)
target_include_directories(render_stats_test PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/game)
target_link_libraries(render_stats_test PRIVATE SDL3::SDL3)
add_test(NAME render_stats COMMAND render_stats_test)

add_executable(country_flags_test country_flags.cpp)
target_link_libraries(country_flags_test PRIVATE opencmr2_platform)
add_test(NAME country_flags COMMAND country_flags_test ${CMAKE_CURRENT_SOURCE_DIR}/reference/menu-flags.txt)

add_executable(vector_sprites_test vector_sprites.cpp)
target_link_libraries(vector_sprites_test PRIVATE opencmr2_platform)
add_test(NAME vector_sprites COMMAND vector_sprites_test)

add_executable(saved_game_test saved_game.cpp)
target_link_libraries(saved_game_test PRIVATE opencmr2_platform)
add_test(NAME saved_game COMMAND saved_game_test)
set_tests_properties(saved_game PROPERTIES TIMEOUT 30)

add_executable(bink_decode bink_decode.cpp)
target_link_libraries(bink_decode PRIVATE opencmr2_bink)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME reference_capture_format
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/reference_capture.py)
    add_test(NAME speed_fov_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/speed_fov_wiring.py)
    add_test(NAME speed_streak_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/speed_streak_wiring.py)
    add_test(NAME speed_lines_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/speed_lines_wiring.py)
    add_test(NAME fov_dolly_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/fov_dolly_wiring.py)
    add_test(NAME blur_mode_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/blur_mode_wiring.py)
    add_test(NAME vapor_trail_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/vapor_trail_wiring.py)
    add_test(NAME dof_race_only_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/dof_race_only_wiring.py)
    add_test(NAME dof_focus_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/dof_focus_wiring.py WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    add_test(NAME tunnel_look_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tunnel_look_wiring.py WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    add_test(NAME gl_tunnel_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gl_tunnel_wiring.py)
    add_test(NAME tuning_pause_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tuning_pause_wiring.py)
    add_test(NAME overlay_layer_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/overlay_layer_wiring.py WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    add_test(NAME raw_access_audit
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/raw_access_audit.py)
endif()

set(OPENCMR2_BINK_TEST_DATA_DIR "" CACHE PATH "Optional original game data for movie and physics regression tests")
if(OPENCMR2_BINK_TEST_DATA_DIR)
    if(Python3_Interpreter_FOUND AND TARGET stage_probe)
        add_test(NAME physics_original_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/physics_parity.py --probe $<TARGET_FILE:stage_probe>
            --data ${OPENCMR2_BINK_TEST_DATA_DIR})
        add_test(NAME runtime_scenarios COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/runtime_scenarios.py --probe $<TARGET_FILE:stage_probe>
            --data ${OPENCMR2_BINK_TEST_DATA_DIR})
        set_tests_properties(runtime_scenarios PROPERTIES TIMEOUT 180)
        add_test(NAME cpu_runs COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/runtime_scenarios.py --probe $<TARGET_FILE:stage_probe>
            --data ${OPENCMR2_BINK_TEST_DATA_DIR} --cpu-runs)
        set_tests_properties(cpu_runs PROPERTIES TIMEOUT 900)
        if(NOT CMAKE_CROSSCOMPILING)
            add_test(NAME lan_game COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/lan_game.py --host-probe $<TARGET_FILE:stage_probe>
                --data ${OPENCMR2_BINK_TEST_DATA_DIR})
            set_tests_properties(lan_game PROPERTIES TIMEOUT 120)
        endif()
        add_test(NAME stage_lighting COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/stage_lighting.py --probe $<TARGET_FILE:stage_probe>
            --data ${OPENCMR2_BINK_TEST_DATA_DIR})
        set_tests_properties(stage_lighting PROPERTIES TIMEOUT 300)
    endif()
    file(GLOB_RECURSE BINK_TEST_FILES "${OPENCMR2_BINK_TEST_DATA_DIR}/*.bik" "${OPENCMR2_BINK_TEST_DATA_DIR}/*.BIK")
    foreach(movie IN LISTS BINK_TEST_FILES)
        get_filename_component(name "${movie}" NAME_WE)
        add_test(NAME "bink_file_${name}" COMMAND bink_decode "${movie}")
    endforeach()
endif()

add_executable(replay_disk_test replay_disk.cpp)
target_link_libraries(replay_disk_test PRIVATE opencmr2_platform)
add_test(NAME replay_disk COMMAND replay_disk_test)

add_executable(replay_events_test replay_events.cpp)
target_link_libraries(replay_events_test PRIVATE opencmr2_platform)
add_test(NAME replay_events COMMAND replay_events_test)

add_executable(sector_culling_test sector_culling.cpp)
target_link_libraries(sector_culling_test PRIVATE opencmr2_platform)
add_test(NAME sector_culling COMMAND sector_culling_test)

add_executable(sector_geometry_test sector_geometry.cpp)
target_link_libraries(sector_geometry_test PRIVATE opencmr2_platform)
add_test(NAME sector_geometry COMMAND sector_geometry_test)

add_executable(headlight_collision_test headlight_collision.cpp)
target_link_libraries(headlight_collision_test PRIVATE opencmr2_platform)
add_test(NAME headlight_collision COMMAND headlight_collision_test)

add_library(high_address_fixture STATIC high_address.c)
add_executable(net_frontend_test net_frontend.cpp)
target_link_libraries(net_frontend_test PRIVATE opencmr2_platform high_address_fixture)
add_test(NAME net_frontend COMMAND net_frontend_test)
set_tests_properties(net_frontend PROPERTIES TIMEOUT 30)
target_link_libraries(headlight_collision_test PRIVATE high_address_fixture)
target_link_libraries(sector_culling_test PRIVATE high_address_fixture)

if(WIN32)
    # Windows searches beside each test executable for SDL3.dll. Copy once
    # before linking the tests, including when only stage_probe is requested.
    add_custom_target(opencmr2_test_runtime
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3-shared> ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS SDL3::SDL3-shared)
    get_property(test_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS test_targets)
        get_target_property(kind ${target} TYPE)
        if(kind STREQUAL "EXECUTABLE")
            add_dependencies(${target} opencmr2_test_runtime)
        endif()
    endforeach()
endif()

# Optional actual-pixel postprocess regression; no proprietary game data.
# Dependencies: Python moderngl/numpy/Pillow, EGL (or use the script manually).
option(OPENCMR2_SHADER_PIXEL_TESTS "Run optional EGL shader pixel tests" OFF)
if(OPENCMR2_SHADER_PIXEL_TESTS)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME post_effects_pixels COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/post_effects_shader.py)
    add_test(NAME post_shader_semantics COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/post_shader_semantics.py)
    add_test(NAME shadow_pixels COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/shadow_shader.py)
    add_test(NAME spatial_aa_pixels COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/spatial_aa_shader.py)
endif()

# Optional diagnostic only. OBJCXX is enabled in the top-level directory.
if(APPLE AND NOT ANDROID AND OPENCMR2_METALFX_PROBE)
    add_executable(metalfx_spatial_probe ../tools/metalfx/spatial_probe.mm)
    target_compile_options(metalfx_spatial_probe PRIVATE -fobjc-arc)
    target_link_libraries(metalfx_spatial_probe PRIVATE "-framework Foundation" "-framework Metal")
    target_link_options(metalfx_spatial_probe PRIVATE "SHELL:-weak_framework MetalFX")
endif()

if(Python3_Interpreter_FOUND)
    add_test(NAME metalfx_patch_safety COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/metalfx_patch.py)
endif()

if(Python3_Interpreter_FOUND)
    add_test(NAME shadow_wiring COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/shadow_wiring.py)
    add_test(NAME metalfx_cmake_safety COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/metalfx_cmake.py)
endif()
# CPU temporal foundation. Does not enable temporal rendering in the game.
add_executable(temporal_history_test temporal_history.cpp)
target_include_directories(temporal_history_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_test(NAME temporal_history COMMAND temporal_history_test)

add_executable(wheel_particle_emission_test wheel_particle_emission.cpp)
target_include_directories(wheel_particle_emission_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
add_test(NAME wheel_particle_emission COMMAND wheel_particle_emission_test)
add_executable(diagnostic_font_test diagnostic_font.cpp)
target_include_directories(diagnostic_font_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
target_link_libraries(diagnostic_font_test PRIVATE opencmr2_platform)
add_test(NAME diagnostic_font COMMAND diagnostic_font_test)

add_executable(dust_plume_test dust_plume.cpp)
target_include_directories(dust_plume_test PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/game)
add_test(NAME dust_plume COMMAND dust_plume_test)
add_executable(speed_mote_streak_test speed_mote_streak.cpp)
target_include_directories(speed_mote_streak_test PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/game)
add_test(NAME speed_mote_streak COMMAND speed_mote_streak_test)
add_executable(speed_mote_spawn_test speed_mote_spawn.cpp)
target_include_directories(speed_mote_spawn_test PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/game)
add_test(NAME speed_mote_spawn COMMAND speed_mote_spawn_test)
if(Python3_Interpreter_FOUND)
    add_test(NAME speed_mote_spawn_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/speed_mote_spawn_wiring.py WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endif()
add_executable(speed_fov_test speed_fov.cpp)
target_include_directories(speed_fov_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
add_test(NAME speed_fov COMMAND speed_fov_test)
add_executable(speed_streak_test speed_streak.cpp)
target_include_directories(speed_streak_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_test(NAME speed_streak COMMAND speed_streak_test)
add_executable(tuning_panel_test tuning_panel.cpp)
target_include_directories(tuning_panel_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
add_test(NAME tuning_panel COMMAND tuning_panel_test)
add_executable(fov_dolly_test fov_dolly.cpp)
target_include_directories(fov_dolly_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
add_test(NAME fov_dolly COMMAND fov_dolly_test)
add_executable(car_region_test car_region.cpp)
target_include_directories(car_region_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_test(NAME car_region COMMAND car_region_test)
add_executable(vapor_trail_test vapor_trail.cpp)
target_include_directories(vapor_trail_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_test(NAME vapor_trail COMMAND vapor_trail_test)
add_executable(fov_preview_test fov_preview.cpp)
target_include_directories(fov_preview_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
add_test(NAME fov_preview COMMAND fov_preview_test)
if(Python3_Interpreter_FOUND)
    add_test(NAME fov_preview_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/fov_preview_wiring.py WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endif()
add_executable(dof_gate_test dof_gate.cpp)
target_include_directories(dof_gate_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_test(NAME dof_gate COMMAND dof_gate_test)
add_executable(dof_focus_test dof_focus.cpp)
target_include_directories(dof_focus_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_test(NAME dof_focus COMMAND dof_focus_test)
add_executable(motion_blur_limits_test motion_blur_limits.cpp)
target_include_directories(motion_blur_limits_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_test(NAME motion_blur_limits COMMAND motion_blur_limits_test)
add_executable(dust_plume_motion dust_plume_motion.cpp)
target_include_directories(dust_plume_motion PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/game)
add_executable(dust_pool_test dust_pool.cpp)
target_include_directories(dust_pool_test PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/game)
target_link_libraries(dust_pool_test PRIVATE opencmr2_platform)
add_test(NAME dust_pool COMMAND dust_pool_test)
if(OPENCMR2_TEST_ELF_WRAP)
add_executable(advanced_menu_test advanced_menu.cpp)
target_include_directories(advanced_menu_test PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/game)
target_link_libraries(advanced_menu_test PRIVATE opencmr2_platform)
 target_link_options(advanced_menu_test PRIVATE -Wl,--wrap=_Z21FrontendDraw_PlayTimev -Wl,--wrap=_Z21FrontendDraw_MenuPathP4MenuiiciPPci -Wl,--wrap=_Z21FrontendDraw_CarouselP4MenucPc -Wl,--wrap=_Z13Font_DrawTexthPcssPij)
 add_test(NAME advanced_menu COMMAND advanced_menu_test)
endif()
add_executable(postprocess_pass_test postprocess_pass.cpp)
target_include_directories(postprocess_pass_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(postprocess_pass_test PRIVATE SDL3::Headers)
add_test(NAME postprocess_pass COMMAND postprocess_pass_test)
add_executable(animated_scenery_lifecycle_test animated_scenery_lifecycle.cpp)
target_include_directories(animated_scenery_lifecycle_test PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/game)
target_link_libraries(animated_scenery_lifecycle_test PRIVATE opencmr2_platform)
add_test(NAME animated_scenery_lifecycle COMMAND animated_scenery_lifecycle_test)
if(Python3_Interpreter_FOUND)
 add_test(NAME renderpass_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/renderpass_wiring.py)
 add_test(NAME billboard_queue_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/billboard_queue_wiring.py)
 add_test(NAME object_mask_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/object_mask_wiring.py)
endif()
if(OPENCMR2_SHADER_PIXEL_TESTS AND Python3_Interpreter_FOUND)
 add_test(NAME dust_volume_pixels COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/dust_volume_shader.py)
endif()
add_executable(dust_volume_math_test dust_volume_math.cpp)
target_include_directories(dust_volume_math_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_test(NAME dust_volume_math COMMAND dust_volume_math_test)

add_executable(scenery_pack_format_test scenery_pack_format.cpp)
target_include_directories(scenery_pack_format_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
add_test(NAME scenery_pack_format COMMAND scenery_pack_format_test)
add_executable(scenery_pack_runtime_test scenery_pack_runtime.cpp
 ${PROJECT_SOURCE_DIR}/game/SceneryPack.cpp ${PROJECT_SOURCE_DIR}/src/render/gfx_core.cpp ${PROJECT_SOURCE_DIR}/src/diagnostics/observer.cpp)
target_include_directories(scenery_pack_runtime_test PRIVATE ${PROJECT_SOURCE_DIR}/game ${PROJECT_SOURCE_DIR}/src)
target_compile_options(scenery_pack_runtime_test PRIVATE "SHELL:-include ${PROJECT_SOURCE_DIR}/src/port/types.h" -fms-extensions)
target_link_libraries(scenery_pack_runtime_test PRIVATE SDL3::SDL3)
add_test(NAME scenery_pack_runtime COMMAND scenery_pack_runtime_test ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/scenery-pack)
add_executable(driver_names_test driver_names.cpp)
target_link_libraries(driver_names_test PRIVATE opencmr2_platform)
add_test(NAME driver_names COMMAND driver_names_test)
if(OPENCMR2_TEST_ELF_WRAP)
 add_executable(graphics_startup_failure_test graphics_startup_failure.cpp)
 target_link_libraries(graphics_startup_failure_test PRIVATE opencmr2_platform)
 target_link_options(graphics_startup_failure_test PRIVATE
  -Wl,--wrap=Gfx_SetVideoMode
  -Wl,--wrap=_Z25Scene_InitFixedMathTablesv)
 add_test(NAME graphics_startup_failure COMMAND graphics_startup_failure_test)
endif()
if(OPENCMR2_TEST_ELF_WRAP)
 add_executable(driver_names_runtime_test driver_names_runtime.cpp)
 target_link_libraries(driver_names_runtime_test PRIVATE opencmr2_platform)
 target_link_options(driver_names_runtime_test PRIVATE
  -Wl,--wrap=Sys_GetOption -Wl,--wrap=Sys_OpenFile -Wl,--wrap=Sys_GetFileSize
  -Wl,--wrap=Sys_ReadFile -Wl,--wrap=Sys_CloseFile -Wl,--wrap=Sys_Log)
 foreach(mode valid missing invalid disabled real)
  add_test(NAME driver_names_runtime_${mode} COMMAND driver_names_runtime_test ${mode})
 endforeach()
endif()
if(OPENCMR2_TEST_ELF_WRAP)
 add_executable(startup_options_test startup_options.cpp)
 target_link_libraries(startup_options_test PRIVATE opencmr2_platform)
 target_link_options(startup_options_test PRIVATE -Wl,--wrap=Sys_Log
  -Wl,--wrap=Sys_GetOption -Wl,--wrap=Sys_GetOptionText -Wl,--wrap=Sys_OpenFile)
 add_test(NAME startup_options_missing COMMAND startup_options_test missing)
 add_test(NAME startup_options_disabled COMMAND startup_options_test disabled)
endif()
add_executable(world_dust_test world_dust.cpp)
target_include_directories(world_dust_test PRIVATE ${PROJECT_SOURCE_DIR}/game ${PROJECT_SOURCE_DIR}/src)
add_test(NAME world_dust COMMAND world_dust_test)
if(OPENCMR2_TEST_ELF_WRAP)
 add_executable(world_dust_routing_test world_dust_routing.cpp)
 target_link_libraries(world_dust_routing_test PRIVATE opencmr2_platform)
 target_link_options(world_dust_routing_test PRIVATE -Wl,--wrap=Gfx_DrawDustVolume)
 add_test(NAME world_dust_routing COMMAND world_dust_routing_test)
endif()
add_executable(dust_volume_recording_test dust_volume_recording.cpp
 ${PROJECT_SOURCE_DIR}/src/render/gfx_core.cpp ${PROJECT_SOURCE_DIR}/src/diagnostics/observer.cpp)
target_include_directories(dust_volume_recording_test PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/game)
target_link_libraries(dust_volume_recording_test PRIVATE SDL3::SDL3)
add_test(NAME dust_volume_recording COMMAND dust_volume_recording_test)

add_test(NAME scenery_separate_root COMMAND scenery_pack_runtime_test ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/scenery-pack separate)
add_executable(scenery_source_fit_test scenery_source_fit.cpp)
target_include_directories(scenery_source_fit_test PRIVATE ${PROJECT_SOURCE_DIR}/game ${PROJECT_SOURCE_DIR}/src)
add_test(NAME scenery_source_fit COMMAND scenery_source_fit_test)

add_test(NAME scenery_legacy_fallback COMMAND scenery_pack_runtime_test ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/scenery-pack legacy)

add_executable(enhancements_test enhancements.cpp)
target_link_libraries(enhancements_test PRIVATE opencmr2_platform)
add_test(NAME enhancements COMMAND enhancements_test)
add_test(NAME enhancement_help COMMAND opencmr2 --help-enhancements)
if(Python3_Interpreter_FOUND)
 add_test(NAME enhancement_prepare COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/scenery-patch/test_prepare.py)
 add_test(NAME enhancement_binary COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/enhancement_binary.py $<TARGET_FILE:opencmr2>)
endif()
add_executable(enhancement_io_probe enhancement_io_probe.cpp)
target_link_libraries(enhancement_io_probe PRIVATE opencmr2_platform)
if(Python3_Interpreter_FOUND)
 add_test(NAME enhancement_overlay_io COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/enhancement_binary.py $<TARGET_FILE:enhancement_io_probe> overlay)
endif()

add_executable(key_remapping_test key_remapping.cpp)
target_link_libraries(key_remapping_test PRIVATE opencmr2_platform)
add_test(NAME key_remapping COMMAND key_remapping_test)

add_test(NAME scenery_identity_routing COMMAND scenery_pack_runtime_test ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/scenery-pack identity)
if(OPENCMR2_TEST_ELF_WRAP)
 add_executable(scenery_texture_capture_test scenery_texture_capture.cpp)
 target_link_libraries(scenery_texture_capture_test PRIVATE opencmr2_platform)
 target_link_options(scenery_texture_capture_test PRIVATE
  -Wl,--wrap=_ZN18CGenericFileLoader17FindFileInArchiveEP11GenericFilePcPj
  -Wl,--wrap=_ZN9CGraphics26CreateTextureFromImageDataEPcPvj
  -Wl,--wrap=_Z26SceneryPack_RegisterSourceP7TexturePKvj
  -Wl,--wrap=_ZN9CGraphics21LoadDDSThenTGATextureEPcj)
 add_test(NAME scenery_texture_capture COMMAND scenery_texture_capture_test)
endif()
if(OPENCMR2_TEST_ELF_WRAP)
 add_executable(car_display_names_test car_display_names.cpp)
 target_link_libraries(car_display_names_test PRIVATE opencmr2_platform)
 target_link_options(car_display_names_test PRIVATE -Wl,--wrap=Sys_GetCarDisplayName)
 add_test(NAME car_display_names COMMAND car_display_names_test)
endif()

add_executable(tree_collision_runtime_test tree_collision_runtime.cpp)
target_link_libraries(tree_collision_runtime_test PRIVATE opencmr2_platform)
add_test(NAME tree_collision_runtime COMMAND tree_collision_runtime_test)

add_executable(research_track_edges_test research_track_edges.cpp)
target_link_libraries(research_track_edges_test PRIVATE opencmr2_platform)
add_test(NAME research_track_edges COMMAND research_track_edges_test)
if(Python3_Interpreter_FOUND)
 add_test(NAME road_research_contract COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/scenery-patch/test_road_contract.py)
 add_test(NAME road_pair_transaction COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/scenery-patch/test_road_pair.py)
endif()
if(OPENCMR2_SHADER_PIXEL_TESTS)
 add_test(NAME road_prepare_recipe COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/scenery-patch/test_road_prepare.py)
endif()
if(OPENCMR2_TEST_ELF_WRAP)
 add_executable(road_loader_probe road_loader_probe.cpp)
 target_link_libraries(road_loader_probe PRIVATE opencmr2_platform)
 target_link_options(road_loader_probe PRIVATE
  -Wl,--wrap=_Z30Graphics_LoadTextureRecordListPhPcjiP11GenericFile
  -Wl,--wrap=_Z19Mesh_UploadVerticesP4Mesh)
 add_executable(road_runtime_probe road_runtime_probe.cpp)
 target_link_libraries(road_runtime_probe PRIVATE opencmr2_platform)
 target_link_options(road_runtime_probe PRIVATE
  -Wl,--wrap=Sys_GetRoadDirectory
  -Wl,--wrap=_ZN18CGenericFileLoader8FindFileEP11GenericFilePcPhPjh
  -Wl,--wrap=_ZN5CGame16RegisterCallbackEPvS0_
  -Wl,--wrap=_Z24Track_ResearchExactEdgesb)
endif()

if(NOT WIN32) # the test uses POSIX mkdtemp
add_executable(attract_capture_test attract_capture.cpp)
target_include_directories(attract_capture_test PRIVATE ${PROJECT_SOURCE_DIR}/game ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/port/compat)
add_test(NAME attract_capture COMMAND attract_capture_test)
endif()
add_executable(camera_operator_test camera_operator.cpp)
target_include_directories(camera_operator_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
add_test(NAME camera_operator COMMAND camera_operator_test)
add_executable(camera_operator_runtime_test camera_operator_runtime.cpp ${PROJECT_SOURCE_DIR}/game/CameraOperator.cpp)
target_include_directories(camera_operator_runtime_test PRIVATE ${PROJECT_SOURCE_DIR}/game ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/port/compat)
target_compile_options(camera_operator_runtime_test PRIVATE "SHELL:-include ${PROJECT_SOURCE_DIR}/src/port/types.h" -fms-extensions)
add_test(NAME camera_operator_runtime COMMAND camera_operator_runtime_test)
add_executable(france1_textures_test france1_textures.cpp
 ${PROJECT_SOURCE_DIR}/game/France1Textures.cpp ${PROJECT_SOURCE_DIR}/src/render/gfx_core.cpp ${PROJECT_SOURCE_DIR}/src/diagnostics/observer.cpp)
target_include_directories(france1_textures_test PRIVATE ${PROJECT_SOURCE_DIR}/game ${PROJECT_SOURCE_DIR}/src)
target_compile_options(france1_textures_test PRIVATE "SHELL:-include ${PROJECT_SOURCE_DIR}/src/port/types.h" -fms-extensions)
target_link_libraries(france1_textures_test PRIVATE SDL3::SDL3)
add_test(NAME france1_textures COMMAND france1_textures_test)
if(Python3_Interpreter_FOUND)
 add_test(NAME france1_prepare_recipe COMMAND ${Python3_EXECUTABLE} -m unittest test_france1_prepare test_road_periodic WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tools/scenery-patch)
endif()
add_executable(cockpit_wipers_test cockpit_wipers.cpp)
target_link_libraries(cockpit_wipers_test PRIVATE opencmr2_platform)
add_test(NAME cockpit_wipers COMMAND cockpit_wipers_test)
add_executable(driving_menu_path_test driving_menu_path.cpp)
target_link_libraries(driving_menu_path_test PRIVATE opencmr2_platform)
add_test(NAME driving_menu_path COMMAND driving_menu_path_test)
add_executable(camera_operator_spring_test camera_operator_spring.cpp)
target_link_libraries(camera_operator_spring_test PRIVATE opencmr2_platform)
add_test(NAME camera_operator_spring COMMAND camera_operator_spring_test)
add_executable(camera_operator_label_test camera_operator_label.cpp)
target_link_libraries(camera_operator_label_test PRIVATE opencmr2_platform)
add_test(NAME camera_operator_label COMMAND camera_operator_label_test)
if(OPENCMR2_TEST_ELF_WRAP)
 add_executable(wiper_menu_layout_test wiper_menu_layout.cpp)
 target_link_libraries(wiper_menu_layout_test PRIVATE opencmr2_platform)
 target_link_options(wiper_menu_layout_test PRIVATE -Wl,--wrap=_Z21FrontendDraw_PlayTimev -Wl,--wrap=_Z23FrontendDraw_BreadcrumbiiPPci -Wl,--wrap=_Z13Font_DrawTexthPcssPij)
 add_test(NAME wiper_menu_layout COMMAND wiper_menu_layout_test)
endif()
add_executable(effect_settings_test effect_settings.cpp)
target_include_directories(effect_settings_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_test(NAME effect_settings COMMAND effect_settings_test)
if(Python3_EXECUTABLE)
 add_test(NAME gl_effects_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gl_effects_wiring.py)
endif()
add_executable(temporal_uniforms_test temporal_uniforms.cpp)
target_include_directories(temporal_uniforms_test PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/render)
add_test(NAME temporal_uniforms COMMAND temporal_uniforms_test)

add_executable(projection_vote_test projection_vote.cpp)
target_include_directories(projection_vote_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_test(NAME projection_vote COMMAND projection_vote_test)
add_executable(cockpit_eye_shift_test cockpit_eye_shift.cpp)
target_include_directories(cockpit_eye_shift_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
add_test(NAME cockpit_eye_shift COMMAND cockpit_eye_shift_test)

add_executable(trail_history_test trail_history.cpp)
target_include_directories(trail_history_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
add_test(NAME trail_history COMMAND trail_history_test)
