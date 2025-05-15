#include"thread.h"

Thread::Thread(ThreadFunc func)
	:func_(func)
	, threadId_(generateId_++)
{}

Thread::~Thread(){}

void::Thread::start(){
	//����һ���̲߳�ִ��һ���̺߳���
	std::thread t(func_, threadId_);
	t.detach();//���÷����߳�
}
int Thread::getId()const {
	return threadId_;
}

int Thread::generateId_ = 0;