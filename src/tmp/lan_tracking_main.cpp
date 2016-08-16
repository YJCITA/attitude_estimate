#include "opencv2/opencv.hpp"
#include "relative_locate.h"
#include "bird_perspective_mapping.h"
#include "imu_attitude_estimate.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <math.h>

using namespace std;
#define ONE_G 9.80665f;

int main(int argc, char *argv[])
{
	ImuAttitudeEstimate imu_attitude_estimate;
	imu_attitude_estimate.Initialize();

	// ��ȡTXT����
	bool isFirstTime = 1; // �Ƿ��ǵ�һ�ν���
	double imu_timestamp = 0.0f;
	double pre_imu_timestamp = 0.0f; // IMU�����ϴεõ���ʱ��
	double att_new[3] = {0.0f, 0.0f, 0.0f };
	double AccData[3] = {1.0f, 0.0f, 8.8f };
	double AccDataFilter[3] = {1.0f, 0.0f, 8.8f}; // һ�׵�֮ͨ�������
	double acc_filt_hz = 5.0f; // ���ٶȼƵĵ�ͨ��ֹƵ��
	
	double GyroData[3] = {0.0f, 0.0f, 0.0f};
	double GyroDrift[3] = {0.0155f, -0.0421f, -0.0217f};  // ��������ƫ�����߹���
	double dt = 0.0f;

	double imu_data[9];
	double accel_range_scale = 8.0f/32768;
	double gyro_range_scale = 2000.0f/180*M_PI/32768;

	// Y1ģ��ļ��ٶȼ�У������
	VectorR3 A0;
	A0.Set(0.0628f, 0.0079f, -0.0003f);
	VectorR3 A1_inv_0 ; // inv(A)�ĵ�һ��
	A1_inv_0.Set(0.9986f, -0.0027f, 0.0139f); 	
	VectorR3 A1_inv_1;
	A1_inv_1.Set(0.0164f, 0.9993f, -0.0176f);
	VectorR3 A1_inv_2;
	A1_inv_2.Set(-0.0159f, 0.0064f, 0.9859f );

	VectorR3 AccRawDataR3;
	VectorR3 GyroRawDataR3;
	VectorR3 AccRawDataR3_tmp;

	// ��ȡTXT����
	string buffer;
    stringstream ss;
    ifstream inputFile("data/log-gsensor.ini");
    while(getline(inputFile,buffer))
    {
        ss.clear();
        ss.str(buffer);
        ss>>imu_data[0]>>imu_data[1]>>imu_data[2]>>imu_data[3]>>imu_data[4]>>imu_data[5]>>imu_data[6]>>imu_data[7]>>imu_data[8];

		imu_timestamp = imu_data[0] + imu_data[1]*1e-6;

		AccData[0] = imu_data[2]*accel_range_scale;
		AccData[1] = imu_data[3]*accel_range_scale;
		AccData[2] = imu_data[4]*accel_range_scale;
		AccRawDataR3.Set(AccData[0], AccData[1], AccData[2]);
		
		AccRawDataR3_tmp = AccRawDataR3 - A0;
		AccData[2]= -(A1_inv_0.x*AccRawDataR3_tmp.x + A1_inv_0.y*AccRawDataR3_tmp.y + A1_inv_0.z*AccRawDataR3_tmp.z)*ONE_G; // ��������ϵZ
		AccData[1]= -(A1_inv_1.x*AccRawDataR3_tmp.x + A1_inv_1.y*AccRawDataR3_tmp.y + A1_inv_1.z*AccRawDataR3_tmp.z)*ONE_G; // ��������ϵY
		AccData[0]= -(A1_inv_2.x*AccRawDataR3_tmp.x + A1_inv_2.y*AccRawDataR3_tmp.y + A1_inv_2.z*AccRawDataR3_tmp.z)*ONE_G;  // ��������ϵX
		
		GyroData[2] = -imu_data[5]*gyro_range_scale - GyroDrift[2]; // ��������ϵZ
		GyroData[1] = -imu_data[6]*gyro_range_scale - GyroDrift[1]; // ��������ϵY
		GyroData[0] = -imu_data[7]*gyro_range_scale - GyroDrift[0]; // ��������ϵX

		if(isFirstTime)
		{
			pre_imu_timestamp = imu_timestamp;
			AccDataFilter[0] = AccData[0];
			AccDataFilter[1] = AccData[1];
			AccDataFilter[2] = AccData[2];
			isFirstTime = 0;
		}else{
			dt = imu_timestamp - pre_imu_timestamp;
			pre_imu_timestamp = imu_timestamp;
			imu_attitude_estimate.LowpassFilter3f(AccDataFilter, AccDataFilter, AccData, dt, acc_filt_hz);
			imu_attitude_estimate.Get_Attitude(att_new, AccDataFilter, GyroData, dt);
		}
		
    }

	

//    RelativeLocate rel_locate;

//    Posture posture;
//    // ������
//    posture.alfa = (-1) * (-0.2) * CV_PI / 180;
//    // �����
//    posture.beta = 0.0 * CV_PI / 180;
//    // ƫ����
//    posture.gamma = (-3.5)* CV_PI / 180;

//    CameraParameter cam_para;
//    cam_para.pixel_x_number = 1280; // ����ĺ�����Ԫ��
//    cam_para.pixel_y_number = 720; // �����������Ԫ��
//    cam_para.camera_pos_x = 0; // ���λ��(����ڵ����ͶӰΪ����ԭ��)
//    cam_para.camera_pos_y = 0;
//    cam_para.camera_pos_z = 1.29744; // ����߶�
//    double fu = 1437.72915; // ��һ������
//    double fv = 1437.72915; // ��һ������
//    cam_para.cu = 640;
//    cam_para.cv = 360;

//    cam_para.stretch_angle_w = atan(cam_para.pixel_x_number / (2.0 * fu)) * 2;
//    cam_para.stretch_angle_h = atan(cam_para.pixel_y_number / (2.0 * fv)) * 2;
//    rel_locate.Initialize(posture, cam_para);

//    cv::Mat image = cv::imread("test.jpg", 1);
//    // IPM
//    // ����IPM�ķ�Χ
//    double x_start_offset = -3.25;
//    double x_end_offset = 3.25;
//    double x_resolution = 0.02;
//    double y_start_offset = 6.0;
//    double y_end_offset = 25.0;
//    double y_resolution = 0.08;

//    int ipm_height = static_cast<int>((y_end_offset - y_start_offset) / y_resolution);
//    int ipm_width = static_cast<int>((x_end_offset - x_start_offset) / x_resolution);
//    cv::Mat ipm_img(ipm_height, ipm_width, CV_8UC1);

//    int row = ipm_height - 1;
//    int col = 0;
//    double y = 0;
//    double x = 0;
//    for (y = y_start_offset, row = ipm_height - 1;
//            row >= 0;
//            y += y_resolution, --row) {
//        for (x = x_start_offset, col = 0;
//                col < ipm_width;
//                x += x_resolution, ++col) {
//            int uu, vv;
//            rel_locate.GetPixelCoordinate(x, y, 0, &uu, &vv);
//            *(ipm_img.ptr<uint8_t>(row, col)) = image.ptr<uint8_t>(vv, uu)[0];
//        }
//    }

//    cv::imwrite("ipm.jpg", ipm_img);

    return 0;
}
