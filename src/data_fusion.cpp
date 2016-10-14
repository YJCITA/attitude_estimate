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
    m_pre_can_timestamp = 0.0f; /// CAN;

    // IMU
    m_acc_filt_hz = 5.0f; // ���ٶȼƵĵ�ͨ��ֹƵ��
    m_gyro_filt_hz = 20.0;
    m_isFirstTime_att = 1; // �Ƿ��ǵ�һ�ν���
    m_pre_imu_timestamp = 0.0f; // IMU�����ϴεõ���ʱ�� 
    
    m_is_first_speed_data = 1; //  1: ��һ�λ�ȡ��speed���� 0:���ǵ�һ��
    
    // read data
    m_is_first_read_data = 1; // �Ƿ��ǵ�һ�ν����ȡ��������ʼ��cur_timestamp
    m_is_first_read_gsensor = 1; 
    m_data_gsensor_update = 0;
    m_data_speed_update = 0;
    m_data_image_update = 0;
    
    // ��ȡ���ݿ���
    m_is_first_fusion_timestamp = 1; // ��һ�θ���
    m_is_first_data_timestamp = 1; // ��һ�θ���
    m_data_save_length = 2; // ������ʷ���ݵĳ���(ʱ��Ϊ��λ: s)
    m_is_continue_read_data = 1; // 1:������ȡ����  2:��ͣ��ȡ���� ��fusion����   

}

// ��ȡ����,���ַ�ʽ:
//      1.���ߴ�log 
//      2.����
int DataFusion::read_data( )
{    
    double log_data[2], timestamp_raw[2];
    string data_flag;
    while(1)
    {  
        //����is_continue_read_data
        update_read_data_state();
        if(DATA_FROM_LOG && m_is_continue_read_data)
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
                
                m_image_frame_info.timestamp = timestamp_raw[0] + timestamp_raw[1]*1e-6;
                m_image_frame_info.index = log_image_index;
                m_data_image_update = 1;
                
            }else if(data_flag == "Gsensor")
            {            
                double AccData_raw[3]; // accԭʼ����ϵ�µ�
                double AccData_NED[3]; // �������ϵ
                static double AccDataFilter[3]; // һ�׵�֮ͨ�������
                double GyroData_raw[3];
                double GyroData_NED[3];  
                static double GyroDataFilter[3]; // һ�׵�֮ͨ�������
                double imu_temperature, imu_timestamp;    
                string imu_flag;

                ss_log>>timestamp_raw[0]>>timestamp_raw[1]>>imu_flag>>AccData_raw[0]>>AccData_raw[1]>>AccData_raw[2]
                        >>GyroData_raw[0]>>GyroData_raw[1]>>GyroData_raw[2]>>imu_temperature;
                imu_timestamp = timestamp_raw[0] + timestamp_raw[1]*1e-6;                 
                imu_attitude_estimate.AccDataCalibation(AccData_NED, AccData_raw);// ԭʼ����У��
                imu_attitude_estimate.GyrocDataCalibation(GyroData_NED, GyroData_raw);

                if(m_is_first_read_gsensor)
                {
                    m_is_first_read_gsensor = 0;
                    m_pre_imu_timestamp = imu_timestamp;
                    AccDataFilter[0] = AccData_NED[0];
                    AccDataFilter[1] = AccData_NED[1];
                    AccDataFilter[2] = AccData_NED[2];
                    GyroDataFilter[0] = GyroData_NED[0];
                    GyroDataFilter[1] = GyroData_NED[1];
                    GyroDataFilter[2] = GyroData_NED[2]; 
                }else
                {
                    double dt_imu = imu_timestamp - m_pre_imu_timestamp;                    
                    imu_attitude_estimate.LowpassFilter3f(AccDataFilter, AccDataFilter, AccData_NED, dt_imu, m_acc_filt_hz);    
                    imu_attitude_estimate.LowpassFilter3f(GyroDataFilter, GyroDataFilter, GyroData_NED, dt_imu, m_gyro_filt_hz);       
                    m_pre_imu_timestamp = imu_timestamp;    
                }                    
                m_imu_data.timestamp = imu_timestamp;
                update_current_data_timestamp(imu_timestamp);
                m_imu_data.acc[0] = AccDataFilter[0];
                m_imu_data.acc[1] = AccDataFilter[1];
                m_imu_data.acc[2] = AccDataFilter[2];
                m_imu_data.gyro[0] = GyroDataFilter[0];
                m_imu_data.gyro[1] = GyroDataFilter[1];
                m_imu_data.gyro[2] = GyroDataFilter[2];
                m_data_gsensor_update = 1; 
            }else if(data_flag == "brake_signal")
            {
                string str_t[10],str_speed;
                int data_t[10];
                double raw_timestamp[2];  
                double speed_can;
                ss_log>>raw_timestamp[0]>>raw_timestamp[1]
                    >>str_t[0]>>data_t[0]>>str_t[1]>>data_t[1]>>str_t[2]>>data_t[2]>>str_t[3]>>data_t[3]>>str_t[4]>>data_t[4]
                    >>str_t[5]>>data_t[5]>>str_t[6]>>data_t[6]>>str_t[7]>>data_t[7]>>str_t[8]>>data_t[8]>>str_t[9]>>data_t[9]
                    >>str_speed>>speed_can;  
                m_can_speed_data.timestamp = raw_timestamp[0] + raw_timestamp[1]*1e-6;;
                update_current_data_timestamp(m_can_speed_data.timestamp);
                m_can_speed_data.speed= speed_can/3.6;// km/h-->m/s   
                m_data_speed_update = 1;  
            }             
        }
        usleep(1);
    }
}


