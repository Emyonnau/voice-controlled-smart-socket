## Voice-Controlled Smart Socket on STM32

A modular STM32 embedded project for real-time voice-controlled smart socket switching.  
The firmware captures microphone input via I2S + DMA, performs voice activity detection with pre-recording, extracts MFCC features, runs keyword spotting inference, and toggles a wireless smart socket.

---
## Features

- STM32-based embedded firmware architecture
- I2S audio capture with DMA callbacks
- Pre-recording ring buffer
- Adaptive voice activity detection
- MFCC feature extraction
- Keyword spotting inference
- Wireless socket on/off transmission
- UART debug output
- LED-based audio level visualization

---
## Architecture Overview

The project is organized into modular components following a typical STM32 firmware structure. Each module has a clearly defined responsibility, improving readability and maintainability.

- **main.c**  
  Entry point of the STM32 application. Initializes HAL, configures peripherals, and runs the main loop.

- **app.cpp**  
  High-level orchestration layer. Coordinates interaction between audio capture, signal processing, inference, and socket control.

- **audio_capture.cpp**  
  Handles I2S audio acquisition using DMA. Implements HAL callbacks (`HAL_I2S_RxHalfCpltCallback`, `HAL_I2S_RxCpltCallback`) and converts incoming data into usable PCM frames.

- **vad_preroll.cpp**  
  Implements voice activity detection (VAD) and manages a pre-recording ring buffer. Detects speech based on adaptive thresholds and prepares audio windows for processing.

- **kws_service.cpp**  
  Performs MFCC feature extraction and runs keyword spotting (KWS) inference. Maps recognized keywords to actions.

- **socket_control.cpp**  
  Controls the wireless socket using GPIO and timing signals. Encodes and transmits control sequences to switch the socket ON/OFF.

- **debug_uart.c**  
  Provides UART-based debug output and LED-based volume visualization for runtime feedback.

- **App/AI/**  
  Contains DSP and AI-related modules:
  - `mfcc.*` for feature extraction
  - `kws.*` for inference logic

---

## Signal Flow

1. Audio is captured from the microphone using **I2S with DMA**
2. Samples are converted into PCM frames
3. Voice activity detection determines when speech is present
4. A pre-recorded buffer is combined with the detected speech segment
5. MFCC features are extracted from the audio window
6. Keyword spotting inference is executed
7. Based on the recognized command, the smart socket is switched ON or OFF

---

## Notes

This repository follows a modular STM32 firmware design with a clear separation between:
- hardware-specific code (HAL, DMA, GPIO, I2S)
- signal processing (VAD, MFCC)
- inference logic (KWS)
- application logic (socket control)
- debugging utilities (UART, LEDs)

The code was refactored to improve structure and maintainability while preserving the original logic of the project.

---

## Future Improvements

- Integrate full STM32CubeMX-generated configuration files
- Add build instructions for STM32CubeIDE and arm-none-eabi toolchain
- Replace hardcoded keyword strings with configurable mappings
- Improve separation between DSP preprocessing and inference backend
- Add documentation for hardware setup and pin configuration
