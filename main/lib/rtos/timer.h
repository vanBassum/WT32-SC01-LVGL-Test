/*
 * timer.h
 *
 *  Created on: Apr 14, 2021
 *      Author: Bas
 */

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

namespace FreeRTOS
{

	class Timer
	{
		TimerHandle_t xTimer;


		static void tCallback(TimerHandle_t xTimer)
		{
			Timer* t = static_cast<Timer*>(pvTimerGetTimerID(xTimer));
			//ESP_LOGI("Timer", "Elapsed '%s'", t->GetName());
			t->OnTimerElapsed();
		}


	public:

		Timer(const char* name, TickType_t period, bool autoReload = false)
		{	period /= portTICK_PERIOD_MS;
			if(period < 1)
				period = 1;
			xTimer = xTimerCreate(name, period, autoReload, this, &tCallback);
		}

		~Timer()
		{
			//todo: what if this fails???
			xTimerDelete(xTimer, 0 );
		}


		bool Start(int timeout=0)
		{
			return xTimerStart(xTimer, timeout / portTICK_PERIOD_MS) == pdPASS;
		}

		bool Stop(int timeout=0)
		{
			return xTimerStop(xTimer, timeout / portTICK_PERIOD_MS) == pdPASS;
		}

		bool Reset(int timeout=0)
		{
			return xTimerReset(xTimer, timeout / portTICK_PERIOD_MS) == pdPASS;
		}

		bool IsRunning()
		{
			return xTimerIsTimerActive( xTimer ) != pdFALSE;
		}

		bool SetPeriod(TickType_t period, int timeout=0)
		{	period /= portTICK_PERIOD_MS;
			if(period < 1)
				period = 1;
			return xTimerChangePeriod(xTimer, period, timeout / portTICK_PERIOD_MS) == pdPASS;
		}

		TickType_t GetPeriod(int timeout=0)
		{
			return xTimerGetPeriod(xTimer);
		}


		TickType_t GetExpireTime()
		{
			return xTimerGetExpiryTime(xTimer) * portTICK_PERIOD_MS;
		}

		TickType_t GetRemainingTime()
		{
			if(xTimerIsTimerActive(xTimer) != pdFALSE)
			{
				return ((xTimerGetExpiryTime( xTimer ) - xTaskGetTickCount()) * portTICK_PERIOD_MS);
			} else
			{
				return 0;
			}
		}

		const char* GetName()
		{
			return pcTimerGetTimerName( xTimer );
		}


		Event<> OnTimerElapsed;

	};
}


#endif /* MAIN_ESPLIB_FREERTOS_CPP_TIMER_H_ */
