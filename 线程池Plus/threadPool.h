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

//线程池类型
class ThreadPool
{
public:

	ThreadPool();

	//线程池析构
	~ThreadPool();

	//设置线程池的工作模式
	void setMode(PoolMode mode);

	//设置task任务队列上限阈值
	void setTaskQueMaxThresHold(int threshhold);

	//设置线程池cached模式的线程阈值
	void setThreadSize(int threshold);

	//给线程池提交任务
	//使用可变参模板，让submitTask可以接收任意任务函数和任意数量的参数
	//返回值future<>
	template<typename Func, typename...Args>
	auto submitTask(Func&& func, Args...args) -> std::future<decltype(func(args...))>
	{
		//打包任务，放入任务
		using RType = decltype(func(args...));
		auto task = std::make_shared<std::packaged_task<RType()>>(
			std::bind(std::forward <Func>(func), std::forward<Args>(args)...));
		std::future<RType>result = task->get_future();

		//获取锁
		std::unique_lock<std::mutex>lock(taskQueMtx_);

		//线程的通信 等待任务队列有空余

		if (!notFull_.wait_for(lock, std::chrono::seconds(1),
			[&]()->bool {return taskQue_.size() < (size_t)taskQueThresHold_; }))
		{
			std::cerr << "task queue is full,submit task fail" << std::endl;
			auto task = std::make_shared<std::packaged_task<RType()>>(
				[]()->RType {return RType(); });
			(*task)();
			return task->get_future();
		}
		//如果有空余，把任务放到队列
		//taskQue_.emplace(sp);
		taskQue_.emplace([task]() {(*task)(); });
		taskSize_++;

		//因为新放了任务，任务队列肯定不空了，在notEmpty_上进行通知
		notEmpty_.notify_all();

		//cached模式 任务处理比较紧急 需要根据任务数量和空闲线程的数量，判断是否需要创建新的线程出来
		if (poolMode_ == PoolMode::MODE_CACHED
			&& taskSize_ > idleThreadSize_
			&& curThreadSize_ < threadSizeThresHold_)
		{
			std::cout << "create new thread" << std::endl;
			//创建新的线程对象
			auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
			int threadId = ptr->getId();
			threads_.emplace(threadId, std::move(ptr));
			//启动线程
			threads_[threadId]->start();
			curThreadSize_++;
			idleThreadSize_++;
		}

		//返回任务的result对象
		return result;

	}

	//开启线程池
	void start(int initThreadSize);

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool operator=(const ThreadPool&) = delete;

private:
	//定义线程函数
	void threadFunc(int threadid);

	//检查pool的运行状态
	bool checkRunningState()const;

private:
	std::unordered_map<int, std::unique_ptr<Thread>>threads_;
	//std::vector<std::unique_ptr<Thread>>threads_;	//线程列表
	size_t initThreadSize_;			//初始的线程数量
	std::atomic_int curThreadSize_;//记录当前线程池里面线程的总数量
	int threadSizeThresHold_;	//线程数量上限阈值
	std::atomic_int idleThreadSize_;//记录空闲线程数量

	//使用智能指针可以自动释放Task对象
	using Task = std::function<void()>;

	std::queue<Task>taskQue_;	//任务队列
	std::atomic_uint taskSize_;		//任务的数量
	int taskQueThresHold_;			//任务队列数量上限阈值

	std::mutex taskQueMtx_;			//保证任务队列的线程安全
	std::condition_variable notFull_;	//保证任务队列不满
	std::condition_variable notEmpty_;	//保证任务队列不空
	std::condition_variable exitCond_;	//等待线程资源全部回收

	PoolMode poolMode_;		//当前线程池的工作模式

	std::atomic_bool isPoolRunning_;//线程持的启动状态

};

#endif