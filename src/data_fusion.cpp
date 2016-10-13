#include "data_fusion.h"
#include <unistd.h>


# define DATA_FROM_LOG 1  // ��log�ж�ȡ����
# define DATA_FROM_ONLINE 0 //���߶�ȡ����



using namespace common;

DataFusion::DataFusion()
{

}


void DataFusion::Initialize( )
{
    infile_log.open("data/doing/log.txt");       // ofstream
    
    /// CAN
    is_steer_angle_OK = 0; // ��ǰ�Ƿ�steer�����Ѿ�����
    pre_can_timestamp = 0.0f;
    isFirstTime_can = 1;

    // IMU
    acc_filt_hz = 5.0f; // ���ٶȼƵĵ�ͨ��ֹƵ��
    isFirstTime_att = 1; // �Ƿ��ǵ�һ�ν���
    imu_timestamp = 0.0f;
    pre_imu_timestamp = 0.0f; // IMU�����ϴεõ���ʱ�� 

    // �ⲿlane
    att_cur[0] = 0.0;
    att_cur[1] = 0.0;
    att_cur[2] = 0.0;
    vehicle_vel[0] = 0.0;
    vehicle_vel[1] = 0.0;
    vehicle_pos[0] = 0.0;
    vehicle_pos[1] = 0.0;
    vehicle_fai = 0.0;
    vehicle_yaw = 0.0;
    
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

    //
}

// ��ȡ����,���ַ�ʽ:
//      1.���ߴ�log 
//      2.����
int DataFusion::red_data()
{    
    double log_data[2], timestamp_raw[2];
    string data_flag;
    while(1)
    {  
        update_read_data_state();
        
        // is_continue_read_data �� run�����ⲿ���õ�timestamp�͵�ǰ���ݵ�ʱ�������
        if(DATA_FROM_LOG && is_continue_read_data)
        {
//            printf("read data \n");
            if(is_first_read_data) // ��һ�ν����ȡ����
            {
                getline(infile_log, buffer_log);
                ss_tmp.clear();
                ss_tmp.str(buffer_log);
                ss_tmp>>timestamp_raw[0]>>timestamp_raw[1]>>data_flag;
                // ��ʼ��ʱ���
                cur_read_data_timestamp = timestamp_raw[0] + timestamp_raw[1]*1e-6;

                is_first_read_data = 0;              
            }else
            {
                getline(infile_log, buffer_log);
                ss_tmp.clear();
                ss_tmp.str(buffer_log);
                ss_tmp>>log_data[0]>>log_data[1]>>data_flag;
                ss_log.clear();
                ss_log.str(buffer_log);

                if(data_flag == "cam_frame")
                {
                    string camera_flag, camera_add, image_index_str;
                    string image_name;
                    int log_image_index;
                    ss_log>>timestamp_raw[0]>>timestamp_raw[1]>>camera_flag>>camera_add>>log_image_index;
                    
                    image_frame_info.timestamp = timestamp_raw[0] + timestamp_raw[1]*1e-6;
                    image_frame_info.index = log_image_index;
                    data_image_update = 1;
                    
                }else if(data_flag == "Gsensor")
                {            
                    double AccData_raw[3]; // accԭʼ����ϵ�µ�
                    double AccData_NED[3]; // �������ϵ
                    double AccDataFilter[3]; // һ�׵�֮ͨ�������
                    double GyroData_raw[3];
                    double GyroData_NED[3];            
                    double imu_temperature;    
                    string imu_flag;
                    double imu_timestamp;

                    ss_log>>timestamp_raw[0]>>timestamp_raw[1]>>imu_flag>>AccData_raw[0]>>AccData_raw[1]>>AccData_raw[2]
                            >>GyroData_raw[0]>>GyroData_raw[1]>>GyroData_raw[2]>>imu_temperature;
                    imu_timestamp = timestamp_raw[0] + timestamp_raw[1]*1e-6;                 
                    imu_attitude_estimate.AccDataCalibation(AccData_NED, AccData_raw);// ԭʼ����У��
                    imu_attitude_estimate.GyrocDataCalibation(GyroData_NED, GyroData_raw);

                    if(is_first_read_gsensor)
                    {
                        is_first_read_gsensor = 0;
                        pre_imu_timestamp = imu_timestamp;
                        AccDataFilter[0] = AccData_NED[0];
                        AccDataFilter[1] = AccData_NED[1];
                        AccDataFilter[2] = AccData_NED[2];                    
                    }else{
                        double dt_imu = imu_timestamp - pre_imu_timestamp;                    
                        imu_attitude_estimate.LowpassFilter3f(AccDataFilter, AccDataFilter, AccData_NED, dt_imu, acc_filt_hz);                        
                        pre_imu_timestamp = imu_timestamp;    
                    }                    
                    imu_data.timestamp = imu_timestamp;
                    update_current_data_timestamp(imu_timestamp);

                    imu_data.acc[0] = AccData_NED[0];
                    imu_data.acc[1] = AccData_NED[1];
                    imu_data.acc[2] = AccData_NED[2];
                    imu_data.gyro[0] = GyroData_NED[0];
                    imu_data.gyro[1] = GyroData_NED[1];
                    imu_data.gyro[2] = GyroData_NED[2];
                    data_gsensor_update = 1;
              
                }else if(data_flag == "speed")
                {    
                    double speed_raw_timestamp[2];    
                    string speed_str;
                    ss_log>>speed_raw_timestamp[0]>>speed_raw_timestamp[1]>>speed_str>>speed_can;  
                    speed_timestamp = speed_raw_timestamp[0] + speed_raw_timestamp[1]*1e-6;
                    speed_can = speed_can/3.6;// km/h-->m/s       

                    can_speed_data.timestamp = speed_raw_timestamp[0] + speed_raw_timestamp[1]*1e-6;
                    update_current_data_timestamp(can_speed_data.timestamp);
                    can_speed_data.speed= speed_can/3.6;// km/h-->m/s   
                    data_speed_update = 1;
                }
            } 
        }
        usleep(10);
    }
}


