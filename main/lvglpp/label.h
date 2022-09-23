#pragma once
#include "lvgl/lvgl.h"
#include "widget.h"
#include <string>

namespace LVGL
{
		 	
	class Label : public Widget
	{
	public:
		void SetText(std::string text)
		{
			lv_label_set_text(handle, text.c_str());
		}
		
		void SetAlignment(lv_align_t align)
		{
			lv_obj_set_align(handle, align);
		}
		
	private:		
		virtual void ApplyDefaultParameters() override
		{ 
			lv_label_set_text(handle, "Some text");                     /*Set the labels text*/
			lv_obj_center(handle);
		}
		
		virtual lv_obj_t* Create(lv_obj_t* parent) override
		{
			return lv_label_create(parent);
		}
	};
}
