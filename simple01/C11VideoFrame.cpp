
#include "C11VideoFrame.h"

BaseThread::BaseThread(MyFrame& mFrame) : m_status(false)
, m_Frame(mFrame)
{
}

BaseThread::~BaseThread(void)
{
    endtk();
}

void BaseThread::begintk(void)
{
    if (m_status)
        endtk();
    m_status = true;
    th = thread([&]()
    {
        this->run();
    });
}

void BaseThread::endtk(void)
{
    m_status = false;
    if (th.joinable())
        th.join();
}

bool BaseThread::getstatus(void)
{
    return m_status;
}


void CollectTd::run(void)
{
    vcap.open(m_mp4);           //打开文件
    vcap.set(cv::CAP_PROP_POS_FRAMES, 1);
    if (!vcap.isOpened())
        return;
    int numflag(0);
    while (getstatus())
    {
        std::string strnum = std::to_string(numflag++);
        vcap >> capture264;     //此处模拟相机采集数据
        if (!capture264.empty())
        {
            std::lock_guard<std::mutex> lock_guard(m_Frame.decode_mutex);
            cout << "CollectThread image_data>> " << strnum << endl;
            m_Frame.image_data_queue.push(capture264);
        }
        else
            endtk();
        this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

void CollectTd::setplayMp4(string url)
{
    m_mp4 = url;
}

void DecodeTd::run(void)
{
    while (getstatus())
    {
        if (!m_Frame.image_data_queue.empty())
        {
            MAT image_data;
            {
                std::lock_guard<std::mutex> lock_guard(m_Frame.decode_mutex);
                image_data = m_Frame.image_data_queue.front();
                m_Frame.image_data_queue.pop();
                cout << "DecodeThread image_data<< "  << endl;
            }
            //从收到的数据进行解码,解码后送到处理线程
            MAT process_data = image_data;
            {
                std::lock_guard<std::mutex> lock_guard(m_Frame.process_mutex);
                cout << "DecodeThread process_data>> " << endl;
                m_Frame.process_data_queue.push(process_data);
            }
        }
        else
            this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void ProcessTd::run(void)
{
    while (getstatus())
    {
        if (!m_Frame.process_data_queue.empty())
        {
            MAT process_data;
            {
                std::lock_guard<std::mutex> lock_guard(m_Frame.process_mutex);
                process_data = m_Frame.process_data_queue.front();
                m_Frame.process_data_queue.pop();
                cout << "ProcessThread process_data<< "  << endl;
            }
            if (!process_data.empty())
            {
                //处理图像 处理完送到渲染线程
                //ImageProcess(process_data);
                MAT draw_data = process_data;
                std::lock_guard<std::mutex> lock_guard(m_Frame.draw_mutex);
                cout << "ProcessThread draw_data>> "  << endl;
                m_Frame.draw_data_queue.push(draw_data);
            }
        }
        else
            this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void DrawTd::run(void)
{
    cv::namedWindow("DrawThread", cv::WINDOW_OPENGL);   //gl
    cv::resizeWindow("DrawThread", 800,600);
    while (getstatus())
    {
        if (!m_Frame.draw_data_queue.empty())
        {
            MAT draw_data;
            {
                std::lock_guard<std::mutex> lock_guard(m_Frame.draw_mutex);
                draw_data = m_Frame.draw_data_queue.front();
                m_Frame.draw_data_queue.pop();
                //用opencv渲染
                cv::imshow("DrawThread", draw_data);
                cv::waitKey(1);
                cout << "DrawThread draw_data<< " << endl;
            }
            //跟踪数据计算
            MAT tracker_data = draw_data;
            {
                std::lock_guard<std::mutex> lock_guard(m_Frame.tracker_mutex);
                cout << "DrawThread tracker_data>> " << endl;
                m_Frame.tracker_data_queue.push(tracker_data);
            }
            //处理推流数据
            MAT push_data = draw_data;
            {
                std::lock_guard<std::mutex> lock_guard(m_Frame.push_mutex);
                cout << "DrawThread push_data>> " << endl;
                m_Frame.push_data_queue.push(push_data);
            }
        }
        else
            this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void PushTd::run(void)
{
    while (getstatus())
    {
        if (!m_Frame.push_data_queue.empty())
        {
            MAT push_data;
            {
                std::lock_guard<std::mutex> lock_guard(m_Frame.push_mutex);
                push_data = m_Frame.push_data_queue.front();
                m_Frame.push_data_queue.pop();
                cout << "PushThread push_data<< " << endl;
            }
            //调用推送方法
            if (!push_data.empty())
            {
                //RtmpPuller(push_data.data, push_data.cols * push_data.rows * push_data.channels());
            }
        }
        else
            this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void TrackerTd::run(void)
{
    while (getstatus())
    {
        if (!m_Frame.tracker_data_queue.empty())
        {
            MAT tracker_data;
            {
                std::lock_guard<std::mutex> lock_guard(m_Frame.tracker_mutex);
                tracker_data = m_Frame.tracker_data_queue.front();
                m_Frame.tracker_data_queue.pop();
                cout << "TrackerThread tracker_data<< " << endl;
            }
            //中间跟踪器算法
            //tracker(tracker_data);
        }
        else
            this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}




C11VideoFrame::C11VideoFrame(void)
{
}

C11VideoFrame::~C11VideoFrame(void)
{
}

void C11VideoFrame::init(void)
{
    m_collect = unique_ptr<CollectTd>(new CollectTd(m_Frame));
    m_collect->setplayMp4("Jay.mp4");
    m_decode = unique_ptr<DecodeTd>(new DecodeTd(m_Frame));
    m_Process = unique_ptr<ProcessTd>(new ProcessTd(m_Frame));
    m_Push = unique_ptr<PushTd>(new PushTd(m_Frame));
    m_Draw = unique_ptr<DrawTd>(new DrawTd(m_Frame));
    m_Tracker = unique_ptr<TrackerTd>(new TrackerTd(m_Frame));
}

void C11VideoFrame::beginwork(void)
{
    m_decode->begintk();
    m_Process->begintk();
    m_Push->begintk();
    m_Draw->begintk();
    m_Tracker->begintk();
    m_collect->begintk();
    th = thread([&]()
    {
        while (true)
        {
            //采集线程停止 其他可以关闭了
            if (!m_collect->getstatus())
            {
                m_decode->endtk();
                m_Process->endtk();
                m_Push->endtk();
                m_Draw->endtk();
                m_Tracker->endtk();
                break;
            }
            this_thread::sleep_for(std::chrono::milliseconds(800));
        }
    });
}

void C11VideoFrame::endwork(void)
{
    m_collect->endtk();
    m_decode->endtk();
    m_Process->endtk();
    m_Push->endtk();
    m_Draw->endtk();
    m_Tracker->endtk();
}


