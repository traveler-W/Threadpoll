#include "Threadpoll.h"

void fun()
{ 
	Sleep(1000);
	std::cout << "������ִ����" << std::endl;
}
int main()
{
	ThreadPool *poll = new ThreadPool(2);
	for (int i = 0; i < 30; i++)
	{
		poll->submit(fun);
	}
	delete poll;
	return 0;

}