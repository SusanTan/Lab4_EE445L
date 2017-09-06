/*
 * clockface.c
 *
 *  Created on: Sep 14, 2018
 *      Author: drew
 */


#include "clockface.h"
#include "font8x8_basic.h"
#include "ST7735.h"
#include "fixed.h"

#define W 0xffff
#define B 0x0000
#define P 0xF81F

static uint16_t draw_char_buf[64] = {0};

//x,y left top
//not thread safe
void drawChar_colorful(int16_t x, int16_t y, char c, uint16_t background) {
    const char* bitmap = font8x8_basic[c];
    int16_t ii,jj, kk = 56;
    for (ii=0;ii<8;ii++) {
        for (jj=0;jj<8;jj++) {
            if (*bitmap & (1 << jj))
                draw_char_buf[kk++] = 0xffff;
            else
                draw_char_buf[kk++] = background;
        }
        kk -= 16; //we have to draw it upside down
        bitmap++;
    }
    ST7735_DrawBitmap(x, y+7, draw_char_buf, 8, 8);
}

void drawChar(int16_t x, int16_t y, char c) {
    drawChar_colorful(x, y, c, 0x0000);
}

void drawChar2(int16_t x, int16_t y, char c) {
    drawChar_colorful(x, y, c, ST7735_MAGENTA);
}

static const int32_t TimeX[12] = {63, 85, 98, 105, 98, 85, 63, 38, 22, 16, 22, 38};
static const int32_t TimeY[12] = {48, 55, 69,  90, 112, 129, 135, 129, 110, 90, 69, 55};

void Clockface_Init(void) {
    ST7735_InitR(INITR_REDTAB);
}

void Clockface_Draw(void) {
    ST7735_FillScreen(ST7735_BLACK);
    ST7735_FillRect(0, 0, 128, 32, ST7735_MAGENTA);
		ST7735_XYplotInit("InnerCircle",-2500, 2500, -2500, 2500);
    ST7735_XYplot(180,(int32_t *)CircleXbuf,(int32_t *)CircleYbuf);
		ST7735_XYplotInit("OuterCircle",-2100, 2100, -2100, 2100);
    ST7735_XYplot(180,(int32_t *)CircleXbuf,(int32_t *)CircleYbuf);
    ST7735_DrawBitmap(60, 83+14, circle, 8, 8);//60, 83+16, circle, 8, 8
   
		drawChar(TimeX[0]-4,TimeY[0], '1');
		drawChar(TimeX[0]+4,TimeY[0], '2');
	
	  drawChar(TimeX[1],TimeY[1], '1');
	  drawChar(TimeX[2],TimeY[2], '2');
	  drawChar(TimeX[3],TimeY[3], '3');
	  drawChar(TimeX[4],TimeY[4], '4');
	  drawChar(TimeX[5],TimeY[5], '5');
	  drawChar(TimeX[6],TimeY[6], '6');
	  drawChar(TimeX[7],TimeY[7], '7');
	  drawChar(TimeX[8],TimeY[8], '8');
	  drawChar(TimeX[9],TimeY[9], '9');
	
	  drawChar(TimeX[10],TimeY[10], '1');
		drawChar(TimeX[10]+8,TimeY[10], '0');
		
	  drawChar(TimeX[11],TimeY[11], '1');
		drawChar(TimeX[11]+8,TimeY[11], '1');
	
}

uint16_t prevDigitalHour = 100;
uint16_t prevDigitalMinute = 100;
uint16_t prevDigitalSecond = 100;
#define DigRow 10

void Clockface_setDigitalTime(uint32_t seconds) {
    uint16_t hour = seconds / 3600;
    uint16_t minute = (seconds - hour*3600) / 60;
    uint16_t second = seconds - hour*3600 - minute*60;

    if (hour != prevDigitalHour) {
        if (hour < 10) {
            drawChar2(32,DigRow,'0');
            drawChar2(40,DigRow,(char)hour | 0x30);
        } else {
            drawChar2(32,DigRow,(char)(hour / 10) | 0x30);
            drawChar2(40,DigRow,(char)(hour % 10) | 0x30);
        }
    }
    drawChar2(48,DigRow,':');
    if (minute != prevDigitalMinute) {
        if (minute < 10) {
            drawChar2(56,DigRow,'0');
            drawChar2(64,DigRow,(char)minute | 0x30);
        } else {
            drawChar2(56,DigRow,(char)(minute / 10) | 0x30);
            drawChar2(64,DigRow,(char)(minute % 10) | 0x30);
        }
    }
    drawChar2(72,DigRow,':');
    if (second != prevDigitalSecond) {
        if (second < 10) {
            drawChar2(80,DigRow,'0');
            drawChar2(88,DigRow,(char)second | 0x30);
        } else {
            drawChar2(80,DigRow,(char)(second / 10) | 0x30);
            drawChar2(88,DigRow,(char)(second % 10) | 0x30);
        }
    }
}