// ���µ�ǰfusion��ʱ��������ڿ��ƶ�ȡ���ݵĳ���
void DataFusion::update_current_fusion_timestamp( double data_timestample)
{
    if(m_is_first_fusion_timestamp)
    {
        m_is_first_fusion_timestamp = 0;
        m_cur_fusion_timestamp = data_timestample;
    }else
    {
        if(m_cur_fusion_timestamp < data_timestample)
        {
            m_cur_fusion_timestamp = data_timestample;
        }
    }
}

// ���µ�ǰfusion��ʱ��������ڿ��ƶ�ȡ���ݵĳ���
void DataFusion::update_current_data_timestamp( double data_timestample)
{
    if(m_is_first_data_timestamp)
    {
        m_cur_data_timestamp = data_timestample;
        m_is_first_data_timestamp = 0;
    }else
    {
        if(m_cur_data_timestamp < data_timestample)
        {
            m_cur_data_timestamp = data_timestample;
        }
    }
}


// �ж��Ƿ�Ҫ������ȡ����
bool  DataFusion::update_read_data_state( )
{
    double dt  = m_cur_data_timestamp - m_call_predict_timestamp;
//    printf("update_read_data_state--dt=%f\n", dt);
    // ��ǰ��ȡdata_save_length���ȵ�����
    if(dt > 0 && dt >= m_data_save_length)  // ʱ�䳬����
    {
        m_is_continue_read_data = 0; //  ��ͣ��ȡ����
    }else
    {
        m_is_continue_read_data = 1;
    }

    return m_is_continue_read_data;
}

// ɾ���趨�����ʱ�����ʷ����
void DataFusion::delete_history_save_data( )
{
    double dt;
    int att_data_length = m_vector_att.size();    
    int delete_conter = 0;
    for(int i=0; i<att_data_length; i++)
    {
        dt = (m_vector_att.begin()+i)->timestamp - m_call_predict_timestamp;
        if(dt < -m_data_save_length)
            delete_conter++;
        else
            break;
    }
    
    if(delete_conter > 0)
    {
        // �����ȵ�ǰm_call_predict_timestamp��data_save_length��ǰ�����ݣ���ɾ��
        m_vector_att.erase(m_vector_att.begin(), m_vector_att.begin()+delete_conter);
    }    

    // �����˶�����
    int vehicle_data_length = m_vector_vehicle_state.size(); 
    delete_conter = 0;
    for(int i=0; i<vehicle_data_length; i++)
    {
        dt = (m_vector_vehicle_state.begin()+i)->timestamp - m_call_predict_timestamp;
        if(dt < -m_data_save_length)
            delete_conter++;
        else
            break;
    }
    
    if(delete_conter > 0)
    {
        // �����ȵ�ǰm_call_predict_timestamp��data_save_length��ǰ�����ݣ���ɾ��
        m_vector_vehicle_state.erase(m_vector_vehicle_state.begin(), m_vector_vehicle_state.begin()+delete_conter);  
    }
    
}



