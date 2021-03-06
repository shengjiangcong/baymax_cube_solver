/** ---------------------------------------------------------------------------
 * Copyright (c) 2018~2019, PS-Micro, Co. Ltd.  All rights reserved.
---------------------------------------------------------------------------- */

#include <ros/ros.h>
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>
#include <stdio.h>
#include <vector>
#include <unordered_map>

#include "cube_color_detector/DetectObjectSrv.h"

using namespace std;
using namespace cv;

image_transport::Publisher image_pub_;

struct HSV
{
    //色相、饱和度、亮度
    int hmin = 35;
    int hmax = 60;

    int smin = 90;
    int smax = 255;

    int vmin = 60;
    int vmax = 255;

    cv::Scalar color = cv::Scalar(255, 255, 255);
};

//初始化h参数
HSV hsv_object[6];

int ROI_x, ROI_y, ROI_wdith, ROI_height;

bool in_hsv_range(cv::Mat image2hsv, Point p, HSV hsv_para)
{
     double h = image2hsv.at<Vec3b>(p)[0];//得到H的数值
     if (h >= 175 && h <= 180)
        h = 1;
     double s = image2hsv.at<Vec3b>(p)[1];//得到S的数值
     double v = image2hsv.at<Vec3b>(p)[2];//得到V的数值

     if (h >= hsv_para.hmin && h <= hsv_para.hmax && s >= hsv_para.smin && s <= hsv_para.smax && v >= hsv_para.vmin && v <= hsv_para.vmax)
         return true;
     else
         return false;
}

