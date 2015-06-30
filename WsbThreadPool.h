#pragma once

#include <windows.h>
#include <memory>
#include <functional>
using std::shared_ptr;
using std::function;

#ifdef DLL_IMPLEMENT_POOL
#define DLL_API_POOL __declspec(dllexport)
#else
#define DLL_API_POOL __declspec(dllimport)
#endif

/********************************************************
****     一个轻量级的线程池实现
****     设计语言：C++
****     设计平台：win32
****     设 计 者：王慎波
****     时    间：20150628
**********************************************************/

namespace wsb
{
	typedef function< void (PVOID) > TaskFunction;//任务运行函数
	class CRealThread;
	
	//线程优先级,两档：普通和高
	enum ThreadPriority
	{
		Normal,
		High
	};

	//工作类接口，基于接口编程，方便维护
	class DLL_API_POOL CJob
	{
	public:
		virtual ~CJob();
		virtual void ExecuteTask()=0;          //执行工作
		virtual void WaitJobFinish() = 0;      //等待工作结束
		virtual void ResetJob() = 0;           //重置工作对象
		virtual ThreadPriority GetJobPri() = 0;//获得作业优先级
		static shared_ptr<CJob> CreateJob(TaskFunction run, ThreadPriority pri=ThreadPriority::Normal, PVOID pParam = NULL);//采用工厂方法创建一个工作对象
	};

	//线程池接口，基于接口编程，方便维护
	class DLL_API_POOL CWsbThreadPool
	{
	public:
		virtual ~CWsbThreadPool();
		virtual void SwitchActiveThread(CRealThread* pThread)=0;//线程完成Job后的转换函数,决定是将其放入空闲栈还是取下一个工作继续执行
		virtual bool SubmitJob(shared_ptr<CJob>& job)=0;        //提交一个工作
		virtual bool CloseThreadPool() = 0;                     //关闭线程池
		static shared_ptr<CWsbThreadPool> CreateThreadPool(size_t minThreadNum, size_t maxThreadNum);//采用工厂方法创建一个线程池对象
	};
}


