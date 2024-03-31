#include <types.h>
#include <gdt.h>
#include <interrupts.h>
#include <keyboard.h>
#include <driver.h>
#include <mouse.h>
#include <multitasking.h>
#include <syscalls.h>
void printf(const int8_t * str)
{
    static int16_t * VideoMemory = (short*)0xb8000;
    static int8_t x =0, y=0;
    for(int32_t i=0;str[i]!='\0';i++)
    {   
        switch (str[i])
        {
        case '\n':
            ++y;
            x=0;
            break;
        
        default:
            VideoMemory[80 * y+ x] =(VideoMemory[80 *y+x] & 0xFF00) | str[i];
            ++x;
            break;
        }
        if(x>=80)
        {
            ++y;
            x=0;
        }
        if(y>=25)
        {
            for (y=0; y < 25; y++)
                for(x=0;x<80;++x)
                    VideoMemory[80 * y+ x] =(VideoMemory[80 *y+x] & 0xFF00) | str[i];
                    x=0;
                    y=0;
        }
        
    }
}
void printfHex(const uint8_t num)
{
	uint8_t c = num;
	static char msg[3] = {'0'};
	const char * hex = "0123456789ABCDEF";
	msg[0] = hex[(c >> 4) & 0x0F];
	msg[1] = hex[c & 0x0F];
	msg[2] = '\0';
	printf(msg);
}

void sysprintf(const int8_t * str)
{
	__asm__("int $0x80" : : "a" (4), "b"(str));//通过int80内嵌汇编进入内核态，安装0x80的中断
}
void systime()
{
	__asm__("int $0x80": : "a"(1));
}

int8_t strcmp(const int8_t * src, const int8_t * dest)
{
	while((*src != '\0') &&(*src == *dest))
	{
		++src;
		++dest;
	}
	return *src - *dest;
}

void simpelShell(const char c, KeyboardDriver * pKeyDriver)
{
	if (c == '\n')
	{
		char cmd[256] = {0};
		pKeyDriver->get_buffer(cmd);

		if(strcmp(cmd,"time")==0)
			systime();
		else if (cmd[0]!= '\0')
			sysprintf("unknown command");

		sysprintf("Zhang:>");

	}
}

void taskA()
{
	while(true)
		sysprintf("B ");
}

void taskB()
{
	while(true)
		sysprintf("A ");
}

void time()
{
	Port8Bit out(0x70),in(0x71);
	uint8_t idx = 4;//小时
	out.Write(idx);
	idx = in.Read();
	printfHex(idx);
	printf(":");

	idx = 2;//分钟
	out.Write(idx);
	idx = in.Read();
	printfHex(idx);
	printf(":");

	idx = 0;//秒
	out.Write(idx);
	idx = in.Read();
	printfHex(idx);
	printf("\n");
}

class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
	void OnKeyDown(int8_t c)
	{
		pDriver->put_buffer(c);
		char msg[2] = {'\0'};
		msg[0] = c;
		printf(msg);
	}
};


class MouseToConsole: public MouseEventHandler
{
	uint16_t x, y;
	static uint16_t *VideoMemory;
public:
	MouseToConsole() : x(40), y(12)
	{
		VideoMemory[80 * y + x] = ((VideoMemory[80 * y + x] & 0xF000) >> 4) |
								  ((VideoMemory[80 * y + x] & 0xF000) << 4) |
								  ((VideoMemory[80 * y + x] & 0xF000));
	}							  

	void OnMouseMove(int16_t xoffset, int16_t yoffset)
	{
		VideoMemory[80 * y + x] = ((VideoMemory[80 * y + x] & 0xF000) >> 4) |
								  ((VideoMemory[80 * y + x] & 0x0F00) << 4) |
								  ((VideoMemory[80 * y + x] & 0x00FF));
		x += xoffset;
		if(x < 0) x = 0;
		if(x >= 80) x = 79;
		y += yoffset;
		if(y < 0) y = 0;
		if(y >= 25) y = 24;

		VideoMemory[80 * y + x] = ((VideoMemory[80 * y + x] & 0xF000) >> 4) |
								  ((VideoMemory[80 * y + x] & 0x0F00) << 4) |
								  ((VideoMemory[80 * y + x] & 0x00FF));				
	}	

	void OnMouseDown(uint8_t button)
	{
		VideoMemory[80 * y + x] = ((VideoMemory[80 * y + x] & 0xF000) >> 4) |
								  ((VideoMemory[80 * y + x] & 0x0F00) << 4) |
								  ((VideoMemory[80 * y + x] & 0x00FF));								  					
	}				
	
	void OnMouseUp(uint8_t button)
	{
		VideoMemory[80 * y + x] = ((VideoMemory[80 * y + x] & 0xF000) >> 4) |
								  ((VideoMemory[80 * y + x] & 0x0F00) << 4) |
								  ((VideoMemory[80 * y + x] & 0x00FF));
	}
};

uint16_t * MouseToConsole::VideoMemory = (uint16_t*)0xB8000;



typedef void (*constructor)();
extern constructor start_ctors;
extern constructor end_ctors;

 extern "C" void callConstructors()
{
    for(constructor * i = &start_ctors;i!=&end_ctors;++i)
    (*i)();
}
 extern "C" void kernelMain(void * multiboo_structure,int32_t magicnumber)
{
    printf("Hello MyOS World!\n");
    printf("Hello MyOS World!\n");

    GlobalDescriptorTable gdt;
	TaskManager taskManager;
	//Task task1(&gdt, taskA);
	//Task task2(&gdt, taskB);
	//taskManager.AddTask(&task1);
	//taskManager.AddTask(&task2);

    InterruptManager interrupts(&gdt, &taskManager);

	SyscallHandler syscalls(&interrupts);
	DriverManager driverManager;

	PrintfKeyboardEventHandler kbhandler;
	KeyboardDriver keyboard(&interrupts, &kbhandler);
	
	//MouseToConsole mousehandler;
	//MouseDriver mouse(&interrupts, &mousehandler);

	driverManager.AddDriver(&keyboard);
	//driverManager.AddDriver(&mouse);
	driverManager.Activate();


    interrupts.Activate();

	sysprintf("Zhang>");
    while (1);
}