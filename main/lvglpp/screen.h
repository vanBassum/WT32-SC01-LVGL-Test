#pragma once


/* Littlevgl specific */
#include "lvgl/lvgl.h"
#include "widget.h"


namespace LVGL
{
	class Screen : public Widget
	{
protected:
		virtual lv_obj_t* Create(lv_obj_t* parent) override
		{
			return NULL;
		}
		
	public:
		Screen()
		{

		}
		
		void InitActualScreen()
		{
			if (handle == NULL)
				handle = lv_scr_act();
		}

	};	
}







