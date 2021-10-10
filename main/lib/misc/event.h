#ifndef _EVENT_H_
#define _EVENT_H_

//Would advise against this...
//#define BIND(func)	Bind(this, &func)

#include <list>
#include "esp_log.h"

template<typename T, typename ...Args>
class EventHandlerMethod;

template<typename ...Args>
class EventHandlerFunc;


template<typename ...Args>
class EventHandler
{
public:
	EventHandler(){}
	virtual ~EventHandler(){}
	virtual void Invoke(Args... args) = 0;
	virtual bool IsMethod() = 0;



	template<typename T>
	bool Compare(T* instance, void(T::*memberFunctionToCall)(Args...))
	{
		if(IsMethod())
		{
			EventHandlerMethod<T, Args...> *evh = static_cast<EventHandlerMethod<T, Args...>* >(this);
			return evh->Compare(instance, memberFunctionToCall);
		}
		else
		{
			return false;
		}
	}

	bool Compare(void(*func)(Args...))
	{
		if(IsMethod())
		{
			return false;
		}
		else
		{
			EventHandlerFunc<Args...> *evh = static_cast<EventHandlerFunc<Args...>* >(this);
			return evh->Compare(func);
		}
	}
};

template<typename T, typename ...Args>
class EventHandlerMethod : public EventHandler<Args...>
{
private:
	void(T::*method)(Args...);
	T* methodInstance;

public:

	EventHandlerMethod<T, Args...>(T* instance, void(T::*memberFunctionToCall)(Args...))
	{
		method = memberFunctionToCall;
		methodInstance = instance;
	}

	void Invoke(Args... args)
	{
		(methodInstance->*method)(args...);
	}

	bool Compare(T* instance, void(T::*memberFunctionToCall)(Args...))
	{
		return methodInstance == instance && memberFunctionToCall == method;
	}

	bool IsMethod()
	{
		return true;
	}
};

template<typename ...Args>
class EventHandlerFunc : public EventHandler<Args...>
{
private:
	void(*func)(Args...);

public:

	EventHandlerFunc<Args...>( void(*functionCall)(Args...))
	{
		func = functionCall;
	}

	void Invoke(Args... args)
	{
		(*func)(args...);
	}

	bool Compare( void(*functionCall)(Args...))
	{
		return func == functionCall;
	}

	bool IsMethod()
	{
		return false;
	}
};

/// <summary>
/// </summary>
/// <example>
/// <code>
/// class MyClass
/// {
/// public:
///		MyClass()
///		{
///			SomeEvent.Bind(this, &MyClass::OnEvent);
///		}
///
///		void Trigger(int i)
///		{
///			SomeEvent(i);
///		}
///
///		void OnEvent(int i)
///		{
///			printf("Hello %d", i);
///		}
///
///		Event<int> SomeEvent;
/// };
///
///
/// </code>
/// </example>
template<typename ...Args>
class Event
{
private:
	std::list<EventHandler<Args...>*> handlers;

public:

	~Event()
	{
		ClearAllHandlers();
	}

	/// <summary>
	/// Removes all subscribed functions and methods to this event.
	/// </summary>
	void ClearAllHandlers()
	{

		for(typename std::list< EventHandler<Args...>* >::iterator iter= handlers.begin(); iter != handlers.end(); ++iter)
		{
			EventHandler<Args...>* pHandler= *iter;
			if(pHandler)
			{
				delete pHandler;
				pHandler= 0;  // just to be consistent
			}
		}
		handlers.clear();
	}

	/// <summary>
	/// Used to bind a method to the event.
	/// </summary>
	/// <example>
	/// <code>
	/// SomeEvent.Bind(this, &MyClass::OnEvent);
	/// </code>
	/// </example>
	/// <param name="methodInstance">The owner of the method</param>
	/// <param name="method">The method to be bound to the event.</param>
	template<typename T>
	void Bind( T* methodInstance, void(T::*method)(Args...))
	{
		handlers.push_back(new EventHandlerMethod<T, Args...>(methodInstance, method));
	}

	/// <summary>
	/// Used to bind a function to the event.
	/// </summary>
	/// <example>
	/// <code>
	/// SomeEvent.Bind(&OnEvent);
	/// </code>
	/// </example>
	/// <param name="function">The function to be bound to the event.</param>
	void Bind(void(*function)(Args...))
	{
		handlers.push_back(new EventHandlerFunc<Args...>(function));
	}

	/// <summary>
	/// Used to remove a method from the event.
	/// </summary>
	/// <example>
	/// <code>
	/// SomeEvent.UnBind(this, &MyClass::OnEvent);
	/// </code>
	/// </example>
	/// <param name="methodInstance">The owner of the method</param>
	/// <param name="method">The method to be unbound from the event.</param>
	template<typename T>
	void UnBind( T* methodInstance, void(T::*method)(Args...))
	{
		auto iter= handlers.begin();
		while(iter != handlers.end())
		{
			EventHandler<Args...>* pHandler= *iter;
			if(pHandler->Compare(methodInstance, method))
			{
				delete pHandler;
				//iter is incremented before calling erase, and the previous value is passed to the function. A function's arguments have to be fully evaluated before the function is called.
				//https://stackoverflow.com/questions/596162/can-you-remove-elements-from-a-stdlist-while-iterating-through-it
				handlers.erase(iter++);
			}
			else
				iter++;
		}
	}

	/// <summary>
	/// Used to remove a function from the event.
	/// </summary>
	/// <example>
	/// <code>
	/// SomeEvent.UnBind(&OnEvent);
	/// </code>
	/// </example>
	/// <param name="func">The function to be unbound from the event.</param>
	void UnBind(void(*func)(Args...))
	{
		auto iter= handlers.begin();
		while(iter != handlers.end())
		{
			EventHandler<Args...>* pHandler= *iter;

			if(pHandler->Compare(func))
			{
				delete pHandler;
				//iter is incremented before calling erase, and the previous value is passed to the function. A function's arguments have to be fully evaluated before the function is called.
				//https://stackoverflow.com/questions/596162/can-you-remove-elements-from-a-stdlist-while-iterating-through-it
				handlers.erase(iter++);
			}
			else
				iter++;
		}
	}

	/// <summary>
	/// Used to invoke the event.
	/// </summary>
	/// <param name="args...">Arguments</param>
	void Invoke(Args... args)
	{

		auto iter= handlers.begin();

		while(iter != handlers.end())
		{
			EventHandler<Args...>* pHandler= *iter;
			pHandler->Invoke(args...);

			iter++;
		}



	}
};



#endif
