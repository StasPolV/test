cmake_minimum_required(VERSION 3.25)
project(C3DProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies")
set(C3D_MATH_DIR   "${DEPS_DIR}/Modeler")
set(C3D_Vision_DIR "${DEPS_DIR}/Vision")
set(QT_PATH "$ENV{QT_PATH}" CACHE PATH "C:/Qt/5.15.2/msvc2019_64")
if(NOT QT_PATH AND EXISTS "C:/Qt/5.15.2/msvc2019_64")
    set(QT_PATH "C:/Qt/5.15.2/msvc2019_64" CACHE PATH "" FORCE)
endif()
list(PREPEND CMAKE_PREFIX_PATH "${QT_PATH}")

set(C3D_Vision_INCLUDE_DIR "${C3D_Vision_DIR}/Include")

set(C3DVision_GUI_QT5_EXAMPLES ON)
set(C3DVision_USING_UNICODE ON)
set(Examples_StandaloneBuild TRUE)
set(C3D_VIS_LIB c3dvision)
set(QtVision_OUTPUT qtvision)
set(SHARED_DIR "${C3D_Vision_DIR}/Shared")
set(MATH_INCLUDE_TARGET "${C3D_MATH_DIR}/Include")
set(MATH_LIBRARY_TARGET
    optimized "${C3D_MATH_DIR}/Release/c3d.lib"
    debug     "${C3D_MATH_DIR}/Debug/c3d.lib")

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(isMultiConfig)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

add_definitions(-DUNICODE -D_UNICODE -D__USE_QT__ -DQT_NO_DEPRECATED_WARNINGS -DNOMINMAX)
include_directories("${SHARED_DIR}")

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)

list(APPEND CMAKE_MODULE_PATH "${C3D_Vision_DIR}/VisionKit/QtVision/CMake")
add_subdirectory("${C3D_Vision_DIR}/VisionKit" VisionKit)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES})

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /MP /bigobj /utf-8)
    target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<NOT:$<CONFIG:Debug>>:_ITERATOR_DEBUG_LEVEL=0>)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    "${C3D_MATH_DIR}/Include"
    "${C3D_Vision_INCLUDE_DIR}")

target_link_directories(${PROJECT_NAME} PRIVATE
    "${C3D_Vision_DIR}/$<IF:$<CONFIG:Debug>,Debug,Release>")

target_link_libraries(${PROJECT_NAME} PRIVATE
    qtvision
    ${MATH_LIBRARY_TARGET}
    c3dvision
    Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL)


set(_cfg_dir "$<IF:$<CONFIG:Debug>,Debug,Release>")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${C3D_MATH_DIR}/${_cfg_dir}/c3d.dll"
        "${C3D_Vision_DIR}/${_cfg_dir}/c3dvision.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E env --modify "PATH=path_list_prepend:${QT_PATH}/bin"
        "${QT_PATH}/bin/windeployqt.exe" --no-translations --no-compiler-runtime
        $<IF:$<CONFIG:Debug>,--debug,--release>
        "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMENT "Copying C3D and Qt runtime next to ${PROJECT_NAME}.exe")
