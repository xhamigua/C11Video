
#include <memory>
#include <iostream>
#include <string>
#include <map>
#include <queue>
#include <functional>
#include <mutex>
#include <memory>
#include <type_traits>
#include <atomic>
using namespace std;
#include <opencv2/core.hpp>
#include <opencv2/core/opengl.hpp>
#include <opencv2/opencv.hpp>
#pragma comment(lib,"opencv3_mpeg.lib") 

//通用数据结构
using MAT = cv::Mat;

void testMutiThread_show(void)
{
    //图像数据结构
    using MAT = cv::Mat;
    //Mat缓存队列
    using Mat_que = class
    {
    public:
        size_t push(MAT& pFrame)
        {
            std::lock_guard<std::mutex> lock_guard(m_mutex);
            {
                m_frames.push_back(std::forward<MAT>(pFrame));    //这里是关键 完美转移
                //m_frames.push_back(pFrame.clone());             //深拷贝还是有性能牺牲             
                //m_frames.push_back(pFrame);                     //浅拷贝会爆闪
            }
            return m_frames.size();
        };
        bool pop(MAT& pFrame)
        {
            bool found = false;
            std::lock_guard<std::mutex> lock_guard(m_mutex);
            if (!m_frames.empty())
            {
                pFrame = m_frames.front();
                m_frames.pop_front();
                found = true;
            }
            return found;
        };
        size_t size()
        {
            std::lock_guard<std::mutex> lock_guard(m_mutex);
            return m_frames.size();
        };
        void clear()
        {
            std::lock_guard<std::mutex> lock_guard(m_mutex);
            m_frames.clear();
        };
    private:
        std::deque<MAT> m_frames;
        std::mutex m_mutex;
    };
    //共享队列
    using MyFrame = struct 
    {
        Mat_que image_data_queue;                   //解码数据
        Mat_que process_data_queue;                 //处理数据
        Mat_que draw_data_queue;                    //渲染数据
    };
    
    static MyFrame m_Frame;
    std::atomic_bool brun1(true), brun2(true), brun3(true);
 
    auto CollectTd = [&]
    {
        cv::VideoCapture vcap;
        MAT capture264;
        vcap.open("jay.mp4");           //打开文件
        vcap.set(cv::CAP_PROP_POS_FRAMES, 1);
        if (!vcap.isOpened())
        {
            brun1 = brun2 = brun3 = false;
            return;
        }
        int numflag(0);
        while (true)
        {
            std::string strnum = std::to_string(numflag++);
            vcap >> capture264;         //此处模拟相机采集数据
            if (!capture264.empty())
            {
                cout << "CollectThread image_data>> " << strnum << endl;
                m_Frame.image_data_queue.push(capture264);
            }
            else {
                brun1 = brun2 = brun3 = false;
                break;
            }
            this_thread::sleep_for(std::chrono::milliseconds(3));
        }
    };
 
    auto DecodeTd =[&]
    {
        while (brun1)
        {
            MAT image_data;
            m_Frame.image_data_queue.pop(image_data);
            cout << "DecodeThread image_data<< " << endl;
            if (!image_data.empty())
            {
                //从收到的数据进行解码,解码后送到处理线程
                cout << "DecodeThread process_data>> " << endl;
                m_Frame.process_data_queue.push(image_data);
            }
            else
                this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    };
 
    auto ProcessTd = [&]
    {
        while (brun2)
        {
            MAT process_data;
            m_Frame.process_data_queue.pop(process_data);
            cout << "ProcessThread process_data<< " << endl;
            if (!process_data.empty())
            {
                //处理图像 处理完送到渲染线程
                cv::circle(process_data, cv::Point(120, 120), 120, cv::Scalar(0, 255, 0), 8);
                //ImageProcess(process_data);
                MAT draw_data = process_data.clone();
                cout << "ProcessThread draw_data>> " << endl;
                m_Frame.draw_data_queue.push(draw_data);
            }
            else
                this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    };
 
    auto DrawTd = [&]
    {
        cv::namedWindow("DrawThread", cv::WINDOW_OPENGL);   //gl
        cv::resizeWindow("DrawThread", 320, 240);
        while (brun3)
        {
            MAT draw_data;
            m_Frame.draw_data_queue.pop(draw_data);
            //用opencv渲染
            if (!draw_data.empty())
            {
                cv::imshow("DrawThread", draw_data);
                cv::waitKey(1);
                cout << "DrawThread draw_data<< " << endl;
            }
            else
                this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    };
 
    std::thread td1(CollectTd);
    std::thread td2(DecodeTd);
    std::thread td3(ProcessTd);
    std::thread td4(DrawTd);
    td1.join();
    td2.join();
    td3.join();
    td4.join();
    this_thread::sleep_for(std::chrono::hours(10));
}


int main(int argc, char** argv)
{
    testMutiThread_show();
    return 0;
}

