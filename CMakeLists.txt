cmake_minimum_required(VERSION 3.19)

set(VCPKG_ROOT "C:/vcpkg" CACHE PATH "Path to vcpkg")
if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if (EXISTS "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
  endif()
endif()

project(droplet_pipeline LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DCAM_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../python_pipeline/Hamamatsu_DCAMSDK4_v25056964/dcamsdk4" CACHE PATH "Path to Hamamatsu DCAM SDK (dcamsdk4)")
set(ONNXRUNTIME_DIR "" CACHE PATH "Path to ONNX Runtime")
set(NIDAQMX_DIR "" CACHE PATH "Path to NI-DAQmx (optional)")

option(ENABLE_NIDAQMX "Enable NI-DAQmx trigger support" ON)
option(ENABLE_DCAM "Enable Hamamatsu DCAM camera support" ON)
option(BUILD_QT_GUI "Build Qt GUI app" ON)
option(BUILD_CLI "Build headless CLI app" OFF)

if (BUILD_QT_GUI AND BUILD_CLI)
  message(FATAL_ERROR "Only one executable can be built at a time. Set either BUILD_QT_GUI=ON BUILD_CLI=OFF (GUI) or BUILD_QT_GUI=OFF BUILD_CLI=ON (CLI).")
endif()
if (NOT BUILD_QT_GUI AND NOT BUILD_CLI)
  message(FATAL_ERROR "No executable selected. Set either BUILD_QT_GUI=ON BUILD_CLI=OFF (GUI) or BUILD_QT_GUI=OFF BUILD_CLI=ON (CLI).")
endif()

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)

if (ONNXRUNTIME_DIR)
  if (NOT ONNXRUNTIME_INCLUDE_DIR)
    find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h PATHS "${ONNXRUNTIME_DIR}/include" NO_DEFAULT_PATH)
    if (NOT ONNXRUNTIME_INCLUDE_DIR)
      find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h PATHS "${ONNXRUNTIME_DIR}/include/onnxruntime" NO_DEFAULT_PATH)
    endif()
  endif()
  if (NOT ONNXRUNTIME_LIB)
    find_library(ONNXRUNTIME_LIB_RELEASE onnxruntime PATHS "${ONNXRUNTIME_DIR}/lib" "${ONNXRUNTIME_DIR}/lib64" NO_DEFAULT_PATH)
    find_library(ONNXRUNTIME_LIB_DEBUG onnxruntime PATHS "${ONNXRUNTIME_DIR}/debug/lib" "${ONNXRUNTIME_DIR}/debug/lib64" NO_DEFAULT_PATH)
    if (ONNXRUNTIME_LIB_RELEASE AND ONNXRUNTIME_LIB_DEBUG)
      set(ONNXRUNTIME_LIB optimized ${ONNXRUNTIME_LIB_RELEASE} debug ${ONNXRUNTIME_LIB_DEBUG})
    elseif(ONNXRUNTIME_LIB_RELEASE)
      set(ONNXRUNTIME_LIB ${ONNXRUNTIME_LIB_RELEASE})
    elseif(ONNXRUNTIME_LIB_DEBUG)
      set(ONNXRUNTIME_LIB ${ONNXRUNTIME_LIB_DEBUG})
    endif()
  endif()
else()
  if (NOT ONNXRUNTIME_INCLUDE_DIR)
    find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h)
    if (NOT ONNXRUNTIME_INCLUDE_DIR)
      find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h PATH_SUFFIXES onnxruntime)
    endif()
  endif()
  if (NOT ONNXRUNTIME_LIB)
    find_library(ONNXRUNTIME_LIB_RELEASE onnxruntime PATH_SUFFIXES lib)
    find_library(ONNXRUNTIME_LIB_DEBUG onnxruntime PATH_SUFFIXES debug/lib)
    if (ONNXRUNTIME_LIB_RELEASE AND ONNXRUNTIME_LIB_DEBUG)
      set(ONNXRUNTIME_LIB optimized ${ONNXRUNTIME_LIB_RELEASE} debug ${ONNXRUNTIME_LIB_DEBUG})
    elseif(ONNXRUNTIME_LIB_RELEASE)
      set(ONNXRUNTIME_LIB ${ONNXRUNTIME_LIB_RELEASE})
    elseif(ONNXRUNTIME_LIB_DEBUG)
      set(ONNXRUNTIME_LIB ${ONNXRUNTIME_LIB_DEBUG})
    endif()
  endif()
endif()
if (NOT ONNXRUNTIME_INCLUDE_DIR OR NOT ONNXRUNTIME_LIB)
  message(FATAL_ERROR "ONNX Runtime not found. Set ONNXRUNTIME_DIR to a valid install path.")
endif()

