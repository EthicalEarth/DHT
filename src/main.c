/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * main.c
 * Copyright (C) 2015 Andrew Ivanov <search_terminal@mail.ru>
 * 
 * DHT is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * DHT is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//##define BCM2708_PERI_BASE        0x20000000
//#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */

//################################################################################​###########
//########################### Geändert zur verwendung auf dem Cubietruck
//################################################################################​##########
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include "gpio_lib.h"
#define MAXTIMINGS 500 //100 Задает значение тайминга
#define PG3  SUNXI_GPG(3)
int dhtpin = PG3;//##  Вывод к которому подключен датчик(в .fex файле)

//#define DEBUG

#define DHT11 11
#define DHT22 22
#define AM2302 22

int readDHT(int type, int pin);
int main(int argc, char **argv)

{
if(SETUP_OK!=sunxi_gpio_init()){

        printf("Ошибка инициализации!\n");

return -1;

}

  if (argc != 2) {

    printf("Укажите модель датчика: %s [11|22|2302]#\n", argv[0]);

    //printf("example: %s 2302 4 - Read from an AM2302 connected to GPIO #4\n", argv[0]);

    return 2;

  }

  int type = 0;
  if (strcmp(argv[1], "11") == 0) type = DHT11;
  if (strcmp(argv[1], "22") == 0) type = DHT22;
  if (strcmp(argv[1], "2302") == 0) type = AM2302;
  if (type == 0) {
    printf("Select 11, 22, 2302 as type!\n");
    return 3;
  }
//atoi(argv[2]);
// if (dhtpin <= 0) {
//    printf("Please select a valid GPIO pin #\n");
//    return 3;
//  }
  readDHT(type, dhtpin);
  return 0;
} // main

  int bits[250], data[100];
  int bitidx = 0;
  int readDHT(int type, int pin) {
  int counter = 0;
  int laststate = HIGH;
  int j=0;


// Переключаем вывод в режим выхода
  sunxi_gpio_set_cfgpin(pin,OUTPUT);
  sunxi_gpio_output(pin,HIGH);
  usleep(500000);//500000
  sunxi_gpio_output(pin,LOW);
  usleep(19000);//18000
  sunxi_gpio_output(pin,HIGH);
  usleep(5);//18
  sunxi_gpio_set_cfgpin(pin,INPUT);
  data[0] = data[1] = data[2] = data[3] = data[4] = 0;
	
//////////////////////////////////////////////////////////////////////////////
  int block;///Счетчик попыток ожидания нуля./////////////////////////////////
  block = 0;///Без этой части, если датчик не успевал вообще//////////////////
  /////////////сработать, или срабатывал быстрее чем мы начинаем//////////////
  /////////////ждать нуля, то так как на датчике в режиме/////////////////////
  /////////////ожидания всегда единица, цикл становился бесконечным.//////////
//////////////////////////////////////////////////////////////////////////////
  while (sunxi_gpio_input(pin) == 1) {   //Если на выходе 1 то ожидаем 0
       printf("Ожидаем нуля от датчика \n" );
//////////////////////////////////////////////////////////////////////////////
  block++;//Блокировка от бесконечного цикла//////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
  usleep(1);//Засыпаем на микросекунду////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
  if (block ==10)/////////////////////////////////////////////////////////////
	break;////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
	}
// Читаем данные!
int i;
for (i=0; i< MAXTIMINGS; i++) {
    counter = 0;
    while (sunxi_gpio_input(pin) == laststate) {
    counter++;
//////////////////////////////////////////////////////////////////////////////
//usleep(1); Задержка в одну микросекунду приводит к//////////////////////////
// потере данных, не использовать!////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
	if (counter == 2000)//1000
      break;
   }
    laststate = sunxi_gpio_input(pin);
    if (counter == 2000) break; //1000
    bits[bitidx++] = counter;
    if ((i>3) && (i%2 == 0)) {
      // Кладем каждый бит в байт переменной для хранения
      data[j/8] <<= 1;
      if (counter > 200)//200
        data[j/8] |= 1;
      j++;
    }
}

#ifdef DEBUG
for ( i=3; i<bitidx; i+=2) {
    printf("bit %d: %d\n", i-3, bits[i]);
    printf("bit %d: %d (%d)\n", i-2, bits[i+1], bits[i+1] > 200);
}

#endif


/////////////////////////////////////
if (j<=39){ //Если бит меньше чем 39, то данные получены не полностью
printf("Данные не могут быть считаны, возможно компьютер сильно загружен \n");
printf("и интерфейс работы с датчиком не работает на должной скорости\n");
//return 1;  // и нет смысла дальнейших действий
wait(2000);
goto LOOP;
}
////////////////////////////////////

	
  printf("Data (%d): 0x%x 0x%x 0x%x 0x%x 0x%x\n", j, data[0], data[1], data[2], data[3], data[4]);

  if ((j >= 39) &&  // if ((j >= 39) &&
      (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) ) {
     // Вау!
     if (type == DHT11)
    printf("Temp = %d *C, Hum = %d \%\n", data[2], data[0]);
     if (type == DHT22) {
    float f, h;
    h = data[0] * 256 + data[1];
    h /= 10;
    f = (data[2] & 0x7F)* 256 + data[3];
        f /= 10.0;
        if (data[2] & 0x80)  f *= -1;
    printf("Temp =  %%.1f *C, Hum = %%.1f \%%\n", f, h);
    }

    return 1;

  }
  return 0;
/////////////////////////////////////////////
LOOP:   {


	if(SETUP_OK!=sunxi_gpio_init()){

        printf("Ошибка инициализации!\n");

  return -1;

   }
	
  readDHT(type, dhtpin); 
	
  return 0;

}
/////////////////////////////////////////////
}

