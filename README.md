This is a thread pool based on a variable parameter template implementation.

基于可变参模板实现的线程池

项目描述：

    基于可变参模板和引用折叠原理，实现线程池submitTask接口，支持任意任务函数和任意参数的传递
  
    使用future类型定制submitTask提交任务的返回值
  
    使用map和queue容器管理线程对象和任务
  
    基于条件变量condition_variable和互斥锁mutex实现任务提交线程和任务执行线程间的通信机制
  
    支持fixed和cached模式的线程池定制
