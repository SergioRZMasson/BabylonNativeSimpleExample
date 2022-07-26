cmake_minimum_required(VERSION 3.0.0)
project(BabylonNativeSample VERSION 0.1.0)

set(BABYLON_NATIVE_BUILD_APPS OFF)
add_subdirectory(BabylonNative)

set(BABYLON_JS_SCRIPTS
    "Scripts/babylonjs.loaders.js"
    "Scripts/babylon.max.js"
    "Scripts/babylonjs.materials.js"
    "Scripts/babylon.gui.js"
    "Scripts/app.js"
    "Scripts/meshwriter.min.js"
    "Scripts/ammo.js")

set(BABYLON_JS_DEBUG
    "babylon.gui.module.d.ts"
    "babylon.module.d.ts"
    "babylonjs.loaders.module.d.ts"
    "babylonjs.materials.module.d.ts")

add_executable(BabylonNativeSample main.cpp ${BABYLON_JS_SCRIPTS})

target_link_libraries(BabylonNativeSample 
    PRIVATE AppRuntime
    PRIVATE NativeEngine
    PRIVATE NativeInput
    PRIVATE NativeOptimizations
    PRIVATE Console
    PRIVATE Window
    PRIVATE ScriptLoader
    PRIVATE XMLHttpRequest
    PRIVATE Canvas
    ${ADDITIONAL_LIBRARIES}
    ${BABYLON_NATIVE_PLAYGROUND_EXTENSION_LIBRARIES})

target_compile_features(BabylonNativeSample PRIVATE cxx_std_17)

foreach(SCRIPT ${BABYLON_JS_SCRIPTS} ${BABYLON_JS_DEBUG})
    get_filename_component(SCRIPT_NAME "${SCRIPT}" NAME)
    add_custom_command(
        OUTPUT "${CMAKE_CFG_INTDIR}/Scripts/${SCRIPT_NAME}"
        COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPT}" "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/Scripts/${SCRIPT_NAME}"
        COMMENT "Copying ${SCRIPT_NAME}"
        MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPT}")
endforeach()