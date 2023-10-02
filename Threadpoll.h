#pragma once
#include <iostream>
#include <Thread>
#include <mutex>
#include <functional>
#include <queue>
#include <vector>
#include <utility>
#include <string>
#include <string.h>
#include <windows.h>
#include <List>

template <class T>
class SafeQueue
{
private:
	std::queue<T> m_queue;
	std::mutex m_mutex;
public:
	SafeQueue() {}
	~SafeQueue() {}

	bool empty()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_queue.empty();
	}
	int size()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_queue.size();
	}

	void enqueue(T& t)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_queue.empty(t);
	}

	bool dequeue(T& t)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		if (m_queue.empty())
		{
			return false;
		}
		t = m_queue.front();
		m_queue.pop();
		return true;
	}
	void inqueue(T& t)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_queue.push(t);
	}
};

class ThreadPool
{
private:
	bool m_shutdown;
	std::mutex poolmutex;
	SafeQueue<std::function<void()>> m_queue;
	std::condition_variable condition_lock;
	std::vector<std::thread*> m_thread;
	std::thread *Manger_Thread;
	//工作线程类
	
	class ThreadWorks {
	private:
		int id;
		ThreadPool* pool;
	public:
		ThreadWorks(ThreadPool* pool,const int id) :pool(pool),id(id){}

		void operator()()
		{
			std::function<void()> fun;
			bool dequeue;

			while (!pool->m_shutdown)
			{
				{
					std::unique_lock<std::mutex> poollock(pool->poolmutex);
					if (pool->m_queue.empty())
					{
						std::cout << "线程:" << std::this_thread::get_id() << "阻塞" << std::endl;
						pool->condition_lock.wait(poollock);
					}
					dequeue = pool->m_queue.dequeue(fun);
					if (pool->m_shutdown||pool->exit_tread!=0)
					{
						std::cout << "线程寄了:" << pool->m_thread[id]->get_id() << std::endl;
						pool->exit_tread--;
						return;
					}
				}
				if (dequeue)
				{
					std::cout << "这个线程函数开始执行:" << std::this_thread::get_id()<< std::endl;
					fun();

				}
				//std::cout << "这个线程函数执行完毕:" << std::this_thread::get_id << std::endl;
			}
		}

	};

	
	class Threadmanger {
	private:
		ThreadPool* poll;
	public:
		Threadmanger(ThreadPool *poll):poll(poll) {	};
		void operator()()
		{
			while (1)
			{
				std::cout << "这是管理者线程:" << std::this_thread::get_id()<<std::endl;
				Sleep(3000);
				if (!poll->m_shutdown) {

					std::unique_lock<std::mutex> polllock(poll->poolmutex);
					if (poll->m_queue.size() > poll->m_thread.size())
					{
						for (int i = 0; i < 2; i++)
						{
							std::cout << "开始增加线程了" << std::endl;
							std::thread *t = new std::thread(ThreadWorks(poll, poll->m_thread.size()));
							poll->m_thread.push_back(t);
						}
					}
					if (poll->m_queue.size() * 2 < poll->m_thread.size())
					{
						for (int i = 0; i < 2; i++)
						{
							std::cout << "开始收窄线程了" << std::endl;
							poll->exit_tread++;
							poll->condition_lock.notify_one();
						}
					}
				}
				else
				{
					std::cout << "管理者线程关闭" << std::endl;
					break;
				}

			}
			return;
		}

	};
public:

	int exit_tread{0};
	ThreadPool(const int n_thread) :m_thread(std::vector<std::thread*>(n_thread)), m_shutdown(false) {
		if (!m_shutdown)
		{
			Manger_Thread = new std::thread(Threadmanger(this));
			for (int i = 0; i < m_thread.size(); i++)
			{
				m_thread[i] = new std::thread(ThreadWorks(this, i));
			}
		}
	}

	~ThreadPool() {
		this->m_shutdown = true;
		std::cout << "*****************" << "唤醒所有线程" << std::endl;
		this->condition_lock.notify_all();
		Manger_Thread->join();
		for (int i = 0; i < m_thread.size(); i++)
		{
			m_thread[i]->join();
			
		}
		delete Manger_Thread;
		Manger_Thread = nullptr;
		for (int i = 0; i < m_thread.size(); i++)
		{
			delete m_thread[i];
			m_thread[i] = nullptr;
		}
	};

	void submit(std::function<void()> f)
	{
		m_queue.inqueue(f);
		std::unique_lock<std::mutex> lock(poolmutex);
		condition_lock.notify_one();
	}
};