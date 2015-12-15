#include "stdio.h"
#include "keyboard_map.h"


// VGA 的显示缓冲的起点是 0xB8000
static uint16_t *video_memory = (uint16_t *)0xB8000;


// 屏幕"光标"的坐标
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static uint8_t is_shift_down = 0;
static uint8_t font_size = 80;

static void move_cursor()
{
    // 屏幕是 80 字节宽
    uint16_t cursorLocation = cursor_y * font_size + cursor_x;
    
    // 在这里用到的两个内部寄存器的编号为14与15，分别表示光标位置
    // 的高8位与低8位。

    outb(0x3D4, 14);                    // 告诉 VGA 我们要设置光标的高字节
    outb(0x3D5, cursorLocation >> 8);   // 发送高 8 位
    outb(0x3D4, 15);                    // 告诉 VGA 我们要设置光标的低字节
    outb(0x3D5, cursorLocation);        // 发送低 8 位
}

void console_clear()
{
    uint8_t attribute_byte = (0 << 4) | (15 & 0x0F);
    uint16_t blank = 0x20 | (attribute_byte << 8);

    int i;
    for (i = 0; i < font_size * 25; i++) {
          video_memory[i] = blank;
    }

    cursor_x = 0;
    cursor_y = 0;
    move_cursor();
}


static void scroll()
{
    // attribute_byte 被构造出一个黑底白字的描述格式
    uint8_t attribute_byte = (0 << 4) | (15 & 0x0F);
    uint16_t blank = 0x20 | (attribute_byte << 8);  // space 是 0x20

    // cursor_y 到 25 的时候，就该换行了
    if (cursor_y >= 25) {
        // 将所有行的显示数据复制到上一行，第一行永远消失了...
        int i;
        
        for (i = 0 * font_size; i < 24 * font_size; i++) {
              video_memory[i] = video_memory[i+80];
        }

        // 最后的一行数据现在填充空格，不显示任何字符
        for (i = 24 * font_size; i < 25 * font_size; i++) {
              video_memory[i] = blank;
        }
        
        // 向上移动了一行，所以 cursor_y 现在是 24
        cursor_y = 24;
    }
}


void putch_color(char c, real_color_t back, real_color_t fore)
{
    uint8_t back_color = (uint8_t)back;
    uint8_t fore_color = (uint8_t)fore;

    uint8_t attribute_byte = (back_color << 4) | (fore_color & 0x0F);
    uint16_t attribute = attribute_byte << 8;

    // 0x08 是退格键的 ASCII 码
    // 0x09 是tab 键的 ASCII 码
    if (c == 0x08 && cursor_x) {
          cursor_x--;
    } else if (c == 0x09) {
          cursor_x = (cursor_x+8) & ~(8-1);
    } else if (c == '\r') {
          cursor_x = 0;
    } else if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c >= ' ') {
        video_memory[cursor_y*80 + cursor_x] = c | attribute;
        cursor_x++;
    }

    // 每 80 个字符一行，满80就必须换行了
    if (cursor_x >= 80) {
        cursor_x = 0;
        cursor_y ++;
    }

    // 如果需要的话滚动屏幕显示
    scroll();

    // 移动硬件的输入光标
    move_cursor();
}

void console_write(char *cstr)
{
    while (*cstr) {
          putch(*cstr++);
    }
}

void putch(char c)
{
    putch_color(c, rc_black, rc_white);
}

void console_write_uint32_t(uint32_t t){

    uint32_t tmp = 1000000000;
    uint32_t tmp1;
    char isFind = 0;
    while(tmp){
        tmp1 = t/tmp;
        t = t%tmp;
        if (tmp1)
        {
            putch('0'+tmp1);
            isFind = 1;
        }
        else
        {
            if (isFind)
            {
                putch('0');
            }
        }
        tmp = tmp/10;
    }

}

void console_write_uint8_t(uint8_t t){

    uint8_t tmp = 100;
    uint8_t tmp1;
    char isFind = 0;
    while(tmp){
        tmp1 = t/tmp;
        t = t%tmp;
        if (tmp1)
        {
            putch('0'+tmp1);
            isFind = 1;
        }
        else
        {
            if (isFind)
            {
                putch('0');
            }
        }
        tmp = tmp/10;
    }

}

void console_write_color(char *cstr, real_color_t back, real_color_t fore)
{
    while (*cstr) {
          putch_color(*cstr++, back, fore);
    }
}


void itoa (char *buf, int base, int d)
     {
       char *p = buf;
       char *p1, *p2;
       unsigned long ud = d;
       int divisor = 10;
     
       /* If %d is specified and D is minus, put `-' in the head. */
       if (base == 'd' && d < 0)
         {
           *p++ = '-';
           buf++;
           ud = -d;
         }
       else if (base == 'x')
         divisor = 16;
     
       /* Divide UD by DIVISOR until UD == 0. */
       do
         {
           int remainder = ud % divisor;
     
           *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
         }
       while (ud /= divisor);
     
       /* Terminate BUF. */
       *p = 0;
     
       /* Reverse BUF. */
       p1 = buf;
       p2 = p - 1;
       while (p1 < p2)
         {
           char tmp = *p1;
           *p1 = *p2;
           *p2 = tmp;
           p1++;
           p2--;
       }
}

void printf (const char *format, ...)
{
       char **arg = (char **) &format;
       int c;
       char buf[20];
     
       arg++;
     
       while ((c = *format++) != 0)
         {
           if (c != '%')
             putch(c);
           else
             {
               char *p;
     
               c = *format++;
               switch (c)
                 {
                 case 'd':
                 case 'u':
                 case 'x':
                   itoa (buf, c, *((int *) arg++));
                   p = buf;
                   goto string;
                   break;
     
                 case 's':
                   p = *arg++;
                   if (! p)
                     p = "(null)";
     
                 string:
                   while (*p)
                     putch (*p++);
                   break;
     
                 default:
                   putch (*((int *) arg++));
                   break;
                 }
             }
}
}

void (*onGetKeyFunction)(char keycode);
void registerListenKey(void (*function)(char keycode)){
    onGetKeyFunction = function;
}

void onKeyDown(char keycode)
{
    if (keycode == 42 || keycode == 54)
    {
        is_shift_down = 1;
    }
    unsigned char ch = keyboard_map[keycode];
    if (is_shift_down)
    {
        if (ch>='a' && ch <= 'z') 
        {
            ch += 'A' - 'a';
        }
        else if (ch>='A' && ch<='Z')
        {
            ch += 'a' - 'A';
        }
        else{
            ch = keyboard_map_1[keycode];
        }
    }
    printf("%c", ch);
    if (ch == '\b')
    {
        putch(' ');
        putch('\b');
    }
    if (onGetKeyFunction)
    {
        (*onGetKeyFunction)(keycode); //Call
        onGetKeyFunction = NULL;
    }
}

void onKeyUp(char keycode)
{
    if (keycode == 42 || keycode == 54)
    {
        is_shift_down = 0;
    }
}