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
template<class T, class D> class AsyncFunction;


template<class T, class D>
struct Pair {
		AsyncObject<T,D> *obj;
		bool completed;
};

//typedef int (*AsyncCallback)(AsyncObject *async, bool completion, long evt);
//typedef int (*AsyncUnexpectedCallback)(AsyncObject *async, AsyncFunction *f,  long evt);
//typedef int (*SyncCallback)(std::vector<Pair> *pairs);

struct strCmp {
      bool operator()( const char* s1, const char* s2 ) const {
        return strcmp( s1, s2 ) < 0;
      }
};

template<class T, class D>
class AsyncFunction {
	const char *name;
	std::vector<T> events;
	int (*functionCallback)(AsyncObject<T, D> *, bool , T );

public:
	enum {
		ok = 0,
		error = -1,
		noEventMacth = -2
	};
	AsyncFunction(const char *n, int (*func)(AsyncObject<T, D> *, bool , T )) : name(n),
		functionCallback(func) {}
	AsyncFunction(const char *n, int (*func)(AsyncObject<T, D> *, bool , T ),  T event) : name(n),
		functionCallback(func) {
		events.push_back(event);
	}
	virtual ~AsyncFunction() {};
	const char *getName() { return name;}
	void addCompletionEvent(long event) { events.push_back(event);}
	void addCompletionEvents(std::vector<T> *evnts) { events = *evnts;}
	virtual int processEvent(AsyncObject<T, D> *obj, bool completed, T evt);
	bool isSyncFunction() {return (events.size() == 0);}
	virtual void registerAsyncObject(AsyncObject<T, D> *obj) {};
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
	const char *name;
	AsyncManager<T, D> *mngr;
	FunctionList<T,D> aList;
	std::map<const char *, void *, strCmp> properties;
	int processCompletion(T evttype);
	State state;
	static int remoteLaunch(AsyncObject<T,D> *obj, bool completed, T evt);
	static AsyncFunction<T, D> remoteLaunchFunction;
	int (*processUnexpected)(AsyncObject<T, D> *async, AsyncFunction<T,D> *f,  T);
	static int defaultUnexpected(AsyncObject<T, D> *obj, AsyncFunction<T, D> *f, T evttype);
public:
	AsyncObject(D d, const char *name, AsyncManager<T, D> *m);
	void registerUnexpectedFunction(int (*func)(AsyncObject<T, D> *async, AsyncFunction<T,D> *f,  T) = nullptr)
	{
		if (func != nullptr)
			processUnexpected = func;
		else
			processUnexpected = defaultUnexpected;
	}
	void addFunction(AsyncFunction<T, D> *);
	void addFunctionList(FunctionList<T, D> *functionList);
	void addRemoteFunction(AsyncObject<T, D> *, AsyncFunction<T, D> *);
	void addRemoteFunctionList(AsyncObject<T, D> *, FunctionList<T, D> *);
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
class AsyncManager {
	std::map<D, AsyncObject<T, D> *> objectMap;
public:
	AsyncManager() {};
	int processEvent(D, T);
	AsyncObject<T, D>* getAsyncObject(D);
	void addAsyncObject(AsyncObject<T, D> *);
};

template<class T, class D> class SyncPoint;

template<class T, class D>
class SyncFunction : public AsyncFunction<T, D> {
	SyncPoint<T, D> *sPoint;
	std::vector<Pair<T,D> > pairs;
public:
	SyncFunction(SyncPoint<T, D> *parent);
	int processEvent(AsyncObject<T, D> *obj, bool completed, T evt);
	void registerAsyncObject(AsyncObject<T, D> *obj);
};


template<class T, class D>
class SyncPoint {
	const char *name;
	T term;
	int (*callback)(std::vector<Pair<T, D> > *pairs);
public:
	SyncPoint(const char *nm, int (*cb)(std::vector<Pair<T, D> > *pairs), T tEvent) : callback(cb), name(nm), term(tEvent) {}
	virtual ~SyncPoint() {}

	AsyncFunction<T, D> *getInstance();
	virtual int sendSyncReadyEvent(std::vector<Pair<T, D> > *pairs) = 0;
	int termRecv(std::vector<Pair<T, D> > *pairs);
	T getTermEvent();
	const char* getName() const { return name;	}
	
};
#endif /* ASYNCLIB_H_ */
