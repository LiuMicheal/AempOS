
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "string.h"
//#include "proc.h"
#include "global.h"
#include "keyboard.h"


/*======================================================================*
                           task_tty
 *======================================================================*/
//是一个内核进程
PUBLIC void task_tty()
{
	while (1) {
		keyboard_read();      //deleted by mingxuan 2019-3-13
                
                //隔一段时间检测一次负载均衡情况
                //load_balance(); //added by mingxuan 2019-4-3

                //disp_str("T ");		//added by mingxuan 2019-3-8
                int i;
                for(i=10000000; i>0; i--) ;
		//milli_delay(100);	//added by mingxuan 2019-3-8
	}
}

/*======================================================================*
				in_process
 *======================================================================*/
/*
PUBLIC void in_process(u32 key)
{
        char output[2] = {'\0', '\0'};

        if (!(key & FLAG_EXT)) {
                output[0] = key & 0xFF;
                disp_str(output);
        }
}
*/

PUBLIC void in_process(u32 key)
{
        char output[2] = {'\0', '\0'};

        if (!(key & FLAG_EXT)) {
                output[0] = key & 0xFF;
                disp_str(output);

		disable_int();
		out_byte(CRTC_ADDR_REG, CURSOR_H);
		out_byte(CRTC_DATA_REG, ((disp_pos/2)>>8)&0xFF);
		out_byte(CRTC_ADDR_REG, CURSOR_L);
		out_byte(CRTC_DATA_REG, (disp_pos/2)&0xFF);
		enable_int();
        }
        else {
                int raw_code = key & MASK_RAW;
                switch(raw_code) {
                case UP:
                        if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
                                disable_int();
                                out_byte(CRTC_ADDR_REG, START_ADDR_H);
                                out_byte(CRTC_DATA_REG, ((80*15) >> 8) & 0xFF);
                                out_byte(CRTC_ADDR_REG, START_ADDR_L);
                                out_byte(CRTC_DATA_REG, (80*15) & 0xFF);
                                enable_int();
                        }
                        break;
                case DOWN:
                        if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
				/* Shift+Down, do nothing */
                        }
                        break;
                default:
                        break;
                }
        }
}