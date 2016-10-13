#ifndef DATA_FUSION_H
#define DATA_FUSION_H

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma execution_character_set("utf-8")
#endif

#include "opencv2/opencv.hpp"
#include "gflags/gflags.h"

#include "relative_locate.h"
#include "bird_perspective_mapping.h"
#include "imu_attitude_estimate.h"
#include "can_vehicle_estimate.h"
#include "datafusion_math.h"

#include "common/base/stdint.h"
#include "common/math/polyfit.h"
#include "common/base/log_level.h"

#include <fstream>
#include <string>
#include <sstream>
#include <math.h>
#include <vector>
#include <queue> 

using namespace std;

class DataFusion
{
private:
    struct StructAtt
    {
        double timestamp;
        double att[3];
    };

    struct StructVehicleState
    {
        double timestamp;
        double pos[2];
        double vel[2];
        double yaw;
    };
    
public:

    /// @brief construct  obj
    DataFusion();
    
    ~DataFusion() {}

    void Initialize( );

    // TODO:
    int red_data();
    
    int data_fusion_main();
    
    int data_fusion_main_new();

    void update_current_fusion_timestamp( double data_timestample);

    void update_current_data_timestamp( double data_timestample);

    // �ж��Ƿ�Ҫ������ȡ����
    bool update_read_data_state( );

    // ֻ�����趨ʱ�䳤�ȵ�����
    void  delete_history_save_data( );

    // ����ʱ������Ҷ�Ӧ������
    int get_timestamp_data(double (&vehicle_pos)[2], double (&att)[3], double timestamp_search);
        
    int run_fusion( string buffer_log,     string data_flag);

    int run_fusion_new( );
    
    int polyfit(std::vector<float>* lane_coeffs, const cv::Mat& xy_feature, int order );

    int GetLanePredictParameter(cv::Mat& lane_coeffs_predict, double image_timestamp, cv::Mat lane_coeffs_pre, double lane_num, double m_order );

    int GetLanePredictParameter_new(cv::Mat& lane_coeffs_predict, double image_timestamp_cur, double image_timestamp_pre, 
                                                     cv::Mat lane_coeffs_pre, double lane_num, double m_order );
    
    int LanePredict(cv::Mat& lane_coeffs_predict, cv::Mat lane_coeffs_pre, double lane_num, double m_order);

    int LanePredict_new(cv::Mat& lane_coeffs_predict, cv::Mat lane_coeffs_pre, double lane_num, double m_order, 
                                  StructVehicleState pre_vehicle_state, StructVehicleState cur_vehicle_state, 
                                  StructAtt pre_att_xy, StructAtt cur_att_xy);

    static void *thread_run_fusion(void *tmp)//�߳�ִ�к���
    {
        DataFusion *p = (DataFusion *)tmp;
        //ͨ��pָ���ӷ�����ķǾ�̬��Ա
        p->run_fusion_new();
    }
    pthread_t data_fusion_id;
    int exec_task_run_fusion()
    {        
        int ERR = pthread_create(&data_fusion_id, NULL, thread_run_fusion, (void *)this); //�����߳�ִ������
        return ERR;
    }

    // read data
    static void *thread_read_data(void *tmp)//�߳�ִ�к���
    {
        DataFusion *p = (DataFusion *)tmp;
        //ͨ��pָ���ӷ�����ķǾ�̬��Ա
        p->red_data();
    }
    pthread_t read_data_id;
    int exec_task_read_data()
    {        
        int ERR = pthread_create(&read_data_id, NULL, thread_read_data, (void *)this); //�����߳�ִ������
        return ERR;
    }



private:

    bool isFirsttimeGetParameter; // �Ƿ����ⲿ��һ�ε��ò���Ԥ��
    double m_call_pre_predict_timestamp; // ��һ֡
    double m_call_predict_timestamp; // ��ǰ�ⲿͼ����ģ�鴦���ͼƬ���ɵ�ʱ���
    bool m_new_lane_parameter_get; // �Ƿ����µ��ⲿ����
    char m_camera_match_state; // 1: ����ƥ�� -1:����cameraʱ��������ⲿ���õ�getʱ���
    
