#ifndef IMU_ATTITUDE_ESTIMATE_H
#define IMU_ATTITUDE_ESTIMATE_H

#include "common/relative_locate/linear_r3.h"

class ImuAttitudeEstimate
{
public:

    /// @brief construct  obj
    ImuAttitudeEstimate();
    
    ~ImuAttitudeEstimate() {}

    void Initialize( );

    /// att_new:�µ���̬
    /// acc_data: ���ٶ�����
    /// gyro_data: ����������
    /// dt: ǰ�������������ݸ��µ�ʱ���
    void UpdataAttitude( const double acc_data[3], const double gyro_data[3], double dt);

    void GetAttitude(double att[3]);

    /// һ�׵�ͨ����
    int LowpassFilter3f(const double y_pre[3], const double x_new[3], double dt, const double filt_hz, double y_new[3] );

    int AccDataCalibation(const double acc_data_raw[3], double acc_data_ned[3] );

    int SetAccCalibationParam(double A0[3], double A1[3][3]);

    int GyrocDataCalibation(const double gyro_data_raw[3], double gyro_data_new[3] );

    void ResetState();    


private:
    enum
    {
      X_AXIS = 0,
      Y_AXIS,
      Z_AXIS
    };
    
    double m_factor_acc_gyro[3]; // ���ٶȼ���������̬��ϵ��
    double m_att[3];
    double m_gyro_angle[3];
    int m_att_init_counter;// = 20;
    
    // Y1ģ��ļ��ٶȼ�У������
    double m_accel_range_scale;
    double m_A0[3];// = {0.0628f, 0.0079f, -0.0003f};
    double m_A1[3][3]; // = {0.9986f, -0.0027f, 0.0139f, 0.0164f, 0.9993f, -0.0176f, -0.0159f, 0.0064f, 0.9859f };
    double m_gyro_range_scale;
    double m_gyro_drift[3]; // = {0.0155f, -0.0421f, -0.0217f};  // ��������ƫ�����߹���    
    double m_gyro_offset[3]; // gyro calibation;
    

};


#endif  // IMU_ATTITUDE_ESTIMATE_H
