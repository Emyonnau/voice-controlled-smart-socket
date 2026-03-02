/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "i2s.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "arm_math.h"
#include "arm_const_structs.h"
#include "arm_common_tables.h"
#include "mfcc.h"
#include "kws.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define I2S_BUF_SIZE 1000
#define ARRAY_SIZE 16000
#define THRESHOLD 8000
#define MFCC_IN_SIZE 800
#define MFCC_STEP_SIZE 320
#define MFCC_OUT_SIZE 10
#define SOCKET_ON 0b111111000010
#define SOCKET_OFF 0b111111000001

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t input_buffer[I2S_BUF_SIZE*2];
int32_t mergedFrame[I2S_BUF_SIZE/2];
int ready = 0;
int32_t array[16000];
int idx = 0;
int array_ready = 0;
float mfccInputFrame[MFCC_IN_SIZE];
q7_t mfccOutTmp[MFCC_OUT_SIZE];
float mfccOut[MFCC_OUT_SIZE];
MFCC* mfcc = new MFCC(MFCC_OUT_SIZE, MFCC_IN_SIZE, 2);
KWS * kws= new KWS();
volatile uint8_t socket_state = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void delay350Microseconds(uint32_t multiplier);
void transmit(uint32_t   numberHighPulses, uint32_t  numberLowPulses);
void sendSequence(uint16_t bitSequence);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void process_audio_data(uint16_t *inputBuffer, int32_t *mergedFrame, uint16_t buffer_start, uint16_t merged_start, uint16_t length);
void merged_frame_output(int32_t *mergedFrame, uint16_t frame_start, uint16_t frame_length);
void aufzeichnen(int half);
int __io_putchar(int ch);
int _write(int file, char *ptr, int len);

void ControlLEDs(uint8_t numLEDs) {
    // Alle Pins zunächst ausschalten
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
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
    if (numLEDs >= 7) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
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
    } else if (averageVolume <= 15000) {
        return 3; // Light up 3 LEDs
    } else if (averageVolume <= 20000) {
        return 4; // Light up 4 LEDs
    } else if (averageVolume <= 25000) {
        return 5; // Light up 5 LEDs
    } else if (averageVolume <= 30000) {
        return 6; // Light up 6 LEDs
    } else if (averageVolume <= 35000) {
        return 7; // Light up 7 LEDs
    } else {
        return 8; // Light up all 8 LEDs
    }
}

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
        socket_state=0;
}
}

void ProcessAudio() {



    printf("Feature extraction started\r\n");
    int feature_counter = 0;
    for (int i = 0; i < 16000 - MFCC_STEP_SIZE; i += MFCC_STEP_SIZE, feature_counter++) {
        for (int j = 0; j < MFCC_IN_SIZE; j++) {
            if (i + j >= 16000)
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

	size_t size = sizeof(mfccOutTmp) / sizeof(mfccOutTmp[0]);

	// Konvertierung
	for (size_t i = 0; i < size; ++i) {
		mfccOut[i] = static_cast<float>(mfccOutTmp[i]);
//		printf("value of mfccOut[%d]: %f\n", i, mfccOut[i]);
	}

    printf("Feature extraction ended\r\n");



}

void extract_and_infer_mfccs(void) {


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
				size_t size = sizeof(mfccOutTmp) / sizeof(mfccOutTmp[0]);

					// Konvertierung
					for (size_t i = 0; i < size; ++i) {
						mfccOut[i] = static_cast<float>(mfccOutTmp[i]);
				//		printf("value of mfccOut[%d]: %f\n", i, mfccOut[i]);
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
	}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */


  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_I2S2_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  /* HAL-Initialisierung */
  	  ai_handle network = AI_HANDLE_NULL;
      kws = new KWS();
      kws->initialize();
      mfcc = new MFCC(MFCC_OUT_SIZE, MFCC_IN_SIZE,2);




  	HAL_TIM_Base_Start(&htim4);

	//sendSequence(0b111111000010);



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {


	  //sendSequence(0b111111000010);

  }


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */


  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
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

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
	if (hi2s == &hi2s2) {
		process_audio_data(input_buffer, mergedFrame, 0, 0, I2S_BUF_SIZE);
		aufzeichnen(1);
	}
}

// Full buffer complete callback
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s) {
	if (hi2s == &hi2s2) {
		// Process the second half of the buffer (I2S_BUF_SIZE / 2 to I2S_BUF_SIZE)
		process_audio_data(input_buffer, mergedFrame, I2S_BUF_SIZE, 250, I2S_BUF_SIZE);
		aufzeichnen(2);
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



int start = 0;
int max = -999999999;
int min = 999999999;
int delay = 0;
uint32_t anfangZeit = 0;
uint32_t endeZeit = 0;
void aufzeichnen(int half) {
	delay += 125;
	//Ein paar sekunde warten auf sinvolle werte
	if (delay > 3200) {
		delay = 4000;
		array_ready = 0;
		//ueberpruefung ob threshhold ueberschritten wird
		if (half == 1) {
			for(int i = 0; i < 	I2S_BUF_SIZE / 4; i++){
				if(mergedFrame[i] > max){
					max = mergedFrame[i];
				}
				if(min > mergedFrame[i]) {
					min = mergedFrame[i];
				}
			}
		} else if (half == 2) {
			for(int i = I2S_BUF_SIZE / 4; i < 	I2S_BUF_SIZE / 2; i++){
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

		}
		// aufzeichnung beginnt
		if (start == 1) {

//			anfangZeit = HAL_GetTick();
			//speicherung die werte in 16000 gross array
			if(idx < ARRAY_SIZE - 1) {

					if (half == 1) {
						for(int j = 0; j < I2S_BUF_SIZE/4; j++){
							array[j + idx] = mergedFrame[j];
							DisplayVolume(j);
						}
					} else if (half == 2) {
						for(int j = 0; j < I2S_BUF_SIZE/4; j++){
							array[j + idx] = mergedFrame[j + I2S_BUF_SIZE / 4];
							DisplayVolume(j);
						}

					}
					idx += I2S_BUF_SIZE/4;

				}
			else {//speicherung endet
				idx = 0;
				start = 0;
				endeZeit = HAL_GetTick() - anfangZeit;
				printf("Dauer: %lu\r\n", endeZeit);
			    array_ready = 1;

			}


		}
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
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
