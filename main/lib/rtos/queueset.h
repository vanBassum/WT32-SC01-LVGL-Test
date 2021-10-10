/*
 * QueueSet.h
 *
 *  Created on: Dec 13, 2019
 *      Author: Bas
 */

#ifndef MAIN_SMARTHOME_LIB_FREERTOS_QUEUESET_H_
#define MAIN_SMARTHOME_LIB_FREERTOS_QUEUESET_H_


#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
//#include "SemaphoreCPP.h"
#include <vector>

namespace FreeRTOS
{

	class QueueSetAddable
	{
	public:
		QueueSetAddable(){}
		virtual ~QueueSetAddable(){}
		virtual int GetSize() = 0;
		virtual QueueSetMemberHandle_t GetHandle() = 0;
	};



	/// <summary>
	/// Queue sets are a FreeRTOS feature that enables an RTOS task to block (pend) when receiving from multiple queues and/or semaphores at the same time. Queues and semaphores are grouped into sets, then, instead of blocking on an individual queue or semaphore, a task instead blocks on the set.
	/// </summary>
	class QueueSet
	{
		QueueSetHandle_t xQueueSet;

	public:


		QueueSet(std::vector<QueueSetAddable*> items)
		{
			int totSize = 0;

			for(auto it = items.begin(); it != items.end(); it++)
				totSize += (*it)->GetSize();

			xQueueSet = xQueueCreateSet(totSize);

			for(auto it = items.begin(); it != items.end(); it++)
				xQueueAddToSet((*it)->GetHandle(), xQueueSet);

		}


		bool Wait(int timeout)
		{
			return xQueueSelectFromSet(xQueueSet, timeout / portTICK_PERIOD_MS) != NULL;
		}



	};


}


#endif /* MAIN_SMARTHOME_LIB_FREERTOS_QUEUESET_H_ */
