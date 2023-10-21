﻿#ifndef _LCD_CONTROLLER_MANAGER_H_
#define _LCD_CONTROLLER_MANAGER_H_

#include "lcd_manager.h"

/*LCD控制器的结构体*/
typedef struct lcd_controller{
	char* name;
	void (*init)(p_lcd_params plcdparams);
	void(*enable)(void);
	void(*disable)(void);
}lcd_controller, *p_lcd_controller;

/**********************************************************************
 * 函数名称： lcd_contoller_add
 * 功能描述： 添加lcd控制器结构体，提供给上层使用
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2020/02/26	     V1.0	  zh(angenao)	      创建
 ***********************************************************************/
void lcd_contoller_add(void);
/**********************************************************************
 * 函数名称： lcd_controller_init
 * 功能描述： 调用下层的LCD控制器初始化函数，提供给上层使用
 * 输入参数： lcd屏幕参数结构体
 * 输出参数： 无
 * 返 回 值： 正常返回0
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2020/02/26	     V1.0	  zh(angenao)	      创建
 ***********************************************************************/
int lcd_controller_init(p_lcd_params plcdparams);
/**********************************************************************
 * 函数名称： lcd_controller_enable
 * 功能描述： 调用下层的LCD控制器使能函数，提供给上层使用
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2020/02/26	     V1.0	  zh(angenao)	      创建
 ***********************************************************************/

void lcd_controller_enable(void);
/**********************************************************************
 * 函数名称： lcd_controller_disable
 * 功能描述： 调用下层的LCD控制器失能函数，提供给上层使用
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2020/02/26	     V1.0	  zh(angenao)	      创建
 ***********************************************************************/

void lcd_controller_disable(void);
/**********************************************************************
 * 函数名称： register_lcd_controller
 * 功能描述： 注册LCD控制器结构体，提供给上层调用
 * 输入参数： LCD控制器结构体
 * 输出参数： 无
 * 返 回 值： 正常返回0
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2020/02/26	     V1.0	  zh(angenao)	      创建
 ***********************************************************************/

int register_lcd_controller(p_lcd_controller plcdcon);
/**********************************************************************
 * 函数名称： select_lcd_controller
 * 功能描述： 选中指定LCD控制器结构体，提供给上层调用
 * 输入参数： lcd控制器的名称
 * 输出参数： 无
 * 返 回 值： 正常返回0
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2020/02/26	     V1.0	  zh(angenao)	      创建
 ***********************************************************************/

int select_lcd_controller(char *name);
/**********************************************************************
 * 函数名称： strcmp
 * 功能描述： 比较字符串函数
 * 输入参数： 1:字符串，2:字符串
 * 输出参数： 无
 * 返 回 值： 相同返回0
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2020/02/26	     V1.0	  zh(angenao)	      创建
 ***********************************************************************/

int strcmp(const char * cs,const char * ct);

#endif

