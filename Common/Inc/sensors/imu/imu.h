#ifndef SENSORS_IMU_H
#define SENSORS_IMU_H

#include "common.h"
#include "hal.h"
#include "sensors/imu/bmixxx.h"
#include <stdint.h>


#define IMU_MAGIC 0xFAFAAFAF
#define IMU_LOG_CALIB_DATA(rslt, error) \
    LOG_DATA (LOG_DATA_TYPE_IMU_CALIB, "{\"rslt\":%u,\"error\":%u}", rslt, error)

typedef enum {
    eIMU_ACC_RANGE_2G  = 0x00,
    eIMU_ACC_RANGE_4G  = 0x01,
    eIMU_ACC_RANGE_8G  = 0x02,
    eIMU_ACC_RANGE_16G = 0x03
} IMU_ACC_RANGE;

// Accelerometer output data rate in Hertz
typedef enum {
    eIMU_ACC_ODR_50   = 0x07,
    eIMU_ACC_ODR_100  = 0x08,
    eIMU_ACC_ODR_200  = 0x09,
    eIMU_ACC_ODR_400  = 0x0A,
    eIMU_ACC_ODR_800  = 0x0B,
    eIMU_ACC_ODR_1600 = 0x0C
} IMU_ACC_ODR;

typedef enum {
    eIMU_ACC_BW_HALF    = BMI3_ACC_BW_ODR_HALF,
    eIMU_ACC_BW_QUARTER = BMI3_ACC_BW_ODR_QUARTER
} IMU_ACC_BW;

typedef enum {
    eIMU_ACC_AVG_1  = BMI3_ACC_AVG1,
    eIMU_ACC_AVG_2  = BMI3_ACC_AVG2,
    eIMU_ACC_AVG_4  = BMI3_ACC_AVG4,
    eIMU_ACC_AVG_8  = BMI3_ACC_AVG8,
    eIMU_ACC_AVG_16 = BMI3_ACC_AVG16,
    eIMU_ACC_AVG_32 = BMI3_ACC_AVG32,
    eIMU_ACC_AVG_64 = BMI3_ACC_AVG64
} IMU_ACC_AVG;

typedef enum {
    eIMU_ACC_MODE_DISABLE   = BMI3_ACC_MODE_DISABLE,
    eIMU_ACC_MODE_LOW_PWR   = BMI3_ACC_MODE_LOW_PWR,
    eIMU_ACC_MODE_NORMAL    = BMI3_ACC_MODE_NORMAL,
    eIMU_ACC_MODE_HIGH_PERF = BMI3_ACC_MODE_HIGH_PERF
} IMU_ACC_MODE;

// degrees per second
typedef enum {
    eIMU_GYRO_RANGE_125  = 0x00,
    eIMU_GYRO_RANGE_250  = 0x01,
    eIMU_GYRO_RANGE_500  = 0x02,
    eIMU_GYRO_RANGE_1000 = 0x03,
    eIMU_GYRO_RANGE_2000 = 0x04
} IMU_GYRO_RANGE;

// Gyro output data rate in Hertz
typedef enum {
    eIMU_GYRO_ODR_50   = 0x07,
    eIMU_GYRO_ODR_100  = 0x08,
    eIMU_GYRO_ODR_200  = 0x09,
    eIMU_GYRO_ODR_400  = 0x0A,
    eIMU_GYRO_ODR_800  = 0x0B,
    eIMU_GYRO_ODR_1600 = 0x0C
} IMU_GYRO_ODR;

typedef enum {
    eIMU_GYRO_BW_HALF    = BMI3_GYR_BW_ODR_HALF,
    eIMU_GYRO_BW_QUARTER = BMI3_GYR_BW_ODR_HALF
} IMU_GYRO_BW;

typedef enum {
    eIMU_GYRO_AVG_1  = BMI3_GYR_AVG1,
    eIMU_GYRO_AVG_2  = BMI3_GYR_AVG2,
    eIMU_GYRO_AVG_4  = BMI3_GYR_AVG4,
    eIMU_GYRO_AVG_8  = BMI3_GYR_AVG8,
    eIMU_GYRO_AVG_16 = BMI3_GYR_AVG16,
    eIMU_GYRO_AVG_32 = BMI3_GYR_AVG32,
    eIMU_GYRO_AVG_64 = BMI3_GYR_AVG64
} IMU_GYRO_AVG;

