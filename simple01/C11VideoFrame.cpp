
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
    vcap.open(m_mp4);           //���ļ�
    vcap.set(cv::CAP_PROP_POS_FRAMES, 1);
    if (!vcap.isOpened())
        return;
    int numflag(0);
    while (getstatus())
    {
        std::string strnum = std::to_string(numflag++);
        vcap >> capture264;     //�˴�ģ������ɼ�����
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
            //���յ������ݽ��н���,������͵������߳�
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
                //����ͼ�� �������͵���Ⱦ�߳�
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
                //��opencv��Ⱦ
                cv::imshow("DrawThread", draw_data);
                cv::waitKey(1);
                cout << "DrawThread draw_data<< " << endl;
            }
            //�������ݼ���
            MAT tracker_data = draw_data;
            {
                std::lock_guard<std::mutex> lock_guard(m_Frame.tracker_mutex);
                cout << "DrawThread tracker_data>> " << endl;
                m_Frame.tracker_data_queue.push(tracker_data);
            }
            //������������
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
            //�������ͷ���
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
            //�м�������㷨
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
            //�ɼ��߳�ֹͣ �������Թر���
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


