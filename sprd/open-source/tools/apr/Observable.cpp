

#include <vector>
#include "Observer.h"
#include "Observable.h"

Observable::Observable()
{
	// initialize
	changed = false;
}

Observable::~Observable()
{
}

void Observable::addObserver(Observer *o)
{
	observer.push_back(o);
	return;
}


void Observable::deleteObserver(Observer *o)
{
	for (vector<Observer*>::iterator iter=observer.begin(); iter!=observer.end(); iter++)
	{
		if (o == *iter)
		{
			observer.erase(iter);
			return;
		}
	}
}

void Observable::notifyObservers()
{
	if (changed)
	{
		for (vector<Observer*>::iterator iter=observer.begin(); iter!=observer.end(); iter++)
		{
			(*iter)->handleEvent();
		}
		changed = false;
	}
}

void Observable::notifyObservers(void *arg)
{
	if (changed)
	{
		for (vector<Observer*>::iterator iter=observer.begin(); iter!=observer.end(); iter++)
		{
			(*iter)->handleEvent(arg);
		}
		changed = false;
	}
}

void Observable::setChanged()
{
	changed = true;
}


