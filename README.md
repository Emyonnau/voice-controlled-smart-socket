# Voice Controlled Smart Socket (STM32 Keyword Spotting)

## Overview
This project implements a voice-controlled smart socket using an STM32 microcontroller.
Audio is captured via I2S + DMA, processed on-device with MFCC feature extraction, and classified by a small keyword spotting (KWS) neural network.
Detected commands ("YES" / "NO") toggle the socket ON/OFF by transmitting a predefined bit sequence via GPIO with timer-based pulse timing.

## Key Features
- Real-time audio capture using I2S and DMA callbacks
- Adaptive voice activity detection (threshold based)
- 16,000-sample recording window for inference
- MFCC feature extraction (10 coefficients, frame size 800, step 320)
- On-device KWS inference using STM32 AI middleware
- Socket control via GPIO transmission with precise timing (TIM4)

## Commands
- "YES" → socket ON
- "NO"  → socket OFF

## Tech Stack
- C/C++
- STM32 HAL (I2S, DMA, UART, TIM, GPIO)
- CMSIS-DSP (MFCC)
- Embedded AI (KWS inference)
