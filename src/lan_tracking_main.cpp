#include "opencv2/opencv.hpp"
#include "gflags/gflags.h"

#include "relative_locate.h"
#include "bird_perspective_mapping.h"

// ployfit
#include "common/base/stdint.h"
#include "common/math/polyfit.h"
#include "common/base/log_level.h"

#include "datafusion_math.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <math.h>
#include <vector>
#include <queue> 
#include "data_fusion.h"
using namespace std;


/// lane
int m_order = 1;
int lane_num = 3;
int pts_num = 4;    
//int frame_index = 0; // ͼ��֡���
int lane_index = 0; // �����ߵ����
double lane_timestamp;
int uv_feature[2][12];
string buffer_lane;
stringstream ss_lane;
ifstream infile_lane("data/lane_data.ini");       // ofstream
cv::Mat uv_feature_pts;
cv::Mat xy_feature; 
cv::Mat xy_feature_pre = cv::Mat::zeros(2, pts_num, CV_32FC1);; // ��һ֡���г����ߵ�������
cv::Mat xy_feature_predict; 
cv::Mat lane_coeffs = cv::Mat::zeros(m_order+1, lane_num, CV_32FC1);
cv::Mat lane_coeffs_pre = cv::Mat::zeros(m_order+1, lane_num, CV_32FC1);
cv::Mat lane_coeffs_predict = cv::Mat::zeros(m_order+1, lane_num, CV_32FC1);
double image_timestamp;

// �ⲿlane
bool isFirstTime_Lane = 1;
double att_cur[3] = {0.0f, 0.0f, 0.0f };    
double att_pre[3] = {0.0f, 0.0f, 0.0f };
double vehicle_vel[2];
double vehicle_pos[2];
double vehicle_pos_pre[2] = {0.0, 0.0};
double vehicle_fai_pre = 0.0;


// ����log
string buffer_log;
string data_flag;    
stringstream ss_log;
stringstream ss_tmp;
ifstream infile_log("data/log.txt");       // ofstream
double log_data[2];

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


DEFINE_string(image_name, "./1.jpg", "image_name");
DEFINE_double(fu, 1506.64297, "fu");
DEFINE_double(fv, 1504.18761, "fv");
DEFINE_double(cu, 664.30351, "cu");
DEFINE_double(cv, 340.94998, "cv");
DEFINE_double(camera_height, 1.3, "camera height mm");    // ??? mm
DEFINE_double(pitch, -1.8, "pitch angle (degree)");
DEFINE_double(yaw, 0.0, "yaw angle (degree)");
DEFINE_int32(image_width, 1280, "image width");
DEFINE_int32(image_height, 720, "image height");
DEFINE_double(x_start_offset, -7.0, "x start offset");
DEFINE_double(x_end_offset, 7.0, "x start offset");
DEFINE_double(y_start_offset, 1.0, "y start offset");
DEFINE_double(y_end_offset, 50.0, "y end offset");
DEFINE_double(x_res, 0.04, "x resolution");
DEFINE_double(y_res, 0.1, "y resolution");

void LoadImage(cv::Mat* image, string image_name)
{
    cv::Mat org_image = cv::imread(image_name, 1);
    cv::Mat channels[3];
    cv::split(org_image, &channels[0]);
    image->create(org_image.rows, org_image.cols, CV_32FC1);
    channels[0].convertTo(*image, CV_32FC1);
    *image = *image * (1.0 / 255);
}

int polyfit1(std::vector<float>* lane_coeffs, const cv::Mat& xy_feature, int order )
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

