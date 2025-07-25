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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include "common.h"
#include "dma.h"
#include "flight_context.h"
#include "log.h"
#include "motion_control/actuators.h"
#include "motion_control/filter.h"
#include "sensors/imu/imu.h"
#include "sync.h"


TIM_HandleTypeDef TimHandle;
DMA_HandleTypeDef hdma_tim;
TIM_OC_InitTypeDef sConfig;
uint32_t aCCValue_Buffer[8] = { 0 };
uint32_t uwTimerPeriod      = 0;


// void DMA1_Stream0_IRQHandler (void) {
//     // LOG_INFO ("DMA1 Stream0 IRQ Handler");
//     HAL_DMA_IRQHandler (&hdma_tim);
// }

void Temp_PWM_Init (TIM_HandleTypeDef* htim) {
    GPIO_InitTypeDef GPIO_InitStruct;


    /*##-1- Enable peripherals and GPIO Clocks #################################*/
    /* TIM8 clock enable */
    __HAL_RCC_TIM8_CLK_ENABLE ();
    __HAL_RCC_DMA1_CLK_ENABLE ();
    __HAL_RCC_GPIOC_CLK_ENABLE ();
    __HAL_RCC_GPIOJ_CLK_ENABLE ();
    /* TIM8 GPIO Configuration
     * PC6     ------> TIM8_CH1
     * PJ7     ------> TIM8_CH2N
     */
    GPIO_InitStruct.Pin       = GPIO_PIN_6;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF3_TIM8;
    HAL_GPIO_Init (GPIOC, &GPIO_InitStruct);

    // #define ARD_D6_Pin GPIO_PIN_7
    GPIO_InitStruct.Pin       = GPIO_PIN_7;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF3_TIM8;
    // #define ARD_D6_GPIO_Port GPIOJ
    HAL_GPIO_Init (GPIOJ, &GPIO_InitStruct);


    /* Set the parameters to be configured */
    hdma_tim.Init.Request             = DMA_REQUEST_TIM8_CH1;
    hdma_tim.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_tim.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_tim.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_tim.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_tim.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    hdma_tim.Init.Mode                = DMA_NORMAL;
    hdma_tim.Init.Priority            = DMA_PRIORITY_MEDIUM;
    hdma_tim.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    hdma_tim.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma_tim.Init.MemBurst            = DMA_MBURST_SINGLE;
    hdma_tim.Init.PeriphBurst         = DMA_PBURST_SINGLE;

    /* Set hdma_tim instance */
    hdma_tim.Instance = DMA1_Stream0;

    /* Link hdma_tim to hdma[TIM_DMA_ID_CC1] (channel1) */
    htim->hdma[TIM_DMA_ID_CC1] = &hdma_tim;
    hdma_tim.Parent            = htim;
    // __HAL_LINKDMA (htim, hdma[TIM_DMA_ID_CC1], hdma_tim);

    /* Initialize TIMx DMA handle */
    HAL_DMA_Init (htim->hdma[TIM_DMA_ID_CC1]);

    /*##-2- Configure the NVIC for DMA #########################################*/
    /* NVIC configuration for DMA transfer complete interrupt */
    HAL_NVIC_SetPriority (DMA1_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ (DMA1_Stream0_IRQn);
}

void Temp_DMA_Start (void) {

    Temp_PWM_Init (&TimHandle);

    uint32_t prescaler = 64U - 1U;
    uwTimerPeriod      = 1000U;
    /* Compute CCR1 value to generate a duty cycle at 75% */
    aCCValue_Buffer[0] = 750U;
    /* Compute CCR2 value to generate a duty cycle at 50% */
    aCCValue_Buffer[1] = 500U;
    /* Compute CCR3 value to generate a duty cycle at 25% */
    aCCValue_Buffer[2] = 250U;

    TimHandle.Instance               = TIM8;
    TimHandle.Init.Period            = uwTimerPeriod;
    TimHandle.Init.RepetitionCounter = 0;
    TimHandle.Init.Prescaler         = prescaler;
    TimHandle.Init.ClockDivision     = 0;
    TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    TimHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_PWM_Init (&TimHandle) != HAL_OK) {
        return;
    }

    /*##-2- Configure the PWM channel 1 ########################################*/
    sConfig.OCMode       = TIM_OCMODE_PWM1;
    sConfig.OCPolarity   = TIM_OCPOLARITY_HIGH;
    sConfig.Pulse        = 0;
    sConfig.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    sConfig.OCFastMode   = TIM_OCFAST_DISABLE;
    sConfig.OCIdleState  = TIM_OCIDLESTATE_RESET;
    sConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel (&TimHandle, &sConfig, TIM_CHANNEL_1) != HAL_OK) {
        return;
    }
    LOG_INFO ("PWM starting");
    /* ##-3- Start PWM signal generation in DMA mode ############################ */
    if (HAL_TIM_PWM_Start_DMA (&TimHandle, TIM_CHANNEL_1, aCCValue_Buffer, 3) != HAL_OK) {
        return;
    }

    while (1) {
        LOG_INFO ("PWM running");
        HAL_Delay (1000);
    }
}


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim8;
TIM_HandleTypeDef htim13;

