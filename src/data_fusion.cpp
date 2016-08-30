#include "data_fusion.h"


using namespace common;

DataFusion::DataFusion()
{

}


void DataFusion::Initialize(CameraPara camera_para_t, IPMPara ipm_para_t )
{
	infile_log.open("data/log.txt");	   // ofstream
	
	/// CAN
	is_steer_angle_OK = 0; // ��ǰ�Ƿ�steer�����Ѿ�����
	can_timestamp_pre = 0.0f;
	isFirstTime_can = 1;

	// IMU
	acc_filt_hz = 5.0f; // ���ٶȼƵĵ�ͨ��ֹƵ��
	isFirstTime_att = 1; // �Ƿ��ǵ�һ�ν���
	imu_timestamp = 0.0f;
	pre_imu_timestamp = 0.0f; // IMU�����ϴεõ���ʱ�� 

	// parameter set up
	camera_para.fu = camera_para_t.fu;
	camera_para.fv = camera_para_t.fv;
	camera_para.cu = camera_para_t.cu;
	camera_para.cv = camera_para_t.cv;
	camera_para.height = camera_para_t.height; // m
	camera_para.pitch = camera_para_t.pitch* CV_PI / 180;
	camera_para.yaw = camera_para_t.yaw* CV_PI / 180;
	camera_para.image_width = camera_para_t.image_width;
	camera_para.image_height = camera_para_t.image_height;
	BirdPerspectiveMapping bp_mapping(camera_para);

	// ipm para
	ipm_para.x_limits[0] = ipm_para_t.x_limits[0];
	ipm_para.x_limits[1] = ipm_para_t.x_limits[1];
	ipm_para.y_limits[0] = ipm_para_t.y_limits[0];
	ipm_para.y_limits[1] = ipm_para_t.y_limits[1];
	ipm_para.x_scale = ipm_para_t.x_scale;
	ipm_para.y_scale = ipm_para_t.y_scale;
	bp_mapping.GetUVLimitsFromXY(&ipm_para);

	// �ⲿlane
	isFirstTime_Lane = 1;
	att_cur[0] = 0.0;
	att_cur[1] = 0.0;
	att_cur[2] = 0.0;
	vehicle_vel[0] = 0.0;
	vehicle_vel[1] = 0.0;
	vehicle_pos[0] = 0.0;
	vehicle_pos[1] = 0.0;
	vehicle_fai = 0.0;
	
	att_pre[0] = 0.0;
	att_pre[1] = 0.0;
	att_pre[2] = 0.0;
	vehicle_vel_pre[0] = 0.0;
	vehicle_vel_pre[1] = 0.0;
	vehicle_pos_pre[0] = 0.0;
	vehicle_pos_pre[1] = 0.0;
	vehicle_fai_pre = 0.0;

	isFirsttimeGetParameter = 1;
	m_new_lane_parameter_get = 0;
}


int DataFusion::data_fusion_main()
{
//	while(1)				
//	{	
		while(m_new_lane_parameter_get && m_camera_match_state != -1)
		{
			if(getline(infile_log, buffer_log))
			{
				string data_flag;
				double camera_timestamp_raw[2];
				ss_tmp.clear();
				ss_tmp.str(buffer_log);
				ss_tmp>>camera_timestamp_raw[0]>>camera_timestamp_raw[1]>>data_flag;
				double camera_timestamp;
				if(data_flag == "cam_frame")
				{
					camera_timestamp = camera_timestamp_raw[0] + camera_timestamp_raw[1]*1e-6;
					if( camera_timestamp == m_cur_image_stamptime)
					{
						m_camera_match_state = 1; // ʱ����Ѿ�ƥ��
						m_new_lane_parameter_get = 0;

//						att_pre[0] = att_cur[0];
//						att_pre[1] = att_cur[1];
//						att_pre[2] = att_cur[2];
//						
//						vehicle_pos_pre[0] = vehicle_pos[0];
//						vehicle_pos_pre[1] = vehicle_pos[1];
//						vehicle_fai_pre = vehicle_fai;

						imu_attitude_estimate.GetAttitude(att_cur);
						can_vehicle_estimate.GetVelPosFai(vehicle_vel, vehicle_pos, vehicle_fai);						
						
					}else if( camera_timestamp > m_cur_image_stamptime)
					{
						m_camera_match_state = -1; // ����ʱ����Ѿ���ʱ
					}
				}else
				{
					// IMU CAN��Ϣ�ں�
					run_fusion( buffer_log, data_flag);
				}
			}			
		}
//	 }

	return 1;
}


