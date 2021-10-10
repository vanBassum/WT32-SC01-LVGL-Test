/*
 * semaphore.h
 *
 *  Created on: Jan 17, 2021
 *      Author: Bas
 */

#ifndef COMPONENTS_FREERTOS_CPP_SEMAPHORE_H_
#define COMPONENTS_FREERTOS_CPP_SEMAPHORE_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace FreeRTOS
{

	class Mutex
	{
		SemaphoreHandle_t handle = NULL;
	public:
		Mutex()
		{
			handle = xSemaphoreCreateMutex();
		}

		bool Take()
		{
			return xSemaphoreTake(handle, portMAX_DELAY ) == pdTRUE;
		}

		bool Take(int timeout)
		{
			return xSemaphoreTake(handle, timeout / portTICK_PERIOD_MS) == pdTRUE;
		}

		bool Give()
		{
			return xSemaphoreGive(handle) == pdTRUE;
		}

	};


	class RecursiveMutex
	{
		SemaphoreHandle_t handle = NULL;
	public:
		RecursiveMutex()
		{
			handle = xSemaphoreCreateRecursiveMutex();
		}

		bool Take()
		{
			return xSemaphoreTakeRecursive(handle, portMAX_DELAY ) == pdTRUE;
		}

		bool Take(int timeout)
		{
			return xSemaphoreTakeRecursive(handle, timeout / portTICK_PERIOD_MS) == pdTRUE;
		}

		bool Give()
		{
			return xSemaphoreGiveRecursive(handle) == pdTRUE;
		}
	};

	class SemaphoreBinary
	{
		SemaphoreHandle_t handle = NULL;

	public:

		SemaphoreBinary(bool free = false)
		{
			handle = xSemaphoreCreateBinary();
			if(free)
				Give();
		}

		virtual ~SemaphoreBinary()
		{
			vSemaphoreDelete(handle);
			handle = NULL;
		}

		bool Take()
		{
			return xSemaphoreTake(handle, portMAX_DELAY ) == pdTRUE;
		}

		bool Take(int timeout)
		{
			return xSemaphoreTake(handle, timeout / portTICK_PERIOD_MS) == pdTRUE;
		}

		bool TakeFromISR(int *higherPriorityTaskWoken = NULL)
		{
			return xSemaphoreTakeFromISR(handle, higherPriorityTaskWoken) == pdTRUE;
		}

		bool Give()
		{
			return xSemaphoreGive(handle) == pdTRUE;
		}

		bool GiveFromISR(int *higherPriorityTaskWoken = NULL)
		{
			return xSemaphoreGiveFromISR(handle, higherPriorityTaskWoken) == pdTRUE;
		}
	};
}

#endif /* COMPONENTS_FREERTOS_CPP_SEMAPHORE_H_ */
