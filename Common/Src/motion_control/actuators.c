#include <string.h>
#include "motion_control/actuators.h"
#include "log.h"
#include "flight_context.h"

STATUS_TYPE UpdateTargetAttitudeThrottle(
    Vec3f maxAttitude, 
    RadioPWMChannels radio, 
    Vec3f *pOutputTargetAttitude, 
    float *pOutputThrottle
)
{
    *pOutputThrottle = clipf32((float)radio.channel1, 0.0f, 1.0f);

    pOutputTargetAttitude->roll = clipf32(
        (float)radio.channel2, -1.0f, 1.0f
    ) * maxAttitude.roll;

    pOutputTargetAttitude->pitch = clipf32(
        (float)radio.channel3, -1.0f, 1.0f
    ) * maxAttitude.pitch;

    pOutputTargetAttitude->yaw = clipf32(
        (float)radio.channel4, -1.0f, 1.0f
    ) * maxAttitude.yaw;

    return eSTATUS_SUCCESS;
}

STATUS_TYPE PIDUpdateAttitude(
    PIDContext *pidContext,
    Vec3f imuGyro, // degrees per second
    Vec3f currentAttitude, // degrees
    Vec3f targetAttitude, // degrees
    Vec3f maxAttitude, // degrees
    float dt, 
    Vec3f *pOutputPIDAttitude // degrees
)
{
    float P, I, D;

    P = pidContext->rollP;
    I = pidContext->rollI;
    D = pidContext->rollD;
    float rollError = targetAttitude.roll - currentAttitude.roll;
    float rollIntegral = clipf32(
        pidContext->prevIntegral.roll + rollError * dt, 
        -pidContext->integralLimit, 
        pidContext->integralLimit
    );
    float rollDerivative = imuGyro.x;
    // pOutputPIDAttitude->roll = 0.01f * (P * rollError + I * rollIntegral - D * rollDerivative);

    // Scale PID output between -1 and 1
    pOutputPIDAttitude->roll = clipf32(
        (P * rollError + I * rollIntegral - D * rollDerivative), -maxAttitude.roll, maxAttitude.roll
    ) / maxAttitude.roll;

    P = pidContext->pitchP;
    I = pidContext->pitchI;
    D = pidContext->pitchD;
    float pitchError = targetAttitude.pitch - currentAttitude.pitch;
    float pitchIntegral = clipf32(
        pidContext->prevIntegral.pitch + pitchError * dt, 
        -pidContext->integralLimit, 
        pidContext->integralLimit
    );
    float pitchDerivative = imuGyro.y;
    // pOutputPIDAttitude->pitch = 0.01f * (P * pitchError + I * pitchIntegral - D * pitchDerivative);

    // Scale PID output between -1 and 1
    pOutputPIDAttitude->pitch = clipf32(
        (P * pitchError + I * pitchIntegral - D * pitchDerivative), -maxAttitude.pitch, maxAttitude.pitch
    ) / maxAttitude.pitch;

    P = pidContext->yawP;
    I = pidContext->yawI;
    D = pidContext->yawD;
    float yawError = targetAttitude.yaw - currentAttitude.yaw;
    float yawIntegral = clipf32(
        pidContext->prevIntegral.yaw + yawError * dt, 
        -pidContext->integralLimit, 
        pidContext->integralLimit
    );
    float yawDerivative = imuGyro.z;
    // pOutputPIDAttitude->yaw = 0.01f * (P * yawError + I * yawIntegral - D * yawDerivative);

    // Scale PID output between -1 and 1
    pOutputPIDAttitude->yaw = clipf32(
        (P * yawError + I * yawIntegral - D * yawDerivative), -maxAttitude.yaw, maxAttitude.yaw
    ) / maxAttitude.yaw;

    pidContext->prevIntegral.roll = rollIntegral;
    pidContext->prevIntegral.pitch = pitchIntegral;
    pidContext->prevIntegral.yaw = yawIntegral;

    pidContext->prevError.roll = rollError;
    pidContext->prevError.pitch = pitchError;
    pidContext->prevError.yaw = yawError;

    return eSTATUS_SUCCESS;
}

STATUS_TYPE PIDInit(PIDContext *pContext)
{
    memset((void*)pContext, 0, sizeof(PIDContext));
    return eSTATUS_SUCCESS;
}

/*
* PWM & Motion Control
*/

static float ServoAngle2PWM(ServoDescriptor *pSer, float targetAngle);

static float ServoAngle2PWM(ServoDescriptor *pSer, float targetAngle)
{
    float usMinDutyCycle = 0.0f, usMaxDutyCycle = 0.0f;
    if(targetAngle < 0)
    {   
        usMinDutyCycle = (float)pSer->usLeftDutyCycle;
        usMaxDutyCycle = (float)pSer->usMiddleDutyCycle;
    }
    else if(targetAngle > 0)
    {
        usMinDutyCycle = (float)pSer->usMiddleDutyCycle;
        usMaxDutyCycle = (float)pSer->usRightDutyCycle;
    }
    else 
    {
        usMinDutyCycle = (float)pSer->usMiddleDutyCycle;
        usMaxDutyCycle = (float)pSer->usMiddleDutyCycle;
    }

    return mapf32(
        targetAngle, -pSer->maxAngle, pSer->maxAngle, usMinDutyCycle, usMaxDutyCycle
    );
}


static Motor leftMotor;
static Servo leftServo;
// static Motor rightMotor;
// static Servo rightServo;