int DataFusion::data_fusion_main()
{
   
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
            // ��ȡlog���ȹ�����camera����
            if(data_flag == "cam_frame")
            {
                camera_timestamp = camera_timestamp_raw[0] + camera_timestamp_raw[1]*1e-6;

                if( fabs(camera_timestamp - m_call_predict_timestamp) < 0.001)
                {
                    m_camera_match_state = 1; // ʱ����Ѿ�ƥ��
                    m_new_lane_parameter_get = 0;
                    // ���µ�ǰ��״̬����
                    imu_attitude_estimate.GetAttitude(att_cur);
                  //can_vehicle_estimate.GetVelPosXY(vehicle_vel, vehicle_pos);                        
                    
                }else if( camera_timestamp - m_call_predict_timestamp > 0.1) // ʱ���ƥ����� ��ֹ��ѭ��
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
    return 1;
}


// ��read_data�ж�ȡ����
int DataFusion::data_fusion_main_new()
{
//    if(data_image_update)
//    {
//        camera_timestamp = camera_timestamp_raw[0] + camera_timestamp_raw[1]*1e-6;
//        if( fabs(camera_timestamp - m_extern_call_predict_timestamp) < 0.001)
//        {
//            m_camera_match_state = 1; // ʱ����Ѿ�ƥ��
//            m_new_lane_parameter_get = 0;
//            // ���µ�ǰ��״̬����
//            imu_attitude_estimate.GetAttitude(att_cur);
//            can_vehicle_estimate.GetVelPosXY(vehicle_vel, vehicle_pos);                        
//            
//        }else if( camera_timestamp - m_extern_call_predict_timestamp > 0.1) // ʱ���ƥ����� ��ֹ��ѭ��
//        {
//            m_camera_match_state = -1; // ����ʱ����Ѿ���ʱ
//        }
//    }else
//    {
//        // IMU CAN��Ϣ�ں�
//        run_fusion( buffer_log, data_flag);
//    }

    return 1;
}


int DataFusion::run_fusion( string buffer_log, string data_flag)
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
//        if( is_steer_angle_OK == 1)
//        {
//            is_steer_angle_OK = 0;
//            double speed_raw_timestamp[2];    
//            string speed_str;
//            ss_log>>speed_raw_timestamp[0]>>speed_raw_timestamp[1]>>speed_str>>speed_can;    
//            speed_can = speed_can/3.6;// km/h-->m/s
//            speed_timestamp = speed_raw_timestamp[0] + speed_raw_timestamp[1]*1e-6;
//            can_timestamp = speed_timestamp;

//            if(isFirstTime_can)
//            {
//                pre_can_timestamp = speed_timestamp;                    
//                isFirstTime_can = 0;
//            }else
//            {
//                double dt_can = can_timestamp - pre_can_timestamp;
//                can_vehicle_estimate.UpdateVehicleState(steer_angle_deg*D2R, speed_can, dt_can );
//                pre_can_timestamp = can_timestamp;
//                
//                // save queue
//                struct_vehicle_state.timestamp = can_timestamp;
//                can_vehicle_estimate.GetVelPosFai(struct_vehicle_state.vel, struct_vehicle_state.pos, struct_vehicle_state.fai);

//                //printf("steer: %f speed: %f  dt: %f\n", steer_angle_deg, speed_can, dt_can);
//                //printf("vel: %f, %f pos: %f, %f\n", struct_vehicle_state.vel[0], struct_vehicle_state.vel[1], struct_vehicle_state.pos[0], struct_vehicle_state.pos[1]);
//            }
//        }

        // ����imu+speed���������˶�
        double speed_raw_timestamp[2];    
        string speed_str;
        ss_log>>speed_raw_timestamp[0]>>speed_raw_timestamp[1]>>speed_str>>speed_can;    
        speed_can = speed_can/3.6;// km/h-->m/s
        speed_timestamp = speed_raw_timestamp[0] + speed_raw_timestamp[1]*1e-6;
        can_timestamp = speed_timestamp;
        
        if(is_first_speed_data)
        {
            is_first_speed_data = 0;
            imu_attitude_estimate.GetAttitude(att_xy_pre);
            pre_can_timestamp = speed_timestamp;  
            
        }else
        {
            double dt_can = can_timestamp - pre_can_timestamp;
            imu_attitude_estimate.GetAttitude(att_xy_cur);
            can_vehicle_estimate.UpdateVehicleState_imu(att_xy_cur[2], speed_can, dt_can );
            pre_can_timestamp = can_timestamp;
            
        }
        
    }
    return 1;
}

