
#ifndef CAN_VEHICLE_ESTIMATE_H  
#define CAN_VEHICLE_ESTIMATE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma execution_character_set("utf-8")
#endif

#include <vector>
#include "linear_r3.h"

#include <math.h>

class CAN_VehicleEstimate
{
public:

    /// @brief construct  obj
    CAN_VehicleEstimate();
	
    ~CAN_VehicleEstimate() {}

    void Initialize( );


	void UpdateVehicleState(double steer_angle, double vehicle_speed           , double dt );
		
	// 获取当前汽车的位置速度
	void GetVelPos(double (&vel)[2], double (&pos)[2]);

	// 重置汽车的状态数据
	// 包括: 速度、位置、航向角
	void ResetState();


private:
	double m_vehicle_L; // 汽车的轴距
	
	double m_min_steer_angle; //最小的有效方向盘转角，小于这个角度不计算
	
	double m_k_steer2wheel_angle; // 方向盘转角到虚拟前轮转角的系数
	
	double m_virtual_front_angle; // 虚拟前轮的角度
	
	double m_beta; // 汽车侧滑角
	
	double m_fai; // 汽车航向角(相对车道线，定时会清零)
	
	double m_vehicle_vel[2];
	double m_vehicle_pos[2];

};


#endif  // CAN_VEHICLE_ESTIMATE_H