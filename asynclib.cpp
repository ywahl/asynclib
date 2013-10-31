/*
 * asynclib.cpp
 *
 *  Created on: Aug 4, 2013
 *      Author: awahl
 */

#include "asynclib.h"



template<class T, class D>
AsyncFunction<T, D> AsyncObject<T, D>::remoteLaunchFunction("remoteLaunch", AsyncObject::remoteLaunch);

template<class T, class D>
void FunctionList<T, D>::add(AsyncFunction<T, D> *f)
{
	functionList.push_back(f);
}

template<class T, class D>
AsyncFunction<T, D> *FunctionList<T, D>::get(bool pop)
{
	AsyncFunction<T, D> *f;

	if (functionList.size())
	{
		f = functionList.front();
		if (pop)
			functionList.pop_front();
		return f;
	}
	return NULL;
}

template<class T, class D>
int FunctionList<T, D>::remove(const char *fname)
{
	typename std::list<AsyncFunction<T, D> *>::iterator iter;
	for (iter = functionList.begin(); iter != functionList.end(); iter++)
	{
		if (strcmp(fname, (*iter)->getName()) ==0)
		{
			functionList.erase(iter);
			return AsyncFunction<T, D>::ok;
		}
	}
	return AsyncFunction<T, D>::error;
}

template<class T, class D>
FunctionList<T, D> & FunctionList<T, D>::operator<<(AsyncFunction<T, D> *f)
{
	add(f);
	return *this;
}

template<class T, class D>
void FunctionList<T, D>::add(FunctionList<T, D> *aList, AsyncObject<T, D> *obj)
{
	typename std::list< AsyncFunction<T, D> * >::iterator iter;
	for (iter = functionList.begin(); iter != functionList.end(); iter++)
	{
		add(*iter);
		if (obj != nullptr)
			(*iter)->registerSyncObject(obj);
	}
}

template<class T, class D>
FunctionList<T, D> & FunctionList<T, D>::operator<<(FunctionList<T, D> *aList)
{
	add(aList, nullptr);
	return *this;
}

template<class T, class D>
int AsyncFunction<T, D>::processEvent(AsyncObject<T, D> *obj, bool completed, T evttype)
{
	if (completed)
	{
		int len = events.size();
		for (int i = 0; i < len; i++) {
			if (evttype == events[i]) {
				if (functionCallback == nullptr)
					return AsyncFunction<T, D>::ok;
				int ret = functionCallback(obj, completed, evttype);
				return ret;
			}
		}
		return AsyncFunction<T, D>::noEventMacth;
	}
	if (functionCallback)
		return functionCallback(obj, completed, evttype);
	return AsyncFunction<T, D>::ok;
}


template<class T, class D>
AsyncObject<T, D>::AsyncObject(D d, const char *nm, AsyncManager<T, D> *m) : dev(d), name(nm) , mngr(m),
		aList("mainFunctionList"), processUnexpected(nullptr)
{
	mngr->addAsyncObject(this);
	state = idle;
}

template<class T, class D>
void AsyncObject<T, D>::addFunction(AsyncFunction<T, D> *function)
{
	aList.add(function);
	function->registerAsyncObject(this);
}

template<class T, class D>
void AsyncObject<T, D>::addFunctionList(FunctionList<T, D> *functionList)
{
	aList.add(functionList, this);
}

template<class T, class D>
int AsyncObject<T, D>::remoteLaunch(AsyncObject<T, D> *obj, bool completed, T evt)
{
	AsyncObject<T, D> *remoteObj = static_cast<AsyncObject<T, D> *>(obj->getProperty("remoteObject"));
	AsyncFunction<T, D> *remoteFunction = static_cast<AsyncFunction<T, D> *>(obj->getProperty("remoteFunction"));
	obj->properties.erase("remoteObject");
	obj->properties.erase("remoteFunction");
	remoteObj->addFunction(remoteFunction);
	//TODO: if funclist is empty
	if (remoteObj->state == idle)
		remoteObj->launchFunctions();
	return AsyncFunction<T, D>::ok;
}

template<class T, class D>
void AsyncObject<T, D>::addRemoteFunction(AsyncObject<T, D> *remoteObj, AsyncFunction<T, D> *remoteFunction)
{
	this->addProperty("remoteObject", remoteObj);
	this->addProperty("remoteFunction", remoteFunction);
	this->addFunction(&remoteLaunchFunction);
}

