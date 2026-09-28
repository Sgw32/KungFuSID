/**
  ******************************************************************************
  * @file    xparam.c
  * @author  Gasper Jersin
  * @date    2025-03-25
  * @brief   xparam
  *
  ******************************************************************************
  */

/* Private includes ----------------------------------------------------------*/
#include "xparam.h"
#include <string.h>
#include <stdio.h>
// #include <math.h>
// #include <stdlib.h>


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#if XPARAM_LOG == 1
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...) do {} while (0)
#endif


/* Private constants ---------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/



// calculate crc32
uint32_t xparam_crc32(uint8_t *buf, size_t size)
{
   int j;
   uint32_t byte, crc, mask;

   crc = 0xFFFFFFFF;
   for (int i = 0; i < size; i++){
      byte = buf[i];            // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--) {    // Do eight times.
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
   }
   return ~crc;
}


uint32_t xparam_get_key(xparam_t* param){
    return xparam_crc32((uint8_t*)param->field_name, strlen(param->field_name));
}

uint32_t xparam_get_value_crc(uint32_t data){
    return xparam_crc32((uint8_t*)&data, sizeof(uint32_t));
}

int xparam_value_type_str(xparam_vtype_t vtype, char* buf){
	switch (vtype) {
		case XPARAM_NONE:
			return 0;
			break;
		case XPARAM_U8:
			return sprintf(buf, "uint8_t");
			break;
		case XPARAM_BOOL:
			return sprintf(buf, "bool");
			break;
		case XPARAM_I8:
			return sprintf(buf, "int8_t");
			break;
		case XPARAM_U16:
			return sprintf(buf, "uint16_t");
			break;
		case XPARAM_I16:
			return sprintf(buf, "int16_t");
			break;
		case XPARAM_U32:
			return sprintf(buf, "uint32_t");
			break;
		case XPARAM_I32:
			return sprintf(buf, "int32_t");
			break;
		case XPARAM_FLOAT:
			return sprintf(buf, "float");
			break;
		case XPARAM_STRING:
			return sprintf(buf, "str");
			break;
	}
	return 0;
}


/* Public functions ----------------------------------------------------------*/
uint8_t* xparam_table_to_blob(xparam_table_t* table)
{
    xparam_img_t* img = malloc(XPARAM_IMAGE_SIZE(table->n_params));
    if (img == NULL){
        return NULL;
    }
    img->header.magic = XPARAM_MAGIC;
    img->header.n_params = table->n_params;
    for (int i = 0; i < table->n_params; i++){
        xparam_t* param = &table->params[i];
        img->params[i].data = param->value;
        img->params[i].key = xparam_get_key(param);
        img->params[i].crc = xparam_get_value_crc(param->value);
    }
    return (uint8_t*) img;
}




uint8_t xparam_table_from_blob(xparam_table_t* table, uint8_t* buf)
{
    xparam_img_t* img = (xparam_img_t*)buf;
	if (img->header.magic == XPARAM_MAGIC){
		for (int i = 0; i < table->n_params; i++){
			xparam_t* param = &table->params[i];
            if (param->value_type == XPARAM_STRING){
                continue;
            }
			uint32_t key_calc = xparam_get_key(param);
            uint8_t found = 0;
			for (int j = 0; j < img->header.n_params; j++){
				// find parameter by crc
				if (img->params[j].key == key_calc){
                    uint32_t crc_calc = xparam_get_value_crc(img->params[j].data);
                    if (crc_calc == img->params[j].crc){
                        param->value = img->params[j].data;
                        xparam_step_value(param, 0); // enforce limits
                    }
                    else{
                        LOG("XPARAM load table:"
                            "Value data corrupted for parameter #%d %s."
                            "Default value keept.\n", i, param->p_name);
                    }
                    found = 1;
				}
			}
			if(!found){
                LOG("XPARAM load table:"
                    "Missing data for parameter #%d %s."
                    "Default value keept.\n", i, param->p_name);
            }
		}
		return 1;
	}
    LOG("XPARAM load table: No magic in blob.");
	return 0;
}



