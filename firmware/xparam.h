/**
  ******************************************************************************
  * @file    xparam.h
  * @author  Gasper Jersin
  * @date    2025-03-25
  * @brief   xparam
  *
  ******************************************************************************
  */
#ifndef INC_XPARAM_H_
#define INC_XPARAM_H_

#ifdef __cplusplus
 extern "C" {
#endif

/* Exported includes ---------------------------------------------------------*/
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#define XPARAM_MAGIC 0xABCD1234
#define XPARAM_LOG   0

/* Exported types ------------------------------------------------------------*/
typedef enum{
	XPARAM_NONE,
	XPARAM_BOOL,
	XPARAM_I8,
	XPARAM_U8,
	XPARAM_I16,
	XPARAM_U16,
	XPARAM_I32,
	XPARAM_U32,
	XPARAM_FLOAT,
	XPARAM_STRING,
}xparam_vtype_t;

#define _COMMON_PARAM_FIELDS() \
		char*					p_name;		\
		char*					p_info;		\
		xparam_vtype_t          value_type;	\
		float					min;		\
		float					max;		\
		float					step_size;  \
		int 			(*print_cb)(struct xparam_s* param, char* buf);  /*return number of written chars*/ \
		uint8_t 		(*change_cb)(struct xparam_s* param, int16_t steps);  /*return 1 if change was within limits*/ \
		char*			field_name;			\

typedef struct xparam_s{
    uint32_t                value;          // parameter value
	_COMMON_PARAM_FIELDS()
}xparam_U32_t;

typedef xparam_U32_t xparam_t;

typedef struct{
	int32_t					value;
	_COMMON_PARAM_FIELDS()
}xparam_I32_t;
static_assert(	sizeof(xparam_U32_t) == sizeof(xparam_I32_t),
		"xparams module expects identical size of parameters");

typedef struct{
	uint32_t				value;
	_COMMON_PARAM_FIELDS()
}xparam_BOOL_t;
static_assert(	sizeof(xparam_U32_t) == sizeof(xparam_BOOL_t),
		"xparams module expects identical size of parameters");

typedef struct{
	int16_t					value;
	uint16_t				_zero;
	_COMMON_PARAM_FIELDS()
}xparam_I16_t;
static_assert(	sizeof(xparam_U32_t) == sizeof(xparam_I16_t),
		"xparams module expects identical size of parameters");

typedef struct{
	uint16_t				value;
	uint16_t				_zero;
	_COMMON_PARAM_FIELDS()
}xparam_U16_t;
static_assert(	sizeof(xparam_U32_t) == sizeof(xparam_U16_t),
		"xparams module expects identical size of parameters");

typedef struct{
	int8_t					value;
	uint8_t					_zero1;
	uint16_t				_zero2;
	_COMMON_PARAM_FIELDS()
}xparam_I8_t;
static_assert(	sizeof(xparam_U32_t) == sizeof(xparam_I8_t),
		"xparams module expects identical size of parameters");

typedef struct{
	uint8_t					value;
	uint8_t					_zero1;
	uint16_t				_zero2;
	_COMMON_PARAM_FIELDS()
}xparam_U8_t;
static_assert(	sizeof(xparam_U32_t) == sizeof(xparam_U8_t),
		"xparams module expects identical size of parameters");

typedef struct{
	float					value;
	_COMMON_PARAM_FIELDS()
}xparam_FLOAT_t;
static_assert(	sizeof(xparam_U32_t) == sizeof(xparam_FLOAT_t),
		"xparams module expects identical size of parameters");

typedef struct{
	uint32_t				value;
	_COMMON_PARAM_FIELDS()
}xparam_STRING_t;
static_assert(	sizeof(xparam_U32_t) == sizeof(xparam_STRING_t),
		"xparams module expects identical size of parameters");

typedef struct{
    xparam_t* params;
    uint32_t  n_params;
}xparam_table_t;


typedef struct{
	uint32_t data;
	uint32_t key;
	uint32_t crc;
	uint32_t _reserved;
}xparam_store_t;

typedef struct{
	uint32_t 		magic;
	uint32_t		n_params;
	uint32_t		_reserved[2];
} xparam_header_t;


typedef struct __attribute__((packed)){
	xparam_header_t 		header;
	xparam_store_t			params[];
}xparam_img_t;


static_assert(	sizeof(xparam_header_t) == 16,
		"changing xparam_img_header_t size will break already written parameters");
static_assert(	sizeof(xparam_store_t)  == 16,
		"changing xparam_store_t size will break already written parameters");

#define XPARAM_IMAGE_SIZE(n) (sizeof(xparam_img_t) + (n * sizeof(xparam_store_t)))

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

/*
 * Following is some macro magic
 *
 * The general idea is to have a per project .h file that defines a macro which lists
 * all the parameters with initialization values (x-macro).
 * Macros bellow are then applied to each element of the parameter list.
 *
 * project_parms.h file uses DECLARE_PARAM macro to declare all parameters.
 * project_params.c file uses DEFINE_PARAM macro to define all parameters with
 * initialization values set in project_params.h file.
 *
 * This allows us to have a list of parameters with all required data defined in
 * one place (the project_params.h file).
 *
 */

/*
 * This macros get applied for each element in parameter list
 * _type can be one of: I8, U8, I16, U16, I32, U32, FLOAT, STRING
 */
#define DECLARE_PARAM(_type, _name, ...) xparam_##_type##_t _name;
#define DEFINE_PARAM(_type, _name, ...) \
		._name = {\
			.value_type = XPARAM_##_type, \
			.field_name = #_name #_type, \
			__VA_ARGS__ \
		},

/*
 * Macro to get count of parameters in a table
 */
#define XPARAM_COUNT(T) (sizeof(T) / sizeof(xparam_t))



/* Exported functions --------------------------------------------------------*/

/*
 * Get data blob from provided table, user needs to free returned pointer.
 * Could return NULL if malloc fails.
 */
uint8_t* xparam_table_to_blob(xparam_table_t* table);

/*
 * Load table values from data blob.
 * Return 1 if table was found in blob, 0 if no table is found.
 */
uint8_t  xparam_table_from_blob(xparam_table_t* table, uint8_t* buf);

/*
 * Copy parameter value converted to string to provided buffer.
 */
int 	 xparam_stringify(xparam_t* param, char* buf);

/*
 * Increment/decrement parameter by number of steps.
 * Returns 0 if change would violate parameter limits and parameter value didn't change.
 */
uint8_t  xparam_step_value(xparam_t* param, int16_t n_steps);

/*
 * Set numeric value
 * Returns 0 if change would violate parameter limits and parameter value didn't change.
 */
uint8_t  xparam_set_value(xparam_t* param, uint32_t value);

/*
 * Convert parameter table to json string.
 * User needs to free the returned pointer. Could return NULL if malloc fails.
 */
char*    xparam_table_to_json(xparam_table_t* table);

/*
 * Find xparam_t from table by name
 */
xparam_t* xparam_find_by_name(xparam_table_t* table, const char* name);

#ifdef __cplusplus
}
#endif

#endif /* INC_XPARAM_H_ */
