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
    DataFusion();
    
    ~DataFusion() {}

    void Initialize( );

    int read_data();
    
    void update_current_fusion_timestamp( double data_timestample);

    void update_current_data_timestamp( double data_timestample);

    // �ж��Ƿ�Ҫ������ȡ����
    bool update_read_data_state( );

    // ֻ�����趨ʱ�䳤�ȵ�����
    void  delete_history_save_data( );

    // ����ʱ������Ҷ�Ӧ������
    int get_timestamp_data(double (&vehicle_pos)[2], double (&att)[3], double timestamp_search);
        
    int run_fusion( );
    
    int polyfit(std::vector<float>* lane_coeffs, const cv::Mat& xy_feature, int order );
   
    int get_lane_predict_parameter(cv::Mat& lane_coeffs_predict, double image_timestamp_cur, double image_timestamp_pre, 
                                              cv::Mat lane_coeffs_pre, double lane_num, double m_order );
  
    int lane_predict(cv::Mat& lane_coeffs_predict, cv::Mat lane_coeffs_pre, double lane_num, double m_order, 
                          double vehicle_pos_pre[2],  double att_pre[3],
                          double vehicle_pos_cur[2],  double att_cur[3]);

    int get_predict_feature(std::vector<cv::Point2f>& vector_feature_predict, std::vector<cv::Point2f> vector_feature_pre ,
                                                int64 image_timestamp_pre, int64 image_timestamp_cur);

    int feature_predict(std::vector<cv::Point2f>& vector_feature_predict, std::vector<cv::Point2f> vector_feature_pre ,
                                        double vehicle_pos_pre[2], double att_pre[3], double vehicle_pos_cur[2], double att_cur[3]);

    // �����ںϵ��߳�
    static void *thread_run_fusion(void *tmp)//�߳�ִ�к���
    {
        DataFusion *p = (DataFusion *)tmp;        
        p->run_fusion(); //ͨ��pָ���ӷ�����ķǾ�̬��Ա;
    }
    pthread_t data_fusion_id;
    int exec_task_run_fusion()
    {        
        int ERR = pthread_create(&data_fusion_id, NULL, thread_run_fusion, (void *)this); //�����߳�ִ������
        return ERR;
    }

    // read data �߳�
    static void *thread_read_data(void *tmp)//�߳�ִ�к���
    {
        DataFusion *p = (DataFusion *)tmp;        
        p->read_data(); //ͨ��pָ���ӷ�����ķǾ�̬��Ա;
    }
    pthread_t read_data_id;
    int exec_task_read_data()
    {        
        int ERR = pthread_create(&read_data_id, NULL, thread_read_data, (void *)this); //�����߳�ִ������
        return ERR;
    }

private:  
    // ����log
    string buffer_log;
    stringstream ss_log;
    stringstream ss_tmp;
    ifstream infile_log;       // ofstream

    // �ⲿ����������Ԥ���ʱ��
    double m_call_predict_timestamp; // ��ǰ�ⲿͼ����ģ�鴦���ͼƬ���ɵ�ʱ���
    
    /// CAN
    CAN_VehicleEstimate can_vehicle_estimate;
    double m_pre_can_timestamp ;
    StructVehicleState m_struct_vehicle_state;
    std::vector<StructVehicleState> m_vector_vehicle_state;

    // IMU
    ImuAttitudeEstimate imu_attitude_estimate;
    double m_acc_filt_hz; // ���ٶȼƵĵ�ͨ��ֹƵ��
    double m_gyro_filt_hz; //�����ǵĵ�ͨ��ֹƵ��
    bool m_isFirstTime_att; // �Ƿ��ǵ�һ�ν���
    double m_pre_imu_timestamp; // IMU�����ϴεõ���ʱ�� 
    double m_pre_att_timestamp; // att�ϴεõ���ʱ�� 
    StructAtt m_struct_att;    
    std::vector<StructAtt> m_vector_att;

    // imu+speed�˶���Ϣ����
    char m_is_first_speed_data; //  1: ��һ�λ�ȡ��speed���� 0:���ǵ�һ��    

    // read data
    bool m_is_first_read_gsensor;    
    bool m_data_gsensor_update; // �ֱ��Ӧ�������Ƿ��Ѿ�����
    bool m_data_speed_update;
    bool m_data_image_update;

    struct StructImageFrameInfo
    {
        double timestamp;
        double index;
    };
    StructImageFrameInfo m_image_frame_info;

    struct StructImuData
    {
        double timestamp;
        double acc_raw[3];
        double gyro_raw[3];
        double acc[3];
        double gyro[3];
    };
    StructImuData m_imu_data;

    struct StructCanSpeedData
    {
        double timestamp;
        double speed;
    };
    StructCanSpeedData m_can_speed_data;

    // ��ȡ���ݿ���
    double m_cur_fusion_timestamp; // ��ǰ�ڽ��м����ʱ��㣬
    double m_cur_data_timestamp; // ��ǰ���ݵ�ʱ���, ���ⲿ����Ԥ���ʱ������бȶ�
    bool m_is_first_fusion_timestamp; // ��һ��fusion����
    bool m_is_first_data_timestamp; // ��һ��read data����
    double m_data_save_length; // ������ʷ���ݵĳ���(ʱ��Ϊ��λ: s)
    bool m_is_continue_read_data; // 1:������ȡ����  0:��ͣ��ȡ���� ��data_timestamp����
};


#endif  // DATA_FUSION_H
