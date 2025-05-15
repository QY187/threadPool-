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

	//等待线程返回，有两种状态:阻塞和正在进行中
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

//设置task任务队列上限阈值
void ThreadPool::setTaskQueMaxThresHold(int threshhold)
{
	if (checkRunningState())
		return;
	taskQueThresHold_ = threshhold;
}

//设置线程池cached模式的线程阈值
void ThreadPool::setThreadSize(int threshold) {
	if (checkRunningState())
		return;
	if (poolMode_ == PoolMode::MODE_CACHED)
		threadSizeThresHold_ = threshold;
}


/**auto ThreadPool::submitTask(Func&& func, Args...args) -> std::future<decltype(func(args...))>
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
*/
void ThreadPool::start(int initThreadSize) {
	//设置线程池的运行状态
	isPoolRunning_ = true;

	//记录初始线程个数
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;
	//创建线程对象
	for (int i = 0; i < initThreadSize_; i++)
	{

		//创建thread线程对象的时候，把线程函数给到thread线程对象
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
	}

	//启动所有线程
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start();
		idleThreadSize_++;	//	记录初始空闲线程的数量

	}
}

//定义线程函数
void ThreadPool::threadFunc(int threadid)
{
	auto lastTime = std::chrono::high_resolution_clock().now();

	for (;;)
	{
		Task task;
		{
			//先获取锁
			std::unique_lock<std::mutex>lock(taskQueMtx_);

			std::cout << "tid:" << std::this_thread::get_id()
				<< "尝试获取任务..." << std::endl;

			//cached模式下，有可能创建了很多线程，但空闲时间超过60s，需要回收掉
			//结束回收掉(超过initThreadSize_数量的线程要进行回收
			//每一秒返回一次

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
							//开始回收当前线程
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
					//等待notEmpty条件
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
				<< "获取任务成功..." << std::endl;

			//从任务队列取出任务
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;

			//如果依然有 剩余任务，继续通知其他线程执行任务
			if (taskQue_.size() > 0)
			{
				notEmpty_.notify_all();
			}

			//取出一个任务，进行通知，通知可以继续提交生产任务
			notFull_.notify_all();
		}
		//当前线程执行这个任务
		if (task != nullptr)
		{

			//	task->run();
			task();

		}
		idleThreadSize_++;

		lastTime = std::chrono::high_resolution_clock().now();//更新线程执行完任务的时间	
	}
}

//检查pool的运行状态
bool ThreadPool::checkRunningState()const {
	return isPoolRunning_;
}