typedef enum {
    eIMU_GYRO_MODE_DISABLE   = BMI3_GYR_MODE_DISABLE,
    eIMU_GYRO_MODE_SUSPEND   = BMI3_GYR_MODE_SUSPEND,
    eIMU_GYRO_MODE_LOW_PWR   = BMI3_GYR_MODE_LOW_PWR,
    eIMU_GYRO_MODE_NORM      = BMI3_GYR_MODE_NORMAL,
    eIMU_GYRO_MODE_HIGH_PERF = BMI3_GYR_MODE_HIGH_PERF
} IMU_GYRO_MODE;

typedef enum {
    eIMU_COM_FAILURE        = -1,
    eIMU_RW_BUFFER_OVERFLOW = -2,
    eIMU_NULL_PTR           = -3,
    eIMU_HARDWARE_ERR       = -4,
} IMU_STATUS;

typedef struct {
    uint16_t err;
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
    IMU_ACC_RANGE range;
    IMU_ACC_ODR odr;
    IMU_ACC_BW bw;
    IMU_ACC_AVG avg;
    IMU_ACC_MODE mode;
} IMUAccConf;

typedef struct {
    IMU_GYRO_RANGE range;
    IMU_GYRO_ODR odr;
    IMU_GYRO_BW bw;
    IMU_GYRO_AVG avg;
    IMU_GYRO_MODE mode;
} IMUGyroConf;

typedef struct {
    uint8_t errStatus;
    uint8_t selfCalibComplete;
    uint8_t gyroSelfCalibResult;
    uint8_t selfTestResult;
    uint8_t axisRemapComplete;
    uint8_t systemState;
} IMUFeatureStatus;

typedef struct {
    SPI_HandleTypeDef* pSPI;
    Vec3 volatile rawAccel;
    Vec3 volatile rawGyro;
    uint32_t volatile msLastAccUpdateTime;
    uint32_t volatile msLastGyroUpdateTime;
    IMUAccConf aconf;
    IMUGyroConf gconf;
    STATUS_TYPE volatile status;
    uint32_t nDummyBytes;
    uint32_t magic;
} IMU;

typedef struct {
    /*! Stores the self-calibration result */
    int8_t result;
    /*! Stores the self-calibration error codes status */
    uint8_t error;
} IMUSelfCalibResult;

/*
 * Forward declared FlightContext
 */
// typedef struct FlightContext__ FlightContext;
// typedef IMU_STATUS (*imu_update_fn)(IMU *pIMU);

STATUS_TYPE IMUGetDeviceErr (IMU* pIMU, IMUErr* pOutErr);
void IMULogDeviceErr (IMU* pIMU, IMUErr const* pErr);
STATUS_TYPE IMUEnableInterrupts (IMU const* pIMU);
STATUS_TYPE IMUHandleErr (IMU* pIMU);
STATUS_TYPE IMUPollData (IMU* pIMU, Vec3f* pOutputAccel, Vec3f* pOutputGyro);
STATUS_TYPE IMUUpdateAccel (IMU* pIMU);
STATUS_TYPE IMUUpdateGyro (IMU* pIMU);
STATUS_TYPE IMUGetConf (IMU* pIMU, IMUAccConf* pAConf, IMUGyroConf* pGConf);
STATUS_TYPE IMUGetAltConf (IMU* pIMU, IMUAccConf* pAConf, IMUGyroConf* pGConf);
STATUS_TYPE
IMUConvertRaw (IMU_ACC_RANGE aRange, Vec3 ra, IMU_GYRO_RANGE gRange, Vec3 rg, Vec3f* pAccelOut, Vec3f* pGyroOut);
STATUS_TYPE
IMUSetConf (IMU* pIMU, IMUAccConf const* pAConf, IMUGyroConf const* pGConf);
STATUS_TYPE
IMUSetAltConf (IMU* pIMU, IMUAccConf const* pAConf, IMUGyroConf const* pGConf);
STATUS_TYPE
IMUCompareConfs (IMUAccConf aconf, IMUGyroConf gconf, IMUAccConf aconf2, IMUGyroConf gconf2);
STATUS_TYPE IMU2CPUInterruptHandler (IMU* pIMU);
STATUS_TYPE IMUReadReg (IMU const* pIMU, uint8_t reg, uint8_t* pBuf, uint32_t len);
STATUS_TYPE IMUWriteReg (IMU const* pIMU, uint8_t reg, uint8_t* pBuf, uint32_t len);
STATUS_TYPE IMUInit (IMU* pIMU, SPI_HandleTypeDef* pSPI, IMUAccConf aconf, IMUGyroConf gconf);

#endif // SENSORS_IMU_H
