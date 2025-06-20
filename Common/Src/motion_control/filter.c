#include "motion_control/filter.h"
#include <math.h>
#include <string.h>

STATUS_TYPE
FilterMadgwick6DOF (FilterMadgwickContext* pContext, Vec3f gyro, Vec3f accel, Vec3f* pOutputAttitude) {
    // Source: https://courses.cs.washington.edu/courses/cse474/17wi/labs/l4/madgwick_internal_report.pdf

    // convert degrees per second to radians per second
    float w_x = gyro.x * 0.017453F;
    float w_y = gyro.y * 0.017453F;
    float w_z = gyro.z * 0.017453F;

    float a_x = accel.x;
    float a_y = accel.y;
    float a_z = accel.z;

    float dt   = pContext->dt;
    float beta = pContext->beta;

    float SEq_1 = pContext->seq1;
    float SEq_2 = pContext->seq2;
    float SEq_3 = pContext->seq3;
    float SEq_4 = pContext->seq4;

    float norm           = 0.0F;
    float SEqDot_omega_1 = 0.0F;
    float SEqDot_omega_2 = 0.0F;
    float SEqDot_omega_3 = 0.0F;
    float SEqDot_omega_4 = 0.0F; // quaternion derrivative from gyroscopes elements float
    float f_1         = 0.0F;
    float f_2         = 0.0F;
    float f_3         = 0.0F; // objective function elements
    float J_11or24    = 0.0F;
    float J_12or23    = 0.0F;
    float J_13or22    = 0.0F;
    float J_14or21    = 0.0F;
    float J_32        = 0.0F;
    float J_33        = 0.0F;
    float SEqHatDot_1 = 0.0F;
    float SEqHatDot_2 = 0.0F;
    float SEqHatDot_3 = 0.0F;
    float SEqHatDot_4 = 0.0F; // Axulirary variables to avoid reapeated calcualtions
    float halfSEq_1 = 0.5F * SEq_1;
    float halfSEq_2 = 0.5F * SEq_2;
    float halfSEq_3 = 0.5F * SEq_3;
    float halfSEq_4 = 0.5F * SEq_4;
    float twoSEq_1  = 2.0F * SEq_1;
    float twoSEq_2  = 2.0F * SEq_2;
    float twoSEq_3  = 2.0F * SEq_3;

    if (!((a_x == 0.0F) && (a_y == 0.0F) && (a_z == 0.0F))) {
        norm = 1.0F / (float)sqrt (a_x * a_x + a_y * a_y + a_z * a_z);
        a_x *= norm;
        a_y *= norm;
        a_z *= norm;
    }

    // Compute the objective function and Jacobian
    f_1 = twoSEq_2 * SEq_4 - twoSEq_1 * SEq_3 - a_x;
    f_2 = twoSEq_1 * SEq_2 + twoSEq_3 * SEq_4 - a_y;
    f_3 = 1.0F - twoSEq_2 * SEq_2 - twoSEq_3 * SEq_3 - a_z;

    J_11or24 = twoSEq_3;
    J_12or23 = 2.0F * SEq_4;
    J_13or22 = twoSEq_1;
    J_14or21 = twoSEq_2;
    J_32     = 2.0F * J_14or21;
    J_33     = 2.0F * J_11or24;

    SEqHatDot_1 = J_14or21 * f_2 - J_11or24 * f_1;
    SEqHatDot_2 = J_12or23 * f_1 + J_13or22 * f_2 - J_32 * f_3;
    SEqHatDot_3 = J_12or23 * f_2 - J_33 * f_3 - J_13or22 * f_1;
    SEqHatDot_4 = J_14or21 * f_1 + J_11or24 * f_2;

    // negated in matrix multiplication
    norm = 1.0F / (float)sqrt (
                  SEqHatDot_1 * SEqHatDot_1 + SEqHatDot_2 * SEqHatDot_2 +
                  SEqHatDot_3 * SEqHatDot_3 + SEqHatDot_4 * SEqHatDot_4);
    SEqHatDot_1 *= norm;
    SEqHatDot_2 *= norm;
    SEqHatDot_3 *= norm;
    SEqHatDot_4 *= norm;

    SEqDot_omega_1 = -halfSEq_2 * w_x - halfSEq_3 * w_y - halfSEq_4 * w_z;
    SEqDot_omega_2 = halfSEq_1 * w_x + halfSEq_3 * w_z - halfSEq_4 * w_y;
    SEqDot_omega_3 = halfSEq_1 * w_y - halfSEq_2 * w_z + halfSEq_4 * w_x;
    SEqDot_omega_4 = halfSEq_1 * w_z + halfSEq_2 * w_y - halfSEq_3 * w_x;

    SEq_1 += (SEqDot_omega_1 - (beta * SEqHatDot_1)) * dt;
    SEq_2 += (SEqDot_omega_2 - (beta * SEqHatDot_2)) * dt;
    SEq_3 += (SEqDot_omega_3 - (beta * SEqHatDot_3)) * dt;
    SEq_4 += (SEqDot_omega_4 - (beta * SEqHatDot_4)) * dt;

    norm =
    1.0F / (float)sqrt (SEq_1 * SEq_1 + SEq_2 * SEq_2 + SEq_3 * SEq_3 + SEq_4 * SEq_4);
    SEq_1 *= norm;
    SEq_2 *= norm;
    SEq_3 *= norm;
    SEq_4 *= norm;

    pContext->seq1 = SEq_1;
    pContext->seq2 = SEq_2;
    pContext->seq3 = SEq_3;
    pContext->seq4 = SEq_4;

    float q0 = SEq_1;
    float q1 = SEq_2;
    float q2 = SEq_3;
    float q3 = SEq_4;

    // Compute quaternion angles.
    // Then convert angles in radians to degrees.
    // North-West-Up (NWU)
    // x-axis pointing to the North
    // y-axis pointing to the West
    // z-axis pointing away from the surface of the Earth
    pOutputAttitude->roll =
    atan2 (q0 * q1 + q2 * q3, 0.5F - q1 * q1 - q2 * q2) * 57.29577951F;
    pOutputAttitude->pitch =
    -asin (clipf32 (-2.0F * (q1 * q3 - q0 * q2), -0.999999F, 0.999999F)) * 57.29577951F;
    pOutputAttitude->yaw =
    -atan2 (q1 * q2 + q0 * q3, 0.5F - q2 * q2 - q3 * q3) * 57.29577951F;

    return eSTATUS_SUCCESS;
}

STATUS_TYPE
FilterMadgwick9DOF (FilterMadgwickContext* pContext, Vec3f gyro, Vec3f accel, Vec3f magno, Vec3f* pOutputAttitude) {
    return eSTATUS_SUCCESS;
}

STATUS_TYPE FilterMadgwickInit (FilterMadgwickContext* pContext) {
    // Initialization values: https://courses.cs.washington.edu/courses/cse474/17wi/labs/l4/madgwick_internal_report.pdf
    memset ((void*)pContext, 0, sizeof (FilterMadgwickContext));
    pContext->dt               = 0.001F;
    pContext->gyroMeasureError = M_PI * (5.0F / 180.0F);
    pContext->beta = sqrt (3.0F / 4.0F) * pContext->gyroMeasureError;
    pContext->seq1 = 1.0F;
    pContext->seq2 = 0.0F;
    pContext->seq3 = 0.0F;
    pContext->seq4 = 0.0F;
    return eSTATUS_SUCCESS;
}