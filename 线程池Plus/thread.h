#pragma once
#include<thread>
#include<functional>
//线程池支持的模式
enum class PoolMode
{
	MODE_FIXED,		//固定数量的线程
	MODE_CACHED,	//线程数量可动态增长
};

//线程类型
class Thread
{
public:

	//线程函数对象类型
	using ThreadFunc = std::function<void(int)>;

	Thread(ThreadFunc func);

	~Thread();

	void start();

	//获取线程id
	int getId()const;

private:
	ThreadFunc func_;
	static int generateId_;
	int threadId_;	//保存线程id
};