#define PRINTF_FLOAT_SUPPORT
// returns size of written string
int xparam_stringify(xparam_t* param, char* buf)
{
	if (param->print_cb){
		return param->print_cb(param, buf);
	}
	if (XPARAM_U32 == param->value_type){
		return sprintf(buf,"%lu" , (unsigned long) param->value);
	}
	else if (XPARAM_I32 == param->value_type){
        xparam_I32_t* p_param = (xparam_I32_t*)param;
		return sprintf(buf,"%li" , (long) p_param->value);
	}
	else if (XPARAM_BOOL == param->value_type){

		xparam_BOOL_t* p_param = (xparam_BOOL_t*)param;
		if (p_param->value){
			return sprintf(buf,"true");
		}
		else{
			return sprintf(buf,"false");
		}
	}
	else if (XPARAM_U16 == param->value_type){
        xparam_U16_t* p_param = (xparam_U16_t*)param;
		return sprintf(buf,"%u" , p_param->value);
	}
	else if (XPARAM_I16 == param->value_type){
        xparam_I16_t* p_param = (xparam_I16_t*)param;
		return sprintf(buf,"%i" , p_param->value);
	}
	else if (XPARAM_U8 == param->value_type){
        xparam_U8_t* p_param = (xparam_U8_t*)param;
		return sprintf(buf,"%u" , p_param->value);
	}
	else if (XPARAM_I8 == param->value_type){
        xparam_I8_t* p_param = (xparam_I8_t*)param;
		return sprintf(buf,"%i" , p_param->value);
	}
	else if (XPARAM_STRING == param->value_type){
        xparam_STRING_t* p_param = (xparam_STRING_t*)param;
		return sprintf(buf, "%s", (char*)p_param->value);
	}
	else if (XPARAM_FLOAT == param->value_type){
        xparam_FLOAT_t* p_param = (xparam_FLOAT_t*)param;
#ifdef PRINTF_FLOAT_SUPPORT
		return sprintf((char*)buf,"%.4f" , p_param->value);
		// the line above should work but sometimes it doesn't even if printf float support is enabled
		// here is an alternative method
//		float d = param_p->value;
//		sprintf(buf,"%d.%02u", (int) d, (int) fabs(((d - (int) d ) * 100)));
		// problems with this method is accuracy (0.09 instead of 0.1) and lost sign between 0 and -1.0
		// here is more complex routine that works better
#else
		float f = p_param->value * 1000;
		int div = (int) f / 1000;
		int rem = 0;
		int offset = 0;
		if (f > 0){
			f = f + 1;
			rem = ((int)f - (div * 1000)) / 10 ;
			if (rem == 100){
				div += 1;
				rem = 0;
			}
		}
		else{
			f = f - 1;
			rem = abs(((int)f - (div * 1000)) / 10) ;
			if (div == 0 && rem != 0){
				buf[0] = '-';
				offset = 1;
			}
			if (rem == 100){
				div -= 1;
				rem = 0;
			}
		}
		return sprintf(&buf[offset],"%d.%02u", div, rem);
#endif
	}
	else {
		return 0;
	}
}

#define PARAM_STEP(param, n_steps) \
	float _step = (param->step_size)? param->step_size : 1; \
	float _change = (n_steps * _step); \
	typeof(param->value) _result = param->value + _change; \
	if(n_steps == 0){ \
		 _result = param->value; \
	} \
	if ((n_steps < 0) && (_result > param->value)){ \
		/*unsigned underflow*/ \
		param->value = 0; \
		return 0; \
	} \
	else if ((_result > param->max) && (param->max != 0)){ \
		param->value = param->max;  \
		return 0; \
	} \
	else if ((_result < param->min) && (param->min != 0)){ \
		param->value = param->min; \
		return 0; \
	} \
	param->value = _result; \
	return 1


#define PARAM_SET(param, new_val) \
	typeof(param->value) _result = new_val; \
	if (param->max!=param->min){\
		if (_result > param->max){ \
			return 0; \
		} \
		else if (_result < param->min){ \
			return 0; \
		} \
	} \
	param->value = _result; \
	return 1


// returns 1 if change was successful according to limits
uint8_t xparam_step_value(xparam_t* param, int16_t n_steps)
{
	if (param->change_cb){
		return param->change_cb(param, n_steps);
	}
	else if (XPARAM_U32 == param->value_type){
		PARAM_STEP(param, n_steps);
	}
	else if (XPARAM_BOOL == param->value_type){
        xparam_BOOL_t* p_param = (xparam_BOOL_t*)param;
		p_param->value = !p_param->value;
	}
	else if (XPARAM_I32 == param->value_type){
        xparam_I32_t* p_param = (xparam_I32_t*)param;
		PARAM_STEP(p_param, n_steps);
	}
	else if (XPARAM_U16 == param->value_type){
        xparam_U16_t* p_param = (xparam_U16_t*)param;
		PARAM_STEP(p_param, n_steps);
	}
	else if (XPARAM_I16 == param->value_type){
        xparam_I16_t* p_param = (xparam_I16_t*)param;
		PARAM_STEP(p_param, n_steps);
	}
	else if (XPARAM_U8 == param->value_type){
        xparam_U8_t* p_param = (xparam_U8_t*)param;
		PARAM_STEP(p_param, n_steps);
	}
	else if (XPARAM_I8 == param->value_type){
        xparam_I8_t* p_param = (xparam_I8_t*)param;
		PARAM_STEP(p_param, n_steps);
	}
	else if (XPARAM_FLOAT == param->value_type){
        xparam_FLOAT_t* p_param = (xparam_FLOAT_t*)param;
		PARAM_STEP(p_param, n_steps);
	}
	else if (XPARAM_STRING == param->value_type){
	    return 1;
    }
	return 0; //0 == fail
}

