#ifndef DATA_FUSION_H
#define DATA_FUSION_H

#include <fstream>
#include <string>
#include <sstream>
#include <math.h>
#include <vector>

#include "opencv2/opencv.hpp"
#include "common/relative_locate/linear_r3.h"
#include "common/base/stdint.h"
#include "common/base/log_level.h"

#include "imu_attitude_estimate.h"
#include "can_vehicle_estimate.h"

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

    int ReadData();
    
    void UpdateCurrentFusionTimestamp( double data_timestample);

    void UpdateCurrentDataTimestamp( double data_timestample);

    // �ж��Ƿ�Ҫ������ȡ����
    bool UpdateRreadDataState( );

    // ֻ�����趨ʱ�䳤�ȵ�����
    void  DeleteHistoryData( );

    // ����ʱ������Ҷ�Ӧ������
    int GetTimestampData(double timestamp_search, double vehicle_pos[2], double att[3] );
        
    int RunFusion( );
    
    int Polyfit(const cv::Mat& xy_feature, int order , std::vector<float>* lane_coeffs);
   
    int GetLanePredictParameter(double image_timestamp_cur, double image_timestamp_pre, const cv::Mat &lane_coeffs_pre, 
                                                double lane_num, double m_order, cv::Mat* lane_coeffs_predict );
  
    int LanePredict( const cv::Mat& lane_coeffs_pre, const double lane_num, double m_order, 
                          const double vehicle_pos_pre[2],  const double att_pre[3],
                          const double vehicle_pos_cur[2],  const double att_cur[3], cv::Mat* lane_coeffs_predict);

    int GetPredictFeature( const std::vector<cv::Point2f>& vector_feature_pre ,int64 image_timestamp_pre, int64 image_timestamp_cur, 
                                      std::vector<cv::Point2f>* vector_feature_predict);

    int FeaturePredict( const std::vector<cv::Point2f>& vector_feature_pre , double vehicle_pos_pre[2], double att_pre[3], 
                                 double vehicle_pos_cur[2], double att_cur[3], std::vector<cv::Point2f>* vector_feature_predict);
    // �����ںϵ��߳�
    static void *ThreadRunFusion(void *tmp)//�߳�ִ�к���
    {
        DataFusion *p = (DataFusion *)tmp;        
        p->RunFusion(); //ͨ��pָ���ӷ�����ķǾ�̬��Ա;
    }
    pthread_t data_fusion_id;
    int ExecTaskRunFusion()
    {        
        int ERR = pthread_create(&data_fusion_id, NULL, ThreadRunFusion, (void *)this); //�����߳�ִ������
        return ERR;
    }

    // read data �߳�
    static void *ThreadReadData(void *tmp)//�߳�ִ�к���
    {
        DataFusion *p = (DataFusion *)tmp;        
        p->ReadData(); //ͨ��pָ���ӷ�����ķǾ�̬��Ա;
    }
    pthread_t read_data_id;
    int ExecTaskReadData()
    {        
        int ERR = pthread_create(&read_data_id, NULL, ThreadReadData, (void *)this); //�����߳�ִ������
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
    CAN_VehicleEstimate m_can_vehicle_estimate;
    double m_pre_can_timestamp ;
    StructVehicleState m_struct_vehicle_state;
    std::vector<StructVehicleState> m_vector_vehicle_state;

    // IMU
    ImuAttitudeEstimate m_imu_attitude_estimate;
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
