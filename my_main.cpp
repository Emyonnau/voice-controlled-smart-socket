/*
 * my_main.cpp
 *
 *  Created on: Jan 17, 2025
 *      Author: kerim
 */
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "eth.h"
#include "i2s.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "my_main.h"
#include "arm_math.h"        // Add FFT library
#include "arm_const_structs.h"
#include "arm_common_tables.h"
#include "mfcc.h"
#include "kws.h"
#include <cstring>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define I2S_BUF_SIZE 1000
#define ARRAY_SIZE 16000
#define THRESHOLD 10000
#define SOCKET_ON 0b111111000010
#define SOCKET_OFF 0b111111000001
#define MFCC_IN_SIZE 800
#define MFCC_STEP_SIZE 320
#define MFCC_OUT_SIZE 10

#define PRE_RECORD_SIZE 4000

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
TIM_HandleTypeDef* htim4__;
I2S_HandleTypeDef* hi2s2__;
UART_HandleTypeDef* huart3__;

uint16_t input_buffer[I2S_BUF_SIZE*2];
int32_t mergedFrame[I2S_BUF_SIZE/2];
int ready = 0;
int32_t array[16000];
unsigned int idx = 0;

volatile uint8_t socket_state = 0;      // 0: off, 1: on
float mfccInputFrame[MFCC_IN_SIZE]; // input frame of mfcc
q7_t mfccOutTmp[MFCC_OUT_SIZE]; // result of mfcc
float mfccOut[MFCC_OUT_SIZE]; // result after typecast
MFCC* mfcc = nullptr;
KWS* kws = nullptr;
int array_ready = 0;
int start = 0;
int pre_recording = 1;
int32_t ring_buffer[PRE_RECORD_SIZE];
unsigned int pre_record_idx = 0;



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void delay350Microseconds(uint32_t multiplier);
void transmit(uint32_t   numberHighPulses, uint32_t  numberLowPulses);
void sendSequence(uint16_t bitSequence);
void process_audio_data(uint16_t *inputBuffer, int32_t *mergedFrame, uint16_t buffer_start, uint16_t merged_start, uint16_t length);
void merged_frame_output(int32_t *mergedFrame, uint16_t frame_start, uint16_t frame_length);
void aufzeichnen(int half);
int __io_putchar(int ch);
int _write(int file, char *ptr, int len);
void DisplayVolume(int index);
uint8_t mapVolumeToLEDs(uint32_t averageVolume);
void ControlLEDs(uint8_t numLEDs);
void toggle_socket(uint8_t state);
int32_t averageVal();
void extract_and_infer_mfccs();

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void ControlLEDs(uint8_t numLEDs) {
    // Alle Pins zunächst ausschalten
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_2, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_RESET);

    // LEDs abhängig von der übergebenen Anzahl einschalten
    if (numLEDs >= 1) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET); // Erste LED
    if (numLEDs >= 2) HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET); // Zweite LED
    if (numLEDs >= 3) HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET); // Dritte LED
    if (numLEDs >= 4) HAL_GPIO_WritePin(GPIOF, GPIO_PIN_3, GPIO_PIN_SET); // Vierte LED
    if (numLEDs >= 5) HAL_GPIO_WritePin(GPIOF, GPIO_PIN_5, GPIO_PIN_SET);
    if (numLEDs >= 6) HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);
    if (numLEDs >= 7) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);
    if (numLEDs >= 8) HAL_GPIO_WritePin(GPIOF, GPIO_PIN_2, GPIO_PIN_SET);
    if (numLEDs >= 9) HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, GPIO_PIN_SET);
    if (numLEDs >= 10) HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_SET);
}

void UART_SendString(char *string)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)string, strlen(string), HAL_MAX_DELAY);
}

uint8_t mapVolumeToLEDs(uint32_t averageVolume) {
    if (averageVolume <= 5000) {
        return 1; // Light up 1 LED
    } else if (averageVolume <= 10000) {
        return 2; // Light up 2 LEDs
    } else if (averageVolume <= 20000) {
        return 3; // Light up 3 LEDs
    } else if (averageVolume <= 40000) {
        return 4; // Light up 4 LEDs
    } else if (averageVolume <= 60000) {
        return 5; // Light up 5 LEDs
    } else if (averageVolume <= 80000) {
        return 6; // Light up 6 LEDs
    } else if (averageVolume <= 100000) {
        return 7; // Light up 7 LEDs
    } else {
        return 8; // Light up all 8 LEDs
    }
}

