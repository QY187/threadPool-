#include"thread.h"

Thread::Thread(ThreadFunc func)
	:func_(func)
	, threadId_(generateId_++)
{}

Thread::~Thread(){}

void::Thread::start(){
	//创建一个线程并执行一个线程函数
	std::thread t(func_, threadId_);
	t.detach();//设置分离线程
}
int Thread::getId()const {
	return threadId_;
}

int Thread::generateId_ = 0;