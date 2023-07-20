
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            klib.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"


/*======================================================================*
                               itoa
 *======================================================================*/
PUBLIC char * itoa(char * str, int num)/* 数字前面的 0 不被显示出来, 比如 0000B800 被显示成 B800 */
{
	char *	p = str;
	char	ch;
	int	i;
	int	flag = FALSE;

	*p++ = '0';
	*p++ = 'x';

	if(num == 0){
		*p++ = '0';
	}
	else{	
		for(i=28;i>=0;i-=4){
			ch = (num >> i) & 0xF;
			if(flag || (ch > 0)){
				flag = TRUE;
				ch += '0';
				if(ch > '9'){
					ch += 7;
				}
				*p++ = ch;
			}
		}
	}

	*p = 0;

	return str;
}


/*======================================================================*
                               disp_int
 *======================================================================*/
PUBLIC void disp_int(int input)
{
	char output[16];
	itoa(output, input);
	disp_str(output);
}

/*======================================================================*
                               delay
 *======================================================================*/
PUBLIC void delay(int time)
{
	int i, j, k;
	for(k=0;k<time;k++){
		/*for(i=0;i<10000;i++){	for Virtual PC	*/
		for(i=0;i<10;i++){/*	for Bochs	*/
			for(j=0;j<10000;j++){}
		}
	}
}


/*======================================================================*
                            clear_screen
	added by mingxuan 2019-5-13
 *======================================================================*/
PUBLIC void clear_screen()
{
	delay(200);
	
	int i;
	disp_pos = 0;
  for(i = 0; i < DISP_POS_MAX; i++) {
		disp_str(" ");
	}
	disp_pos = 0;
}

/*======================================================================*
                        disp_color_str_clear_screen
	added by mingxuan 2019-5-13
 *======================================================================*/
PUBLIC void	disp_color_str_clear_screen(char * info, int color)
{	
	// int iflag = Begin_Int_Atomic();
	if(disp_pos >= DISP_POS_MAX) {
		clear_screen();
	}
	disp_color_str(info, color);
	// End_Int_Atomic(iflag);
}

/*======================================================================*
                        disp_str_clear_screen
	added by mingxuan 2019-5-13
 *======================================================================*/
PUBLIC void	disp_str_clear_screen(char * info)
{
	// int iflag = Begin_Int_Atomic();
	if(disp_pos >= DISP_POS_MAX) {
		clear_screen();
	}
	disp_str(info);	
	// End_Int_Atomic(iflag);
}

/*======================================================================*
                               Atomic
	added by mingxuan 2019-5-13
 *======================================================================*/

PRIVATE int Interrupts_Enabled() {
	u32 eflags = Get_Current_EFLAGS();
	return ((eflags & (1<<9)) != 0);
}

PUBLIC int Begin_Int_Atomic() {
	int enabled = Interrupts_Enabled();
	if(enabled) Disable_Interrupts();
	return enabled;
}

PUBLIC void End_Int_Atomic(int iflag) {
	if(iflag) {
		Enable_Interrupts();
	}
}