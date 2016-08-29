/// @file relative_locate.h
/// @brief
/// @author Devin (devin@minieye.cc)
/// @date 2015-06-19
/// Copyright (C) 2015 - MiniEye INC.

#ifndef  RELATIVE_LOCATE_H
#define  RELATIVE_LOCATE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma execution_character_set("utf-8")
#endif

#include <vector>
#include "linear_r3.h"
struct Posture
{
    Posture& operator=(const Posture& lhs)
    {
        alfa = lhs.alfa;
        beta = lhs.beta;
        gamma = lhs.gamma;
        return *this;
    }

    double alfa; /// alfa - ������
    double beta; /// beta - �����
    double gamma; /// gamma - ƫ����
};

struct CameraParameter
{
    CameraParameter()
    {
        camera_pos_x = 0; //
        camera_pos_y = 0;
        camera_pos_z = 0;
    }

    CameraParameter& operator=(const CameraParameter& lhs)
    {
        stretch_angle_w = lhs.stretch_angle_w;
        stretch_angle_h = lhs.stretch_angle_h;
        pixel_x_number = lhs.pixel_x_number;
        pixel_y_number = lhs.pixel_y_number;
        camera_pos_x = lhs.camera_pos_x;
        camera_pos_y = lhs.camera_pos_y;
        camera_pos_z = lhs.camera_pos_z;
        cu = lhs.cu;
        cv = lhs.cv;
        return *this;
    }

    double stretch_angle_w; /// stretch_angle_w �����Ž�(����)
    double stretch_angle_h; /// stretch_angle_h �����Ž�(����)
    int pixel_x_number; /// pixel_width ������Ԫ��
    int pixel_y_number; /// pixel_height ������Ԫ��
    double camera_pos_x;
    double camera_pos_y;
    double camera_pos_z;

    double cu; // ��������(����)
    double cv; // ��������(����)
};

class RelativeLocate
{
public:

    /// @brief construct RelativeLocate obj
    RelativeLocate() {}

    ~RelativeLocate() {}

    void Initialize(const Posture& posture, const CameraParameter& cammera_para);


    /// @brief ���̽�����ڲ�ͬ����̬�µ����߷�����������ߵķ���
    ///
    /// @param axis
    /// axis[0]�ǿ�ȷ���
    /// axis[1]�����߷���
    /// axis[2]�Ǹ߶ȷ���
    void GetCoordXYZ(std::vector<VectorR3>* axis);


    /// @brief ����ÿ������Ԫ���ӵ�ķ�������
    ///
    /// @param pixel_x ��������λ������X
    /// @param pixel_y ��������λ������Y
    /// @param orientation ��������
    void GetOrientation(int pixel_x, int pixel_y, VectorR3* orientation);


    /// @brief ��ȡ����������λ�ö�Ӧ�ڿռ�����ϵ������
    ///
    /// @param position
    /// @param pixel_x
    /// @param pixel_y
    /// @param geo_x
    /// @param geo_y
    /// @param geo_z
    void GetGeoCoordinate(int pixel_x, int pixel_y,
            double* geo_x, double* geo_y, double* geo_z);

    void GetPixelCoordinate(double geo_x, double geo_y, double geo_z,
            int* pixel_x, int* pixel_y);

private:
    Posture m_posture;
    CameraParameter m_camera_para;
    VectorR3 m_axis[3];
    double m_camera_w_length; /// ��ȷ�����Ԫ�ܳ���(assume ������1����λ)
    double m_camera_h_length; /// �߶ȷ�����Ԫ�ܳ���(assume ������1����λ)
    double m_camera_w_ratio;
    double m_camera_h_ratio;
    VectorR3 m_camera_position;
    double m_opt_center_u;
    double m_opt_center_v;
};


#endif  // RELATIVE_LOCATE_H