// Function to calculate the volume and display it on LEDs
void DisplayVolume(int index) {
    // Calculate the average volume
    int32_t volumeVal = mergedFrame[index];

    // Map the average volume to LED levels
    uint8_t volumeLevel = mapVolumeToLEDs(volumeVal);

    // Display LEDs based on the mapped volume level
    ControlLEDs(volumeLevel);
}
int32_t averageVal() {
	if(array_ready == 1) {
		int32_t sum = 0;
		for(int i = 0; i < ARRAY_SIZE - 1; i++) {
			sum += abs(array[i]);
		}
		return sum / ARRAY_SIZE;
	}
	else {
		return 0;
	}
}

void toggle_socket(uint8_t state) {
    if (state == 1) {
        sendSequence(SOCKET_ON);
        socket_state = 1;
    } else {
        sendSequence(SOCKET_OFF);
        socket_state = 0;
    }
}

void extract_and_infer_mfccs()
{
	if(array_ready == 1) {
// 	MFCC* mfcc = new MFCC(MFCC_OUT_SIZE, MFCC_IN_SIZE, 2);
//	KWS* kws = new KWS();
		array_ready=0;

	float mfccInputFrame[MFCC_IN_SIZE]; // input frame of mfcc
	q7_t mfccOutTmp[MFCC_OUT_SIZE]; // result of mfcc
	float mfccOut[MFCC_OUT_SIZE]; // result after typecast

	// Feature extraction
printf("Feature extraction started\r\n");

int feature_counter = 0;
for (int i = 0; i <= ARRAY_SIZE - MFCC_IN_SIZE; i += MFCC_STEP_SIZE, feature_counter++) {

    // 1) Fill MFCC input frame
    for (int j = 0; j < MFCC_IN_SIZE; j++) {
        mfccInputFrame[j] = (float)array[i + j];
    }

    // 2) Compute MFCC for this frame  ✅ moved inside the loop
    mfcc->mfcc_compute(mfccInputFrame, mfccOutTmp);

    // 3) Store into KWS input tensor
    for (int k = 0; k < MFCC_OUT_SIZE; k++) {
        kws->mMFCC[feature_counter][k] = (ai_float)mfccOutTmp[k];
    }
}

printf("Feature extraction ended\r\n");
//	printf("Feature extraction ended\r\n");
//	for (int i = 0; i < MFCC_OUT_SIZE; ++i)
//	{
//		mfccOut[i] = static_cast<float>(mfccOutTmp[i]);
//////		printf("value of mfccOut[%d]: %f\n", i, mfccOut[i]);
//	}
	int index2=kws->runInference();

	std::string word=kws->indexToWord(index2);
	delete kws;
	delete mfcc;
	char *str = (char*)word.data();
	printf("%s\n",str);

	if (word == "YES")
	{
		toggle_socket(1);
	}
	else if (word == "TWO")
	{
		toggle_socket(0);
	}
	}

}


