#include"threadPool.h"
#include"thread.h"

ThreadPool::ThreadPool()
	:initThreadSize_(4)
	, taskSize_(0)
	, idleThreadSize_(0)
	, curThreadSize_(0)
	, taskQueThresHold_(TASK_MAX_THRESHHOLD)
	, threadSizeThresHold_(THREAD_MAX_THRESHOLD)
	, poolMode_(PoolMode::MODE_FIXED)
	, isPoolRunning_(false)
{}

ThreadPool::~ThreadPool() {
	isPoolRunning_ = false;

	//�ȴ��̷߳��أ�������״̬:���������ڽ�����
	std::unique_lock<std::mutex>lock(taskQueMtx_);
	notEmpty_.notify_all();
	exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0; });

}

void ThreadPool::setMode(PoolMode mode)
{
	if (!checkRunningState())
		poolMode_ = mode;
	else return;
}

//����task�������������ֵ
void ThreadPool::setTaskQueMaxThresHold(int threshhold)
{
	if (checkRunningState())
		return;
	taskQueThresHold_ = threshhold;
}

//�����̳߳�cachedģʽ���߳���ֵ
void ThreadPool::setThreadSize(int threshold) {
	if (checkRunningState())
		return;
	if (poolMode_ == PoolMode::MODE_CACHED)
		threadSizeThresHold_ = threshold;
}


/**auto ThreadPool::submitTask(Func&& func, Args...args) -> std::future<decltype(func(args...))>
{
	//������񣬷�������
	using RType = decltype(func(args...));
	auto task = std::make_shared<std::packaged_task<RType()>>(
		std::bind(std::forward <Func>(func), std::forward<Args>(args)...));
	std::future<RType>result = task->get_future();

	//��ȡ��
	std::unique_lock<std::mutex>lock(taskQueMtx_);

	//�̵߳�ͨ�� �ȴ���������п���

	if (!notFull_.wait_for(lock, std::chrono::seconds(1),
		[&]()->bool {return taskQue_.size() < (size_t)taskQueThresHold_; }))
	{
		std::cerr << "task queue is full,submit task fail" << std::endl;
		auto task = std::make_shared<std::packaged_task<RType()>>(
			[]()->RType {return RType(); });
		(*task)();
		return task->get_future();
	}
	//����п��࣬������ŵ�����
	//taskQue_.emplace(sp);
	taskQue_.emplace([task]() {(*task)(); });
	taskSize_++;

	//��Ϊ�·�������������п϶������ˣ���notEmpty_�Ͻ���֪ͨ
	notEmpty_.notify_all();

	//cachedģʽ ������ȽϽ��� ��Ҫ�������������Ϳ����̵߳��������ж��Ƿ���Ҫ�����µ��̳߳���
	if (poolMode_ == PoolMode::MODE_CACHED
		&& taskSize_ > idleThreadSize_
		&& curThreadSize_ < threadSizeThresHold_)
	{
		std::cout << "create new thread" << std::endl;
		//�����µ��̶߳���
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		//�����߳�
		threads_[threadId]->start();
		curThreadSize_++;
		idleThreadSize_++;
	}

	//���������result����
	return result;

}
*/
void ThreadPool::start(int initThreadSize) {
	//�����̳߳ص�����״̬
	isPoolRunning_ = true;

	//��¼��ʼ�̸߳���
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;
	//�����̶߳���
	for (int i = 0; i < initThreadSize_; i++)
	{

		//����thread�̶߳����ʱ�򣬰��̺߳�������thread�̶߳���
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
	}

	//���������߳�
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start();
		idleThreadSize_++;	//	��¼��ʼ�����̵߳�����

	}
}

//�����̺߳���
void ThreadPool::threadFunc(int threadid)
{
	auto lastTime = std::chrono::high_resolution_clock().now();

	for (;;)
	{
		Task task;
		{
			//�Ȼ�ȡ��
			std::unique_lock<std::mutex>lock(taskQueMtx_);

			std::cout << "tid:" << std::this_thread::get_id()
				<< "���Ի�ȡ����..." << std::endl;

			//cachedģʽ�£��п��ܴ����˺ܶ��̣߳�������ʱ�䳬��60s����Ҫ���յ�
			//�������յ�(����initThreadSize_�������߳�Ҫ���л���
			//ÿһ�뷵��һ��

			while (taskQue_.size() == 0)

			{
				if (!isPoolRunning_)
				{
					threads_.erase(threadid);
					std::cout << "threadid:" << std::this_thread::get_id() << "exit!" << std::endl;
					exitCond_.notify_all();

					return;
				}
				if (poolMode_ == PoolMode::MODE_CACHED)
				{
					if (std::cv_status::timeout ==
						notEmpty_.wait_for(lock, std::chrono::seconds(1)))
					{
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);

						if (dur.count() >= THREAD_MAX_IDLE_TIME
							&& curThreadSize_ > initThreadSize_)
						{
							//��ʼ���յ�ǰ�߳�
							threads_.erase(threadid);
							curThreadSize_--;
							idleThreadSize_--;

							std::cout << "threadid:" << std::this_thread::get_id() << "exit!" << std::endl;
							return;
						}
					}
				}
				else
				{
					//�ȴ�notEmpty����
					notEmpty_.wait(lock);
				}

				/*if (!isPoolRunning_)
				{
					threads_.erase(threadid);
					std::cout << "threadid:" << std::this_thread::get_id() << "exit!" << std::endl;
					exitCond_.notify_all();
					return;
				}*/
			}


			idleThreadSize_--;

			std::cout << "tid:" << std::this_thread::get_id()
				<< "��ȡ����ɹ�..." << std::endl;

			//���������ȡ������
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;

			//�����Ȼ�� ʣ�����񣬼���֪ͨ�����߳�ִ������
			if (taskQue_.size() > 0)
			{
				notEmpty_.notify_all();
			}

			//ȡ��һ�����񣬽���֪ͨ��֪ͨ���Լ����ύ��������
			notFull_.notify_all();
		}
		//��ǰ�߳�ִ���������
		if (task != nullptr)
		{

			//	task->run();
			task();

		}
		idleThreadSize_++;

		lastTime = std::chrono::high_resolution_clock().now();//�����߳�ִ���������ʱ��	
	}
}

//���pool������״̬
bool ThreadPool::checkRunningState()const {
	return isPoolRunning_;
}
