
#ifndef IMU_ATTITUDE_ESTIMATE_H
#define IMU_ATTITUDE_ESTIMATE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma execution_character_set("utf-8")
#endif

#include <vector>
#include "linear_r3.h"

struct def_attitude_T
{
	double mAccAVSFactor[3]; // ���ٶȼ�ƽ���Ĳ���
	double mFactorAccGyro[3]; // ���ٶȼ���������̬��ϵ��
	double	mAtt[3];
	double	SmoothAcc[2];
	double	SmoothAccAngle[3]; // ƽ��֮��ļ��ٶȼƽǶ�
	double	RawAccAngle[3]; // ԭʼ�ļ��ٶȼƽǶ�
	double  mGyroAngle[3];
	
};

class ImuAttitudeEstimate
{
public:

    /// @brief construct  obj
    ImuAttitudeEstimate() {}
	
    ~ImuAttitudeEstimate() {}

    void Initialize( );

	
	/// @brief ��ȡ����ͷ��̬
	///
	/// @param axis
	/// att_new:�µ���̬
	/// AccData: ���ٶ�����
	/// GyroData: ����������
	/// dt: ǰ�������������ݸ��µ�ʱ���
	void Get_Attitude(double (&att_new)[3], double AccData[3], double GyroData[3], double dt);

	/// һ�׵�ͨ����
	int LowpassFilter3f(double (&y_new)[3], double y_pre[3], double x_new[3], double dt, double filt_hz);



	enum
	{
	  X_AXIS = 0,
	  Y_AXIS,
	  Z_AXIS
	};

	def_attitude_T gAttVar;


private:
	int m_index_counter = 0;
	 



};


#endif  // IMU_ATTITUDE_ESTIMATE_H