/*void extract_and_infer_mfccs(void) {
		if (array_ready == 1) {
			array_ready = 0; // Reset the array_ready flag

			printf("Feature extraction started\r\n");
			int feature_counter = 0;
			for (int i = 0; i < ARRAY_SIZE - MFCC_STEP_SIZE; i += MFCC_STEP_SIZE, feature_counter++) {
				for (int j = 0; j < MFCC_IN_SIZE; j++) {
					if (i + j >= ARRAY_SIZE)
						mfccInputFrame[j] = 0;
					else {
						 mfccInputFrame[j] = (float)array[i + j];
					}
				}
				mfcc->mfcc_compute(mfccInputFrame, mfccOutTmp);
				for (int k = 0; k < MFCC_OUT_SIZE; k++) {
					kws->mMFCC[feature_counter][k] = (ai_float)mfccOutTmp[k];
				}


			}

			printf("Feature extraction ended\r\n");

			// Run inference
			kws->getAiInput()[0].data = AI_HANDLE_PTR(kws->mMFCC);
			kws->getAiOutput()[0].data = AI_HANDLE_PTR(mfccOutTmp);

			if (ai_network_run(kws->getNetwork(), kws->getAiInput(), kws->getAiOutput())) {
				int result = kws->runInference();
				printf("Detected Keyword: %s\r\n", kws->indexToWord(result).c_str());
				if (result == 0 && socket_state == 0) {
					toggle_socket(1); // Turn on the socket for "YES"
				} else if (result == 1 && socket_state == 1) {
					toggle_socket(0); // Turn off the socket for "NO"
				}
			}
			else {
				printf("Inference Error\r\n");
		}
	   }
	}*/

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
void my_main(I2S_HandleTypeDef* hi2s2,TIM_HandleTypeDef* htim4, UART_HandleTypeDef* huart3)
{
	huart3__= huart3;
	hi2s2__ = hi2s2;
	htim4__= htim4;
	HAL_I2S_Receive_DMA(hi2s2__, &input_buffer[0], I2S_BUF_SIZE);
	mfcc = new MFCC(MFCC_OUT_SIZE, MFCC_IN_SIZE, 2);
	kws = new KWS();


	while(1)
	{


	}
}

/* USER CODE BEGIN 4 */
void delay350Microseconds(uint32_t multiplier) {
    uint32_t delay = 330 * multiplier;
    __HAL_TIM_SET_COUNTER(&htim4, 0);
    while (__HAL_TIM_GET_COUNTER(&htim4) <= delay){

    }

}

void transmit(uint32_t   numberHighPulses, uint32_t  numberLowPulses)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
	delay350Microseconds(numberHighPulses);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
	delay350Microseconds(numberLowPulses);
}

void sendSequence(uint16_t bitSequence){


	for (int x = 0; x < 10; x++){
		int length = 12;
		for (int i = length-1; i >=0; i--)
		{
			transmit(1, 3);
			if( bitSequence & (1 << i))
			{
				transmit (1, 3); //1

			}
			else
			{

				transmit (3, 1); //0
			}
		}

		transmit(1, 31); // Ende der Übertragung
    }
}
// Half buffer complete callback
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
	if (hi2s == &hi2s2) {
		process_audio_data(input_buffer, mergedFrame, 0, 0, I2S_BUF_SIZE);
		aufzeichnen(1);
		if(array_ready==1){
//			HAL_I2S_DMAPause(hi2s2__);
			extract_and_infer_mfccs();
//			HAL_I2S_DMAResume(hi2s2__);


		}
	}
}

// Full buffer complete callback
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s) {
	if (hi2s == &hi2s2) {
		// Process the second half of the buffer (I2S_BUF_SIZE / 2 to I2S_BUF_SIZE)
		process_audio_data(input_buffer, mergedFrame, I2S_BUF_SIZE, 250, I2S_BUF_SIZE);
		aufzeichnen(2);
		if(array_ready==1){
//		HAL_I2S_DMAPause(hi2s2__);
		extract_and_infer_mfccs();
//		HAL_I2S_DMAResume(hi2s2__);


		}
	}

}

void process_audio_data(uint16_t *inputBuffer, int32_t *mergedFrame, uint16_t buffer_start, uint16_t merged_start, uint16_t length) {
	for (uint16_t i = 0; i < length; i += 4) {
		// Combine two 16-bit values into a 32-bit signed PCM sample
		int32_t mergeValue = ((inputBuffer[i + buffer_start]) << 16 | inputBuffer[i + buffer_start + 1]); // Merge high and low (18-bit PCM)

		// Adjust to 32-bit signed range (shift to discard unused bits)
		mergeValue = mergeValue >> 14; // Bring 18-bit PCM data into 32-bit alignment
		mergedFrame[i/4 + merged_start] = mergeValue;
//		printf("merged");
	}

}




//int max = -999999999;
//int min = 999999999;
int delay = 0;
uint32_t anfangZeit = 0;
uint32_t endeZeit = 0;