template<class T, class D>
int AsyncObject<T, D>::launchFunctions()
{
	int ret = AsyncFunction<T, D>::error;
	AsyncFunction<T, D> *function;

	while ((function = aList.get(false)) != NULL)
	{
		std::cerr << "Initiating function= " << function->getName() << "   device=" << name << std::endl;
		ret =  function->processEvent(this, false, 0);
		//if it is a sync function proceed with next one.
		state = busy;
		if (!function->isSyncFunction())
			return ret;
		aList.get(true);
	}
	state = idle;
	return ret;
}

template<class T, class D>
int AsyncObject<T, D>::defaultUnexpected(AsyncObject<T, D> *obj, AsyncFunction<T, D> *f, T evttype)
{
	const char *name = "nofunction";
	if (f != nullptr)
		name = f->getName();

	std::cout << "Unexpected Event: " << obj->getName() << " function " << name <<
			"  event=" << evttype << std::endl;
	return AsyncFunction<T, D>::ok;
}


template<class T, class D>
int AsyncObject<T, D>::processEvent(T evttype)
{
	int ret = processCompletion(evttype);

	if (ret)
		return ret;

	return launchFunctions();
}

template<class T, class D>
int AsyncObject<T, D>::processCompletion(T evttype)
{
	int ret;

	const char *functionName ="no function ";
	AsyncFunction<T, D> *function = aList.get(true);

	if (function == nullptr)
	{
		if (processUnexpected != nullptr)
			ret = processUnexpected(this, function, evttype);
	}
	else
	{
	  ret = function->processEvent(this, true, evttype);
	  functionName = function->getName();
	  if (ret == AsyncFunction<T, D>::noEventMacth)
	  {
		  if (processUnexpected != nullptr)
			  ret = processUnexpected(this, function, evttype);
	  }
	}

	std::cerr << "Processed  function= " << functionName << "   device=" << name << "  evt=" << std::hex << evttype << " ret=" << ret <<
	" remaining functions=" << aList.size()	<< std::endl;

	return ret;
}

template<class T, class D>
void *AsyncObject<T, D>::getProperty(const char *key)
{
	std::map<const char *, void *>::iterator it;
	it = properties.find(key);
	if (it == properties.end())
	  return nullptr;
	return it->second;
}

template<class T, class D>
AsyncObject<T, D>& AsyncObject<T, D>::operator<<(AsyncFunction<T, D> *f)
{
	addFunction(f);
	return *this;
}

template<class T, class D>
AsyncObject<T, D>& AsyncObject<T, D>::operator<<(FunctionList<T, D> *alist)
{
	addFunctionList(aList);
	return *this;
}

template<class T, class D>
int AsyncManager<T, D>::processEvent(D dev, T evttype)
{
	AsyncObject<T, D> *asyncObject = objectMap[dev];
	if (!asyncObject) {
		return AsyncFunction<T, D>::error;
	}
	return asyncObject->processEvent(evttype);
}

template<class T, class D>
void AsyncManager<T, D>::addAsyncObject(AsyncObject<T, D> *obj)
{
	objectMap[obj->getDev()] = obj;
}


template<class T, class D>
SyncFunction<T, D>::SyncFunction(SyncPoint<T, D> *parent) : AsyncFunction<T, D>(sPoint->getName(), NULL), sPoint(parent)
{
}

template<class T, class D>
int SyncFunction<T, D>::processEvent(AsyncObject<T, D> *obj, bool completed, T evt)
{
	if (completed)
	{
		//check if correct termination event]'
		if (evt != sPoint->getTermEvent())
			return AsyncFunction<T, D>::noEventMacth;
		return 0;
	}
	unsigned alreadycompleted = 0;
	bool found = false;
	Pair<T, D> *pair;

	for(unsigned i = 0; i < pairs.size(); i ++)
	{
		pair = &pairs[i];
		if (!found && pair->obj == obj &&
			!pair->completed)
		{
			pair->completed = true;
			pair->event = evt;
			found = true;
		}

		if (pair->completed)
			alreadycompleted++;
	}
	if (alreadycompleted == pairs.size())
		return sPoint->termRecv(&pairs);
	return AsyncFunction<T, D>::ok;
}

template<class T, class D>
void SyncFunction<T, D>::registerAsyncObject(AsyncObject<T, D> *obj)
{
	Pair<T, D> pair= { obj, false};
	pairs.push_back(pair);
}

template<class T, class D>
int SyncPoint<T, D>::termRecv(std::vector<Pair<T, D> > *pairs)
{
	int ret = callback(pairs);
	sendSyncReadyEvent(pairs);
	return ret;

}

template<class T, class D>
AsyncFunction<T, D> *SyncPoint<T, D>::getInstance()
{
	AsyncFunction<T, D> *func = new SyncFunction<T, D>(name, this, term);
	return func;
}

