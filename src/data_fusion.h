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
public:

    /// @brief construct  obj
    DataFusion();
    
    ~DataFusion() {}

    void Initialize( );

    // TODO:
    int red_data();
    
    int data_fusion_main();
        
    int run_fusion( string buffer_log,     string data_flag);
    
    int polyfit(std::vector<float>* lane_coeffs, const cv::Mat& xy_feature, int order );

    int GetLanePredictParameter(cv::Mat& lane_coeffs_predict, double image_timestamp, cv::Mat lane_coeffs_pre, double lane_num, double m_order );
    
    int LanePredict(cv::Mat& lane_coeffs_predict, cv::Mat lane_coeffs_pre, double lane_num, double m_order);

    static void *thread_data_fusion(void *tmp)//�߳�ִ�к���
    {
        DataFusion *p = (DataFusion *)tmp;
        //ͨ��pָ���ӷ�����ķǾ�̬��Ա
        p->data_fusion_main();
    }

    pthread_t data_fusion_id;
    int exec_task_data_fusion()
    {        
        int ERR = pthread_create(&data_fusion_id, NULL, thread_data_fusion, (void *)this); //�����߳�ִ������
        return ERR;
    }

    // read data
    static void *thread_read_data(void *tmp)//�߳�ִ�к���
    {
        DataFusion *p = (DataFusion *)tmp;
        //ͨ��pָ���ӷ�����ķǾ�̬��Ա
        p->red_data();
    }

//    pthread_t read_data_id;
//    int exec_task_data_fusion()
//    {        
//        int ERR = pthread_create(&read_data_id,NULL,thread_data_fusion,(void *)this); //�����߳�ִ������
//        return ERR;
//    }



private:

    bool isFirsttimeGetParameter; // �Ƿ����ⲿ��һ�ε��ò���Ԥ��
    double m_extern_call_pre_image_stamptime; // ��һ֡
    double m_extern_call_cur_image_stamptime; // ��ǰ�ⲿͼ����ģ�鴦���ͼƬ���ɵ�ʱ���
    bool m_new_lane_parameter_get; // �Ƿ����µ��ⲿ����
    char m_camera_match_state; // 1: ����ƥ�� -1:����cameraʱ��������ⲿ���õ�getʱ���
    
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
        double fai;
    };

    // ����log
    string buffer_log;
    stringstream ss_log;
    stringstream ss_tmp;
    ifstream infile_log;       // ofstream
    double log_data[2];

    /// CAN
    CAN_VehicleEstimate can_vehicle_estimate;    
    bool is_steer_angle_OK; // ��ǰ�Ƿ�steer�����Ѿ�����
    double steer_angle_deg;
    double steer_timestamp; 
    double speed_can;
    double speed_timestamp; 
    double can_timestamp;
    double can_timestamp_pre ;
    bool isFirstTime_can;
    StructVehicleState struct_vehicle_state;
    std::queue<StructVehicleState> queue_vehicle_state;

    // IMU
    ImuAttitudeEstimate imu_attitude_estimate;
    double acc_filt_hz; // ���ٶȼƵĵ�ͨ��ֹƵ��
    bool isFirstTime_att; // �Ƿ��ǵ�һ�ν���
    double imu_timestamp;
    double pre_imu_timestamp; // IMU�����ϴεõ���ʱ�� 
    StructAtt struct_att;    
    std::queue<StructAtt> queue_att;// �������;

    // lane
    double att_cur[3] ;        
    double vehicle_vel[2];
    double vehicle_pos[2];
    double vehicle_fai;

    double att_pre[3] ;    
    double vehicle_vel_pre[2];
    double vehicle_pos_pre[2];
    double vehicle_fai_pre ;

    // imu+speed�˶���Ϣ����
    char is_first_speed_data = 1; //  1: ��һ�λ�ȡ��speed���� 0:���ǵ�һ��
    double att_xy_pre[3]; // ��һstamp�ĽǶ�
    double att_xy_cur[3]; // ��ǰstamp�ĽǶ�
    double vehicle_yaw;
    double vehicle_yaw_pre;


};


#endif  // DATA_FUSION_H
