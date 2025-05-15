#pragma once
#include<thread>
#include<functional>
//�̳߳�֧�ֵ�ģʽ
enum class PoolMode
{
	MODE_FIXED,		//�̶��������߳�
	MODE_CACHED,	//�߳������ɶ�̬����
};

//�߳�����
class Thread
{
public:

	//�̺߳�����������
	using ThreadFunc = std::function<void(int)>;

	Thread(ThreadFunc func);

	~Thread();

	void start();

	//��ȡ�߳�id
	int getId()const;

private:
	ThreadFunc func_;
	static int generateId_;
	int threadId_;	//�����߳�id
};

