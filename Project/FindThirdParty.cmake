set(ThirdParty_PREBUILT_DIR ${Kaleido3D_SOURCE_DIR}/Source/Window_DEPS)
execute_process(COMMAND 
${PYTHON_EXECUTABLE} 
${Kaleido3D_SOURCE_DIR}/Tools/Build/Windows/get_dependencies.py
${ThirdParty_PREBUILT_DIR}
)
message(STATUS "** 3rd party ** ${ThirdParty_PREBUILT_DIR}")

unset(THIRDPARTY_FOUND CACHE)

unset(VULKANSDK_INCLUDE_DIR CACHE)
unset(RAPIDJSON_INCLUDE_DIR CACHE)
unset(GLSLANG_INCLUDE_DIR CACHE)
unset(SPIRV2CROSS_INCLUDE_DIR CACHE)
unset(FREETYPE2_INCLUDE_DIR CACHE)
unset(STEAMSDK_INCLUDE_DIR CACHE)

set(FREETYPE2_LIBRARY freetype)

macro(_imported_khr_target_properties TNAME Configuration IMPLIB_LOCATION)
    set_property(TARGET KHR::${TNAME} APPEND PROPERTY IMPORTED_CONFIGURATIONS ${Configuration})
    if(NOT "${IMPLIB_LOCATION}" STREQUAL "")
        set_target_properties(KHR::${TNAME} PROPERTIES
        "IMPORTED_IMPLIB_${Configuration}" "${ThirdParty_PREBUILT_DIR}/lib/${IMPLIB_LOCATION}"
        )
    endif()
endmacro()

macro(_imported_khr_target TNAME)
    add_library(KHR::${TNAME} SHARED IMPORTED)
    if(WIN32)
        _imported_khr_target_properties(${TNAME} RELEASE "${TNAME}.lib" )
        _imported_khr_target_properties(${TNAME} DEBUG "${TNAME}d.lib")
    endif()
endmacro()

_imported_khr_target(glslang)
_imported_khr_target(HLSL)
_imported_khr_target(OGLCompiler)
_imported_khr_target(OSDependent)
_imported_khr_target(SPIRV)
_imported_khr_target(SPVRemapper)

set(GLSLANG_LIBRARIES KHR::glslang KHR::HLSL KHR::OGLCompiler KHR::OSDependent KHR::SPIRV KHR::SPVRemapper)

set(SPIRV2CROSS_LIBRARY spirv-cross-core spirv-cross-msl spirv-cross-glsl)
set(STEAMSDK_LIBRARY steam_api64)

if(WIN32)
    unset(DXSDK_INCLUDE_DIR CACHE)
endif()

if(WIN32 OR ANDROID)
find_path(VULKANSDK_INCLUDE_DIR
    vulkan/vulkan.h
    PATH_SUFFIXES include
    PATHS ${ThirdParty_PREBUILT_DIR}
)
endif()

if(WIN32)
find_library(VULKAN_LIB vulkan-1
	PATH_SUFFIXES lib
	PATHS ${ThirdParty_PREBUILT_DIR})
message(STATUS "** Vulkan \(Windows\) ** ${VULKAN_LIB}")
elseif(ANDROID)
find_library(VULKAN_LIB vulkan
	PATH_SUFFIXES platforms/android-24/arch-${ANDROID_SYSROOT_ABI}/usr/lib
	PATHS $ENV{ANDROID_NDK})
message(STATUS "** Vulkan Lib \(Android\) ** ${VULKAN_LIB}")
elseif(UNIX)
    find_library(VULKAN_LIB libvulkan.so.1
            PATHS
            /usr/lib/x86_64-linux-gnu
            /usr/lib/x86-linux-gnu)
    message(STATUS "** Vulkan \(Linux\) ** ${VULKAN_LIB}")
endif()

find_path(RAPIDJSON_INCLUDE_DIR
    rapidjson/rapidjson.h
    PATH_SUFFIXES include
    PATHS ${ThirdParty_PREBUILT_DIR}
)

find_path(GLSLANG_INCLUDE_DIR
    SPIRV/GlslangToSpv.h
    PATH_SUFFIXES include
    PATHS ${ThirdParty_PREBUILT_DIR}
)

find_path(SPIRV2CROSS_INCLUDE_DIR
    spirv_cross/spirv.hpp
    PATH_SUFFIXES include
    PATHS ${ThirdParty_PREBUILT_DIR}
)

find_path(FREETYPE2_INCLUDE_DIR
    ft2build.h
    PATH_SUFFIXES include/freetype2
    PATHS ${ThirdParty_PREBUILT_DIR}
)

find_library(FREETYPE2_LIBRARY
	NAMES freetype
	HINTS
	$ENV{FREETYPE2_DIR}
	PATH_SUFFIXES lib
	PATHS
	${ThirdParty_PREBUILT_DIR}
)

