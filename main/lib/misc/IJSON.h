#pragma once


#include <cJSON.h>
#include <stdint.h>
#include <vector>



class IJSON
{
	cJSON* root = NULL;
		
		
public:
		
	IJSON()
	{
		root = cJSON_CreateObject();
	}
	
	~IJSON()
	{
		
	}
	
	void Dispose()
	{
		if (root != NULL)
		{
			cJSON_Delete(root);
				
		}
	}
	
	std::string Serialize()
	{
		char* str = cJSON_Print(root);
		std::string result(str);
		free(str);
		return result;
	}
			
	
	void AddChild(std::string name, IJSON* obj)
	{
		cJSON_AddItemToObject(root, name.c_str(), obj->root);
	}
	
	
	void AddChild(std::string name, uint64_t obj)
	{
		cJSON_AddNumberToObject(root, name.c_str(), obj);
	}
	
	void AddChild(std::string name, std::string obj)
	{
		cJSON_AddStringToObject(root, name.c_str(), obj.c_str());
	}
};

class IJsonConvertable
{
public:
	
	virtual bool PopulateFromJSON(IJSON *json) = 0;
	virtual IJSON* ToJSON() = 0;
	
	std::string Serialize()
	{
		IJSON* json = ToJSON();
		std::string result = json->Serialize();
		json->Dispose();
		delete json;
		return result;
	}
	
};


/*
class IJsonConvertable
{
public:
	
	virtual bool PopulateFromJSON(cJSON *json) = 0;
	virtual cJSON* ToJSON() = 0;
	

	static cJSON* ToJSON(IJSON* val)
	{
		return val->ToJSON();
	}
	
	static bool TryParse(cJSON* json, IJSON* val)
	{
		return val->PopulateFromJSON(json);
	}
	
	static cJSON* ToJSON(int8_t* val)
	{
		return cJSON_CreateNumber(*val);
	}
	
	static bool TryParse(cJSON* json, int8_t* val)
	{
		if (cJSON_IsNumber(json))
		{
			*val = json->valueint;
			return true;
		}
		return false;
	}
	
	static cJSON* ToJSON(uint8_t* val)
	{
		return cJSON_CreateNumber(*val);
	}
	
	static bool TryParse(cJSON* json, uint8_t* val)
	{
		if (cJSON_IsNumber(json))
		{
			*val = json->valueint;
			return true;
		}
		return false;
	}
	
	
	static cJSON* ToJSON(int16_t* val)
	{
		return cJSON_CreateNumber(*val);
	}
	
	static bool TryParse(cJSON* json, int16_t* val)
	{
		if (cJSON_IsNumber(json))
		{
			*val = json->valueint;
			return true;
		}
		return false;
	}
	
	static cJSON* ToJSON(uint16_t* val)
	{
		return cJSON_CreateNumber(*val);
	}
	
	static bool TryParse(cJSON* json, uint16_t* val)
	{
		if (cJSON_IsNumber(json))
		{
			*val = json->valueint;
			return true;
		}
		return false;
	}
	
	
	static cJSON* ToJSON(int32_t* val)
	{
		return cJSON_CreateNumber(*val);
	}
	
	static bool TryParse(cJSON* json, int32_t* val)
	{
		if (cJSON_IsNumber(json))
		{
			*val = json->valueint;
			return true;
		}
		return false;
	}
	
	static cJSON* ToJSON(uint32_t* val)
	{
		return cJSON_CreateNumber(*val);
	}
	
	static bool TryParse(cJSON* json, uint32_t* val)
	{
		if (cJSON_IsNumber(json))
		{
			*val = json->valueint;
			return true;
		}
		return false;
	}
	
	
	
	static cJSON* ToJSON(std::string* val)
	{
		return cJSON_CreateString(val->c_str());
	}
	
	static bool TryParse(cJSON* json, std::string* val)
	{
		if (cJSON_IsNumber(json))
		{
			*val = std::string (json->valuestring);
			return true;
		}
		return false;
	}
	
	template<typename T>
	static cJSON* ToJSON(std::vector<T>* val)
	{
		cJSON* json = cJSON_CreateArray();
		for (int i = 0; i < val->size(); i++)
		{
			cJSON_AddItemToArray(json, IJSON::ToJSON((*val)[i]));
		}
		
		return json;
	}
	
	template<typename T>
	static bool TryParse(cJSON* json, std::vector<T>* val)
	{
		cJSON* iterator;
		cJSON_ArrayForEach(iterator, json) 
		{
			T* obj = new T;
			IJSON::TryParse(iterator, *obj);	
			val->push_back(*obj);
		}
		return true;
	}
	

	
};
*/