int DataFusion::run_fusion( string buffer_log, 	string data_flag)
{
	ss_log.clear();
	ss_log.str(buffer_log);

	if(data_flag == "Gsensor")
	{			
		double AccData_raw[3]; // accԭʼ����ϵ�µ�
		double AccData_NED[3]; // �������ϵ
		double AccDataFilter[3]; // һ�׵�֮ͨ�������
		double GyroData_raw[3];
		double GyroData_NED[3];			
		double imu_time_raw[2];
		double imu_temperature;	
		string imu_flag;

        ss_log>>imu_time_raw[0]>>imu_time_raw[1]>>imu_flag>>AccData_raw[0]>>AccData_raw[1]>>AccData_raw[2]
			  >>GyroData_raw[0]>>GyroData_raw[1]>>GyroData_raw[2]>>imu_temperature;
		imu_timestamp = imu_time_raw[0] + imu_time_raw[1]*1e-6;			
		imu_attitude_estimate.AccDataCalibation(AccData_NED, AccData_raw);// ԭʼ����У��
		imu_attitude_estimate.GyrocDataCalibation(GyroData_NED, GyroData_raw);
		if(isFirstTime_att)
		{
			isFirstTime_att = 0;
			pre_imu_timestamp = imu_timestamp;
			AccDataFilter[0] = AccData_NED[0];
			AccDataFilter[1] = AccData_NED[1];
			AccDataFilter[2] = AccData_NED[2];					
		}else{
			double dt_imu = imu_timestamp - pre_imu_timestamp;					
			imu_attitude_estimate.LowpassFilter3f(AccDataFilter, AccDataFilter, AccData_NED, dt_imu, acc_filt_hz);						
			imu_attitude_estimate.UpdataAttitude(AccDataFilter, GyroData_NED, dt_imu);
			pre_imu_timestamp = imu_timestamp;	

			// save att
			imu_attitude_estimate.GetAttitude(struct_att.att);
			struct_att.timestamp = imu_timestamp;
			queue_att.push(struct_att);
		}
		
	}else if(data_flag == "StDir")
	{
		int steer_direction;	
		double steer_angle_t;
		double steer_raw_timestamp[2];
		string steer_str[2];			
		ss_log>>steer_raw_timestamp[0]>>steer_raw_timestamp[1]>>steer_str[0]>>steer_direction>>steer_str[0]>>steer_angle_t;				
		steer_timestamp = steer_raw_timestamp[0] + steer_raw_timestamp[1]*1e-6;
		if(steer_direction == 1)
		{
			steer_angle_deg = steer_angle_t; // 1:right +
		}else{
			steer_angle_deg = -steer_angle_t;
		}
		is_steer_angle_OK = 1;		
	}else if(data_flag == "speed")
	{	
		if( is_steer_angle_OK == 1)
		{
			is_steer_angle_OK = 0;
			double speed_raw_timestamp[2];	
			string speed_str;
			ss_log>>speed_raw_timestamp[0]>>speed_raw_timestamp[1]>>speed_str>>speed_can;	
			speed_can = speed_can/3.6;// km/h-->m/s
			speed_timestamp = speed_raw_timestamp[0] + speed_raw_timestamp[1]*1e-6;
			can_timestamp = speed_timestamp;

			if(isFirstTime_can)
			{
				can_timestamp_pre = speed_timestamp;					
				isFirstTime_can = 0;
			}else{
				double dt_can = can_timestamp - can_timestamp_pre;
				can_vehicle_estimate.UpdateVehicleState(steer_angle_deg*D2R, speed_can, dt_can );
				can_timestamp_pre = can_timestamp;
				
				// save queue
				struct_vehicle_state.timestamp = can_timestamp;
				can_vehicle_estimate.GetVelPosFai(struct_vehicle_state.vel, struct_vehicle_state.pos, struct_vehicle_state.fai);

//				printf("steer: %f speed: %f  dt: %f\n", steer_angle_deg, speed_can, dt_can);
//				printf("vel: %f, %f pos: %f, %f\n", struct_vehicle_state.vel[0], struct_vehicle_state.vel[1], struct_vehicle_state.pos[0], struct_vehicle_state.pos[1]);
			}
		}			
	}
	return 1;
}


