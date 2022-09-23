#pragma once
#include "lvgl/lvgl.h"
#include "widget.h"

namespace LVGL
{
		 	
	class Button : public Widget
	{
	public:
		Callback<void, Button*> OnClicked;
		
		
		void SetPos(int x, int y)
		{
			lv_obj_set_pos(handle, x, y);
		}
		
		void SetSize(int width, int height)
		{
			lv_obj_set_size(handle, width, height);
		}
		
	private:
		
		static void StaticCallback(lv_event_t * e)
		{
			Button* button = (Button*)e->user_data;
			lv_event_code_t code = lv_event_get_code(e);
			
			switch (code)
			{
			case LV_EVENT_CLICKED:
				if (button->OnClicked.IsBound())
					button->OnClicked.Invoke(button);
				break;
			default:
				break;
			}
		}
		
		virtual void ApplyDefaultParameters() override
		{ 
			SetPos(0, 0);
			SetSize(120, 50);
			
			lv_obj_add_event_cb(handle, StaticCallback, LV_EVENT_ALL, this);           /*Assign a callback to the button*/
		}
		
		virtual lv_obj_t* Create(lv_obj_t* parent) override
		{
			return lv_btn_create(parent);
		}
	};
}
