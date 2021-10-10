/*
 * callback.h
 *
 *  Created on: Jan 17, 2021
 *      Author: Bas
 */

#ifndef COMPONENTS_MISC_CALLBACK_H_
#define COMPONENTS_MISC_CALLBACK_H_


template<typename R, typename ...Args>
class CallbackHandler
{
public:
	CallbackHandler(){}
	virtual ~CallbackHandler(){}
	virtual R Invoke(Args... args) = 0;
};


template<typename T, typename R, typename ...Args>
class CallbackHandlerMethod : public CallbackHandler<R, Args...>
{
private:
	R(T::*method)(Args...);
	T* methodInstance;

public:

	CallbackHandlerMethod<T, R, Args...>(T* instance, R(T::*memberFunctionToCall)(Args...))
	{
		method = memberFunctionToCall;
		methodInstance = instance;
	}

	R Invoke(Args... args)
	{
		return (methodInstance->*method)(args...);
	}
};


template<typename R, typename ...Args>
class CallbackHandlerFunction : public CallbackHandler<R, Args...>
{
private:
	R(*func)(Args...);

public:
	CallbackHandlerFunction<R, Args...>( R(*functionCall)(Args...))
	{
		func = functionCall;
	}

	R Invoke(Args... args)
	{
		return (*func)(args...);
	}
};





template<typename R, typename ...Args>
class Callback
{
	CallbackHandler<R, Args...> *callback = 0;

public:
	Callback()
	{

	}

	template<typename T>
	Callback (T* instance, R(T::*method)(Args...))
	{
		callback = new CallbackHandlerMethod<T, R, Args...>(instance, method);
	}

	Callback(R(*func)(Args...))
	{
		callback = new CallbackHandlerFunction<R, Args...>(func);
	}


	virtual ~Callback()
	{
		delete callback;
	}

	template<typename T>
	void Bind(T* instance, R(T::*method)(Args...))
	{
		if(IsBound())
			delete callback;
		callback = new CallbackHandlerMethod<T, R, Args...>(instance, method);
	}

	void Bind(R(*func)(Args...))
	{
		if(IsBound())
			delete callback;
		callback = new CallbackHandlerFunction<R, Args...>(func);
	}
	
	void Unbind()
	{
		if (IsBound())
			delete callback;
		callback = 0;
	}

	bool IsBound()
	{
		return callback != 0;
	}

	R Invoke(Args... args)
	{
		//todo figure out a way to do a null check. (maybe return a pointer of the result that could be NULL)
		return callback->Invoke(args...);
	}
};






#endif
