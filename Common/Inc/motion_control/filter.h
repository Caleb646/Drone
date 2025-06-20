#ifndef MOTION_CONTROL_FILTER_H
#define MOTION_CONTROL_FILTER_H

#include "common.h"
#include <stdint.h>

typedef struct {
    // estimated orientation quaternion elements with initial conditions
    float seq1, seq2, seq3, seq4;
    // sampling period in seconds
    float dt;
    // gyroscope measurement error in rad/s
    float gyroMeasureError;
    float beta;
} FilterMadgwickContext;

STATUS_TYPE
FilterMadgwick6DOF (FilterMadgwickContext* pContext, Vec3f gyro, Vec3f accel, Vec3f* pOutputAttitude);
STATUS_TYPE
FilterMadgwick9DOF (FilterMadgwickContext* pContext, Vec3f gyro, Vec3f accel, Vec3f magno, Vec3f* pOutputAttitude);
STATUS_TYPE FilterMadgwickInit (FilterMadgwickContext* pContext);

#endif // MOTION_CONTROL_FILTER_H
