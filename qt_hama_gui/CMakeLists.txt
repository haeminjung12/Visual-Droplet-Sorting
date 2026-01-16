cmake_minimum_required(VERSION 3.19)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(VCPKG_ROOT "C:/vcpkg" CACHE PATH "Path to vcpkg")
  if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if (EXISTS "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
      set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    endif()
  endif()
  project(qt_hama_gui LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(DCAM_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../python_pipeline/Hamamatsu_DCAMSDK4_v25056964/dcamsdk4" CACHE PATH "Path to Hamamatsu DCAM SDK (dcamsdk4)")
set(ONNXRUNTIME_DIR "" CACHE PATH "Path to ONNX Runtime")
set(NIDAQMX_DIR "" CACHE PATH "Path to NI-DAQmx (optional)")
option(ENABLE_NIDAQMX "Enable NI-DAQmx trigger support" ON)
option(ENABLE_DCAM "Enable Hamamatsu DCAM camera support" ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
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

set(GUI_CAMERA_SOURCES
  dcam_controller_stub.cpp
  frame_grabber_stub.cpp
)
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
    set(GUI_CAMERA_SOURCES
      dcam_controller.cpp
      frame_grabber.cpp
    )
    set(DCAM_FOUND ON)
  endif()
endif()

add_executable(qt_hama_gui
  main.cpp
  dcam_controller.h
  frame_grabber.h
  frame_types.h
  ${GUI_CAMERA_SOURCES}
  pipeline_runner.cpp
  pipeline_runner.h
  ../daq_trigger.cpp
  ../fast_event_detector.cpp
  ../metadata_loader.cpp
  ../onnx_classifier.cpp
)

if (WIN32)
  set_target_properties(qt_hama_gui PROPERTIES WIN32_EXECUTABLE ON)
  if (TARGET Qt6::EntryPoint)
    target_link_libraries(qt_hama_gui PRIVATE Qt6::EntryPoint)
  elseif (TARGET Qt6::WinMain)
    target_link_libraries(qt_hama_gui PRIVATE Qt6::WinMain)
  endif()
endif()

set_target_properties(qt_hama_gui PROPERTIES
  OUTPUT_NAME droplet_pipeline
)

target_include_directories(qt_hama_gui PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${ONNXRUNTIME_INCLUDE_DIR}
  ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(qt_hama_gui PRIVATE
  Qt6::Widgets
  ${ONNXRUNTIME_LIB}
  ${OpenCV_LIBS}
)

if (DCAM_FOUND)
  target_compile_definitions(qt_hama_gui PRIVATE HAVE_DCAM=1)
  target_include_directories(qt_hama_gui PRIVATE ${DCAM_INCLUDE_DIR_FOUND})
  target_link_libraries(qt_hama_gui PRIVATE ${DCAM_LIBRARY})
else()
  target_compile_definitions(qt_hama_gui PRIVATE HAVE_DCAM=0)
  message(WARNING "DCAM SDK not found; building GUI without camera support.")
endif()

if (ONNXRUNTIME_DIR)
  set(ONNX_DLLS
    "${ONNXRUNTIME_DIR}/lib/onnxruntime.dll"
    "${ONNXRUNTIME_DIR}/lib/onnxruntime_providers_shared.dll"
    "${ONNXRUNTIME_DIR}/lib/onnxruntime_providers_cuda.dll"
    "${ONNXRUNTIME_DIR}/lib/onnxruntime_providers_tensorrt.dll"
  )
  foreach(onnx_dll IN LISTS ONNX_DLLS)
    if (EXISTS "${onnx_dll}")
      add_custom_command(TARGET qt_hama_gui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${onnx_dll}" $<TARGET_FILE_DIR:qt_hama_gui>
      )
    endif()
  endforeach()
endif()

if (ENABLE_NIDAQMX)
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
    target_compile_definitions(qt_hama_gui PRIVATE HAVE_NIDAQMX=1)
    target_include_directories(qt_hama_gui PRIVATE ${NIDAQMX_INCLUDE_DIR})
    target_link_libraries(qt_hama_gui PRIVATE ${NIDAQMX_LIBRARY})
  else()
    message(WARNING "NI-DAQmx not found; building with stub trigger.")
  endif()
endif()
