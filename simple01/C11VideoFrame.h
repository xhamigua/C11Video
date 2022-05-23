//**********************************************************************
//                        C11 ��Ƶ���߳̿��
//                                          @2021-01-04��������
//**********************************************************************
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

//ͨ�����ݽṹ
using MAT = cv::Mat;

//�������ݽṹ
struct MyFrame
{
    std::queue<MAT> image_data_queue;                   //��������
    std::mutex decode_mutex;
    std::queue<MAT> process_data_queue;                 //��������
    std::mutex process_mutex;
    std::queue<MAT> push_data_queue;                    //��������
    std::mutex push_mutex;
    std::queue<MAT> draw_data_queue;                    //��Ⱦ����
    std::mutex draw_mutex;
    std::queue<MAT> tracker_data_queue;                 //��������
    std::mutex tracker_mutex;
    //��շ���
    void clear(queue<MAT>& my_queue)
    {
        queue<MAT> empty_queue;
        std::swap(my_queue, empty_queue);
    }
    void clearall(void)
    {
        clear(image_data_queue);
        clear(process_data_queue);
        clear(push_data_queue);
        clear(draw_data_queue);
        clear(tracker_data_queue);
    }
};

//�̻߳���
class BaseThread
{
public:
    BaseThread(MyFrame& mFrame);
    virtual ~BaseThread(void);
    MyFrame& m_Frame;
public:
    void begintk(void);
    void endtk(void);
    bool getstatus(void);
    //�̺߳���������̬����
    virtual void run(void) = 0;
private:
    std::atomic_bool m_status;
    std::thread th;
};

//�ɼ��߳�
class CollectTd :public BaseThread
{
public:
    using BaseThread::BaseThread;
    virtual void run(void);
    void setplayMp4(string url);
private:
    string m_mp4;
    cv::VideoCapture vcap;
    MAT capture264;
};

//�����߳�
class DecodeTd :public BaseThread
{
public:
    using BaseThread::BaseThread;
    virtual void run(void);
};

//��Ⱦ�߳�
class DrawTd :public BaseThread
{
public:
    using BaseThread::BaseThread;
    virtual void run(void);
};

//�����߳�
class ProcessTd :public BaseThread
{
public:
    using BaseThread::BaseThread;
    virtual void run(void);
};

//�����߳�
class PushTd :public BaseThread
{
public:
    using BaseThread::BaseThread;
    virtual void run(void);
};

//�����߳�
class TrackerTd :public BaseThread
{
public:
    using BaseThread::BaseThread;
    virtual void run(void);
};

//������
class C11VideoFrame
{
public:
    static void testrun(void)
    {
        C11VideoFrame mgr;
        mgr.init();
        mgr.beginwork();
        this_thread::sleep_for(std::chrono::hours(10));
    }
public:
    C11VideoFrame(void);
    ~C11VideoFrame(void);
public:
    void init(void);
    void beginwork(void);
    void endwork(void);
private:
    std::unique_ptr<CollectTd> m_collect;
    std::unique_ptr<DecodeTd> m_decode;
    std::unique_ptr<ProcessTd> m_Process;
    std::unique_ptr<PushTd> m_Push;
    std::unique_ptr<DrawTd> m_Draw;
    std::unique_ptr<TrackerTd> m_Tracker;
    std::thread th;
    MyFrame m_Frame;                    //�м����ݽṹ
};