int DataFusion::run_fusion( )
{
    double att_xy_cur[3]; // ��ǰstamp�ĽǶ�
    while(1)
    {
        // m_is_continue_read_data
        if(m_data_gsensor_update)
        {
            double cur_att_timestamp = m_imu_data.timestamp;
            if(m_isFirstTime_att)
            {
                m_isFirstTime_att = 0;
                m_pre_att_timestamp = cur_att_timestamp; 
            }else
            {            
                double dt_att = cur_att_timestamp - m_pre_att_timestamp;                
                imu_attitude_estimate.UpdataAttitude(m_imu_data.acc, m_imu_data.gyro, dt_att);
                m_pre_att_timestamp = cur_att_timestamp;

                // save att
                imu_attitude_estimate.GetAttitude(m_struct_att.att);
                m_struct_att.timestamp = cur_att_timestamp;
                m_vector_att.push_back(m_struct_att);
            } 
            update_current_fusion_timestamp( cur_att_timestamp );
            m_data_gsensor_update = 0;          
        }

        if(m_data_speed_update)
        {    
            // ����imu+speed���������˶�
            double cur_can_timestamp = m_can_speed_data.timestamp; 
            if(m_is_first_speed_data)
            {
                m_is_first_speed_data = 0;
                m_pre_can_timestamp = cur_can_timestamp;  
                
            }else
            {
                double dt_can = cur_can_timestamp - m_pre_can_timestamp;                
                imu_attitude_estimate.GetAttitude(att_xy_cur);
                can_vehicle_estimate.UpdateVehicleState_imu(att_xy_cur[2], m_can_speed_data.speed, dt_can );
                m_pre_can_timestamp = cur_can_timestamp;

                // save vehicle state            
                can_vehicle_estimate.GetVehicleState(m_struct_vehicle_state.vel, m_struct_vehicle_state.pos, m_struct_vehicle_state.yaw);
                m_struct_vehicle_state.timestamp = cur_can_timestamp;
                m_vector_vehicle_state.push_back(m_struct_vehicle_state);
            }
            update_current_fusion_timestamp( cur_can_timestamp );            
            m_data_speed_update = 0;        
        }        
        delete_history_save_data();
        usleep(1); // 1000us
    }
        
    return 1;
}

// �������
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


// ����ʱ������Ҷ�Ӧ������
int DataFusion::get_timestamp_data(double (&vehicle_pos)[2], double (&att)[3], double timestamp_search)
{
    // m_vector_att
    bool att_data_search_ok = 0;
    bool vehicle_data_search_ok = 0;
    int att_data_length = m_vector_att.size();
    double timestamp_t, timestamp_t_1, dt_t, dt_t_pre;
    for(int i = 1; i<att_data_length; i++)
    {
        timestamp_t = (m_vector_att.end()-i)->timestamp;
        timestamp_t_1 = (m_vector_att.end()-i-1)->timestamp;
        
        dt_t = timestamp_t - timestamp_search;
        dt_t_pre = timestamp_t_1 - timestamp_search;
        // ѡȡ��timestamp_search�����ʱ�̣�2����������һ�ּ���
        if(dt_t_pre<0 && dt_t>0)
        {
            att[0] = (m_vector_att.end()-i)->att[0];
            att[1] = (m_vector_att.end()-i)->att[1];
            att[2] = (m_vector_att.end()-i)->att[2];
            att_data_search_ok = 1;
            break;
        }
    }

    // m_vector_vehicle_state
    int vehicle_data_length = m_vector_vehicle_state.size();
    for(int i = 1; i<vehicle_data_length; i++)
    {
        timestamp_t = (m_vector_vehicle_state.end()-i)->timestamp;
        timestamp_t_1 = (m_vector_vehicle_state.end()-i-1)->timestamp;
        
        dt_t = timestamp_t - timestamp_search;
        dt_t_pre = timestamp_t_1 - timestamp_search;
        // ѡȡ��timestamp_search�����ʱ�̣�2����������һ�ּ���
        if(dt_t_pre<0 && dt_t>0)
        {
            vehicle_pos[0] = (m_vector_vehicle_state.end()-i)->pos[0];
            vehicle_pos[1] = (m_vector_vehicle_state.end()-i)->pos[1];
            vehicle_data_search_ok = 1;
            break;
        }
    }

    if(att_data_search_ok && vehicle_data_search_ok)
        return 1;
    else
        return 0;    
}