int main(int argc, char *argv[])
{
// ��ʼ��
    google::InitGoogleLogging(argv[0]);
    FLAGS_log_dir = "./log/";
    
    CameraPara camera_para;
    camera_para.fu = FLAGS_fu;
    camera_para.fv = FLAGS_fv;
    camera_para.cu = FLAGS_cu;
    camera_para.cv = FLAGS_cv;
    camera_para.height = FLAGS_camera_height; // m
    camera_para.pitch = FLAGS_pitch * CV_PI / 180;
    camera_para.yaw = FLAGS_yaw * CV_PI / 180;
    camera_para.image_width = FLAGS_image_width;
    camera_para.image_height = FLAGS_image_height;
    BirdPerspectiveMapping bp_mapping(camera_para);

    IPMPara ipm_para;
    ipm_para.x_limits[0] = FLAGS_x_start_offset;
    ipm_para.x_limits[1] = FLAGS_x_end_offset;
    ipm_para.y_limits[0] = FLAGS_y_start_offset;
    ipm_para.y_limits[1] = FLAGS_y_end_offset;
    ipm_para.x_scale = FLAGS_x_res;
    ipm_para.y_scale = FLAGS_y_res;
    bp_mapping.GetUVLimitsFromXY(&ipm_para);
    
    DataFusion data_fusion;
    data_fusion.Initialize(camera_para, ipm_para);    
//    data_fusion.exec_task_data_fusion(); // �������е�ʱ��Ӧ�������ö����̳߳������е�

// �������ñ�ע�����ݲ���
    // �ⲿlaneѭ������
    //int start_image_index = 30; // ����һ֡��ʼ
    int image_cal_step = 4;// ÿ������֡����һ�γ�����Ԥ��
    int image_cal_counter = 0;  //����
    bool is_lane_match_image = 0;        
    while(getline(infile_log, buffer_log))                
    {    
        ss_tmp.clear();
        ss_tmp.str(buffer_log);
        ss_tmp>>log_data[0]>>log_data[1]>>data_flag;
        ss_log.clear();
        ss_log.str(buffer_log);

        if(data_flag == "cam_frame")
        {
            /// IPM
            double camera_raw_timestamp[2];
            string camera_flag, camera_add, image_index_str;
            string image_name;
            int image_index;
            ss_log>>camera_raw_timestamp[0]>>camera_raw_timestamp[1]>>camera_add>>camera_flag>>image_index;
            image_timestamp = camera_raw_timestamp[0] + camera_raw_timestamp[1]*1e-6;            
            image_index = image_index + 1; // image�Ǵ�0��ʼ������
            if(++image_cal_counter >= image_cal_step)
            {
                //printf("image_index: %d\n", image_index);    
                LOG(INFO)<<"image_index: "<<image_index;
                image_cal_counter = 0; // ����
                // ����ƥ��ĳ����߱�ע����
                is_lane_match_image = 0; // �������ƥ���lane
                while(!is_lane_match_image)
                {
                    getline(infile_lane, buffer_lane);
                    ss_lane.clear();
                    ss_lane.str(buffer_lane);
                    ss_lane>>lane_index>>lane_timestamp
                        >>uv_feature[0][0]>>uv_feature[1][0]>>uv_feature[0][1]>>uv_feature[1][1]>>uv_feature[0][2]>>uv_feature[1][2]>>uv_feature[0][3]>>uv_feature[1][3]
                        >>uv_feature[0][4]>>uv_feature[1][4]>>uv_feature[0][5]>>uv_feature[1][5]>>uv_feature[0][6]>>uv_feature[1][6]>>uv_feature[0][7]>>uv_feature[1][7]
                        >>uv_feature[0][8]>>uv_feature[1][8]>>uv_feature[0][9]>>uv_feature[1][9]>>uv_feature[0][10]>>uv_feature[1][10]>>uv_feature[0][11]>>uv_feature[1][11];
                    if(lane_index == image_index)
                    {
                        is_lane_match_image = 1;
                    }else if(lane_index < image_index)
                    {
                        continue;
                    }else{
                        printf("error: lane index is bigger than image index!!!\n");
                        break;
                    }
                }

                stringstream ss;
                ss << image_index;
                ss >> image_index_str;
                image_name = "data/1/" + image_index_str + ".jpg";

                cv::Mat org_image;
                LoadImage(&org_image, image_name);
                cv::Mat ipm_image = cv::Mat::zeros(ipm_para.height+1, ipm_para.width+1, CV_32FC1);
                for (int i = 0; i < ipm_para.height; ++i) 
                {
                    int base = i * ipm_para.width;
                    for (int j = 0; j < ipm_para.width; ++j) 
                    {
                        int offset = base + j;
                        float ui = ipm_para.uv_grid.at<float>(0, offset);
                        float vi = ipm_para.uv_grid.at<float>(1, offset);
                        if (ui < ipm_para.u_limits[0] || ui > ipm_para.u_limits[1] || vi < ipm_para.v_limits[0] || vi > ipm_para.v_limits[1])
                            continue;
                        int x1 = int(ui), x2 = int(ui + 1);
                        int y1 = int(vi), y2 = int(vi + 1);
                        float x = ui - x1, y = vi - y1;
                        float val = org_image.at<float>(y1, x1) * (1 - x) * (1-y) +    org_image.at<float>(y1, x2) * x * (1-y) +
                                    org_image.at<float>(y2, x1) * (1-x) * y + org_image.at<float>(y2, x2) * x * y;
                        ipm_image.at<float>(i, j) = static_cast<float>(val);
                    }
                }
                
                // ִ��Ԥ��lane
                lane_coeffs.copyTo(lane_coeffs_pre);                
                data_fusion.GetLanePredictParameter(lane_coeffs_predict, image_timestamp, lane_coeffs_pre, lane_num, m_order );
                

                /// ��ǰlane
                xy_feature = cv::Mat::zeros(2, pts_num, CV_32FC1);            
                uv_feature_pts = cv::Mat::zeros(2, 4, CV_32FC1);                
                for(int k=0; k<lane_num; k++)
                {
                    for(int i1 = 0; i1<pts_num; i1++)
                    {
                        uv_feature_pts.at<float>(0, i1) = uv_feature[0][k*pts_num + i1];
                        uv_feature_pts.at<float>(1, i1) = uv_feature[1][k*pts_num + i1];
                    }        
                    // get these points on the ground plane
                    bp_mapping.TransformImage2Ground(uv_feature_pts, &xy_feature);                        
                    std::vector<float> lane_coeffs_t;
                    polyfit1(&lane_coeffs_t, xy_feature, m_order); // ���������    Y = AX(X������);
                    
                    lane_coeffs.at<float>(0, k) = lane_coeffs_t[0];
                    lane_coeffs.at<float>(1, k) = lane_coeffs_t[1];
                    
                }

                /// ��IPM�б�ע��ǰlane
                std::vector<int> x(ipm_para.height+2);
                std::vector<int> y(ipm_para.height+2);
                int i_index = -1;            
                for (float i = ipm_para.y_limits[0]; i < ipm_para.y_limits[1]; i+=ipm_para.y_scale) // x
                {
                    i_index += 1;
                    y[i_index] = (-i + ipm_para.y_limits[1])/ipm_para.y_scale;
                    float x_t = lane_coeffs.at<float>(0, 1) + lane_coeffs.at<float>(1, 1)*i;
                    x[i_index] = (x_t + ipm_para.x_limits[1])/ipm_para.x_scale;

                    if (x[i_index] < 0 || x[i_index] > ipm_para.width )
                    {
                        continue;
                    }else{
                        ipm_image.at<float>(y[i_index], x[i_index]) = 0.9;
                    }            
                }    

                /// ��IPM�б�ע��һ֡lane
                i_index = -1;            
                for (float i = ipm_para.y_limits[0]; i < ipm_para.y_limits[1]; i+=ipm_para.y_scale) // x
                {
                    i_index += 1;
                    y[i_index] = (-i + ipm_para.y_limits[1])/ipm_para.y_scale;
                    float x_t = lane_coeffs_pre.at<float>(0, 1) + lane_coeffs_pre.at<float>(1, 1)*i;
                    x[i_index] = (x_t + ipm_para.x_limits[1])/ipm_para.x_scale;

                    if (x[i_index] < 0 || x[i_index] > ipm_para.width )
                    {
                        continue;
                    }else{
                        ipm_image.at<float>(y[i_index], x[i_index]) = 0.75;
                    }            
                }

                /// ��IPM�б�עԤ��lane
                std::vector<int> x_predict(ipm_para.height+2);
                std::vector<int> y_predict(ipm_para.height+2);
                i_index = -1;            
                for (float i = ipm_para.y_limits[0]; i < ipm_para.y_limits[1]; i+=ipm_para.y_scale) // x
                {
                    i_index += 1;
                    y_predict[i_index] = (-i + ipm_para.y_limits[1])/ipm_para.y_scale;
                    float x_t = lane_coeffs_predict.at<float>(0, 1) + lane_coeffs_predict.at<float>(1, 1)*i;
                    x_predict[i_index] = (x_t + ipm_para.x_limits[1])/ipm_para.x_scale;

                    if (x_predict[i_index] < 0 || x_predict[i_index] > ipm_para.width )
                    {
                        continue;
                    }else{
                        ipm_image.at<float>(y_predict[i_index], x_predict[i_index]) = 0.1; // ��ɫ
                    }            
                }    
                
                cv::imshow("ipm", ipm_image);
                if(cv::waitKey(50))
                {}
                
            }    
        }

    }
    
    return 0;
}