int DataFusion::polyfit(std::vector<float>* lane_coeffs, const cv::Mat& xy_feature, int order )
{  
	int feature_points_num = xy_feature.cols;
	std::vector<float> x(feature_points_num);
	std::vector<float> y(feature_points_num);
	cv::Mat A = cv::Mat(feature_points_num, order + 1, CV_32FC1);
	cv::Mat b = cv::Mat(feature_points_num+1, 1, CV_32FC1);

		 for (int i = 0; i < feature_points_num; i++) 
		{
			x[i] = xy_feature.at<float>(0, i);
			y[i] = xy_feature.at<float>(1, i); 
	
			for (int j = 0; j <= order; j++) 
			{
				A.at<float>(i, j) = pow(y[i], j);
			}
			b.at<float>(i) = x[i];
		}
		
		cv::Mat coeffs;
		int ret = cv::solve(A, b, coeffs, CV_SVD);
		if(ret<=0)
		{	
			printf("cv:solve error!!!\n");
			return -1;
		}
		
		for(int i=0; i<order+1; i++)
		{
			lane_coeffs->push_back(coeffs.at<float>(i,0));
		}	
		return 1;
}


int DataFusion::GetLanePredictParameter(cv::Mat& lane_coeffs_predict, double image_timestamp, cv::Mat lane_coeffs_pre, double lane_num, double m_order )
{
	// ����ʱ���
	m_cur_image_stamptime = image_timestamp;
	
	m_new_lane_parameter_get = 1;
	m_camera_match_state = 0; // ����Ϊ��ʼ״̬	
	if(isFirsttimeGetParameter)
	{
		isFirsttimeGetParameter = 0;
		m_pre_image_stamptime = image_timestamp;
		lane_coeffs_pre.copyTo(lane_coeffs_predict);
	}else
	{
		// ����ʱ��������в��ң�֪������cameraʱ�����get��ʱ���ƥ��
		data_fusion_main();
		
		if(m_camera_match_state == 1)
		{			
			LanePredict(lane_coeffs_predict, lane_coeffs_pre, lane_num, m_order);
		}else // if (m_camera_match_state == -1)
		{
			std::cout<<"!!!error camera timestamp dismatch"<<endl;
			return -1;
		}
		
	}
	return 1;	
	
}

