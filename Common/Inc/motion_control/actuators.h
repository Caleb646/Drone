#ifndef MOTION_CONTROL_ACTUATORS_H
#define MOTION_CONTROL_ACTUATORS_H

#include "FreeRTOS.h"
#include "stm32h7xx.h"
#include "task.h"
#include <stdint.h>

#include "common.h"
#include "flight_context.h"


typedef struct {
    uint32_t channel1;
    uint32_t channel2;
    uint32_t channel3;
    uint32_t channel4;
    uint32_t channel5;
    uint32_t channel6;
} RadioPWMChannels;

STATUS_TYPE UpdateTargetAttitudeThrottle (
Vec3f maxAttitude,
RadioPWMChannels radio,
Vec3f* pOutputTargetAttitude,
float* pOutputThrottle);

/*
 * PID
 */
typedef struct {
    float rollP, rollI, rollD;
    float pitchP, pitchI, pitchD;
    float yawP, yawI, yawD;
    float integralLimit;
    Vec3f prevError;
    Vec3f prevIntegral;
} PIDContext;

STATUS_TYPE PIDUpdateAttitude (
PIDContext* pidContext,
Vec3f imuGyro,         // degrees per second
Vec3f currentAttitude, // degrees
Vec3f targetAttitude,  // degrees
Vec3f maxAttitude,     // degrees
float dt,
Vec3f* pOutputPIDAttitude // degrees
);

STATUS_TYPE PIDInit (PIDContext* pContext);

/*
 * PWM & Motion Control
 */
typedef struct {
    TIM_HandleTypeDef* pTimerHandle;
    TIM_TypeDef* pTimerRegisters;
    uint32_t timerChannelID;
    uint32_t usTargetDutyCycle;
} PWMHandle;

typedef struct {
    uint32_t usLeftDutyCycle;
    uint32_t usMiddleDutyCycle;
    uint32_t usRightDutyCycle;
    float maxAngle;
    // Between -max angle and +max angle
    float curAngle;
    float pitchMix;
    float yawMix;
    float rollMix;
} ServoDescriptor;

typedef struct {
    PWMHandle pwmHandle;
    ServoDescriptor pwmDescriptor;
} Servo;

typedef struct {
    uint32_t usMinDutyCycle;
    uint32_t usMaxDutyCycle;
} MotorDescriptor;

typedef struct {
    PWMHandle pwmHandle;
    MotorDescriptor pwmDescriptor;
} Motor;

typedef struct {
    Vec3 forward;
    Vec3 up;
    Vec3 right;
} AxisMap;

STATUS_TYPE PID2PWMMixer (Vec3f pidAttitude, float targetThrottle);
STATUS_TYPE PWMSend (PWMHandle* pPWM);
STATUS_TYPE ActuatorsInit (PWMHandle leftMotorPWM, PWMHandle leftServoPWM);

// void MotionControlUpdatePWM(
//     AxisMap axisConf, Vec3 mmVelSteps, Vec3 mmAngVelSteps, void *devs, uint32_t nDevs
// );

#endif // MOTION_CONTROL_ACTUATORS_H
