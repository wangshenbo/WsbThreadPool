// UseDll.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "WsbThreadPool.h"
#include "iostream"

void JobFun(PVOID p)
{
	static int i = 0;
	HANDLE mutex = static_cast<HANDLE>(p);
	WaitForSingleObject(mutex, INFINITE);
	i++;
	std::cout << "My Job" << i << std::endl;
	ReleaseMutex(mutex);
}

int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE mutex = CreateMutex(NULL, false, L"MyJob");
	shared_ptr<wsb::CWsbThreadPool> mypool=wsb::CWsbThreadPool::CreateThreadPool(2, 12);
	shared_ptr<wsb::CJob> myjob = wsb::CJob::CreateJob(JobFun,wsb::ThreadPriority::Normal,mutex);
	for (int i = 0; i < 1500; i++)
	{
		mypool->SubmitJob(myjob);
		//myjob->WaitJobFinish();
	}
	system("pause");
	return 0;
}