// lane_coeffs_pre: ÿһ�д���һ������
int DataFusion::LanePredict(cv::Mat& lane_coeffs_predict, cv::Mat lane_coeffs_pre, double lane_num, double m_order)
{
/// iput:
//	m_order = 1;
//	lane_num = 1;
	


/// init:(����)
	int lane_points_nums = 5; // ÿһ�쳵����ȡ������������
	double X[5] = {2.0, 5.0, 10.0, 20.0, 35.0};

//	att_cur[0] = 0;  // ͨ��get���
//	att_cur[1] = 0;
//	att_cur[2] = 0;
//	vehicle_pos[0] = 0;
//	vehicle_pos[1] = 0;

// ������������֮֡���״̬�仯	
	if (isFirstTime_Lane)
	{
		isFirstTime_Lane = 0;
		att_pre[0] = att_cur[0];
		att_pre[1] = att_cur[1];
		att_pre[2] = att_cur[2];
		vehicle_pos_pre[0] = vehicle_pos[0];
		vehicle_pos_pre[1] = vehicle_pos[1];
	}
	// ��pos��������ϵת����ת����preʱ��Ϊ��ʼ����
	double d_pos_tmp[2]; // ǰ����֡�ڳ�ʼ����ϵ�µ������˶�
	double d_pos_new_c[2]; // ����preΪ�����µ������˶�
	d_pos_tmp[0] = vehicle_pos[0] - vehicle_pos_pre[0];
	d_pos_tmp[1] = vehicle_pos[1] - vehicle_pos_pre[1]; 		
	d_pos_new_c[0] = cosf(vehicle_fai_pre)*d_pos_tmp[0] + sinf(vehicle_fai_pre)*d_pos_tmp[1];
	d_pos_new_c[1] = -sinf(vehicle_fai_pre)*d_pos_tmp[0] + cos(vehicle_fai_pre)*d_pos_tmp[1];
	
	double dyaw = att_cur[2] - att_pre[2]; 
	double Rn2c_kT[2][2];
	Rn2c_kT[0][0] = cosf(dyaw);
	Rn2c_kT[0][1] = sinf(dyaw);
	Rn2c_kT[1][0] = -Rn2c_kT[0][1];
	Rn2c_kT[1][1] = Rn2c_kT[0][0];
	
	LOG(INFO)<<"dyaw: "<<dyaw; 
	LOG(INFO)<<"vehicle_pos: "<<vehicle_pos[0]<<vehicle_pos[1];
	LOG(INFO)<<"vehicle_pos_pre: "<<vehicle_pos_pre[0]<<vehicle_pos_pre[1]; 
	LOG(INFO)<<"d_pos_new_c: "<<d_pos_new_c[0]<<d_pos_new_c[1]; 
	LOG(INFO)<<"vehicle_fai_pre: "<<vehicle_fai_pre; 	
	
// �ӳ����߲����л�ȡ������,��Ԥ��������

	cv::Mat xy_feature_pre = cv::Mat::zeros(2, lane_points_nums, CV_32FC1);; // ��һ֡���г����ߵ�������
	cv::Mat xy_feature_predict = cv::Mat::zeros(2, lane_points_nums, CV_32FC1); //  Ԥ��ĵ�ǰ֡���г����ߵ�������
	for(int lane_index=0; lane_index<lane_num; lane_index++)
	{		
		for(int points_index = 0; points_index<lane_points_nums; points_index++)
		{
			// Y = aX + b, X = {5, 10, 20, 30, 50}		
			xy_feature_pre.at<float>(0, points_index) = X[points_index];
			xy_feature_pre.at<float>(1, points_index) = lane_coeffs_pre.at<float>(0, lane_index) + lane_coeffs_pre.at<float>(1, lane_index)*X[points_index];

			// NED����ϵ�µ�
			double dx = xy_feature_pre.at<float>(1, points_index) - d_pos_new_c[0];
			double dy = xy_feature_pre.at<float>(0, points_index) - d_pos_new_c[1];
			xy_feature_predict.at<float>(1, points_index) = Rn2c_kT[0][0]*dx + Rn2c_kT[0][1]*dy;
			xy_feature_predict.at<float>(0, points_index) = Rn2c_kT[1][0]*dx + Rn2c_kT[1][1]*dy;
		}
		
		// ���������	Y = AX(X������)
		std::vector<float> lane_coeffs_t;
		polyfit(&lane_coeffs_t, xy_feature_predict, m_order );
		
		lane_coeffs_predict.at<float>(0, lane_index) = lane_coeffs_t[0];
		lane_coeffs_predict.at<float>(1, lane_index) = lane_coeffs_t[1];

		LOG(INFO)<<"predict_lane_coeffs: "<<lane_coeffs_t[0]<<" "<<lane_coeffs_t[1]; 	
		
	}

	// ����pre��ֵ
	att_pre[0] = att_cur[0];
	att_pre[1] = att_cur[1];
	att_pre[2] = att_cur[2];
	vehicle_pos_pre[0] = vehicle_pos[0];
	vehicle_pos_pre[1] = vehicle_pos[1];
	vehicle_fai_pre = vehicle_fai; //struct_vehicle_state.fai;

	return 1;
}
