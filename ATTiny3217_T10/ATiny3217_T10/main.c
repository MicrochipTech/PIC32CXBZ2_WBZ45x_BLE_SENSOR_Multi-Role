#include <atmel_start.h>
#include "led_driver.h"
#include <math.h>
#include <string.h>
extern volatile uint8_t measurement_done_touch;

int button_value,on_off_button=0,i=0;
int prev_button_val[4],status[4],curr_button_val[4];                        //Button variables
int scroller[3],SliderData[3],prev_slider_val[3],curr_slider_val[3];       //Slider variables
int button_led[4]={128,64,32,16};
int slider_led[4]={128,224,252,255};
uint16_t rgb_data;
char read_data_buffer[15];


#define LED_OFF 0
#define COMPARE "B_On:"
#define COMPARE_SIZE 5


int main(void)
{
	/* Initializes MCU, drivers and middle ware */
	atmel_start_init();
	#if ENABLE_LED== 1u
	init_led_driver();
	led_reset();
	#endif

	/* Replace with your application code */
	while (1)
	{
		touch_process();
		rgb_button(3);                   //ON_OF BUTTON
		rgb_button(0);					 //RED BUTTON
		rgb_button(1);                   //GREEN BUTTON
		rgb_button(2);                   //BLUE BUTTON
		read_data();
		
	}
}

void read_data(void)
{
	char rx_data='0';
	while(USART_0_is_rx_ready())
	{
		rx_data= USART_0_get_data();
		if(rx_data=='B')
		{
			i=0;
		}
		else if(rx_data=='\n')
		{
			write_data(read_data_buffer,5,0);
		}
		read_data_buffer[i]=rx_data;
		i++;
	}
}

void write_data(char *buffer,int num_char, int position)                             // num_char:No of characters to be copied; position:From which position to copy
{
	uint8_t  r,g,b,button_state;
	uint16_t rgb_val;
	if((strncmp(buffer,COMPARE,COMPARE_SIZE))==0)
	{
		if ((num_char+position-1) <= strlen(buffer))
		{
			strcpy(&buffer[position-1],&buffer[num_char+position-1]);
		}
		rgb_val = strtoul(buffer, &buffer, 16);
		r=((rgb_val & 0xF000)>>12);
		g=((rgb_val & 0x0F00)>>8);
		b=((rgb_val & 0x00F0)>>4);
		button_state=(rgb_val & 0x000F);
		rgb_data=((button_state<<12)|(r<<8)|(g<<4)|b);
		scroller[0]=slider_led[r-1];
		scroller[1]=slider_led[g-1];
		scroller[2]=slider_led[b-1];
		if(button_state==5)
		{
			led_gpio_update(button_led[3],LED_BUTTON);
			led_gpio_update(LED_OFF,LED_SCROLLER);
		}
		else
		{
			led_gpio_update(LED_OFF,LED_BUTTON);
			led_gpio_update(LED_OFF,LED_SCROLLER);
		}
	}
}



void rgb_button(int i)
{
	uint8_t b1 = get_sensor_state(i) & KEY_TOUCHED_MASK;
	if(b1!=0)
	{
		button_value = i+1;
		prev_button_val[i]=curr_button_val[i];
		curr_button_val[i]=1;
	}
	else
	{
		curr_button_val[i]=0;
	}
	if((button_value==1 && i==0)|(button_value==2 && i==1)|(button_value==3 && i==2))
	{
		slider(i);
	}
	if((prev_button_val[i]==0 && curr_button_val[i]==1))
	{
		if(button_value==4 && on_off_button==1)
		{
			on_off_button=0;
		}
		else if(button_value==4 && on_off_button==0)
		{
			on_off_button=1;
		}
		else
		{
			on_off_button=1;
			status[3]=1;
		}
		touch_detected(i);
	}
	else if((prev_button_val[i]==1 && curr_button_val[i]==0))
	{
		prev_button_val[i]=0;
		curr_button_val[i]=0;     //touch released
	}
}

void touch_detected(int i)
{
	if(status[i]==1)
	{
		if(on_off_button==0 && status[3]==1)
		{
			rgb_data=rgb_data & 0x0FFF;
			rgb_data=rgb_data|(1<<12);
			printf("B_On:0x%3x\r\n",rgb_data);
			led_gpio_update(LED_OFF,LED_BUTTON);
			led_gpio_update(LED_OFF,LED_SCROLLER);
		}
		else
		{
			led_gpio_update(button_led[3],LED_BUTTON);
			led_gpio_update(LED_OFF,LED_SCROLLER);
		}
		status[i]=0;
	}
	else
	{
		rgb_data=rgb_data & 0x0FFF;
		rgb_data=rgb_data|((i+2)<<12);
		printf("B_On:0x%3x\r\n",rgb_data);
		if(i<3)
		{
			led_gpio_update(button_led[i]|button_led[3],LED_BUTTON);
			led_gpio_update(scroller[i], LED_SCROLLER);
		}
		else
		{
			led_gpio_update(button_led[i],LED_BUTTON);
			led_gpio_update(LED_OFF,LED_SCROLLER);
		}
		for(int a=0;a<3;a++)                  //To switch between buttons
		{
			if(status[a]==1 && a!=i)
			{
				status[a]=0;
			}
		}
		status[i]=1;
	}
}

void slider(int i)
{
	int x=0;
	prev_slider_val[i]=curr_slider_val[i];
	slider_status(i);
	if(prev_slider_val[i]!=curr_slider_val[i])
	{
		if(button_value==0x01 && i==0)
		{
			rgb_data=rgb_data & 0xF0FF;
			rgb_data=rgb_data|((curr_slider_val[i])<<8);
		}
		else if(button_value==0x02 && i==1)
		{
			rgb_data=rgb_data & 0xFF0F;
			rgb_data=rgb_data|((curr_slider_val[i])<<4);
		}
		else if(button_value==0x03 && i==2)
		{
			rgb_data=rgb_data & 0xFFF0;
			rgb_data=rgb_data|((curr_slider_val[i]));
		}
		if(SliderData[i]!=(x))
		{
			printf("B_On:0x%3x\r\n",rgb_data);
			x=SliderData[i];
		}
	}
}


void slider_status(int i)
{
	for (uint8_t k=4, j=1; k<8; k++,j++)
	{
		if(0u!=(get_sensor_state(k) & KEY_TOUCHED_MASK))
		{
			curr_slider_val[i]=j;
			SliderData[i]=j;
			scroller[i]=slider_led[j-1];
			led_gpio_update(slider_led[j-1],LED_SCROLLER);
		}
	}
}

