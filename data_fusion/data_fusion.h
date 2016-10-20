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
#include "datafusion_math.h"


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

    // 
    int ReadData();
    
    void UpdateCurrentFusionTimestamp( double data_timestample);

    void UpdateCurrentDataTimestamp( double data_timestample);

    // 判断是否还要继续读取数据
    bool UpdateRreadDataState( );

    // 只保留设定时间长度的数据
    void  DeleteHistoryData( );

    // 根据时间戳查找对应的数据
    int GetTimestampData(double timestamp_search, double vehicle_pos[2], double att[3] );
        
    int RunFusion( );
    
    int Polyfit(const cv::Mat& xy_feature, int order , std::vector<float>* lane_coeffs);
   
    int GetLanePredictParameter(double image_timestamp_cur, double image_timestamp_pre, const cv::Mat &lane_coeffs_pre, 
                                                double lane_num, double m_order, cv::Mat* lane_coeffs_predict );
  
    int LanePredict( const cv::Mat& lane_coeffs_pre, const double lane_num, double m_order, const double vehicle_pos_pre[2],  const double att_pre[3],
                          const double vehicle_pos_cur[2],  const double att_cur[3], cv::Mat* lane_coeffs_predict);

    int GetPredictFeature( const std::vector<cv::Point2f>& vector_feature_pre ,int64 image_timestamp_pre, int64 image_timestamp_cur, 
                                      std::vector<cv::Point2f>* vector_feature_predict);

    int FeaturePredict( const std::vector<cv::Point2f>& vector_feature_pre , double vehicle_pos_pre[2], double att_pre[3], 
                                 double vehicle_pos_cur[2], double att_cur[3], std::vector<cv::Point2f>* vector_feature_predict);
    // 数据融合的线程
    static void *ThreadRunFusion(void *tmp)//线程执行函数
    {
        DataFusion *p = (DataFusion *)tmp;        
        p->RunFusion(); //通过p指针间接访问类的非静态成员;
    }
    pthread_t data_fusion_id;
    int ExecTaskRunFusion()
    {        
        int ERR = pthread_create(&data_fusion_id, NULL, ThreadRunFusion, (void *)this); //启动线程执行例程
        return ERR;
    }

    // read data 线程
    static void *ThreadReadData(void *tmp)//线程执行函数
    {
        DataFusion *p = (DataFusion *)tmp;        
        p->ReadData(); //通过p指针间接访问类的非静态成员;
    }
    pthread_t read_data_id;
    int ExecTaskReadData()
    {        
        int ERR = pthread_create(&read_data_id, NULL, ThreadReadData, (void *)this); //启动线程执行例程
        return ERR;
    }

private:  
    // 读入log
    string buffer_log;
    stringstream ss_log;
    stringstream ss_tmp;
    ifstream infile_log;       // ofstream

    // 外部调用特征点预测的时间
    double m_call_predict_timestamp; // 当前外部图像处理模块处理的图片生成的时间戳
    
    /// CAN
    CAN_VehicleEstimate m_can_vehicle_estimate;
    double m_pre_can_timestamp ;
    StructVehicleState m_struct_vehicle_state;
    std::vector<StructVehicleState> m_vector_vehicle_state;

    // IMU
    ImuAttitudeEstimate m_imu_attitude_estimate;
    double m_acc_filt_hz; // 加速度计的低通截止频率
    double m_gyro_filt_hz; //陀螺仪的低通截止频率
    bool m_isFirstTime_att; // 是否是第一次进入
    double m_pre_imu_timestamp; // IMU数据上次得到的时刻 
    double m_pre_att_timestamp; // att上次得到的时刻 
    StructAtt m_struct_att;    
    std::vector<StructAtt> m_vector_att;

    // imu+speed运动信息解算
    char m_is_first_speed_data; //  1: 第一次获取到speed数据 0:不是第一次    

    // read data
    bool m_is_first_read_gsensor;    
    bool m_data_gsensor_update; // 分别对应的数据是否已经更新
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

    // 读取数据控制
    double m_cur_fusion_timestamp; // 当前在进行计算的时间点，
    double m_cur_data_timestamp; // 当前数据的时间戳, 跟外部调用预测的时间戳进行比对
    bool m_is_first_fusion_timestamp; // 第一次fusion更新
    bool m_is_first_data_timestamp; // 第一次read data更新
    double m_data_save_length; // 保存历史数据的长度(时间为单位: s)
    bool m_is_continue_read_data; // 1:继续读取数据  0:暂停读取数据 由data_timestamp控制
};


#endif  // DATA_FUSION_H