/*
* \param pidAttitude roll, pitch, and yaw are between -1 and 1 
*/
STATUS_TYPE PID2PWMMixer(Vec3f pidAttitude, float targetThrottle)
{
    /*
    *   NWU coordinate system:
    *       + x-axis pointing to the North
    *       + y-axis pointing to the West
    *       + z-axis pointing away from the surface of the Earth
    *   + roll (pivot on x axis) right motor is higher
    *   + pitch (pivot on y axis) nose is higher than tail
    *   + yaw (pivot on z axis) right wing tip is more forward
    */

    /*
    *  Left Motor Mixing
    */
    float target = 0.0f;
    PWMHandle *pH = &leftMotor.pwmHandle;
    MotorDescriptor *pM = &leftMotor.pwmDescriptor;
    target = targetThrottle - pidAttitude.pitch + pidAttitude.roll + pidAttitude.yaw;
    pH->usTargetDutyCycle = (uint32_t)mapf32(
        target, -3.0f, 4.0f, (float)pM->usMinDutyCycle, (float)pM->usMaxDutyCycle
    );

    /*
    *  Left Servo Mixing
    */ 
    pH = &leftServo.pwmHandle;
    ServoDescriptor *pSer = &leftServo.pwmDescriptor;
    target = pSer->pitchMix * pidAttitude.pitch + pSer->rollMix * pidAttitude.roll + pSer->yawMix * pidAttitude.yaw;
    // NOTE: Maybe Roll should have a negative impact on target angle. Meaning the magnitude of the target angle is closer to 0
    // the larger pid roll is.
    // float target = pSer->pitchMix * pidAttitude.pitch + pSer->yawMix * pidAttitude.yaw;
    // pSer->targetAngle = ( clipf32(target, -1.0f, 1.0f) * pSer->maxAngle ) / ( clipf32(pSer->rollMix * pidAttitude.roll, -1.0f, 1.0f) * pSer->maxAngle );
    target = clipf32(target, -1.0f, 1.0f) * pSer->maxAngle;
    pSer->curAngle = target;
    pH->usTargetDutyCycle = (uint32_t)ServoAngle2PWM(pSer, target);

    return eSTATUS_SUCCESS;
}

STATUS_TYPE PWMSend(PWMHandle *pPWM)
{
    // uint32_t volatile *pCCR;
    // uint32_t volatile *pARR;
    // uint32_t volatile *pPSC;

    // 1,000,000 Hz / ARR = 50Hz --> ARR = 20,000
    // 1.5ms duty cycle = td (target duty cycle)
    // td / period = percentage
    // CCR / ARR = percentage
    // CCR = ARR * (1500us / 20000us)

    // 1,000,000 Hz / ARR = 2000Hz --> ARR = 500
    // 250us duty cycle = td (target duty cycle)
    // td / period = percentage
    // CCR / ARR = percentage
    // CCR = ARR * (250us / 500us)

    // *pCCR = (uint16_t)pPWM->usTargetDutyCycle;
    __HAL_TIM_SET_COMPARE(pPWM->pTimerHandle, pPWM->timerChannelID, pPWM->usTargetDutyCycle);
    
    return eSTATUS_SUCCESS;
} 

STATUS_TYPE ActuatorsInit(
    PWMHandle leftMotorPWM, PWMHandle leftServoPWM
)
{
    // Prescale 64MHz clock to 1MHz
    __HAL_TIM_SET_PRESCALER(leftMotorPWM.pTimerHandle, 64);
    // Use ARR register to scale clock from 64MHz to 4000Hz (250us)
    __HAL_TIM_SET_AUTORELOAD(leftMotorPWM.pTimerHandle, 250);
    // Set duty cycle to 0 percent
    __HAL_TIM_SET_COMPARE(leftMotorPWM.pTimerHandle, leftMotorPWM.timerChannelID, 0);

    // Prescale 64MHz clock to 1MHz
    __HAL_TIM_SET_PRESCALER(leftServoPWM.pTimerHandle, 64);
    // Use ARR register to scale clock from 64MHz to 50Hz
    // 64MHz / 20,000 = 50Hz
    __HAL_TIM_SET_AUTORELOAD(leftServoPWM.pTimerHandle, 20000);
    // Set duty cycle to 0 percent
    __HAL_TIM_SET_COMPARE(leftServoPWM.pTimerHandle, leftServoPWM.timerChannelID, 0);

    HAL_TIM_PWM_Start(leftMotorPWM.pTimerHandle, leftMotorPWM.timerChannelID);
    HAL_TIM_PWM_Start(leftServoPWM.pTimerHandle, leftServoPWM.timerChannelID);
    
    
    memset((void*)&leftMotor, 0, sizeof(Motor));
    memset((void*)&leftServo, 0, sizeof(Servo));
    // memset((void*)&rightMotor, 0, sizeof(Motor));
    // memset((void*)&rightServo, 0, sizeof(Servo));

    MotorDescriptor motorDescriptor;
    memset((void*)&motorDescriptor, 0, sizeof(MotorDescriptor));
    motorDescriptor.usMinDutyCycle = 125;
    motorDescriptor.usMaxDutyCycle = 250;

    leftMotor.pwmDescriptor = motorDescriptor;
    leftMotor.pwmHandle = leftMotorPWM;

    ServoDescriptor servoDescriptor;
    memset((void*)&servoDescriptor, 0, sizeof(ServoDescriptor));
    servoDescriptor.usLeftDutyCycle = 1000;
    servoDescriptor.usMiddleDutyCycle = 1500;
    servoDescriptor.usRightDutyCycle = 2000;

    // servoDescriptor.minAngle = -90;
    servoDescriptor.maxAngle = 20;
    servoDescriptor.curAngle = 0;

    servoDescriptor.rollMix = -0.25f;
    servoDescriptor.yawMix = 0.5f;
    servoDescriptor.pitchMix = 0.5f;


    leftServo.pwmDescriptor = servoDescriptor;
    leftServo.pwmHandle = leftServoPWM;

    return eSTATUS_SUCCESS;
}