// ���µ�ǰfusion��ʱ��������ڿ��ƶ�ȡ���ݵĳ���
void DataFusion::update_current_fusion_timestamp( double data_timestample)
{
    if(is_first_fusion_timestamp)
    {
        is_first_fusion_timestamp = 0;
        cur_fusion_timestamp = data_timestample;
    }else
    {
        if(cur_fusion_timestamp < data_timestample)
        {
            cur_fusion_timestamp = data_timestample;
        }
    }
}

// ���µ�ǰfusion��ʱ��������ڿ��ƶ�ȡ���ݵĳ���
void DataFusion::update_current_data_timestamp( double data_timestample)
{
    if(is_first_data_timestamp)
    {
        cur_data_timestamp = data_timestample;
        is_first_data_timestamp = 0;
    }else
    {
        if(cur_data_timestamp < data_timestample)
        {
            cur_data_timestamp = data_timestample;
        }
    }
}


// �ж��Ƿ�Ҫ������ȡ����
bool  DataFusion::update_read_data_state( )
{
    double dt  = cur_data_timestamp - m_call_predict_timestamp;
//    printf("update_read_data_state--dt=%f\n", dt);
    // ��ǰ��ȡdata_save_length���ȵ�����
    if(dt > 0 && dt >= data_save_length)  // ʱ�䳬����
    {
        is_continue_read_data = 0; //  ��ͣ��ȡ����
    }else
    {
        is_continue_read_data = 1;
    }

    return is_continue_read_data;
}

