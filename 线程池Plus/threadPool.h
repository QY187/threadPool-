#ifndef THREADPOLL_H
#define THREADPOOL_H

#include<queue>
#include<vector>
#include<memory>
#include<atomic>
#include<mutex>
#include<condition_variable>
#include<functional>
#include<iostream>
#include<unordered_map>
#include<thread>
#include<future>
#include<iostream>
#include"thread.h"

const int TASK_MAX_THRESHHOLD = 2;// INT32_MAX;
const int THREAD_MAX_THRESHOLD = 1024;
const int THREAD_MAX_IDLE_TIME = 60;

//�̳߳�����
class ThreadPool
{
public:

	ThreadPool();

	//�̳߳�����
	~ThreadPool();

	//�����̳߳صĹ���ģʽ
	void setMode(PoolMode mode);

	//����task�������������ֵ
	void setTaskQueMaxThresHold(int threshhold);

	//�����̳߳�cachedģʽ���߳���ֵ
	void setThreadSize(int threshold);

	//���̳߳��ύ����
	//ʹ�ÿɱ��ģ�壬��submitTask���Խ������������������������Ĳ���
	//����ֵfuture<>
	template<typename Func, typename...Args>
	auto submitTask(Func&& func, Args...args) -> std::future<decltype(func(args...))>
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

	//�����̳߳�
	void start(int initThreadSize);

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool operator=(const ThreadPool&) = delete;

private:
	//�����̺߳���
	void threadFunc(int threadid);

	//���pool������״̬
	bool checkRunningState()const;

private:
	std::unordered_map<int, std::unique_ptr<Thread>>threads_;
	//std::vector<std::unique_ptr<Thread>>threads_;	//�߳��б�
	size_t initThreadSize_;			//��ʼ���߳�����
	std::atomic_int curThreadSize_;//��¼��ǰ�̳߳������̵߳�������
	int threadSizeThresHold_;	//�߳�����������ֵ
	std::atomic_int idleThreadSize_;//��¼�����߳�����

	//ʹ������ָ������Զ��ͷ�Task����
	using Task = std::function<void()>;

	std::queue<Task>taskQue_;	//�������
	std::atomic_uint taskSize_;		//���������
	int taskQueThresHold_;			//�����������������ֵ

	std::mutex taskQueMtx_;			//��֤������е��̰߳�ȫ
	std::condition_variable notFull_;	//��֤������в���
	std::condition_variable notEmpty_;	//��֤������в���
	std::condition_variable exitCond_;	//�ȴ��߳���Դȫ������

	PoolMode poolMode_;		//��ǰ�̳߳صĹ���ģʽ

	std::atomic_bool isPoolRunning_;//�ֵ̳߳�����״̬

};

#endif