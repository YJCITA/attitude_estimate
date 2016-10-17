#ifndef CAN_VEHICLE_ESTIMATE_H  
#define CAN_VEHICLE_ESTIMATE_H

#include <vector>
#include <math.h>
#include <stdio.h>

#include "common/relative_locate/linear_r3.h"
#include "datafusion_math.h"

class CAN_VehicleEstimate
{
public:

    /// @brief construct  obj
    CAN_VehicleEstimate();
    
    ~CAN_VehicleEstimate() {}

    void Initialize( );

    void UpdateVehicleState(double steer_angle, double vehicle_speed           , double dt );

    // ����IMU+speed���㳵���˶���Ϣ
    void UpdateVehicleStateImu(double yaw, double vehicle_speed, double dt );

    // ��ȡ��ǰ������λ���ٶ�
    void GetVehicleState(double vel[2], double pos[2], double *yaw);

    // ����������״̬����
    // ����: �ٶȡ�λ�á������
    void ResetState();


private:
    double m_vehicle_L; // ���������    
    double m_min_steer_angle; //��С����Ч������ת�ǣ�С������ǶȲ�����    
    double m_k_steer2wheel_angle; // ������ת�ǵ�����ǰ��ת�ǵ�ϵ��    
    double m_virtual_front_angle; // ����ǰ�ֵĽǶ�    
    double m_beta; // �����໬��    
    double m_fai; // ���������(��Գ����ߣ���ʱ������)    
    double m_vehicle_vel[2];
    double m_vehicle_pos[2];
    double m_yaw; // ���������(��Գ����ߣ���ʱ������)

};


#endif  // CAN_VEHICLE_ESTIMATE_H