// UART_HandleTypeDef huart1;

/* Definitions for defaultTask */
// osThreadId_t defaultTaskHandle;
// const osThreadAttr_t defaultTask_attributes = {
//     .name       = "defaultTask",
//     .stack_size = 128 * 4,
//     .priority   = (osPriority_t)osPriorityNormal,
// };
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config (void);
static void MX_GPIO_Init (void);
static void MX_SPI2_Init (void);
// static void MX_TIM8_Init (void);
// static void MX_TIM13_Init (void);
void StartDefaultTask (void* argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// NOLINTBEGIN
IMU gIMU                                     = { 0 };
FlightContext gFlightContext                 = { 0 };
FilterMadgwickContext gFilterMadgwickContext = { 0 };
PIDContext gPIDAngleContext                  = { 0 };
TaskHandle_t gpTaskMotionControlUpdate       = { 0 };
// NOLINTEND

void HAL_GPIO_EXTI_Callback (uint16_t gpioPin) {
    if (gpioPin == IMU_INT_Pin) {
        STATUS_TYPE status = IMU2CPUInterruptHandler (&gIMU);
        if (status == eSTATUS_SUCCESS) {
            if (gpTaskMotionControlUpdate != NULL) {
                xTaskNotifyFromISR (gpTaskMotionControlUpdate, 0, eSetBits, NULL);
            }
        } else {
            gIMU.status = status;
        }
    }
}

void TaskMotionControlUpdate (void* pvParameters) {

    float msStartTime        = (float)xTaskGetTickCount ();
    uint32_t msLogStart      = xTaskGetTickCount ();
    uint32_t const msLogStep = 500;
    Vec3f currentAttitude    = { 0.0F };
    Vec3f targetAttitude     = { 0.0F };
    Vec3f maxAttitude = { .roll = 45.0F, .pitch = 45.0F, .yaw = 180.0F };

    if (ActuatorsStart () != eSTATUS_SUCCESS) {
        LOG_ERROR ("Failed to start actuators");
    } else {
        ActuatorsArm ();
        LOG_INFO ("Actuators armed");
    }

    if (IMUEnableInterrupts (&gIMU) != eSTATUS_SUCCESS) {
        LOG_ERROR ("Failed to enable IMU interrupts");
    }
    LOG_INFO ("Motion control task started");

    while (1) {
        ulTaskNotifyTake (pdTRUE, pdMS_TO_TICKS (1000));
        /* Add error handling */
        if (gIMU.status != eSTATUS_SUCCESS) {
            if (IMUHandleErr (&gIMU) != eSTATUS_SUCCESS) {
                LOG_ERROR ("Failed to handle IMU error");
                continue;
            }
        }

        STATUS_TYPE status = eSTATUS_SUCCESS;
        Vec3f accel        = { 0.0F };
        Vec3f gyro         = { 0.0F };
        status             = IMUConvertRaw (
        gIMU.aconf.range, gIMU.rawAccel, gIMU.gconf.range, gIMU.rawGyro, &accel, &gyro);

        if (status != eSTATUS_SUCCESS) {
            LOG_ERROR ("Failed to convert IMU raw data");
            continue;
        }

        // RadioPWMChannels radio;
        float msCurrentTime = (float)xTaskGetTickCount ();
        // Convert milliseconds to seconds
        float dt    = (msCurrentTime - msStartTime) / 1000.0F;
        msStartTime = msCurrentTime;

        if (dt <= 0.0F) {
            continue;
        }

        // Vec3f currentAttitude = gFlightContext.currentAttitude;
        status =
        FilterMadgwick6DOF (&gFilterMadgwickContext, &accel, &gyro, dt, &currentAttitude);
        if (status != eSTATUS_SUCCESS) {
            LOG_ERROR ("Failed to filter IMU data with Madgwick filter");
            continue;
        }

        Vec3f pidAttitude = { 0.0F };
        status            = PIDUpdateAttitude (
        &gPIDAngleContext, currentAttitude, targetAttitude, maxAttitude, dt, &pidAttitude);
        if (status != eSTATUS_SUCCESS) {
            LOG_ERROR ("Failed to update PID attitude");
            continue;
        }

        status = ActuatorsWrite (pidAttitude, 0.25F);
        if (status != eSTATUS_SUCCESS) {
            LOG_ERROR ("Failed to write actuators");
            continue;
        }

        if ((xTaskGetTickCount () - msLogStart) >= msLogStep) {

            // Servo* pServo    = ActuatorsGetLeftServo ();
            // uint32_t usAngle = 1500;
            // if (switchAngle == 0) {
            //     usAngle     = 500;
            //     switchAngle = 1;
            // } else {
            //     switchAngle = 0;
            // }
            // PWMWrite (&pServo->pwm, usAngle);
            // status = ActuatorsWrite (pidAttitude, 0.5F);
            // TIM13->CCR1 = 20000;
            // TIM8->CCR1 = 2000;
            // TIM8->ARR  = 20000;
            // TIM8->PSC  = 64;
            // LOG_INFO (
            // "TIM8 CCR1: %u ARR: %u PSC: %u", (uint16_t)TIM8->CCR1,
            // (uint16_t)TIM8->ARR, (uint16_t)TIM8->PSC);
            // LOG_INFO ("H");

            msLogStart = xTaskGetTickCount ();

            Vec3f a  = accel;
            Vec3f g  = gyro;
            Vec3f ca = currentAttitude;

            LOG_DATA_IMU_DATA (a, g);
            LOG_DATA_CURRENT_ATTITUDE (ca);
        }
    }
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main (void) {
    /* USER CODE BEGIN Boot_Mode_Sequence_1 */

    /* Wait until CPU2 boots and enters in stop mode */
    while (__HAL_RCC_GET_FLAG (RCC_FLAG_D2CKRDY) != RESET) {
        asm volatile ("NOP");
    }

    /* USER CODE END Boot_Mode_Sequence_1 */
    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init ();

    /* Configure the system clock */
    SystemClock_Config ();
    /* USER CODE BEGIN Boot_Mode_Sequence_2 */

    MX_GPIO_Init ();
    MX_SPI2_Init ();
    // MX_TIM8_Init ();
    // MX_TIM13_Init ();

    /* When system initialization is finished, Cortex-M7 will release Cortex-M4 by means of HSEM notification */

    /*HW semaphore Clock enable*/
    __HAL_RCC_HSEM_CLK_ENABLE ();
    // /*Take HSEM */
    HAL_HSEM_FastTake (HSEM_ID_0);
    // /*Release HSEM in order to notify the CPU2(CM4)*/
    HAL_HSEM_Release (HSEM_ID_0, 0);
    // /* wait until CPU2 wakes up from stop mode */
    while (__HAL_RCC_GET_FLAG (RCC_FLAG_D2CKRDY) == RESET) {
        asm volatile ("NOP");
    }
    /* USER CODE END Boot_Mode_Sequence_2 */

    if (SyncInit () != eSTATUS_SUCCESS) {
        CriticalErrorHandler ();
    }

    if (LoggerInit (NULL, NULL) != eSTATUS_SUCCESS) {
        CriticalErrorHandler ();
    }
    // Wait for CM4 to initialize UART
    HAL_Delay (1000);


    // Temp_DMA_Start ();


    if (DMASystemInit () != eSTATUS_SUCCESS) {
        LOG_ERROR ("Failed to init DMA system");
    }

    STATUS_TYPE status = eSTATUS_SUCCESS;
    /*
     * Init IMU
     */
    {
        IMUAccConf aconf  = { 0 };
        aconf.odr         = eIMU_ACC_ODR_100;
        aconf.range       = eIMU_ACC_RANGE_2G;
        aconf.avg         = eIMU_ACC_AVG_16;
        aconf.bw          = eIMU_ACC_BW_HALF;
        aconf.mode        = eIMU_ACC_MODE_HIGH_PERF;
        IMUGyroConf gconf = { 0 };
        gconf.odr         = eIMU_GYRO_ODR_100;
        gconf.range       = eIMU_GYRO_RANGE_250;
        gconf.avg         = eIMU_GYRO_AVG_16;
        gconf.bw          = eIMU_GYRO_BW_HALF;
        gconf.mode        = eIMU_GYRO_MODE_HIGH_PERF;
        status            = IMUInit (&gIMU, &hspi2, aconf, gconf);
        if (status != eSTATUS_SUCCESS) {
            LOG_ERROR ("Failed to init IMU");
        }
    }

    /* USER CODE BEGIN 2 */
    /* With an ODR of 100 Hz on the IMU 1000 iterations will take 10 seconds */
    /* With an ODR of 200 Hz on the IMU 1000 iterations will take 5 seconds */
    status = FilterMadgwickWarmUp (500U, &gIMU, 1.5F, 3.0F, &gFilterMadgwickContext);
    if (status != eSTATUS_SUCCESS) {
        LOG_ERROR ("Failed to warm up Madgwick Filter");
    }

    status = FlightContextInit (&gFlightContext);
    if (status != eSTATUS_SUCCESS) {
        LOG_ERROR ("Failed to init Flight Context");
    }
    MotorConfig left_Motor =
    MOTOR_CREATE_CONF (TIM8, TIM_CHANNEL_1, DMA1_Stream0, DMA_REQUEST_TIM8_CH1);
    PWMConfig left_Servo = PWM_CREATE_CONF (TIM13, TIM_CHANNEL_1, 50, TRUE);
    status = ActuatorsInit (left_Servo, left_Motor);
    if (status != eSTATUS_SUCCESS) {
        LOG_ERROR ("Failed to init Actuators");
    }

    /*
     *
     * Setup FreeRTOS Tasks
     *
     * NOTE: Once a FreeRTOS task is created ALL interrupts will be disabled until the scheduler is started. So functions
     * like HAL_Delay will not work.
     */
    LOG_INFO ("Starting FreeRTOS");
    BaseType_t taskStatus = xTaskCreate (
    TaskMotionControlUpdate, "Motion Control Update Task", configMINIMAL_STACK_SIZE,
    NULL, tskIDLE_PRIORITY, &gpTaskMotionControlUpdate);

    if (taskStatus != pdPASS) {
        LOG_ERROR ("Failed to create motion control update task");
    }

    vTaskStartScheduler ();

    /* USER CODE END 2 */
    while (1) {
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config (void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

    /** Supply configuration update enable
     */
    HAL_PWREx_ConfigSupply (PWR_DIRECT_SMPS_SUPPLY);

    /** Configure the main internal regulator output voltage
     */
    __HAL_PWR_VOLTAGESCALING_CONFIG (PWR_REGULATOR_VOLTAGE_SCALE1);

    while (!__HAL_PWR_GET_FLAG (PWR_FLAG_VOSRDY)) {
    }

    /** Initializes the RCC Oscillators according to the specified
     * parameters in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType =
    RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState            = RCC_HSE_ON;
    RCC_OscInitStruct.HSIState            = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.LSIState            = RCC_LSI_ON;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM            = 5;
    RCC_OscInitStruct.PLL.PLLN            = 48;
    RCC_OscInitStruct.PLL.PLLP            = 2;
    RCC_OscInitStruct.PLL.PLLQ            = 5;
    RCC_OscInitStruct.PLL.PLLR            = 2;
    RCC_OscInitStruct.PLL.PLLRGE          = RCC_PLL1VCIRANGE_2;
    RCC_OscInitStruct.PLL.PLLVCOSEL       = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN        = 0;
    if (HAL_RCC_OscConfig (&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler ();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType =
    RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 |
    RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.SYSCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

    if (HAL_RCC_ClockConfig (&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        Error_Handler ();
    }
    HAL_RCC_MCOConfig (RCC_MCO1, RCC_MCO1SOURCE_HSI, RCC_MCODIV_1);
}

/**
 * @brief SPI2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI2_Init (void) {

    /* USER CODE BEGIN SPI2_Init 0 */

    /* USER CODE END SPI2_Init 0 */

    /* USER CODE BEGIN SPI2_Init 1 */

    /* USER CODE END SPI2_Init 1 */
    /* SPI2 parameter configuration*/
    hspi2.Instance               = SPI2;
    hspi2.Init.Mode              = SPI_MODE_MASTER;
    hspi2.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi2.Init.NSS               = SPI_NSS_HARD_OUTPUT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    hspi2.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial     = 0x0;
    hspi2.Init.NSSPMode          = SPI_NSS_PULSE_ENABLE;
    hspi2.Init.NSSPolarity       = SPI_NSS_POLARITY_LOW;
    hspi2.Init.FifoThreshold     = SPI_FIFO_THRESHOLD_01DATA;
    hspi2.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    hspi2.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    hspi2.Init.MasterKeepIOState      = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    hspi2.Init.IOSwap                 = SPI_IO_SWAP_DISABLE;
    if (HAL_SPI_Init (&hspi2) != HAL_OK) {
        Error_Handler ();
    }
    /* USER CODE BEGIN SPI2_Init 2 */

    /* USER CODE END SPI2_Init 2 */
}

/**
 * @brief TIM8 Initialization Function
 * @param None
 * @retval None
 */
// static void MX_TIM8_Init (void) {

//     /* USER CODE BEGIN TIM8_Init 0 */

//     /* USER CODE END TIM8_Init 0 */

//     TIM_ClockConfigTypeDef sClockSourceConfig           = { 0 };
//     TIM_MasterConfigTypeDef sMasterConfig               = { 0 };
//     TIM_OC_InitTypeDef sConfigOC                        = { 0 };
//     TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = { 0 };

//     /* USER CODE BEGIN TIM8_Init 1 */

//     /* USER CODE END TIM8_Init 1 */
//     htim8.Instance               = TIM8;
//     htim8.Init.Prescaler         = 0;
//     htim8.Init.CounterMode       = TIM_COUNTERMODE_UP;
//     htim8.Init.Period            = 65535;
//     htim8.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
//     htim8.Init.RepetitionCounter = 0;
//     htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
//     if (HAL_TIM_Base_Init (&htim8) != HAL_OK) {
//         Error_Handler ();
//     }
//     sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
//     if (HAL_TIM_ConfigClockSource (&htim8, &sClockSourceConfig) != HAL_OK) {
//         Error_Handler ();
//     }
//     if (HAL_TIM_PWM_Init (&htim8) != HAL_OK) {
//         Error_Handler ();
//     }
//     sMasterConfig.MasterOutputTrigger  = TIM_TRGO_RESET;
//     sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
//     sMasterConfig.MasterSlaveMode      = TIM_MASTERSLAVEMODE_DISABLE;
//     if (HAL_TIMEx_MasterConfigSynchronization (&htim8, &sMasterConfig) != HAL_OK) {
//         Error_Handler ();
//     }
//     sConfigOC.OCMode       = TIM_OCMODE_PWM1;
//     sConfigOC.Pulse        = 0;
//     sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
//     sConfigOC.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
//     sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
//     sConfigOC.OCIdleState  = TIM_OCIDLESTATE_RESET;
//     sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
//     if (HAL_TIM_PWM_ConfigChannel (&htim8, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
//         Error_Handler ();
//     }
//     sBreakDeadTimeConfig.OffStateRunMode  = TIM_OSSR_DISABLE;
//     sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
//     sBreakDeadTimeConfig.LockLevel        = TIM_LOCKLEVEL_OFF;
//     sBreakDeadTimeConfig.DeadTime         = 0;
//     sBreakDeadTimeConfig.BreakState       = TIM_BREAK_DISABLE;
//     sBreakDeadTimeConfig.BreakPolarity    = TIM_BREAKPOLARITY_HIGH;
//     sBreakDeadTimeConfig.BreakFilter      = 0;
//     sBreakDeadTimeConfig.Break2State      = TIM_BREAK2_DISABLE;
//     sBreakDeadTimeConfig.Break2Polarity   = TIM_BREAK2POLARITY_HIGH;
//     sBreakDeadTimeConfig.Break2Filter     = 0;
//     sBreakDeadTimeConfig.AutomaticOutput  = TIM_AUTOMATICOUTPUT_DISABLE;
//     if (HAL_TIMEx_ConfigBreakDeadTime (&htim8, &sBreakDeadTimeConfig) != HAL_OK) {
//         Error_Handler ();
//     }
//     /* USER CODE BEGIN TIM8_Init 2 */

//     /* USER CODE END TIM8_Init 2 */
//     HAL_TIM_MspPostInit (&htim8);
// }

// /**
//  * @brief TIM13 Initialization Function
//  * @param None
//  * @retval None
//  */
// static void MX_TIM13_Init (void) {

//     /* USER CODE BEGIN TIM13_Init 0 */

//     /* USER CODE END TIM13_Init 0 */

//     TIM_OC_InitTypeDef sConfigOC = { 0 };

//     /* USER CODE BEGIN TIM13_Init 1 */

//     /* USER CODE END TIM13_Init 1 */
//     htim13.Instance               = TIM13;
//     htim13.Init.Prescaler         = 0;
//     htim13.Init.CounterMode       = TIM_COUNTERMODE_UP;
//     htim13.Init.Period            = 65535;
//     htim13.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
//     htim13.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
//     if (HAL_TIM_Base_Init (&htim13) != HAL_OK) {
//         Error_Handler ();
//     }
//     if (HAL_TIM_PWM_Init (&htim13) != HAL_OK) {
//         Error_Handler ();
//     }
//     sConfigOC.OCMode     = TIM_OCMODE_PWM1;
//     sConfigOC.Pulse      = 0;
//     sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
//     sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
//     if (HAL_TIM_PWM_ConfigChannel (&htim13, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
//         Error_Handler ();
//     }
//     /* USER CODE BEGIN TIM13_Init 2 */

//     /* USER CODE END TIM13_Init 2 */
//     HAL_TIM_MspPostInit (&htim13);
// }

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
// static void MX_USART1_UART_Init (void) {

//     /* USER CODE BEGIN USART1_Init 0 */

//     /* USER CODE END USART1_Init 0 */

//     /* USER CODE BEGIN USART1_Init 1 */

//     /* USER CODE END USART1_Init 1 */
//     huart1.Instance                    = USART1;
//     huart1.Init.BaudRate               = 115200;
//     huart1.Init.WordLength             = UART_WORDLENGTH_8B;
//     huart1.Init.StopBits               = UART_STOPBITS_1;
//     huart1.Init.Parity                 = UART_PARITY_NONE;
//     huart1.Init.Mode                   = UART_MODE_TX_RX;
//     huart1.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
//     huart1.Init.OverSampling           = UART_OVERSAMPLING_16;
//     huart1.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
//     huart1.Init.ClockPrescaler         = UART_PRESCALER_DIV1;
//     huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
//     if (HAL_UART_Init (&huart1) != HAL_OK) {
//         Error_Handler ();
//     }
//     if (HAL_UARTEx_SetTxFifoThreshold (&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
//         Error_Handler ();
//     }
//     if (HAL_UARTEx_SetRxFifoThreshold (&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
//         Error_Handler ();
//     }
//     if (HAL_UARTEx_DisableFifoMode (&huart1) != HAL_OK) {
//         Error_Handler ();
//     }
//     /* USER CODE BEGIN USART1_Init 2 */

//     /* USER CODE END USART1_Init 2 */
// }

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init (void) {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    /* USER CODE BEGIN MX_GPIO_Init_1 */

    /* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE ();
    __HAL_RCC_GPIOA_CLK_ENABLE ();
    __HAL_RCC_GPIOH_CLK_ENABLE ();
    __HAL_RCC_GPIOF_CLK_ENABLE ();
    __HAL_RCC_GPIOJ_CLK_ENABLE ();

    /*Configure GPIO pin : CEC_CK_MCO1_Pin */
    GPIO_InitStruct.Pin       = CEC_CK_MCO1_Pin;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
    HAL_GPIO_Init (CEC_CK_MCO1_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : IMU_INT_Pin */
    GPIO_InitStruct.Pin  = IMU_INT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init (IMU_INT_GPIO_Port, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority (IMU_INT_EXTI_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ (IMU_INT_EXTI_IRQn);

    /* USER CODE BEGIN MX_GPIO_Init_2 */

    /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask (void* argument) {
    /* USER CODE BEGIN 5 */
    /* Infinite loop */
    for (;;) {
        osDelay (1);
    }
    /* USER CODE END 5 */
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM4 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to
 * increment a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef* htim) {
    /* USER CODE BEGIN Callback 0 */

    /* USER CODE END Callback 0 */
    if (htim->Instance == TIM4) {
        HAL_IncTick ();
    }
    /* USER CODE BEGIN Callback 1 */

    /* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler (void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq ();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed (uint8_t* file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