if(FREETYPE2_LIBRARY)
# Find dependencies
    foreach (d ZLIB BZip2 PNG HarfBuzz)
        string(TOUPPER "${d}" D)

        if (DEFINED WITH_${d} OR DEFINED WITH_${D})
            if (WITH_${d} OR WITH_${D})
            find_package(${d} QUIET REQUIRED)
            endif ()
        else ()
            find_package(${d} QUIET)
        endif ()

        if (${d}_FOUND OR ${D}_FOUND)
            message(STATUS "link FreeType with ${d}")
        endif ()
    endforeach ()

	include_directories(${FREETYPE2_INCLUDE_DIR})
	set(HAS_FREETYPE true)

    if (ZLIB_FOUND)
        list(APPEND FREETYPE2_LIBRARY ${ZLIB_LIBRARIES})
    endif ()
    if (BZIP2_FOUND)
        list(APPEND FREETYPE2_LIBRARY ${BZIP2_LIBRARIES})
    endif ()
    if (PNG_FOUND)
        list(APPEND FREETYPE2_LIBRARY ${PNG_LIBRARIES})
    endif ()
    if (HARFBUZZ_FOUND)
        list(APPEND FREETYPE2_LIBRARY ${HARFBUZZ_LIBRARIES})
    endif ()
endif()

message(STATUS "GLSLang = ${GLSLANG_INCLUDE_DIR}")

if(WIN32)
	find_path(DXSDK_INCLUDE_DIR
	    d3d12.h
	    PATH_SUFFIXES include
	    PATHS ${ThirdParty_PREBUILT_DIR}
	)
    find_path(STEAMSDK_INCLUDE_DIR
        steam/steam_api.h
        PATH_SUFFIES include
        PATHS ${ThirdParty_PREBUILT_DIR}
    )
    message(STATUS "STEAMSDK = ${STEAMSDK_INCLUDE_DIR}")
    find_library(STEAMSDK_LIBRARY
        NAMES steam_api64 steam_api
        PATH_SUFFIXES lib
        PATHS
        ${ThirdParty_PREBUILT_DIR}
    )
    find_path(V8_INCLUDE_DIR
        v8.h
        PATH_SUFFIXES include
	    PATHS ${ThirdParty_PREBUILT_DIR}
    )
    message(STATUS "V8_INCLUDE_DIR = ${V8_INCLUDE_DIR}")
    if(V8_INCLUDE_DIR)
        set(V8_VERSION 0)
        file(READ "${V8_INCLUDE_DIR}/v8-version.h" _V8_VERSION_CONTENTS)
        string(REGEX REPLACE ".*#define V8_MAJOR_VERSION ([0-9]+).*" "\\1" V8_MAJOR_VERSION "${_V8_VERSION_CONTENTS}")
        set(V8_MAJOR_VERSION "${V8_MAJOR_VERSION}")
        string(REGEX REPLACE ".*#define V8_MINOR_VERSION ([0-9]+).*" "\\1" V8_MINOR_VERSION "${_V8_VERSION_CONTENTS}")
        set(V8_MINOR_VERSION "${V8_MINOR_VERSION}")
        string(REGEX REPLACE ".*#define V8_BUILD_NUMBER ([0-9]+).*" "\\1" V8_BUILD_NUMBER "${_V8_VERSION_CONTENTS}")
        set(V8_BUILD_NUMBER "${V8_BUILD_NUMBER}")
        set(V8_VERSION "${V8_MAJOR_VERSION}.${V8_MINOR_VERSION}.${V8_BUILD_NUMBER}")
        message(STATUS "V8_VERSION = ${V8_VERSION}")
        set(V8_LIBRARIES icui18n.dll.lib icuuc.dll.lib v8_libbase.dll.lib v8_libplatform.dll.lib v8.dll.lib)
    endif()
endif()

mark_as_advanced(VULKANSDK_INCLUDE_DIR)
mark_as_advanced(RAPIDJSON_INCLUDE_DIR)
mark_as_advanced(GLSLANG_INCLUDE_DIR)
mark_as_advanced(SPIRV2CROSS_INCLUDE_DIR)
mark_as_advanced(FREETYPE2_INCLUDE_DIR)
mark_as_advanced(V8_INCLUDE_DIR)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")

	#find_library(LUA_LIBRARY lua51.lib
    #	HINTS ${_LUA_LIBRARY_DIR}
    #	PATHS ENV LIBRARY_PATH ENV LD_LIBRARY_PATH)

endif()

include_directories(${ThirdParty_PREBUILT_DIR}/include)
link_directories(${ThirdParty_PREBUILT_DIR}/lib)

include(FindPackageHandleStandardArgs)

if(WIN32 OR ANDROID)
find_package_handle_standard_args(ThirdParty DEFAULT_MSG
    VULKANSDK_INCLUDE_DIR
    RAPIDJSON_INCLUDE_DIR
    GLSLANG_INCLUDE_DIR
    SPIRV2CROSS_INCLUDE_DIR
    FREETYPE2_INCLUDE_DIR
    STEAMSDK_INCLUDE_DIR
    V8_INCLUDE_DIR
)
mark_as_advanced(
    VULKANSDK_INCLUDE_DIR
    RAPIDJSON_INCLUDE_DIR
    GLSLANG_INCLUDE_DIR
    SPIRV2CROSS_INCLUDE_DIR
    FREETYPE2_INCLUDE_DIR
    STEAMSDK_INCLUDE_DIR
    V8_INCLUDE_DIR
)
elseif(IOS OR MACOS)
find_package_handle_standard_args(ThirdParty DEFAULT_MSG
    RAPIDJSON_INCLUDE_DIR
    GLSLANG_INCLUDE_DIR
    SPIRV2CROSS_INCLUDE_DIR
    FREETYPE2_INCLUDE_DIR
)
mark_as_advanced(
    RAPIDJSON_INCLUDE_DIR
    GLSLANG_INCLUDE_DIR
    SPIRV2CROSS_INCLUDE_DIR
    FREETYPE2_INCLUDE_DIR
)
endif()
