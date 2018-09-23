#include <stdio.h>
#include <stdint.h>
#include "string.h"
#include "ST7735.h"
#include "PLL.h"
#include "fixed.h"
#include "tm4c123gh6pm.h"
int32_t minx;
int32_t maxx;
int32_t miny;
int32_t maxy;


void ST7735_sDecOut2(int32_t n){
	
	int32_t n_unsigned;
	if(n > 0) n_unsigned = n;
	else n_unsigned = -n;
	char output_str[7]= " **.**";
	if(n < 0) output_str[0] = '-';
	
	if(n <= 9999 && n >= -9999)
	{
		output_str[5] = (n_unsigned%10) + '0';
		n_unsigned = n_unsigned/10;
		output_str[4] = (n_unsigned%10) + '0';
		n_unsigned = n_unsigned/10;
		output_str[2] = (n_unsigned%10) + '0';
		n_unsigned = n_unsigned/10;
		
	  if(n_unsigned%10 == 0 && n < 0) 
		{
			output_str[1] = '-';
			output_str[0] = ' ';
		}
		else if(n_unsigned%10 == 0 && n >= 0)
			output_str[1] = ' ';
		else 
			output_str[1] = (n_unsigned%10) + '0';

	}
	
	ST7735_OutString(output_str);
	
}

void ST7735_uBinOut6(uint32_t n){
	char output_str[7]= "***.**";
	uint32_t result = n*100/64;
	uint32_t roundup = (n*1000/64)%10;
	if(n < 64000)
	{
		if(roundup >= 5)output_str[5] = (result%10) + '0'+1;
		else output_str[5] = (result%10) + '0';
		result = result/10;
		output_str[4] = (result%10) + '0';
		result = result/10;
		output_str[2] = (result%10) + '0';
		result = result/10;
		output_str[1] = (result%10 == 0) ? ' ' : (result%10) + '0';
		result = result/10;n = n/10;
		output_str[0] = (result%10 == 0) ? ' ' : (result%10) + '0';
		if(output_str[0] != ' ' && output_str[1] == ' ')output_str[1] = '0'; 
	}
	ST7735_OutString(output_str);
}
void ST7735_XYplotInit(char *title, int32_t minX, int32_t maxX, int32_t minY, int32_t maxY){
 // ST7735_OutString(title);
	//ST7735_PlotClear(0,4095);
	minx = minX;
	maxx = maxX;
	miny = minY;
	maxy = maxY;
}
void ST7735_XYplot(uint32_t num, int32_t bufX[], int32_t bufY[]){
	int i,j,x,y;
	for(int k =0; k<num; k++){
		x=bufX[k];
		y=bufY[k];
		i = (127*(x-minx))/(maxx-minx); 
		j = 32+(127*(maxy-y))/(maxy-miny);
		ST7735_DrawPixel(i,   j,   ST7735_MAGENTA);
		ST7735_DrawPixel(i+1, j,   ST7735_MAGENTA);
		ST7735_DrawPixel(i,   j+1, ST7735_MAGENTA);
		ST7735_DrawPixel(i+1, j+1, ST7735_MAGENTA);
		ST7735_DrawPixel(i, j, ST7735_MAGENTA);
	} 
}
