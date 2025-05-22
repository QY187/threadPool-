#include<iostream>
#include"threadpool.h"
#include<chrono>
#include<thread>
#include<Windows.h>
using namespace std;

using uLong = unsigned long long;
class MyTask :public Task {
public:
	MyTask(int begin, int end)
	{
		begin_ = begin;
		end_ = end;
	}
	  Any run()
	{
		cout << "tid:" << std::this_thread::get_id()
			<< "begin!" << endl;
	//	std::this_thread::sleep_for(std::chrono::seconds(3));
		uLong sum = 0;
		for (uLong i = begin_; i <= end_; i++)
		{
			sum += i;
		}
		//std::this_thread::sleep_for(std::chrono::seconds(5));
		cout << "tid:" << std::this_thread::get_id()
			<< "end!" << endl;
		return sum;
	}
private:
	int begin_;
	int end_;
};

int main()
{
	{
		ThreadPool pool;
		pool.setMode(PoolMode::MODE_CACHED);
		pool.start(2);		
		Result res1 = pool.submitTsk(std::make_shared<MyTask>(1, 100000000));
		Result res2 = pool.submitTsk(std::make_shared<MyTask>(1, 100000000));
		pool.submitTsk(std::make_shared<MyTask>(1, 100000000));
		pool.submitTsk(std::make_shared<MyTask>(1, 100000000));
		pool.submitTsk(std::make_shared<MyTask>(1, 100000000));

		uLong sum1 = res1.get().cast_<uLong>();
		cout << sum1 << endl;
	}
	cout << "main over" << endl;
	getchar();

	/*
	{
		ThreadPool pool;
		pool.setMode(PoolMode::MODE_CACHED);

		pool.start(4);

		Result res1 = pool.submitTsk(std::make_shared<MyTask>(1, 100000000));
		Result res2 = pool.submitTsk(std::make_shared<MyTask>(100000001, 200000000));
		Result res3 = pool.submitTsk(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTsk(std::make_shared<MyTask>(200000001, 300000000));

		pool.submitTsk(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTsk(std::make_shared<MyTask>(200000001, 300000000));

		uLong sum1 = res1.get().cast_<uLong>();
		uLong sum2 = res2.get().cast_<uLong>();
		uLong sum3 = res3.get().cast_<uLong>();

		//Master-Slave线程函数
		//Master线程用来分解任务，然后给各个Salve线程分配任务
		//等待各个Slave线程执行完任务，返回结果
		//Master线程合并各个任务结果，输出



		cout << sum1 + sum2 + sum3 << endl;
	}
	long long sum = 0;
	for (int i = 1; i <= 300000000; i++)
		sum += i;


	cout << sum;	
	getchar();
	*/


	return 0;
}