// service回调函数，输入参数req，输出参数res
bool detectCallback(cube_color_detector::DetectObjectSrv::Request  &req,
                    cube_color_detector::DetectObjectSrv::Response &res)
{
    sensor_msgs::ImageConstPtr msg;
    try
    {
        msg = ros::topic::waitForMessage<sensor_msgs::Image>("/camera/color/image_raw", ros::Duration(5.0));
    }
    catch (ros::Exception& e)
    {
        ROS_ERROR("Timeout waiting for image data: %s", e.what());
        res.result = cube_color_detector::DetectObjectSrvResponse::TIMEOUT;
        return false;
    }

    cv_bridge::CvImagePtr cv_ptr;
    try
    {
        cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
    }
    catch (cv_bridge::Exception& e)
    {
        ROS_ERROR("cv_bridge exception: %s", e.what());
        return false;
    }

    //初始化存储目标检测框的vector
    std::vector<cv::Rect> box;

    // 图像裁剪
    cv::Rect select = cv::Rect(ROI_x, ROI_y, ROI_wdith, ROI_height);
    cv::Mat& img_input = cv_ptr->image;

    img_input = img_input(select);

    //彩色图像的灰度值归一化，颜色空间转换，输出为HSV格式图像
    cv::Mat image2hsv, bgr;
    img_input.convertTo(bgr, CV_32FC3, 1.0 / 255, 0);
    cv::cvtColor(img_input, image2hsv,  cv::COLOR_BGR2HSV);

    
    static int countt = 0;
    int widdth = 0;
    int length = 0;
    Point p1(224, 120);

    if (countt == 0 || countt == 1)
    {
        p1.x = 230;
        p1.y = 100;
        widdth = 60;
        length = 50;
    }
    else if (countt == 2)
    {
        p1.x = 380;
        p1.y = 72;
         widdth = 100;
         length = 110;
    }
    else if (countt == 3 || countt == 4)
    {
        p1.x = 190;
        p1.y = 115;
        widdth = 100;
        length = 95;
    }
    else if (countt == 5)
    {
        p1.x = 166;
        p1.y = 150;
        widdth = 110;
        length = 101;
    }

    vector<Point> a(9);

    for (int i = 0; i < a.size(); i++)
    {
        a[i].x = p1.x + (i % 3) * widdth;
        a[i].y = p1.y + (i / 3) * length;

        circle(img_input, a[i], 3,hsv_object[3].color,-1);
        
        double h = image2hsv.at<Vec3b>(a[i])[0];//得到H的数值
        if (h >= 175 && h <= 180)
            h = 1;       
        double s = image2hsv.at<Vec3b>(a[i])[1];//得到S的数值
        double v = image2hsv.at<Vec3b>(a[i])[2];//得到V的数值

        cout << "Point" << i + 1 << ": h" << h << " s" << s << " v" << v << endl; 
    }

    unordered_map<int, char> hashtab;
    hashtab[0] = 'F';
    hashtab[1] = 'L';
    hashtab[2] = 'R';
    hashtab[3] = 'U';
    hashtab[4] = 'D';
    hashtab[5] = 'B';

    string detect_res;

    unordered_map<int, int> hashtab1;

    hashtab1[0] = 0;
    hashtab1[1] = 0;
    hashtab1[2] = 0;
    hashtab1[3] = 0;
    hashtab1[4] = 0;
    hashtab1[5] = 0;

    for (int i = 0; i < a.size(); i++)
    {
        for (int j = -1; j < 2; j++)
        {
            for (int k = -1; k < 2; k++)
            {
                for (int cou = 0; cou <6; cou++)
                {
                   Point tmp;
                   tmp.x = a[i].x + j;
                   tmp.y = a[i].y + k;
                   if (in_hsv_range(image2hsv, tmp, hsv_object[cou]))
                   {
                      hashtab1[cou]++;
                      break;
                   }
                      

                }
            }
        }
 
        if (hashtab1[0] > 3)
        {
            detect_res += hashtab[0];
            cout << "红  ";
        }   
        else if (hashtab1[1] > 3)
        {
            detect_res += hashtab[1];
            cout << "绿  ";
        }    
        else if (hashtab1[2] > 3)
        {
            detect_res += hashtab[2];
            cout << "蓝  ";
        }  
        else if (hashtab1[3] > 3)
        {
            detect_res += hashtab[3];
            cout << "白  ";
        } 
        else if (hashtab1[4] > 3)
        {
            detect_res += hashtab[4];
            cout << "黄  ";
        } 
        else if (hashtab1[5] > 3)
        {
            detect_res += hashtab[5];
            cout << "橙  ";
        } 
        else
        {
            detect_res += hashtab[3];
            cout << "白  ";
        }
        if (i == 2 || i == 5 || i == 8)
           cout << endl;
    hashtab1[0] = 0;
    hashtab1[1] = 0;
    hashtab1[2] = 0;
    hashtab1[3] = 0;
    hashtab1[4] = 0;
    hashtab1[5] = 0;
    }

    /*for (int i = 0; i < a.size(); i++)
    {
        for (int j = 0; j < 6; j++)//红绿蓝白黄橘
        {
           if (in_hsv_range(image2hsv, a[i], hsv_object[j]))
           {
               // cout << "point" << i << ": ";
                //cout << j << endl;
                detect_res += hashtab[j];
                if (j == 0)
                    cout << "红  ";
                if (j == 1)
                    cout << "绿  ";
                if (j == 2)
                    cout << "蓝  ";
                if (j == 3)
                    cout << "白  ";
                if (j == 4)
                    cout << "黄  ";
                if (j == 5)
                    cout << "橙  ";
                break;
           }
           if (j == 5)
           {
              detect_res += hashtab[3];
              cout << "白  ";
           }
        }
        if (i == 2 || i == 5 || i == 8)
           cout << endl;
    }*/

    //服务返回：detect_res是颜色对应的九位字母序列，result是成功标志。
    res.detect_res = detect_res;
    res.result = cube_color_detector::DetectObjectSrvResponse::SUCCESS;

    image_pub_.publish(cv_ptr->toImageMsg());
    countt ++;
    return true;
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "object_detector");
    ros::NodeHandle nh;
    image_transport::ImageTransport it_(nh);

    //加载yaml参数
    //h
    nh.param("red/hmin", hsv_object[0].hmin, 0);
    nh.param("red/hmax", hsv_object[0].hmax, 40);

    nh.param("green/hmin", hsv_object[1].hmin, 60);
    nh.param("green/hmax", hsv_object[1].hmax, 140);

    nh.param("blue/hmin", hsv_object[2].hmin, 160);
    nh.param("blue/hmax", hsv_object[2].hmax, 230);

    nh.param("white/hmin", hsv_object[3].hmin, 160);
    nh.param("white/hmax", hsv_object[3].hmax, 230);

    nh.param("yellow/hmin", hsv_object[4].hmin, 160);
    nh.param("yellow/hmax", hsv_object[4].hmax, 230);

    nh.param("orange/hmin", hsv_object[5].hmin, 160);
    nh.param("orange/hmax", hsv_object[5].hmax, 230);

    //s
    nh.param("red/smin", hsv_object[0].smin, 0);
    nh.param("red/smax", hsv_object[0].smax, 40);

    nh.param("green/smin", hsv_object[1].smin, 60);
    nh.param("green/smax", hsv_object[1].smax, 140);

    nh.param("blue/smin", hsv_object[2].smin, 160);
    nh.param("blue/smax", hsv_object[2].smax, 230);

    nh.param("white/smin", hsv_object[3].smin, 160);
    nh.param("white/smax", hsv_object[3].smax, 230);

    nh.param("yellow/smin", hsv_object[4].smin, 160);
    nh.param("yellow/smax", hsv_object[4].smax, 230);

    nh.param("orange/smin", hsv_object[5].smin, 160);
    nh.param("orange/smax", hsv_object[5].smax, 230);

    //v
    nh.param("red/vmin", hsv_object[0].vmin, 0);
    nh.param("red/vmax", hsv_object[0].vmax, 40);

    nh.param("green/vmin", hsv_object[1].vmin, 60);
    nh.param("green/vmax", hsv_object[1].vmax, 140);

    nh.param("blue/vmin", hsv_object[2].vmin, 160);
    nh.param("blue/vmax", hsv_object[2].vmax, 230);

    nh.param("white/vmin", hsv_object[3].vmin, 160);
    nh.param("white/vmax", hsv_object[3].vmax, 230);

    nh.param("yellow/vmin", hsv_object[4].vmin, 160);
    nh.param("yellow/vmax", hsv_object[4].vmax, 230);

    nh.param("orange/vmin", hsv_object[5].vmin, 160);
    nh.param("orange/vmax", hsv_object[5].vmax, 230);

    nh.param("image/ROI_x", ROI_x, 200);
    nh.param("image/ROI_y", ROI_y, 100);
    nh.param("image/ROI_wdith", ROI_wdith, 600);
    nh.param("image/ROI_height", ROI_height, 300);

    hsv_object[0].color = cv::Scalar(0, 0, 255);
    hsv_object[1].color = cv::Scalar(0, 255, 0);
    hsv_object[2].color = cv::Scalar(255, 0, 0);
    hsv_object[3].color = cv::Scalar(0, 0, 0);
    hsv_object[4].color = cv::Scalar(0, 255, 255);
    hsv_object[5].color = cv::Scalar(0, 69, 255);

    ros::ServiceServer service = nh.advertiseService("/cube_detect", detectCallback);

    image_pub_ = it_.advertise("/cube_detection_result", 1);

    ros::Rate loop_rate(10);
    while(ros::ok())
    {
        ros::spinOnce();

        loop_rate.sleep();
    }

    return 0;
}