    // ����log
    string buffer_log;
    stringstream ss_log;
    stringstream ss_tmp;
    ifstream infile_log;       // ofstream
    
    /// CAN
    CAN_VehicleEstimate can_vehicle_estimate;    
    bool is_steer_angle_OK; // ��ǰ�Ƿ�steer�����Ѿ�����
    double steer_angle_deg;
    double steer_timestamp; 
    double speed_can;
    double speed_timestamp; 
    double can_timestamp;
    double pre_can_timestamp ;
    bool isFirstTime_can;
    StructVehicleState struct_vehicle_state;
    std::queue<StructVehicleState> queue_vehicle_state;
    std::vector<StructVehicleState> vector_vehicle_state;

    // IMU
    ImuAttitudeEstimate imu_attitude_estimate;
    double acc_filt_hz; // ���ٶȼƵĵ�ͨ��ֹƵ��
    bool isFirstTime_att; // �Ƿ��ǵ�һ�ν���
    double imu_timestamp;
    double pre_imu_timestamp; // IMU�����ϴεõ���ʱ�� 
    double pre_att_timestamp; // att�ϴεõ���ʱ�� 
    StructAtt struct_att;    
    std::queue<StructAtt> queue_att;// �������;
    std::vector<StructAtt> vector_att;

    // lane
    double att_cur[3] ;        
    double vehicle_vel[2];
    double vehicle_pos[2];
    double vehicle_fai;
    double att_pre[3] ;    
    double vehicle_vel_pre[2];
    double vehicle_pos_pre[2];
    double vehicle_fai_pre ;
    
    StructAtt predict_struct_att_pre; 
    StructAtt predict_struct_att_cur; 
    StructVehicleState predict_struct_vehicle_pos_pre;
    StructVehicleState predict_struct_vehicle_pos_cur; 


    // imu+speed�˶���Ϣ����
    char is_first_speed_data = 1; //  1: ��һ�λ�ȡ��speed���� 0:���ǵ�һ��
    double att_xy_pre[3]; // ��һstamp�ĽǶ�
    double att_xy_cur[3]; // ��ǰstamp�ĽǶ�
    double vehicle_yaw;
    double vehicle_yaw_pre;

    // read data
    bool is_first_read_data = 1; // �Ƿ��ǵ�һ�ν����ȡ��������ʼ��cur_timestamp
    bool is_first_read_gsensor = 1; 
    double cur_read_data_timestamp; // ��ǰ�ο��Ķ�ȡ����ʱ��(���ⲿ����ʱ���ݽ����ĵ�ǰ֡ʱ��)
   
    bool data_gsensor_update = 0;
    bool data_speed_update = 0;
    bool data_image_update = 0;

    struct StructImageFrameInfo
    {
        double timestamp;
        double index;
    };
    StructImageFrameInfo image_frame_info;

    struct StructImuData
    {
        double timestamp;
        double acc_raw[3];
        double gyro_raw[3];
        double acc[3];
        double gyro[3];
    };
    StructImuData imu_data;

    struct StructCanSpeedData
    {
        double timestamp;
        double speed;
    };
    StructCanSpeedData can_speed_data;

    // ��ȡ���ݿ���
    double cur_fusion_timestamp; // ��ǰ�ڽ��м����ʱ��㣬���ⲿ����Ԥ���ʱ������бȶ�
    double cur_data_timestamp; // ��ǰ���ݵ�ʱ���
    bool is_first_fusion_timestamp = 1; // ��һ�θ���
    bool is_first_data_timestamp = 1; // ��һ�θ���
    double data_save_length = 2; // ������ʷ���ݵĳ���(ʱ��Ϊ��λ: s)
    bool is_continue_read_data = 1; // 1:������ȡ����  2:��ͣ��ȡ���� ��fusion����
    bool extern_call_timestamp_update = 0; // �ⲿ����Ԥ�⣬������ʱ���


};


#endif  // DATA_FUSION_H