set(DCAM_INCLUDE_DIR "${DCAM_SDK_DIR}/inc")
set(DCAM_LIB_DIR "${DCAM_SDK_DIR}/lib/win64")

if (BUILD_CLI)
  set(CLI_CAMERA_SOURCE dcam_camera_stub.cpp)
  set(DCAM_FOUND OFF)
  if (ENABLE_DCAM)
    find_path(DCAM_INCLUDE_DIR_FOUND dcamapi4.h
      PATHS
        "${DCAM_SDK_DIR}/inc"
      NO_DEFAULT_PATH
    )
    find_library(DCAM_LIBRARY NAMES dcamapi
      PATHS
        "${DCAM_SDK_DIR}/lib/win64"
      NO_DEFAULT_PATH
    )
    if (DCAM_INCLUDE_DIR_FOUND AND DCAM_LIBRARY)
      set(CLI_CAMERA_SOURCE dcam_camera.cpp)
      set(DCAM_FOUND ON)
    endif()
  endif()

  add_executable(droplet_pipeline_cli
    main.cpp
    ${CLI_CAMERA_SOURCE}
    fast_event_detector.cpp
    event_detector.cpp
    metadata_loader.cpp
    onnx_classifier.cpp
    daq_trigger.cpp
  )

  target_include_directories(droplet_pipeline_cli PRIVATE
    ${ONNXRUNTIME_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_link_libraries(droplet_pipeline_cli PRIVATE
    ${ONNXRUNTIME_LIB}
    ${OpenCV_LIBS}
  )

  if (DCAM_FOUND)
    target_compile_definitions(droplet_pipeline_cli PRIVATE HAVE_DCAM=1)
    target_include_directories(droplet_pipeline_cli PRIVATE ${DCAM_INCLUDE_DIR_FOUND})
    target_link_libraries(droplet_pipeline_cli PRIVATE ${DCAM_LIBRARY})
  else()
    target_compile_definitions(droplet_pipeline_cli PRIVATE HAVE_DCAM=0)
    message(WARNING "DCAM SDK not found; building CLI without camera support.")
  endif()
endif()

if (ONNXRUNTIME_DIR AND BUILD_CLI)
  set(ONNX_DLLS
    "${ONNXRUNTIME_DIR}/lib/onnxruntime.dll"
    "${ONNXRUNTIME_DIR}/lib/onnxruntime_providers_shared.dll"
    "${ONNXRUNTIME_DIR}/lib/onnxruntime_providers_cuda.dll"
    "${ONNXRUNTIME_DIR}/lib/onnxruntime_providers_tensorrt.dll"
  )
  foreach(onnx_dll IN LISTS ONNX_DLLS)
    if (EXISTS "${onnx_dll}")
      add_custom_command(TARGET droplet_pipeline_cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${onnx_dll}" $<TARGET_FILE_DIR:droplet_pipeline_cli>
      )
    endif()
  endforeach()
endif()

if (ENABLE_NIDAQMX AND BUILD_CLI)
  find_path(NIDAQMX_INCLUDE_DIR NIDAQmx.h
    PATHS
      "${NIDAQMX_DIR}/include"
      "C:/Program Files/National Instruments/NI-DAQ/DAQmx ANSI C Dev/include"
      "C:/Program Files (x86)/National Instruments/NI-DAQ/DAQmx ANSI C Dev/include"
  )
  find_library(NIDAQMX_LIBRARY NAMES NIDAQmx
    PATHS
      "${NIDAQMX_DIR}/lib/x64/msvc"
      "${NIDAQMX_DIR}/lib/msvc"
      "C:/Program Files/National Instruments/NI-DAQ/DAQmx ANSI C Dev/lib/msvc"
      "C:/Program Files (x86)/National Instruments/Shared/ExternalCompilerSupport/C/lib64/msvc"
      "C:/Program Files (x86)/National Instruments/NI-DAQ/DAQmx ANSI C Dev/lib/x64/msvc"
      "C:/Program Files (x86)/National Instruments/NI-DAQ/DAQmx ANSI C Dev/lib/msvc"
  )
  if (NIDAQMX_INCLUDE_DIR AND NIDAQMX_LIBRARY)
    target_compile_definitions(droplet_pipeline_cli PRIVATE HAVE_NIDAQMX=1)
    target_include_directories(droplet_pipeline_cli PRIVATE ${NIDAQMX_INCLUDE_DIR})
    target_link_libraries(droplet_pipeline_cli PRIVATE ${NIDAQMX_LIBRARY})
  else()
    message(WARNING "NI-DAQmx not found; building with stub trigger.")
  endif()
endif()

if (BUILD_QT_GUI)
  add_subdirectory(qt_hama_gui)
endif()