// ɾ���趨�����ʱ�����ʷ����
void DataFusion::delete_history_save_data( )
{
    double dt;
    int att_data_length = vector_att.size();    
    int delete_conter = 0;
    for(int i=0; i<att_data_length; i++)
    {
        dt = (vector_att.begin()+i)->timestamp - m_call_predict_timestamp;
        if(dt < -data_save_length)
        {
            delete_conter++;
        }else
        {            
            break;
        }
    }
    if(delete_conter > 0)
    {
        // �����ȵ�ǰm_call_predict_timestamp��data_save_length��ǰ�����ݣ���ɾ��
        vector_att.erase(vector_att.begin(), vector_att.begin()+delete_conter);  
    }

    // �����˶�����
    int vehicle_data_length = vector_vehicle_state.size(); 
    delete_conter = 0;
    for(int i=0; i<vehicle_data_length; i++)
    {
        dt = (vector_vehicle_state.begin()+i)->timestamp - m_call_predict_timestamp;
        if(dt < -data_save_length)
        {
            delete_conter++;
        }else
        {            
            break;
        }
    }
    if(delete_conter > 0)
    {
        // �����ȵ�ǰm_call_predict_timestamp��data_save_length��ǰ�����ݣ���ɾ��
        vector_vehicle_state.erase(vector_vehicle_state.begin(), vector_vehicle_state.begin()+delete_conter);  

    }
    
}



