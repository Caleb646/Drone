#ifndef SENSORS_IMU_H
#define SENSORS_IMU_H

#include "common.h"
#include "stm32h7xx.h"
#include <stdint.h>

#define IMU_MAGIC 0xFAFAAFAF

typedef enum {
    IMU_ACC_RANGE_2G  = 0x00,
    IMU_ACC_RANGE_4G  = 0x01,
    IMU_ACC_RANGE_8G  = 0x02,
    IMU_ACC_RANGE_16G = 0x03
} IMU_ACC_RANGE;

// Accelerometer output data rate in Hertz
typedef enum {
    IMU_ACC_ODR_50   = 0x07,
    IMU_ACC_ODR_100  = 0x08,
    IMU_ACC_ODR_200  = 0x09,
    IMU_ACC_ODR_400  = 0x0A,
    IMU_ACC_ODR_800  = 0x0B,
    IMU_ACC_ODR_1600 = 0x0C
} IMU_ACC_ODR;

// degrees per second
typedef enum {
    IMU_GYRO_RANGE_125  = 0x00,
    IMU_GYRO_RANGE_250  = 0x01,
    IMU_GYRO_RANGE_500  = 0x02,
    IMU_GYRO_RANGE_1000 = 0x03,
    IMU_GYRO_RANGE_2000 = 0x04
} IMU_GYRO_RANGE;

// Gyro output data rate in Hertz
typedef enum {
    IMU_GYRO_ODR_50   = 0x07,
    IMU_GYRO_ODR_100  = 0x08,
    IMU_GYRO_ODR_200  = 0x09,
    IMU_GYRO_ODR_400  = 0x0A,
    IMU_GYRO_ODR_800  = 0x0B,
    IMU_GYRO_ODR_1600 = 0x0C
} IMU_GYRO_ODR;

typedef enum {
    eIMU_COM_FAILURE        = -1,
    eIMU_RW_BUFFER_OVERFLOW = -2,
    eIMU_NULL_PTR           = -3,
    eIMU_HARDWARE_ERR       = -4,
} IMU_STATUS;

typedef struct {
    /* Indicates fatal error */
    uint8_t fatalErr;
    /* Overload of the feature engine detected. */
    uint8_t featEngOvrld;
    /* Watchdog timer of the feature engine triggered. */
    uint8_t featEngWd;
    /* Indicates accel configuration error */
    uint8_t accConfErr;
    /* Indicates gyro configuration error */
    uint8_t gyrConfErr;
    /* Indicates SDR parity error */
    uint8_t i3cErr0;
    /* Indicates I3C error */
    uint8_t i3cErr1;
} IMUErr;

typedef struct {
    SPI_HandleTypeDef* pSPI;
    Vec3 volatile rawAccel;
    Vec3 volatile rawGyro;
    uint32_t volatile msLastAccUpdateTime;
    uint32_t volatile msLastGyroUpdateTime;
    uint32_t accRange, accODR, gyroRange, gyroODR;
    STATUS_TYPE volatile status;
    uint32_t nDummyBytes;
    uint32_t magic;
} IMU;

/*
 * Forward declared FlightContext
 */
// typedef struct FlightContext__ FlightContext;
// typedef IMU_STATUS (*imu_update_fn)(IMU *pIMU);

STATUS_TYPE IMUGetErr (IMU* pIMU, IMUErr* pOutErr);
void IMULogErr (STATUS_TYPE curImuStatus, IMUErr const* pOutErr);
STATUS_TYPE IMU2CPUInterruptHandler (IMU* pIMU, Vec3* pOutputAccel, Vec3* pOutputGyro);
STATUS_TYPE IMUReadReg (IMU const* pIMU, uint8_t reg, uint8_t* pBuf, uint32_t len);
STATUS_TYPE IMUWriteReg (IMU const* pIMU, uint8_t reg, uint8_t* pBuf, uint32_t len);
STATUS_TYPE
IMUInit (IMU* pIMU, SPI_HandleTypeDef* pSPI, IMU_ACC_RANGE accRange, IMU_ACC_ODR accODR, IMU_GYRO_RANGE gyroRange, IMU_GYRO_ODR gyroODR);

#endif // SENSORS_IMU_H
