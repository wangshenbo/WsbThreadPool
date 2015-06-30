# WsbThreadPool
一个基于windows平台C++实现的线程池

    能够实现两种优先级任务的调度，对线程进行管理，空闲线程过多时能够给线程池自动进行减容操作，同时
    线程不足时自动进行扩容操作，向线程池提交作业完后可以等待作业完成，也可以不等待。
    
    内部维护四个容器：分别是空闲线程栈、活动线程链表、高优先级作业队列、普通优先级作业队列。
    
    作业调度算法：高优先级优先与FIFO结合
    
    线程池扩容操作：当线程池内的线程数太少时，同时又低于设置的最大线程数时，而此时有大量作业提交，线程池按活动线程数
    的两倍进行扩容
    
    线程池减容操作：线程池开启的时候启动一个线程每隔15s自动检测空闲线程栈中的线程数，并记录在一个链表中，当链表中线程
    数达到20个时，计算链表中所有数的方差，然后根据方差是否小于1.5来判断空闲线程数是否变化大，若小于则说明变化不大，可
    进行减容操作，减容操作具体做法：减少到当前空闲线程数的一半
    
    整个线程池基于接口编程，对外只提供接口，与具体实现隔离，采用工厂方法去创建相应的线程池和作业