int DataFusion::get_lane_predict_parameter(cv::Mat& lane_coeffs_predict, double image_timestamp_cur, double image_timestamp_pre, 
                                                     cv::Mat lane_coeffs_pre, double lane_num, double m_order )
{
    bool data_search_cur = 0; // ����ָ��ʱ���������
    bool data_search_pre = 0;    
    double att_cur[3] ;        
    double vehicle_pos[2];
    double att_pre[3] ;    
    double vehicle_pos_pre[2];
    // ����ʱ���
    m_call_predict_timestamp = image_timestamp_cur; 
   
    // Ѱ�Ҹ������ timestamp��Ӧ��att,vehicle����
    data_search_cur = get_timestamp_data(vehicle_pos, att_cur, image_timestamp_cur);
    data_search_pre = get_timestamp_data(vehicle_pos_pre, att_pre, image_timestamp_pre);

    if(data_search_cur && data_search_pre)
    {
        lane_predict(lane_coeffs_predict, lane_coeffs_pre, lane_num, m_order, vehicle_pos_pre, att_pre, vehicle_pos, att_cur);
        
        LOG(INFO)<<"LanePredict_new-vehicle_pos_pre "<<vehicle_pos_pre[0]<<" "<<vehicle_pos_pre[1]<<endl;   
        LOG(INFO)<<"LanePredict_new-vehicle_pos_cur "<<vehicle_pos[0]<<" "<<vehicle_pos[1]<<endl; 
        LOG(INFO)<<"LanePredict_new-att_cur "<<att_cur[0]<<" "<<att_cur[1]<<" "<<att_cur[2]<<" "<<endl; 
        LOG(INFO)<<"LanePredict_new-att_pre "<<att_pre[0]<<" "<<att_pre[1]<<" "<<att_pre[2]<<" "<<endl; 
    }
    
    if( !data_search_cur )
    {
        std::cout<<"!!!error cur--camera timestamp dismatch"<<endl;
    }

    if(!data_search_pre)
    {
        std::cout<<"!!!error pre--camera timestamp dismatch"<<endl;
    }
    return 1;    
    
}


// lane_coeffs_pre: ÿһ�д���һ������
int DataFusion::lane_predict(cv::Mat& lane_coeffs_predict, cv::Mat lane_coeffs_pre, double lane_num, double m_order, 
                                    double vehicle_pos_pre[2],  double att_pre[3],
                                    double vehicle_pos_cur[2],  double att_cur[3])
{
    /// init:(����)
    int lane_points_nums = 5; // ÿһ��������ȡ������������
    double X[5] = {2.0, 5.0, 10.0, 20.0, 35.0};
    LOG(INFO)<<"lane_coeffs_pre: "<<lane_coeffs_pre<<endl<<endl;

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
    LOG(INFO)<<"vehicle_yaw_pre: "<<att_pre[2]*180/3.14<<endl;     
    
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

        LOG(INFO)<<"predict_lane_coeffs: "<<lane_coeffs_t[0]<<" "<<lane_coeffs_t[1]<<" "<<lane_coeffs_t[2]<<endl;     
        
    }

    return 1;
}



