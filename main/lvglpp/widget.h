#pragma once
#include "lib/rtos/freertos.h"
#include "lib/misc/callback.h"

namespace LVGL
{
	static FreeRTOS::Mutex lvglMutex;
	class Widget
	{
	protected:
		lv_obj_t* handle = NULL;
		virtual lv_obj_t* Create(lv_obj_t* parent) = 0;
		virtual void ApplyDefaultParameters() {}
		
	public:
		
		
		Widget()
		{
		}
		
		~Widget()
		{
			lv_obj_del_async(handle);
		}
		
		void AddWidget(Widget& widget)
		{
			LVGL::lvglMutex.Take();
			if (widget.handle != NULL)
				lv_obj_del(widget.handle);
			widget.handle = widget.Create(handle);
			widget.handle->user_data = this;
			widget.ApplyDefaultParameters();
			lvglMutex.Give();
		}
	};
	
}