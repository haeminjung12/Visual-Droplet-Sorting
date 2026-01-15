# Internal Architecture Notes

This document describes how the C++ pipeline is organized and how data flows.

## Data flow (runtime)
1) DCAM capture (Hamamatsu) produces frames in grayscale.
2) Fast event detector builds/updates a rolling background, thresholds the diff, and gates detections.
3) When an event fires, the crop is squared and passed to the ONNX classifier.
4) If the predicted label matches the target label, the NI-DAQ trigger fires.

The same flow is used in both the headless CLI (`droplet_pipeline_cli`) and the Qt GUI
pipeline (`droplet_pipeline.exe`).

## Key files (cpp_pipeline)
- `main.cpp`  
  Headless pipeline: camera -> fast/precise detection -> ONNX -> NI-DAQ.  
  Builds as `droplet_pipeline_cli.exe` with CLI args for camera settings, detection thresholds,
  and trigger config.

- `fast_event_detector.h/.cpp`  
  Fast detection logic ported from `components/sequence_event_detection.cpp`.  
  Handles background mean, rolling updates, thresholding, morphology, and gating with gap-fire logic.

- `event_detector.h/.cpp`  
  Precise detection path (background subtraction + Otsu + contour filtering).

- `event_gate.h/.cpp`  
  Gating utility for offline tools that use the precise detector.

- `onnx_classifier.h/.cpp`  
  ONNX Runtime wrapper that resizes input, normalizes, and runs inference.

- `metadata_loader.h/.cpp`  
  Minimal JSON parser for classes, input size, and normalization parameters.

- `daq_trigger.h/.cpp`  
  NI-DAQ digital/counter trigger wrapper. Stubs when DAQmx is not available.

- `dcam_camera.h/.cpp`  
  Headless DCAM camera wrapper used by `main.cpp`.

- `models/`  
  Local ONNX models for packaging (e.g., `squeezenet_final_new_condition.onnx`).

## Qt GUI (cpp_pipeline/qt_hama_gui)
- `main.cpp`  
  Live viewer with camera controls, save/record tools, and the Pipeline tab.
  The Pipeline tab loads ONNX + metadata, runs the fast detector, and triggers NI-DAQ.

- `pipeline_runner.h/.cpp`  
  GUI-side pipeline runner: FastEventDetector + OnnxClassifier + DaqTrigger.
  Processes frames from `FrameGrabber` (non-UI thread) and posts status to the UI.

- `dcam_controller.h/.cpp`  
  DCAM device controller for the Qt app (open, apply, lock frame).

- `frame_grabber.h/.cpp`  
  Worker thread that waits for frames and emits them to the UI / pipeline.

- `frame_types.h`  
  Shared structs for camera settings and frame metadata.

- `log_teebuf.h`  
  Tee buffer for logging to both console and GUI log output.

## Components (cpp_pipeline/components)
Standalone utilities for offline testing and dataset generation:
- `sequence_event_detection.cpp`: Batch processing on image sequences (fast/precise modes).
- `sequence_dataset_extractor.cpp`: Extract crops from sequences into a dataset.
- `dataset_background_detection.cpp`: Background/threshold experiments from MATLAB port.
- `sorting_cases_metric.cpp`: Offline metrics for detection gating.
- `onnx_classifier_test.cpp`, `dcam_capture_test.cpp`, `daq_trigger_test.cpp`: module tests.

## Notes
- Fast-mode defaults are aligned with the last `sequence_event_detection` tuning.
- Place `metadata.json` next to the ONNX model or point the GUI/CLI to the correct path.