int DataFusion::run_fusion_new( )
{
    while(1)
    {
        // is_continue_read_data
        if(data_gsensor_update)
        {
            double cur_att_timestamp = imu_data.timestamp;
            if(isFirstTime_att)
            {
                isFirstTime_att = 0;
                pre_att_timestamp = cur_att_timestamp; 
            }else
            {            
                double dt_att = cur_att_timestamp - pre_att_timestamp;
                
                imu_attitude_estimate.UpdataAttitude(imu_data.acc, imu_data.gyro, dt_att);
                pre_att_timestamp = cur_att_timestamp;

                // save att
                imu_attitude_estimate.GetAttitude(struct_att.att);
                struct_att.timestamp = cur_att_timestamp;
                vector_att.push_back(struct_att);
            } 
            update_current_fusion_timestamp( cur_att_timestamp );
            data_gsensor_update = 0;          
        }

        if(data_speed_update)
        {    
            // ����imu+speed���������˶�
            double cur_can_timestamp = can_speed_data.timestamp;        
            if(is_first_speed_data)
            {
                is_first_speed_data = 0;
                imu_attitude_estimate.GetAttitude(att_xy_pre);
                pre_can_timestamp = cur_can_timestamp;  
                
            }else
            {
                double dt_can = cur_can_timestamp - pre_can_timestamp;                
                imu_attitude_estimate.GetAttitude(att_xy_cur);
                can_vehicle_estimate.UpdateVehicleState_imu(att_xy_cur[2], speed_can, dt_can );
                pre_can_timestamp = cur_can_timestamp;

                // save vehicle state            
                can_vehicle_estimate.GetVehicleState(struct_vehicle_state.vel, struct_vehicle_state.pos, struct_vehicle_state.yaw);
                struct_vehicle_state.timestamp = cur_can_timestamp;
                //queue_vehicle_state.push(struct_vehicle_state); 
                vector_vehicle_state.push_back(struct_vehicle_state);
            }
            update_current_fusion_timestamp( cur_can_timestamp );            
            data_speed_update = 0;        
        }        
        delete_history_save_data();
//        usleep(10); // 1000us
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
    m_call_predict_timestamp = image_timestamp;    
    m_new_lane_parameter_get = 1;
    m_camera_match_state = 0; // ����Ϊ��ʼ״̬    

    // ����ʱ��������в��Һͼ��㣬ֱ������cameraʱ�����get��ʱ���ƥ��
//    data_fusion_main();
    
    if(isFirsttimeGetParameter)
    {
        std::cout<<"isFirsttimeGetParameter=1"<<endl;
        isFirsttimeGetParameter = 0;
        m_call_pre_predict_timestamp = image_timestamp;

        att_pre[0] = att_cur[0];
        att_pre[1] = att_cur[1];
        att_pre[2] = att_cur[2];
        vehicle_pos_pre[0] = vehicle_pos[0];
        vehicle_pos_pre[1] = vehicle_pos[1];
        
        lane_coeffs_pre.copyTo(lane_coeffs_predict);
    }else
    {
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

// ����ʱ������Ҷ�Ӧ������
// TODO:
int DataFusion::get_timestamp_data(double (&vehicle_pos)[2], double (&att)[3], double timestamp_search)
{
    // vector_att
    bool att_data_search_ok = 0;
    bool vehicle_data_search_ok = 0;
    int att_data_length = vector_att.size();
    double timestamp_t, timestamp_t_1, dt_t, dt_t_pre;
    for(int i = 1; i<att_data_length; i++)
    {
        timestamp_t = (vector_att.end()-i)->timestamp;
        timestamp_t_1 = (vector_att.end()-i-1)->timestamp;
        
        dt_t = timestamp_t - timestamp_search;
        dt_t_pre = timestamp_t_1 - timestamp_search;
        // ѡȡ��timestamp_search�����ʱ�̣�2����������һ�ּ���
        // 1. dt<0.01
        // 2. �ҵ���ֵ 
        if(dt_t_pre<0 && dt_t>0)
        {
            att[0] = (vector_att.end()-i)->att[0];
            att[1] = (vector_att.end()-i)->att[1];
            att[2] = (vector_att.end()-i)->att[2];
            att_data_search_ok = 1;
            break;
        }
    }

    // vector_vehicle_state
    int vehicle_data_length = vector_vehicle_state.size();
    for(int i = 1; i<vehicle_data_length; i++)
    {
        timestamp_t = (vector_vehicle_state.end()-i)->timestamp;
        timestamp_t_1 = (vector_vehicle_state.end()-i-1)->timestamp;
        
        dt_t = timestamp_t - timestamp_search;
        dt_t_pre = timestamp_t_1 - timestamp_search;
        // ѡȡ��timestamp_search�����ʱ�̣�2����������һ�ּ���
        // 1. dt<0.01
        // 2. �ҵ���ֵ 
        if(dt_t_pre<0 && dt_t>0)
        {
            vehicle_pos[0] = (vector_vehicle_state.end()-i)->pos[0];
            vehicle_pos[1] = (vector_vehicle_state.end()-i)->pos[1];
            vehicle_data_search_ok = 1;
            break;
        }
    }

    if(att_data_search_ok && vehicle_data_search_ok)
    {
        return 1;
    }else
    {
        return 0;
    }
    
}
int DataFusion::GetLanePredictParameter_new(cv::Mat& lane_coeffs_predict, double image_timestamp_cur, double image_timestamp_pre, 
                                                             cv::Mat lane_coeffs_pre, double lane_num, double m_order )
{
    bool data_search_cur = 0; // ����ָ��ʱ���������
    bool data_search_pre = 0;
    // ����ʱ���
    m_call_predict_timestamp = image_timestamp_cur; 
    m_camera_match_state = 0; // ����Ϊ��ʼ״̬  
    
    // Ѱ�Ҹ������ timestamp��Ӧ��att,vehicle����
    data_search_cur = get_timestamp_data(vehicle_pos, att_cur, image_timestamp_cur);
    data_search_pre = get_timestamp_data(vehicle_pos_pre, att_pre, image_timestamp_pre);

    if(data_search_cur && data_search_pre)
    {
        LanePredict(lane_coeffs_predict, lane_coeffs_pre, lane_num, m_order);
    }else if( !data_search_cur )
    {
        std::cout<<"!!!error cur--camera timestamp dismatch"<<endl;
    }else if(!data_search_pre)
    {
        std::cout<<"!!!error pre--camera timestamp dismatch"<<endl;
    }
    extern_call_timestamp_update = 1;
    return 1;    
    
}



// lane_coeffs_pre: ÿһ�д���һ������
int DataFusion::LanePredict(cv::Mat& lane_coeffs_predict, cv::Mat lane_coeffs_pre, double lane_num, double m_order)
{
/// init:(����)
    int lane_points_nums = 5; // ÿһ��������ȡ������������
    double X[5] = {2.0, 5.0, 10.0, 20.0, 35.0};
    LOG(INFO)<<"lane_coeffs_pre: "<<lane_coeffs_pre<<endl<<endl;

// ������������֮֡���״̬�仯    
    // ��pos��������ϵת����ת����preʱ��Ϊ��ʼ����
    double d_pos_tmp[2]; // ǰ����֡�ڳ�ʼ����ϵ�µ������˶�
    double d_pos_new_c[2]; // ����preΪ�����µ������˶�
    d_pos_tmp[0] = vehicle_pos[0] - vehicle_pos_pre[0];
    d_pos_tmp[1] = vehicle_pos[1] - vehicle_pos_pre[1];         
    d_pos_new_c[0] = cosf(att_pre[2])*d_pos_tmp[0] + sinf(att_pre[2])*d_pos_tmp[1];
    d_pos_new_c[1] = -sinf(att_pre[2])*d_pos_tmp[0] + cos(att_pre[2])*d_pos_tmp[1];
    
    double dyaw = att_cur[2] - att_pre[2]; 
    double Rn2c_kT[2][2];
    Rn2c_kT[0][0] = cosf(dyaw);
    Rn2c_kT[0][1] = sinf(dyaw);
    Rn2c_kT[1][0] = -Rn2c_kT[0][1];
    Rn2c_kT[1][1] = Rn2c_kT[0][0];
    
    LOG(INFO)<<"dyaw: "<<dyaw<<endl; 
    LOG(INFO)<<"vehicle_pos: "<<vehicle_pos[0]<<" "<<vehicle_pos[1]<<endl;
    LOG(INFO)<<"vehicle_pos_pre: "<<vehicle_pos_pre[0]<<" "<<vehicle_pos_pre[1]<<endl; 
    LOG(INFO)<<"d_pos_new_c: "<<d_pos_new_c[0]<<" "<<d_pos_new_c[1]<<endl; 
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
            xy_feature_pre.at<float>(1, points_index) = lane_coeffs_pre.at<float>(0, lane_index) + lane_coeffs_pre.at<float>(1, lane_index)*X[points_index] + lane_coeffs_pre.at<float>(2, lane_index)*X[points_index]*X[points_index];

            // NED����ϵ�µ�
            double dx = xy_feature_pre.at<float>(0, points_index) - d_pos_new_c[0];
            double dy = xy_feature_pre.at<float>(1, points_index) - d_pos_new_c[1];
            xy_feature_predict.at<float>(1, points_index) = Rn2c_kT[0][0]*dx + Rn2c_kT[0][1]*dy;
            xy_feature_predict.at<float>(0, points_index) = Rn2c_kT[1][0]*dx + Rn2c_kT[1][1]*dy;
        }

        LOG(INFO)<<"xy_feature_predict: "<<xy_feature_predict<< endl << endl; 
        
        // ���������    Y = AX(X������)
        std::vector<float> lane_coeffs_t;
        polyfit(&lane_coeffs_t, xy_feature_predict, m_order );

        for(int i = 0; i<m_order+1; i++)
        {
            lane_coeffs_predict.at<float>(i, lane_index) = lane_coeffs_t[i];
        }

        LOG(INFO)<<"predict_lane_coeffs: "<<lane_coeffs_t[0]<<" "<<lane_coeffs_t[1]<<endl;     
        
    }

    return 1;
}


// lane_coeffs_pre: ÿһ�д���һ������
int DataFusion::LanePredict_new(cv::Mat& lane_coeffs_predict, cv::Mat lane_coeffs_pre, double lane_num, double m_order, 
                                          StructVehicleState pre_vehicle_state, StructVehicleState cur_vehicle_state, 
                                          StructAtt pre_att_xy, StructAtt cur_att_xy)
{
    /// init:(����)
    int lane_points_nums = 5; // ÿһ��������ȡ������������
    double X[5] = {2.0, 5.0, 10.0, 20.0, 35.0};
    LOG(INFO)<<"lane_coeffs_pre: "<<lane_coeffs_pre<<endl<<endl;

    // ��������
    double vehicle_pos_cur[2], vehicle_pos_pre[2], att_pre[2], att_cur[3];
    memcpy(vehicle_pos_pre, pre_vehicle_state.pos, sizeof(pre_vehicle_state.pos));
    memcpy(vehicle_pos_cur, cur_vehicle_state.pos, sizeof(cur_vehicle_state.pos));
    memcpy(att_pre, pre_att_xy.att, sizeof(pre_att_xy.att));
    memcpy(att_cur, cur_att_xy.att, sizeof(cur_att_xy.att));    


    // ������������֮֡���״̬�仯    
    // ��pos��������ϵת����ת����preʱ��Ϊ��ʼ����
    double d_pos_tmp[2]; // ǰ����֡�ڳ�ʼ����ϵ�µ������˶�
    double d_pos_new_c[2]; // ����preΪ�����µ������˶�
    d_pos_tmp[0] = vehicle_pos_cur[0] - vehicle_pos_pre[0];
    d_pos_tmp[1] = vehicle_pos_cur[1] - vehicle_pos_pre[1];         
    d_pos_new_c[0] = cosf(att_pre[2])*d_pos_tmp[0] + sinf(att_pre[2])*d_pos_tmp[1];
    d_pos_new_c[1] = -sinf(att_pre[2])*d_pos_tmp[0] + cos(att_pre[2])*d_pos_tmp[1];
    
    double dyaw = att_cur[2] - att_pre[2]; 
    double Rn2c_kT[2][2];
    Rn2c_kT[0][0] = cosf(dyaw);
    Rn2c_kT[0][1] = sinf(dyaw);
    Rn2c_kT[1][0] = -Rn2c_kT[0][1];
    Rn2c_kT[1][1] = Rn2c_kT[0][0];
    
    LOG(INFO)<<"dyaw: "<<dyaw<<endl; 
    LOG(INFO)<<"vehicle_pos: "<<vehicle_pos_cur[0]<<" "<<vehicle_pos_cur[1]<<endl;
    LOG(INFO)<<"vehicle_pos_pre: "<<vehicle_pos_pre[0]<<" "<<vehicle_pos_pre[1]<<endl; 
    LOG(INFO)<<"d_pos_new_c: "<<d_pos_new_c[0]<<" "<<d_pos_new_c[1]<<endl; 
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
            xy_feature_pre.at<float>(1, points_index) = lane_coeffs_pre.at<float>(0, lane_index) + lane_coeffs_pre.at<float>(1, lane_index)*X[points_index] + lane_coeffs_pre.at<float>(2, lane_index)*X[points_index]*X[points_index];

            // NED����ϵ�µ�
            double dx = xy_feature_pre.at<float>(0, points_index) - d_pos_new_c[0];
            double dy = xy_feature_pre.at<float>(1, points_index) - d_pos_new_c[1];
            xy_feature_predict.at<float>(1, points_index) = Rn2c_kT[0][0]*dx + Rn2c_kT[0][1]*dy;
            xy_feature_predict.at<float>(0, points_index) = Rn2c_kT[1][0]*dx + Rn2c_kT[1][1]*dy;
        }

        LOG(INFO)<<"xy_feature_predict: "<<xy_feature_predict<< endl << endl; 
        
        // ���������    Y = AX(X������)
        std::vector<float> lane_coeffs_t;
        polyfit(&lane_coeffs_t, xy_feature_predict, m_order );

        for(int i = 0; i<m_order+1; i++)
        {
            lane_coeffs_predict.at<float>(i, lane_index) = lane_coeffs_t[i];
        }

        LOG(INFO)<<"predict_lane_coeffs: "<<lane_coeffs_t[0]<<" "<<lane_coeffs_t[1]<<endl;     
        
    }

    return 1;
}