//graphical time stuff
uint32_t previous_secondX = CenterX;
uint32_t previous_secondY = CenterY;
uint32_t previous_minuteX = CenterX;
uint32_t previous_minuteY = CenterY;
uint32_t previous_hourX = CenterX;
uint32_t previous_hourY = CenterY;
uint32_t secondX;
uint32_t secondY;
uint32_t minuteX;
uint32_t minuteY;
uint32_t hourX;
uint32_t hourY;

void Clockface_setGraphicalTime(uint32_t seconds) {

    secondX = (127*(CircleXbuf[(223-(seconds*3)%180)%180]+3500))/7000;
    secondY = 32+(127*(3500 - CircleYbuf[(223-(seconds*3)%180)%180]))/7000;
    minuteX = (127*(CircleXbuf[(223-(seconds/20)%180)%180]+4000))/8000;
    minuteY = 32+(127*(4000 - CircleYbuf[(223-(seconds/20)%180)%180]))/8000;
    hourX = (127*(CircleXbuf[(223-(seconds/240)%180)%180]+5000))/10000;
    hourY = 32+(127*(5000 - CircleYbuf[(223-(seconds/240)%180)%180]))/10000;

    ST7735_Line(CenterX, CenterY, minuteX, minuteY, ST7735_MAGENTA);
    ST7735_Line(CenterX, CenterY, secondX, secondY, ST7735_MAGENTA);
    ST7735_Line(CenterX, CenterY, hourX, hourY, ST7735_MAGENTA);

    if((previous_minuteX != minuteX) || (previous_minuteY != minuteY)){
        ST7735_Line(CenterX, CenterY, previous_minuteX, previous_minuteY, ST7735_BLACK);
        previous_minuteX = minuteX;
        previous_minuteY = minuteY;
        ST7735_DrawBitmap(60, 83+14, circle, 8, 8);
    }
    if((previous_secondX != secondX) || (previous_secondY != secondY)){
        ST7735_Line(CenterX, CenterY, previous_secondX, previous_secondY, ST7735_BLACK);
        previous_secondX = secondX;
        previous_secondY = secondY;
        ST7735_DrawBitmap(60, 83+14, circle, 8, 8);
    }
    if((previous_hourX != hourX) || (previous_hourY != hourY)){
        ST7735_Line(CenterX, CenterY, previous_hourX, previous_hourY, ST7735_BLACK);
        previous_hourX = hourX;
        previous_hourY = hourY;
        ST7735_DrawBitmap(60, 83+14, circle, 8, 8);
    }
}

#define AlarmRow 2

void Clockface_setAlarmOn(bool on) {
    if (on) {
        drawChar2(32, AlarmRow, 'A');
        drawChar2(40, AlarmRow, 'l');
        drawChar2(48, AlarmRow, 'a');
        drawChar2(56, AlarmRow, 'r');
        drawChar2(64, AlarmRow, 'm');
        drawChar2(72, AlarmRow, ' ');
        drawChar2(80, AlarmRow, 'O');
        drawChar2(88, AlarmRow, 'n');
    } else {
        ST7735_FillRect(32,AlarmRow,64,8,ST7735_MAGENTA);
    }
}

#define SetTimeRow 66

void Clockface_DrawClockSet(void) {
    ST7735_FillScreen(ST7735_BLACK);
    drawChar(32, SetTimeRow, 'S');
    drawChar(40, SetTimeRow, 'e');
    drawChar(48, SetTimeRow, 't');
    drawChar(56, SetTimeRow, ' ');
    drawChar(64, SetTimeRow, 'T');
    drawChar(72, SetTimeRow, 'i');
    drawChar(80, SetTimeRow, 'm');
    drawChar(88, SetTimeRow, 'e');
}

#define SetTimeNumRow 76

void Clockface_setClockSetTime(uint32_t hour, uint32_t minute) {
    if (hour < 10) {
        drawChar(44,SetTimeNumRow,'0');
        drawChar(52,SetTimeNumRow,(char)hour | 0x30);
    } else {
        drawChar(44,SetTimeNumRow,(char)(hour / 10) | 0x30);
        drawChar(52,SetTimeNumRow,(char)(hour % 10) | 0x30);
    }
    drawChar(60,SetTimeNumRow,':');
    if (minute < 10) {
        drawChar(68,SetTimeNumRow,'0');
        drawChar(76,SetTimeNumRow,(char)minute | 0x30);
    } else {
        drawChar(68,SetTimeNumRow,(char)(minute / 10) | 0x30);
        drawChar(76,SetTimeNumRow,(char)(minute % 10) | 0x30);
    }
}

void Clockface_DrawAlarmSet(void) {
    ST7735_FillScreen(ST7735_BLACK);
    drawChar(28, SetTimeRow, 'S');
    drawChar(36, SetTimeRow, 'e');
    drawChar(44, SetTimeRow, 't');
    drawChar(52, SetTimeRow, ' ');
    drawChar(60, SetTimeRow, 'A');
    drawChar(68, SetTimeRow, 'l');
    drawChar(76, SetTimeRow, 'a');
    drawChar(84, SetTimeRow, 'r');
    drawChar(92, SetTimeRow, 'm');
}
