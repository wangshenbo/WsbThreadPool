#pragma once

#include "WsbThreadPool.h"
#include "stack"
#include "list"
#include "queue"
using std::list;
using std::stack;
using std::queue;

namespace wsb
{
	//Job类，继承接口CJob
	class CRealJob :public CJob
	{
	public:
		CRealJob(TaskFunction run,ThreadPriority pri,PVOID pParam);
		~CRealJob();
		void ExecuteTask();//执行工作
		void WaitJobFinish();//等待工作结束
		void ResetJob();//重置工作
		ThreadPriority GetJobPri();//获取作业优先级
	private:
		//禁止拷贝与赋值
		CRealJob(const CRealJob&);
		CRealJob& operator=(const CRealJob&);
	private:
		TaskFunction m_TaskFun;//任务执行体
		PVOID m_Param;//任务执行参数
		HANDLE m_hFinishEvent;//结束事件, 工作结束触发
		ThreadPriority m_Pri;//作业优先级
	};

	//线程类
	class CRealThread
	{
	public:
		CRealThread(CWsbThreadPool* pool);
		~CRealThread();
		bool suspendThread();//挂起线程
		bool resumeThread();//恢复线程
		bool AssignJob(shared_ptr<CJob>& job);//分配作业
		bool RunJob();//运行作业
		void notifyStartJob();//通知线程工始工作
		void notifyThreadQuit();//将线程退出事件设置成有信号状态
	public:
		static unsigned int WINAPI threadFun(PVOID pParam);//线程函数
	private:
		HANDLE m_hThread;//线程句柄
		unsigned int m_ThreadID;//线程ID
		bool m_bisExit;//用于标记线程是否退出
		HANDLE m_hWaitEvent;//等待事件，用于线程挂起
		HANDLE m_hQuitEvent;//用于线程退出事件
		shared_ptr<CJob> m_Job;//线程工作
		CWsbThreadPool* m_ThreadPoool;//所属线程池,便于线程回收
	};

	//互斥量
	class CMutex
	{
	public:
		CMutex();
		~CMutex();
		void waitMutex();
		void releaseMutex();
	private:
		HANDLE m_hMutex;
	private:
		CMutex(const CMutex&);
		CMutex& operator=(const CMutex&);
	};

	//锁
	class CLock
	{
	public:
		explicit CLock(CMutex&);
		~CLock();
	private:
		CMutex& m_mutex;
	private:
		CLock(const CLock&);
		CLock& operator=(const CLock&);
	};

	//空闲线程类
	class CIdleThreadStack
	{
	public:
		CIdleThreadStack();
		~CIdleThreadStack();
		size_t GetSize() const;//获取大小
		bool isEmpty() const;//是否为空
		void push(CRealThread* thread);//压入线程
		CRealThread* pop();//弹出线程
		void clear();//清空
	private:
		stack<CRealThread*> m_ThreadStack;//空闲线程栈
		CMutex m_mutex;//互斥量
	};

	//激活的线程队列
	class CActiveThreadList
	{
	public:
		CActiveThreadList();
		~CActiveThreadList();
		size_t GetSize() const;//获取大小
		bool isEmpty() const;//是否为空
		void addThread(CRealThread* thread);//添加线程
		void removeThread(CRealThread* thread);//移除线程
		void clear();//清空
	private:
		list<CRealThread*> m_ActiveThread;//活动线程链表
		CMutex m_mutex;//互斥量
	};

	//工作队列类
	class CJobQueue
	{
	public:
		CJobQueue();
		~CJobQueue();
		size_t GetSize() const;//获取大小
		bool isEmpty() const;//是否为空
		void pushJob(shared_ptr<CJob>& job);//压入Job
		shared_ptr<CJob> popJop();//弹出Job
		void clear();//清空
	private:
		queue<shared_ptr<CJob>> m_JobQueue;// 工作队列
		CMutex m_mutex;//互斥量
	};

	//具体线程池类
	class CRealThreadPool :public CWsbThreadPool
	{
	public:
		CRealThreadPool(size_t minNum,size_t maxNum);
		~CRealThreadPool();
		void SwitchActiveThread(CRealThread* pThread);//线程完成Job后的转换函数
		bool SubmitJob(shared_ptr<CJob>& job);//提交一个工作
		bool CloseThreadPool();//关闭线程池
	private:
		CRealThreadPool(const CRealThreadPool&);
		CRealThreadPool& operator=(const CRealThreadPool&);
		void IncreaseCapacity();//增加线程数,增加为当前线程数的两倍或最大线程数
		void DecreaseCapacity();//减少空闲线程数
		static unsigned int WINAPI CheckIdleThread(PVOID pParam);//检测空闲线程数是否发生较大变化，若是，则不对空闲线程进行减容处理
	private:
		CActiveThreadList m_ActiveThread;//活动线程队列
		CIdleThreadStack m_IdleThread;//空闲线程栈
		CJobQueue m_NormalJob;;//正常优先级job队列
		CJobQueue m_HighJob;//高优先级job队列
		const size_t m_minThreadNum;//最小的线程数
		const size_t m_maxThreadNum;//最多的线程数
		size_t m_ThreadNum;//实际的线程数量
		HANDLE m_hQuitEvent;//空闲线程数侦测线程退出信号
	};
}
