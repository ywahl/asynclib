/*
 * asynclib.h
 *
 *  Created on: Aug 4, 2013
 *      Author: awahl
 */

#ifndef ASYNCLIB_H_
#define ASYNCLIB_H_

#include <list>
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <string.h>


#ifndef _WIN32
#define nullptr NULL
#endif

template<class T, class D> class AsyncManager;
template<class T, class D> class AsyncObject;
template<class T, class D> class SyncObject;
template<class T, class D> class AsyncFunction;


template<class T, class D>
struct Pair {
		AsyncObject<T,D> *obj;
		std::vector<T> *events;
		bool completedeted;
		int event;
};

//typedef int (*AsyncCallback)(AsyncObject *async, bool completion, long evt);
//typedef int (*AsyncUnexpectedCallback)(AsyncObject *async, AsyncFunction *f,  long evt);
//typedef int (*SyncCallback)(std::vector<Pair> *pairs);


template<class T, class D>
class AsyncFunction {
	const char *name;
	std::vector<T> events;
	int (*functionCallback)(AsyncObject<T, D> *, bool , T );
	SyncObject<T, D> *syncObject;

public:
	enum {
		ok = 0,
		error = -1,
		noEventMacth = -2
	};
	AsyncFunction(const char *n, int (*func)(AsyncObject<T, D> *, bool , T ),  SyncObject<T, D> *syncObj = NULL) : name(n),
		functionCallback(func),	syncObject(syncObj) {}
	AsyncFunction(const char *n, int (*func)(AsyncObject<T, D> *, bool , T ),  T event) : name(n),
		functionCallback(func),	syncObject(NULL) {
		events.push_back(event);
	}
	virtual ~AsyncFunction() {};
	const char *getName() { return name;}
	void addCompletionEvent(long event) { events.push_back(event);}
	void addCompletionEvents(std::vector<T> *evnts) { events = *evnts;}
	virtual int processEvent(AsyncObject<T, D> *obj, bool completed, T evt);
	bool isSyncFunction() {return (events.size() == 0);}
	void registerSyncObject(AsyncObject<T, D> *obj);
};

template<class T, class D>
class FunctionList {
	std::list<AsyncFunction<T, D> *> functionList;
	const char *name;
public:
	FunctionList(const char *nm) : name(nm) {}
	void add(AsyncFunction<T, D> *f);
	void add(FunctionList<T, D> *aList, AsyncObject<T, D> *obj = NULL);
	int size() { return functionList.size();}
	AsyncFunction<T, D> *get(bool pop);
	int remove(const char *);
	FunctionList<T, D>& operator<<(AsyncFunction<T, D> *f);
	FunctionList<T, D>& operator<<(FunctionList<T, D> *functionList);
};

template<class T, class D>
class AsyncObject {
public:
	enum State {
		idle,
		busy
	};
protected:
	D dev;
	char *name;
	AsyncManager<T, D> *mngr;
	FunctionList<T,D> aList;
	std::map<const char *, void *> properties;
	int processCompletion(T evttype);
	State state;
	static int remoteLaunch(AsyncObject<T,D> *obj, bool completed, T evt);
	static AsyncFunction<T, D> remoteLaunchFunction;
	int (*processUnexpected)(AsyncObject<T, D> *async, AsyncFunction<T,D> *f,  T);
	static int defaultUnexpected(AsyncObject<T, D> *obj, AsyncFunction<T, D> *f, T evttype);
public:
	AsyncObject(D d, char *name, AsyncManager<T, D> *m);
	void registerUnexpectedFunction(int (*func)(AsyncObject<T, D> *async, AsyncFunction<T,D> *f,  T) = nullptr)
	{
		if (func != nullptr)
			processUnexpected = func;
		else
			processUnexpected = defaultUnexpected;
	}
	void addFunction(AsyncFunction<T, D> *);
	void addRemoteFunction(AsyncObject<T, D> *, AsyncFunction<T, D> *);
	void removeFunction(char *name);
	void clearFunctions();
	int launchFunctions();

	D getDev() { return dev;}
	char *getName() { return name;}

	int processEvent(T evttype);
	void addProperty(const char *key, void *ptr) { properties[key] = ptr;}
	void *getProperty(const char *key);
	State getState() {return state;}
	AsyncFunction<T, D> *getCurrentFunction() {return aList.get(false);}
	AsyncObject<T, D>& operator<<(AsyncFunction<T, D> *f);
	AsyncObject<T, D>& operator<<(FunctionList<T, D> *functionList);
};


template<class T, class D>
class SyncObject {
	char *name;
	int (*callback)(std::vector<Pair<T, D> > *pairs);
	std::vector<Pair<T,D> > pairs;
public:
	SyncObject(int (*cb)(std::vector<Pair<T, D> > *pairs)) : callback(cb) {}
	char *getName() { return name;}
	void addSync(AsyncObject<T, D> *, std::vector<T> *);

	int processEvent(AsyncObject<T,D>  *async, T);
};

template<class T, class D>
class AsyncManager {
	std::map<D, AsyncObject<T, D> *> objectMap;
public:
	AsyncManager();
	int processEvent(D, T);
	AsyncObject<T, D>* getAsyncObject(D);
	void addAsyncObject(AsyncObject<T, D> *);
};


#endif /* ASYNCLIB_H_ */
