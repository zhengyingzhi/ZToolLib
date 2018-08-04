# ZToolLib
根据工作中经常需要用到的，基于C语言封装的一些常用工具类库，均不依赖于第三方库，支持 Windows 和 Linux 平台，便于在工程项目中参考引用。很多实现都来自于开源项目如nginx, redis和其它网络博客上的开源实现。

文件结构说明：
-ZToolLib 各种工具库的基本实现
    lockfreequeue   一个可跨进程的、基于循环数组实现的无锁队列，支持MPMC(multiple producers multiple consumers)
    ztl_aes         开源的aes加密算法实现，只是以C语言封装
    ztl_array       一个可动态扩展的C数组
    ztl_atomic      Windows和Linux上的原子操作接口，为
    ztl_base64      Base64编码/解码
    ztl_bitset      位操作相关封装，可一次操作一块内存的某位置的bit
    ztl_buffer      可方便对一块buffer相关添加删除，自动扩展等操作
    ztl_common      一些公共定义
    ztl_config      配置文件读取
    ztl_crypt       程序加密接口，内部以aes加密，并以base64编码得到最终数据，解密则相反
    ztl_dict        从redis过来的C语言的字典库，基于siphash的key，支持动态扩展
    ztl_dyso        对Windows和Linux的动态库dll/so的加载、卸载的封装
    ztl_event_timer 用于ZToolLib的IO框架中的定时事件的掊
    ztl_evloop      IO框架的对外统一接口，创建、事件添加、事件删除等
    ztl_fixapi      fix协议的基本打包、解包的实现
    ztl_hash        常用的hash算法
    ztl_linklist    从nginx来的双向链表实现
    ztl_locks       基于原子操作的自旋锁/读写锁实现
    ztl_logger      自己封装的简单日志库，支持同步/异步/网络日志
    ztl_malloc      系统/开源的malloc的统一封装
    ztl_map         基于红黑树实现的map，类似于C++中的std::map
    ztl_mempool     一个高效的内存对象分配池，可快速分配固定大小的对象
    ztl_msg_buffer  一条消息的简单封装
    ztl_network     封装了Windows和Linux平台的操作系统的网络操作接口差异，便于快速开发基于socket相关的程序，而不用担心OS的细节差异
    ztl_palloc      从nginx来的小块内存分配池，没有free接口
    ztl_producer_consumer   一个生产者消费者模型，只需要定义相应的业务处理的回调函数即可 
    ztl_protocol    ZToolLib的网络协议头
    ztl_rbtree      红黑树实现
    ztl_shm         对不同操作系统平台的共享内存操作的封装，创建和销毁，可创建匿名共享内存或文件映射共享内存等
    ztl_simple_event        用于跨线程/进程的的事件同步操作
    ztl_tcp_server  一个简单的tcpserver，但包括连接的建立，可读/可写事件，消息收发回调等完整逻辑
    ztl_threadpool  一个易用的线程池接口
    ztl_threads     对不同操作系统平台的线程创建的相关封装
    ztl_times       丰富的时间处理的相关接口
    ztl_unit_test   自用的简单单元测试框架
    ztl_win32_ipc   Windows下的共享内存文件的相关依赖
    ztl_win32_stacktrace    可帮助程序崩溃时自动打印堆栈信息
-ZToolLib.Test 对ZToolLib库的一些快速调试的工程

-ZToolLib.UnitTest 一些对ZToolLib的单元测试用例