xparam_t* xparam_find_by_name(xparam_table_t* table, const char* name) {
    for (int i = 0; i < table->n_params; i++) {
        if (strcmp(table->params[i].p_name, name) == 0) {
            return &table->params[i];
        }
    }
    return NULL;
}

uint8_t xparam_set_value(xparam_t* param, uint32_t value)
{
    if (param->change_cb){
        return param->change_cb(param, value);
    }
    else if (XPARAM_U32 == param->value_type){
        PARAM_SET(param, value);
    }
    else if (XPARAM_BOOL == param->value_type){
        xparam_BOOL_t* p_param = (xparam_BOOL_t*)param;
        p_param->value = (value > 0);
    }
    else if (XPARAM_I32 == param->value_type){
        xparam_I32_t* p_param = (xparam_I32_t*)param;
        // Convert uint32_t to int32_t using union
        union {
            uint32_t u32;
            int32_t i32;
        } u;
        u.u32 = value;
        PARAM_SET(p_param, u.i32);
    }
    else if (XPARAM_U16 == param->value_type){
        xparam_U16_t* p_param = (xparam_U16_t*)param;
        PARAM_SET(p_param, (uint16_t)value);
    }
    else if (XPARAM_I16 == param->value_type){
        xparam_I16_t* p_param = (xparam_I16_t*)param;
        PARAM_SET(p_param, (int16_t)value);
    }
    else if (XPARAM_U8 == param->value_type){
        xparam_U8_t* p_param = (xparam_U8_t*)param;
#if XPARAM_LOG == 1
		// Log parameter info before setting
		printf("xparam_set_value(call) Setting param '%s' to value (u8): 0x%02X", p_param->p_name, (uint8_t)value);
#endif
        PARAM_SET(p_param, (uint8_t)value);
    }
    else if (XPARAM_I8 == param->value_type){
        xparam_I8_t* p_param = (xparam_I8_t*)param;
        PARAM_SET(p_param, (int8_t)value);
    }
    else if (XPARAM_FLOAT == param->value_type){
        xparam_FLOAT_t* p_param = (xparam_FLOAT_t*)param;
        // Convert uint32_t to float using union
        union {
            uint32_t u32;
            float f;
        } u;
        u.u32 = value;
        PARAM_SET(p_param, u.f);
    }
    else if (XPARAM_STRING == param->value_type){
        return 1;
    }
    return 0; //0 == fail
}

// {"params":[
// {"name":"","value":,"info":"","type":"","min":,"max":,"step":,"id":"966e826e"}
// , ---
//]}
size_t xparam_get_json_buf_size(xparam_table_t* table){
	size_t size = 11;
	char val_buf[50];
	for (int i = 0; i < table->n_params; i++){
		xparam_t* param = &table->params[i];
		if(i!=0){size+=1;} //comma for array members
		size += 78; //json field names + constant size data
		size += strlen(param->p_name);
		size += strlen(param->p_info);
		size += xparam_stringify(param, val_buf);
		size += xparam_value_type_str(param->value_type, val_buf);
		size += sprintf(val_buf, "%.4f", param->min);
		size += sprintf(val_buf, "%.4f", param->max);
		size += sprintf(val_buf, "%.4f", param->step_size);
	}
	size+=2; //closing brackets
	size+=1; //zero termination
	return size;
}

char* xparam_to_json(xparam_t* param, char* buf){
	buf += sprintf(buf, "{\"name\":\"%s\",", param->p_name);
	char str_buf[40];
	xparam_stringify(param, str_buf);
	if (param->value_type == XPARAM_STRING){
		buf += sprintf(buf, "\"value\":\"%s\",", str_buf);
	}
	else{
		buf += sprintf(buf, "\"value\":%s,", str_buf);
	}
	buf += sprintf(buf, "\"info\":\"%s\",", param->p_info);
	xparam_value_type_str(param->value_type, str_buf);
	buf += sprintf(buf, "\"type\":\"%s\",", str_buf);
	buf += sprintf(buf, "\"min\":%.4f,", param->min);
	buf += sprintf(buf, "\"max\":%.4f,", param->max);
	buf += sprintf(buf, "\"step\":%.4f,", param->step_size);
	// buf += sprintf(buf, "\"field_name\":\"%s\",", param->field_name);
	buf += sprintf(buf, "\"id\":\"%08lx\"}", (unsigned long) xparam_get_key(param));
	return buf;
}

char* xparam_table_to_json(xparam_table_t* table){
	char* ret = malloc(xparam_get_json_buf_size(table));
	if (ret == NULL){
		LOG("xparam_table_to_json: buffer malloc failed!");
		return ret;
	}
	char* buf = ret;
	buf += sprintf(buf, "{\"params\":[");
	for (int i = 0; i< table->n_params; i++){
		if(i!=0){ buf += sprintf(buf, ",");	}
		buf = xparam_to_json(&table->params[i], buf);
	}
	buf += sprintf(buf, "]}");
	return ret;
}
