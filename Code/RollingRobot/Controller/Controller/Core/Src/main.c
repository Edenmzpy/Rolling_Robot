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
#include "adc.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "math.h"

#include "OLED.h"
#include "nrf24l01.h"
#include "debug_printf.h"


/*�궨���¼��ı�־λ*/  
#define PUMP_EVE 0x01   /* ��ʾ�����¼� */
#define ADD_PRE_EVE 0x02  /* ��ʾ��ѹ�¼���ע������ָ������ֵ�趨 */
#define SUB_PRE_EVE 0x04  /* ��ʾ��ѹ�¼���ע������ָ������ֵ�趨 */
#define PUMP_STOP_EVE   0x08
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

	static uint8_t tx[NRF_PAYLOAD_LEN] = {0};
	static uint8_t rx[NRF_PAYLOAD_LEN] = {0};
	/*���ڴ洢���ͺͽ��յ����ݣ�32�ֽڣ�ÿһλһ�ֽڡ���Щ����ͨ������ģ�� NRF24L01 ����ͨ��*/
	volatile uint16_t values[2];
	volatile uint16_t adc_avg[2];
	/*�洢�� ADC ��ȡ��ҡ�˵� X �� Y ���ģ��ֵ��*/
	
	/*�洢ҡ�˵�ƽ��ֵ�����ڸ��ȶ��Ŀ��ơ�*/
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* 	SW1 -- PA11
	SW2 -- PA12
	SW3 -- PB10
	SW4 -- PB11
	SW5 -- PB6 ����
	SW6 -- PB8 ��ѹ
	SW7 -- PB7
	SW8 -- PB9 ��ѹ
*/
	



/*�����ж���cubemxѡ��exti��input��pullup����*/
/*����EXTI����ʹ�õ�������ģʽ�������º󴥷��ж�ʹ�ûص������ı�txֵ��txִֵֻ��һ�ξͻᱻ��ʼ��*/
/*����stm32f4xx_it.c�е��жϻص�����*/	
/*��������ʹ����PB689����*/
/*ֻ��tx[5]���и���*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	static uint32_t last_pin6 = 0;
	static uint32_t last_pin7 = 0;
	static uint32_t last_pin8 = 0;
	static uint32_t last_pin9 = 0;
	uint32_t now = HAL_GetTick();
    if (GPIO_Pin == GPIO_PIN_6)
	{
		if (now - last_pin6 >= 10)
		{
			last_pin6 = now;
			if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) == GPIO_PIN_RESET)
			{
				tx[5] |= PUMP_EVE;
			}
		}
    }
	if (GPIO_Pin == GPIO_PIN_7)
    {
        if (now - last_pin7 >= 10)
        {
            last_pin7 = now;
            if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_RESET)
            {
                tx[5] |= PUMP_STOP_EVE;
            }
        }
    }
    if (GPIO_Pin == GPIO_PIN_8)
	{
		if (now - last_pin8 >= 10)
		{
			last_pin8 = now;
			if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8) == GPIO_PIN_RESET)
			{
				tx[5] |= ADD_PRE_EVE;
			}
		}
    }
    if (GPIO_Pin == GPIO_PIN_9)
	{
		if (now - last_pin9 >= 10)
		{
			last_pin9 = now;
			if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9) == GPIO_PIN_RESET)
			{
				tx[5] |= SUB_PRE_EVE;
			}
		}
    }
}


// ҡ��adc���
// 
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	if (hadc->Instance == ADC1) //����Ƿ��� ADC1 ��ɵ�ת��
	{
		static uint32_t acc1 = 0;
		static uint32_t acc2 = 0;
		static uint8_t cnt = 0;

		acc1 += values[0];
		acc2 += values[1];
		if (++cnt >= 10) // ÿʮ��ȡƽ��ֵ
		{
			adc_avg[0] = (uint16_t)(acc1 / 10u);
			adc_avg[1] = (uint16_t)(acc2 / 10u);
			acc1 = 0;
			acc2 = 0;
			cnt = 0;
		}	
	}
}
/*������adc�Ļص�������adc���Զ�ת����ÿ��ת������ûص�������
ֻ��Ҫ��cubemx���úü��ɣ�����ֻ����adc������ͨ��in0��in1��adc
ͨ��ֵͨ��dma��value�󶨣����ҽ�����ֵͨ��ƽ�����롣*/


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
  MX_ADC1_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
	char buf[32];
	float pressure = 0.0f;
	float vel_dir = 0.0f;
	
	HAL_ADCEx_Calibration_Start(&hadc1);
	HAL_TIM_Base_Start(&htim3);
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)values, 2);
	
	OLED_Init();
	OLED_Clear();
	HAL_Delay(500);
	
	
	while  (nrf24_check() != 0)
	{
		HAL_Delay(50);
	}//�������ͨѶģ���Ƿ�׼���ã�ͨ������ֵȷ��
	
	nrf24_init();
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{	
		if (nrf24_receive32(rx) == 0) // �������ݽ��� ��32byte�������ճɹ�����ֵΪ0��
		{
			uint8_t maxpre = rx[0];

			uint8_t idx = 1;
			memcpy(&pressure, &rx[idx], sizeof(float));
			
			idx = 5;
			memcpy(&vel_dir, &rx[idx], sizeof(float));
	
			snprintf(buf, sizeof(buf), "Max:%d", maxpre);
			OLED_ShowString(1, 1, buf);

			snprintf(buf, sizeof(buf), "Pre:%6.2f", pressure);
			OLED_ShowString(2, 1, buf);
			
			snprintf(buf, sizeof(buf), "V:%7.3f", vel_dir);
			OLED_ShowString(3, 1, buf);
			
		}
		
		float dir = 0;
		
		int32_t dx = 2050 - (int32_t)adc_avg[0]; //x�����ֵ������������
		int32_t dy = (int32_t)adc_avg[1] - 2050; //y�����ֵ���µ�������
		int32_t r = dx * dx + dy * dy;
		if ((r > 1000 * 1000))
		{ 
			dir = atan2f((float)dy, (float)dx);	
			tx[0] = 1; //tx[0]Ϊ�˶���־λ
		} 
		if ((r <= 1000 * 1000))
		{
			tx[0] = 0;
			dir = 0.0f;
		}
		memcpy(&tx[1], &dir, sizeof(float));
	
		static uint32_t last_bt_ms = 0;
		uint32_t now = HAL_GetTick();

		if (now - last_bt_ms >= 100)
		{
			last_bt_ms = now;

			char uart_buf[128];

			float t_s = HAL_GetTick() / 1000.0f;

			int n = snprintf(uart_buf, sizeof(uart_buf),
							 "%.3f,%.2f,%.3f\r\n",
							 t_s,
							 pressure,
							 vel_dir);//unit��s, kpa, m/s

			HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, (uint16_t)n, 100);
		}

		
		if (tx[5] != 0)
		{
			nrf24_send32(tx, 100);
			tx[5] = 0;
		}
		else
		{
			nrf24_send32(tx, 100);
		}
		HAL_Delay(50);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
