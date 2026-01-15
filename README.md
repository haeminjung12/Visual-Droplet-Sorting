Droplet pipeline (C++)

This folder contains the GUI-first pipeline and an optional headless CLI:
DCAM capture -> event detection -> ONNX inference -> NI-DAQ trigger.

Dependencies
- Hamamatsu DCAM SDK (dcamapi)
- OpenCV (core, imgproc, imgcodecs)
- ONNX Runtime
- NI-DAQmx (optional, for trigger output)

Build (PowerShell)
1) Configure paths:
   - DCAM SDK: default is ../python_pipeline/Hamamatsu_DCAMSDK4_v25056964/dcamsdk4
   - ONNX Runtime: set ONNXRUNTIME_DIR
   - NI-DAQmx: set NIDAQMX_DIR (optional)

2) Configure and build:
   cmake -S . -B build -D ONNXRUNTIME_DIR="C:/onnxruntime" -D DCAM_SDK_DIR="C:/path/to/dcamsdk4" -D NIDAQMX_DIR="C:/Program Files (x86)/National Instruments/NI-DAQ/DAQmx ANSI C Dev"
   cmake --build build --config Release

   Optional: add `-DBUILD_CLI=OFF` to skip building the headless CLI.

Run (CLI example)
  build/Release/droplet_pipeline_cli.exe ^
    --onnx "C:/path/to/squeezenet_final_new_condition.onnx" ^
    --metadata "C:/path/to/metadata.json" ^
    --width 1152 --height 1152 --binning 1 --bits 12 --exposure-ms 5 ^
    --bg-frames 50 --min-area 40 --max-area-frac 0.10 --margin 5 --sigma 1.0 ^
    --target-label Single ^
    --trigger-mode counter --daq-device Dev1 --daq-counter ctr0 --pulse-high-ms 2.0 --pulse-low-ms 2.0 ^
    --output-dir output_events

Run (GUI)
  build/qt_hama_gui/Release/droplet_pipeline.exe

Notes
- A default model is stored in `cpp_pipeline/models` for packaging.
- Fast mode uses the updated sequence-detector background subtraction + blob gating logic.
- Precise mode uses background subtraction + Otsu threshold + blob filtering.
- Metadata JSON is parsed for classes, input_size, and normalization mean/std.
- If NI-DAQmx is not found at build time, trigger calls are stubbed and log to stdout.

Component builds
Use the separate CMake file in `cpp_pipeline/components` to test modules independently.

Build components:
  cmake -S components -B build_components -D ONNXRUNTIME_DIR="C:/onnxruntime" -D DCAM_SDK_DIR="C:/path/to/dcamsdk4"
  cmake --build build_components --config Release

Executables:
- dcam_capture_test
- event_detector_test
- onnx_classifier_test
- daq_trigger_test

Qt GUI
The Qt live view app builds as `droplet_pipeline.exe` and is the main entry point.
It lives under `cpp_pipeline/qt_hama_gui` and builds from the main `cpp_pipeline`
CMake (set `-DBUILD_QT_GUI=ON`) or standalone from `cpp_pipeline/qt_hama_gui`.