void aufzeichnen(int half) {
	delay += 125;
	int max = -999999999;
	int min = 999999999;
	//Ein paar sekunde warten auf sinvolle werte

	if (delay > 3200) {
		delay = 4000;
		if(pre_recording == 1) {
					if(pre_record_idx < PRE_RECORD_SIZE - 1) {

						if (half == 1) {
							for(unsigned int j = 0; j < I2S_BUF_SIZE/4; j++){
								ring_buffer[j + pre_record_idx] = mergedFrame[j];

							}
						} else if (half == 2) {
							for(unsigned int j = 0; j < I2S_BUF_SIZE/4; j++){
								ring_buffer[j + pre_record_idx] = mergedFrame[j + I2S_BUF_SIZE / 4];

							}

						}
						pre_record_idx += I2S_BUF_SIZE/4;

					}
					else {
						pre_record_idx = 0;
						if (half == 1) {
							for(unsigned int j = 0; j < I2S_BUF_SIZE/4; j++){
								ring_buffer[j + pre_record_idx] = mergedFrame[j];

							}
						} else if (half == 2) {
							for(unsigned int j = 0; j < I2S_BUF_SIZE/4; j++){
								ring_buffer[j + pre_record_idx] = mergedFrame[j + I2S_BUF_SIZE / 4];

							}

						}
						pre_record_idx += I2S_BUF_SIZE/4;
					}
				}

		//ueberpruefung ob threshhold ueberschritten wird
		if (half == 1) {
			for(unsigned int i = 0; i < I2S_BUF_SIZE / 4; i++){
				if(mergedFrame[i] > max){
					max = mergedFrame[i];
				}
				if(min > mergedFrame[i]) {
					min = mergedFrame[i];
				}
			}
		} else if (half == 2) {
			for(unsigned int i = I2S_BUF_SIZE / 4; i < 	I2S_BUF_SIZE / 2; i++){
				if(mergedFrame[i] > max){
					max = mergedFrame[i];
				}
				if(min > mergedFrame[i]) {
					min = mergedFrame[i];
				}
			}
		}
		int32_t adaptiverSchwellwert = averageVal();
		if (adaptiverSchwellwert == 0) {
		    adaptiverSchwellwert = THRESHOLD;
		} else {
		    adaptiverSchwellwert = averageVal();
		}

 //Wenn threshold ueberschritten wird, wird start flag 1
		if (max > adaptiverSchwellwert || min < -adaptiverSchwellwert) {
			start = 1;
			pre_recording = 0;

		}else{
			start=0;
		}


//		printf("idx1 %d", idx);
		// aufzeichnung beginnt
		if (start == 1) {

			array_ready = 0;
			anfangZeit = HAL_GetTick();
			int j = 0;
			for(unsigned int i = pre_record_idx; i <= PRE_RECORD_SIZE ; i++) {

							array[j] = ring_buffer[i];
							j++;
						}
			j = PRE_RECORD_SIZE - pre_record_idx;
			for(unsigned int i = 0; i < pre_record_idx; i++) {
										array[j] = ring_buffer[i];
										j++;
									}
//			printf("idx1 %d", idx);
			//speicherung die werte in 16000 gross array
			if(idx < 12000) {
					if (half == 1) {

						for(int j = 0; j < I2S_BUF_SIZE/4; j++){

							array[j + idx + PRE_RECORD_SIZE] = mergedFrame[j];
							DisplayVolume(j);


						}
					} else if (half == 2) {
						for( int j = 0; j < I2S_BUF_SIZE/4; j++){

							array[j + idx + PRE_RECORD_SIZE] = mergedFrame[j + I2S_BUF_SIZE / 4];
							DisplayVolume(j);


						}

					}
					idx += I2S_BUF_SIZE/4;
				}
			else {//speicherung endet
//				printf("idx2 %d", idx);
				idx = 0;
				pre_record_idx = 0;
				start = 0;
				endeZeit = HAL_GetTick() - anfangZeit;
//				printf("Dauer: %lu\r\n", endeZeit);
			    array_ready = 1;
			    pre_recording = 1;


			}


		}
	}
}

int __io_putchar(int ch)
{
	uint8_t c[1];
	c[0] = ch & 0x00FF;
	HAL_UART_Transmit(&huart3, &*c, 1, 10);
	return ch;
}
int _write(int file, char *ptr, int len)
{
	int DataIdx;
	for(DataIdx = 0; DataIdx < len; DataIdx++)
	{
		__io_putchar(*ptr++);
	}
	return len;
}



#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */





