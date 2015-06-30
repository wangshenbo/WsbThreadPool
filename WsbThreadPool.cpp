#define DLL_IMPLEMENT_POOL

#include "WsbThreadPoolReal.h"
#include "process.h"

namespace wsb
{
	//工作类部分开始  修改时间：20150628
	CJob::~CJob()
	{

	}
	
	//************************************************************
	//********功能：创建一个Job对象
	//********参数：Job对象的shared_ptr指针
	//********返回值：
	//************************************************************
	shared_ptr<CJob> CJob::CreateJob(TaskFunction run, ThreadPriority pri, PVOID pParam)//创建工作类函数
	{
		return shared_ptr<CJob>(new CRealJob(run,pri,pParam));
	}

	//************************************************************
	//********功能：CRealJob类的构造函数
	//********参数：
	//********返回值：
	//************************************************************
	CRealJob::CRealJob(TaskFunction run, ThreadPriority pri, PVOID pParam) :m_TaskFun(run), m_Pri(pri), m_Param(pParam)
	{
		m_hFinishEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	//************************************************************
	//********功能：CRealJob类的析构函数
	//********参数：
	//********返回值：
	//************************************************************
	CRealJob::~CRealJob()
	{
		if (m_hFinishEvent)
		{
			CloseHandle(m_hFinishEvent);
		}
	}

	//************************************************************
	//********功能：Job对象执行具体的任务
	//********参数：
	//********返回值：
	//************************************************************
	void CRealJob::ExecuteTask()
	{
		m_TaskFun(m_Param);
		SetEvent(m_hFinishEvent);//触发事件，通知工作结束
	}

	//************************************************************
	//********功能：等待Job对象完成
	//********参数：
	//********返回值：
	//************************************************************
	void CRealJob::WaitJobFinish()
	{
		WaitForSingleObject(m_hFinishEvent, INFINITE);
	}

	//************************************************************
	//********功能：重置Job对象
	//********参数：
	//********返回值：
	//************************************************************
	void CRealJob::ResetJob()
	{
		ResetEvent(m_hFinishEvent);//重置事件，以重新开始工作
	}

	//************************************************************
	//********功能：获取Job对象的优先级
	//********参数：
	//********返回值：工作对象的优先级
	//************************************************************
	ThreadPriority CRealJob::GetJobPri()
	{
		return m_Pri;
	}
	//工作类部分结束

	//线程类部分开始   修改时间：20150628
	//************************************************************
	//********功能：线程类的构造函数，创建线程，并设置所属的线程池
	//********参数：
	//********返回值：
	//************************************************************
	CRealThread::CRealThread(CWsbThreadPool* pool) :
		m_hThread(NULL),
		m_ThreadID(0),
		m_bisExit(false),
		m_hWaitEvent(NULL),
		m_hQuitEvent(NULL),
		m_Job(NULL),
		m_ThreadPoool(pool)
	{
		m_hWaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hQuitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		m_hThread = (HANDLE)_beginthreadex(NULL, 0, threadFun, this,0, &m_ThreadID);//创建一个线程
	}

	//************************************************************
	//********功能：线程类的析构函数，退出线程
	//********参数：
	//********返回值：
	//************************************************************
	CRealThread::~CRealThread()
	{
		if (!m_bisExit&&m_hThread)
		{
			m_bisExit = true;
			notifyStartJob();
			WaitForSingleObject(m_hQuitEvent, INFINITE);
			CloseHandle(m_hThread);
			m_hThread = NULL;
			m_ThreadID = 0;
		}
		if (m_hWaitEvent)
		{
			CloseHandle(m_hWaitEvent);
		}
		if (m_hQuitEvent)
		{
			CloseHandle(m_hQuitEvent);
		}
	}

	//************************************************************
	//********功能：线程执行体
	//********参数：线程对象指针
	//********返回值：
	//************************************************************
	unsigned int WINAPI CRealThread::threadFun(PVOID pParam)//线程函数
	{
		CRealThread* pThread = static_cast<CRealThread*>(pParam);
		if (pThread == NULL)return 0;
		while (!pThread->m_bisExit)
		{
			DWORD ret = WaitForSingleObject(pThread->m_hWaitEvent, INFINITE);
			if (ret == WAIT_OBJECT_0)
			{
				pThread->RunJob();//运行工作
			}
		}
		pThread->notifyThreadQuit();
		return 0;
	}

	//************************************************************
	//********功能：挂起正在运行的线程
	//********参数：
	//********返回值：
	//************************************************************
	bool CRealThread::suspendThread()//挂起线程,空闲线程堆栈内的线程全部挂起
	{
		if (m_hThread)
		{
			SuspendThread(m_hThread);
		}
		return true;
	}

	//************************************************************
	//********功能：将挂起的线程恢复
	//********参数：
	//********返回值：成功与否
	//************************************************************
	bool CRealThread::resumeThread()//恢复线程，空闲堆栈内线程转移到运动线程链表内时调用，以恢复线程运行
	{
		if (m_hThread)
		{
			int ret = ResumeThread(m_hThread);
			if (ret > 1)return false;
			return true;
		}
		return false;
	}

	//************************************************************
	//********功能：分配给线程一个Job,并重置Job
	//********参数：Job对象的shared_ptr指针
	//********返回值：
	//************************************************************
	bool CRealThread::AssignJob(shared_ptr<CJob>& job)
	{
		m_Job = job;
		m_Job->ResetJob();//重置作业
		return true;
	}

	//************************************************************
	//********功能：运行处理Job,处理完成后将Job置空并调用线程池的转换函数
	//********参数：无
	//********返回值：
	//************************************************************
	bool CRealThread::RunJob()
	{
		if (m_Job != NULL)
		{
			m_Job->ExecuteTask();
			m_Job = NULL;
			if (m_ThreadPoool)m_ThreadPoool->SwitchActiveThread(this);//完成Job后，进行转换：或取下一个任务给线程，或将线程压入空闲栈中
		}
		return true;
	}

	//************************************************************
	//********功能：通知线程开始处理Job
	//********参数：无
	//********返回值：
	//************************************************************
	void CRealThread::notifyStartJob()//通知线程工始工作
	{
		SetEvent(m_hWaitEvent);
	}

	//************************************************************
	//********功能：通知线程已经退出函数
	//********参数：无
	//********返回值：
	//************************************************************
	void CRealThread::notifyThreadQuit()
	{
		SetEvent(m_hQuitEvent);
	}
	//线程类部分结束   
	  

	//互斥量、锁类部分开始  修改时间：20150628
	CMutex::CMutex()
	{
		m_hMutex = CreateMutex(NULL, FALSE, NULL);
	}

	CMutex::~CMutex()
	{
		if (m_hMutex)
		{
			CloseHandle(m_hMutex);
		}
	}

	void CMutex::waitMutex()
	{
		if (m_hMutex == NULL)return;
		WaitForSingleObject(m_hMutex, INFINITE);
	}
	void CMutex::releaseMutex()
	{
		if (m_hMutex == NULL)return;
		ReleaseMutex(m_hMutex);
	}

	CLock::CLock(CMutex& mutex) :m_mutex(mutex)
	{
		m_mutex.waitMutex();
	}

	CLock::~CLock()
	{
		m_mutex.releaseMutex();
	}
	//互斥量、锁类部分结束


	//空闲线程堆栈类部分开始   修改时间：20150628
	CIdleThreadStack::CIdleThreadStack()
	{

	}

	CIdleThreadStack::~CIdleThreadStack()
	{
		clear();//清空
	}

	//************************************************************
	//********功能：获取空闲线程栈的大小
	//********参数：无
	//********返回值：返回活动线程链表的size
	//************************************************************
	size_t CIdleThreadStack::GetSize() const
	{
		return m_ThreadStack.size();
	}

	//************************************************************
	//********功能：获取空闲线程栈是否为空
	//********参数：无
	//********返回值：true or false
	//************************************************************
	bool CIdleThreadStack::isEmpty() const
	{
		return m_ThreadStack.empty();
	}

	//************************************************************
	//********功能：向空闲线程栈中添加一个线程对象指针
	//********参数：线程对象指针
	//********返回值：
	//************************************************************
	void CIdleThreadStack::push(CRealThread* thread)
	{
		if (thread != NULL)
		{
			CLock myLock(m_mutex);
			m_ThreadStack.push(thread);
		}
	}

	//************************************************************
	//********功能：弹出空闲线程栈中的一个线程对象指针
	//********参数：
	//********返回值：线程对象指针
	//************************************************************
	CRealThread* CIdleThreadStack::pop()
	{
		CLock myLock(m_mutex);
		if (m_ThreadStack.empty())return NULL;
		CRealThread* res = m_ThreadStack.top();
		m_ThreadStack.pop();
		return res;
	}

	//************************************************************
	//********功能：清空一个空闲线程栈
	//********参数：无
	//********返回值：
	//************************************************************
	void CIdleThreadStack::clear()
	{
		CLock myLock(m_mutex);
		while (!m_ThreadStack.empty())
		{
			CRealThread* res = m_ThreadStack.top();
			m_ThreadStack.pop();
			delete res;
		}
	}
	//空闲线程堆栈类部分结束


	//激活的线程队列部分开始  修改时间：20150628
	CActiveThreadList::CActiveThreadList()
	{

	}

	CActiveThreadList::~CActiveThreadList()
	{
		//while (!m_ActiveThread.empty())
		//{
		//	//*******************可能有问题！！！！！！！！！！！！
		//	CRealThread* p = m_ActiveThread.front();
		//	m_ActiveThread.pop_front();
		//	delete p;
		//}
		clear();
	}

	//************************************************************
	//********功能：获取活动线程链表的大小
	//********参数：无
	//********返回值：返回活动线程链表的size
	//************************************************************
	size_t CActiveThreadList::GetSize() const
	{
		return m_ActiveThread.size();
	}

	//************************************************************
	//********功能：获取活动线程链表是否为空
	//********参数：无
	//********返回值：true or false
	//************************************************************
	bool CActiveThreadList::isEmpty() const
	{
		return m_ActiveThread.empty();
	}

	//************************************************************
	//********功能：向活动线程链表中添加一个线程对象指针
	//********参数：线程对象指针
	//********返回值：
	//************************************************************
	void CActiveThreadList::addThread(CRealThread* thread)
	{
		if (thread)
		{
			CLock myLock(m_mutex);
			m_ActiveThread.push_back(thread);
		}
	}

	//************************************************************
	//********功能：移除一个线程对象指针
	//********参数：无
	//********返回值：
	//************************************************************
	void CActiveThreadList::removeThread(CRealThread* thread)
	{
		if (thread)
		{
			CLock myLock(m_mutex);
			m_ActiveThread.remove(thread);
		}
	}

	//************************************************************
	//********功能：清空一个活动线程链表
	//********参数：无
	//********返回值：
	//************************************************************
	void CActiveThreadList::clear()
	{
		CLock myLock(m_mutex);
		while (!m_ActiveThread.empty())
		{
			CRealThread* p = m_ActiveThread.front();
			m_ActiveThread.pop_front();
			delete p;
		}
	}
	//激活的线程队列部分结束
	

	//工作队列类部分开始   修改时间：20150628
	CJobQueue::CJobQueue()
	{

	}

	CJobQueue::~CJobQueue()
	{
		clear();
	}

	//************************************************************
	//********功能：获取工作队列的大小
	//********参数：无
	//********返回值：返回工作队列的size
	//************************************************************
	size_t CJobQueue::GetSize() const
	{
		return m_JobQueue.size();
	}

	//************************************************************
	//********功能：获取工作队列是否为空
	//********参数：无
	//********返回值：true or false
	//************************************************************
	bool CJobQueue::isEmpty() const
	{
		return m_JobQueue.empty();
	}

	//************************************************************
	//********功能：向工作队列中压入一个工作对象
	//********参数：工作对象的shared_ptr指针
	//********返回值：
	//************************************************************
	void CJobQueue::pushJob(shared_ptr<CJob>& job)
	{
		if (job != NULL)
		{
			CLock mLock(m_mutex);
			m_JobQueue.push(job);
		}
	}

	//************************************************************
	//********功能：pop一个工作对象
	//********参数：无
	//********返回值：工作对象的shared_ptr指针
	//************************************************************
	shared_ptr<CJob> CJobQueue::popJop()
	{
		CLock mLock(m_mutex);
		if (m_JobQueue.empty())return NULL;
		shared_ptr<CJob> job = m_JobQueue.front();
		m_JobQueue.pop();
		return job;
	}

	//************************************************************
	//********功能：清空一个工作队列
	//********参数：无
	//********返回值：
	//************************************************************
	void CJobQueue::clear()
	{
		CLock mLock(m_mutex);
		while (!m_JobQueue.empty())
			m_JobQueue.pop();
	}
	//工作队列类部分结束
	

	//线程池部分开始 修改时间：20150628
	CWsbThreadPool::~CWsbThreadPool()
	{
	}

	//************************************************************
	//********功能：创建线程池对象
	//********参数：线程池内线程数量的最小值与最大值
	//********返回值：线程池shared_ptr指针，利用RAII管理对象
	//************************************************************
	shared_ptr<CWsbThreadPool> CWsbThreadPool::CreateThreadPool(size_t minThreadNum, size_t maxThreadNum)
	{
		return shared_ptr<CWsbThreadPool>(new CRealThreadPool(minThreadNum, maxThreadNum));
	}

	//************************************************************
	//********功能：具体线程池类构造函数，创建最大值与最小值均值个的线程
	//********参数：线程池内线程数量的最小值与最大值
	//********返回值：
	//************************************************************
	CRealThreadPool::CRealThreadPool(size_t minNum, size_t maxNum) :m_minThreadNum(minNum), m_maxThreadNum(maxNum), m_ThreadNum((minNum + maxNum) / 2), m_hQuitEvent(NULL)
	{
		for (size_t i = 0; i < m_ThreadNum; i++)//创建最大值与最小值均值个的线程
		{
			m_IdleThread.push(new CRealThread(this));
		}
		m_hQuitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, CheckIdleThread, this, 0, NULL);//创建空闲线程栈检测线程，每隔15s检测一次
		CloseHandle(hThread);
	}

	//************************************************************
	//********功能：具体线程池类析构函数
	//********参数：
	//********返回值：
	//************************************************************
	CRealThreadPool::~CRealThreadPool()
	{
		if (m_hQuitEvent)
		{
			SetEvent(m_hQuitEvent);//退出检测线程
			CloseHandle(m_hQuitEvent);
		}
		CloseThreadPool();//关闭线程池
	}

	//************************************************************
	//********功能：空闲线程栈检测线程具体实现
	//********做法：每隔15s收集当前空闲线程栈中线程数量，收集满20个后计算方差，若方差小于1.5，则说明空闲栈空闲线程数基本维持不变，可进行减容处理
	//********参数：线程池指针
	//********返回值：
	//************************************************************
	unsigned int WINAPI CRealThreadPool::CheckIdleThread(PVOID pParam)
	{
		CRealThreadPool* pool = static_cast<CRealThreadPool*>(pParam);
		if (pool == NULL)return 0;
		list<int> vThrNum;//空闲线程数队列
		list<int>::const_iterator iter;
		double flag1 = 0.0;//方差
		double flag2 = 0.0;//均值
		while (WaitForSingleObject(pool->m_hQuitEvent, 15000) == WAIT_TIMEOUT)
		{
			if (vThrNum.size() < 20)
			{
				vThrNum.push_back(pool->m_IdleThread.GetSize());
			}
			else
			{
				vThrNum.pop_front();
				vThrNum.push_back(pool->m_IdleThread.GetSize());
				//计算均值与方差
				for (iter = vThrNum.begin(); iter != vThrNum.end(); iter++)
				{
					flag2 += (*iter);
				}
				flag2 /= vThrNum.size();
				for (iter = vThrNum.begin(); iter != vThrNum.end(); iter++)
				{
					flag1 += ((*iter) - flag2)*((*iter) - flag2);
				}
				flag1 /= vThrNum.size();
				flag1 = sqrt(flag1);
				//若方差小于1.5，则进行减容处理
				if (flag1 < 1.5)
				{
					pool->DecreaseCapacity();//减容
					vThrNum.clear();//清空记录
				}
				flag1 = 0.0;
				flag2 = 0.0;
			}
		}
		return 0;
	}

	//************************************************************
	//********功能：线程完成Job后的转换函数
	//********做法：每个线程对象会带有一个线程池对象指针，以表示该线程所归属的线程池，当线程完成Job后，线程池对象要对这个线程进行转换，以决定是放入空闲线程栈还是继续处理下一个Job
	//********参数：线程对象指针
	//********返回值：无
	//************************************************************
	void CRealThreadPool::SwitchActiveThread(CRealThread* pThread)
	{
		if (m_NormalJob.isEmpty()&&m_HighJob.isEmpty())//若高优先级与普通优先级队列都没有工作对象，则将该线程从活动线程链表转移到空闲线程栈中，否则就分配工作给该线程
		{
			m_ActiveThread.removeThread(pThread);//从活动链表中移除
			m_IdleThread.push(pThread);//压入空闲线程栈
		}
		else
		{
			shared_ptr<CJob> job = NULL;
			//取工作
			if (!m_HighJob.isEmpty())
			{
				job = m_HighJob.popJop();
			}
			else
			{
				job = m_NormalJob.popJop();
			}
			//分配工作
			if (job != NULL)
			{
				pThread->AssignJob(job);
				pThread->notifyStartJob();
			}
		}
	}

	//************************************************************
	//********功能：向线程池提交工作对象
	//********做法：若线程池中有空闲线程则立即给该工作给配线程并通知处理，否则进行线程池扩容或将工作根据优先级压入相应的工作队列： 高/普通工作队列
	//********参数：工作对象shared_ptr指针
	//********返回值：无
	//************************************************************
	bool CRealThreadPool::SubmitJob(shared_ptr<CJob>& job)//提交一个工作
	{
		if (m_IdleThread.isEmpty())//空闲线程栈为空
		{
			if (m_ThreadNum >= m_maxThreadNum)//当前线程数大于等于最大线程数，将工作压入工作队列
			{
				if (job->GetJobPri() == ThreadPriority::Normal)
					m_NormalJob.pushJob(job);//压入普通工作队列
				else
					m_HighJob.pushJob(job);//压入高优先级工作队列
				return true;
			}
			else
			{
				IncreaseCapacity();//扩容
			}
		}
		CRealThread* pThread=m_IdleThread.pop();//从空闲栈取出
		if (pThread == NULL)return false;

		m_ActiveThread.addThread(pThread);//放入活动链表
		pThread->AssignJob(job);//分配任务
		pThread->notifyStartJob();//开始做任务

		return true;
	}

	//************************************************************
	//********功能：关闭线程池
	//********做法：清空活动线程队列、清空空闲线程栈，清空工作队列
	//********参数：
	//********返回值：
	//************************************************************
	bool CRealThreadPool::CloseThreadPool()//关闭线程池
	{
		m_ThreadNum -= m_IdleThread.GetSize();
		m_IdleThread.clear();//清空空闲线程栈
		m_ThreadNum -= m_ActiveThread.GetSize();
		m_ActiveThread.clear();//清空活动线程链表
		m_NormalJob.clear();//清空普通优先级工作队列
		m_HighJob.clear();//清空高优先级工作队列
		return true;
	}

	//************************************************************
	//********功能：线程池扩容函数，增加当前线程池中线程数量
	//********做法：增加到当前活动线程数的两倍或线程数最大值
	//********参数：
	//********返回值：
	//************************************************************
	void CRealThreadPool::IncreaseCapacity()
	{
		size_t nums = min(m_maxThreadNum, 2 * m_ThreadNum);
		while (m_ThreadNum < nums)
		{
			m_IdleThread.push(new CRealThread(this));
			m_ThreadNum++;
		}
	}

	//************************************************************
	//********功能：线程池减容函数，减少当前线程池中线程数量
	//********做法：减少当前空闲线程数的一半，且满足最后空闲线程数量要不少于1个，总线程数不少于线程数最小值
	//********参数：
	//********返回值：
	//************************************************************
	void CRealThreadPool::DecreaseCapacity()
	{
		size_t nums = m_IdleThread.GetSize() / 2;
		while (m_IdleThread.GetSize()>1&&m_ThreadNum>m_minThreadNum&&nums > 0)
		{
			CRealThread* p = m_IdleThread.pop();
			delete p;
			m_ThreadNum--;
			nums--;
		}
	}
	//线程池部分结束
}

