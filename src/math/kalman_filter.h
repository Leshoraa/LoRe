/**
 * @file kalman_filter.h
 * @brief Discrete 2D linear Kalman tracking filter with dynamic measurement covariance.
 */

#ifndef LORE_KALMAN_FILTER_H
#define LORE_KALMAN_FILTER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct KalmanFilter1D
 * @brief Discrete 1D linear Kalman filter state vector [p, v]^T.
 */
typedef struct {
    float p;       /* Estimated position (pixels) */
    float v;       /* Estimated velocity (pixels/sec) */
    float P00;     /* State covariance P[0,0] */
    float P01;     /* State covariance P[0,1] */
    float P11;     /* State covariance P[1,1] */
} KalmanFilter1D;

/**
 * @struct KalmanTracker2D
 * @brief 2D Cartesian target tracking container managing X and Y Kalman channels.
 */
typedef struct {
    KalmanFilter1D kf_x;
    KalmanFilter1D kf_y;
    float w;
    float h;
    bool active;
    uint32_t last_update_us;
} KalmanTracker2D;

void kf1d_init(KalmanFilter1D *kf, float init_p);
void kf1d_predict(KalmanFilter1D *kf, float dt, float q_accel);
void kf1d_update(KalmanFilter1D *kf, float z, float R);
void kf2d_tracker_init(KalmanTracker2D *tracker);
float kf2d_compute_dynamic_r(float dist_innovation, int skin_pixel_cnt, float lock_conf);

#ifdef __cplusplus
}
#endif

#endif /* LORE_KALMAN_FILTER_H */
