# Voice Controlled Smart Socket (STM32 Keyword Spotting)

## Overview
This project implements a voice-controlled smart socket using an STM32 microcontroller.
Audio is captured in real time via I2S + DMA and buffered for keyword spotting (KWS).
To avoid losing the beginning of a spoken command, the system uses a pre-recording ring buffer (pre-roll), then builds a 16,000-sample inference window, extracts MFCC features (CMSIS-DSP), and runs on-device inference.
Detected commands toggle the socket ON/OFF by transmitting a predefined bit sequence via GPIO with timer-based pulse timing.

## Key Features
- Real-time audio capture using I2S + DMA callbacks
- Adaptive voice activity detection (threshold based)
- Pre-recording ring buffer (pre-roll) to preserve the beginning of spoken commands
- 16,000-sample inference window (pre-roll + post-trigger samples)
- MFCC feature extraction (10 coefficients, frame size 800, step 320)
- On-device KWS inference using STM32 AI middleware
- Socket control via GPIO transmission with precise timing (TIM-based)

## Commands
- "YES" → socket ON
- "NO"  → socket OFF

## Tech Stack
- C/C++
- STM32 HAL (I2S, DMA, UART, TIM, GPIO)
- CMSIS-DSP (MFCC)
- Embedded AI (KWS inference)
