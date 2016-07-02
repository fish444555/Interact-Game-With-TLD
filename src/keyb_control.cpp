// #include <stdio.h>
// #include <linux/input.h>
// #include <fcntl.h>
// #include <sys/time.h>
// #include <unistd.h>
// #include <string.h>
// // http://blog.csdn.net/zouxy09/article/details/7920253		交互系统的构建之（二）Linux下鼠标和键盘的模拟控制
// //按键模拟，按键包含按下和松开两个环节
// void simulate_key(int fd, int kval)
// {
//     struct input_event event;
//     gettimeofday(&event.time, 0);
//     //按下kval键
//     event.type = EV_KEY;
//     event.value = 1;
//     event.code = kval;
//     write(fd, &event, sizeof(event));
//     //同步，也就是把它报告给系统
//     event.type = EV_SYN;
//     event.value = 0;
//     event.code = SYN_REPORT;
//     write(fd, &event, sizeof(event));

//     memset(&event, 0, sizeof(event));
//     gettimeofday(&event.time, 0);
//     //松开kval键
//     event.type = EV_KEY;
//     event.value = 0;
//     event.code = kval;
//     write(fd, &event, sizeof(event));
//     //同步，也就是把它报告给系统
//     event.type = EV_SYN;
//     event.value = 0;
//     event.code = SYN_REPORT;
//     write(fd, &event, sizeof(event));
// }
// //鼠标移动模拟
// void simulate_mouse(int fd, int rel_x, int rel_y)
// {
//     struct input_event event;
//     gettimeofday(&event.time, 0);
//     //x轴坐标的相对位移
//     event.type = EV_REL;
//     event.value = rel_x;
//     event.code = REL_X;
//     write(fd, &event, sizeof(event));
//     //y轴坐标的相对位移
//     event.type = EV_REL;
//     event.value = rel_y;
//     event.code = REL_Y;
//     write(fd, &event, sizeof(event));
//     //同步
//     event.type = EV_SYN;
//     event.value = 0;
//     event.code = SYN_REPORT;
//     write(fd, &event, sizeof(event));
// }
// int main(int argc, char **argv)
// {
//     int fd_mouse = -1;
//     int fd_kbd = -1;
//     int i = 0;

//     fd_kbd = open("/dev/input/event3", O_RDWR);
//     if(fd_kbd <= 0)
//     {
//              printf("Can not open keyboard input file\n");
//              return -1;
//     }

//     fd_mouse = open("/dev/input/event4", O_RDWR);
//     if(fd_mouse <= 0)
//     {
//           printf("Can not open mouse input file\n");
//           return -1;
//     }

//     for (i = 0; i < 50; i++)
//     {
//              simulate_key(fd_mouse, BTN_LEFT);  //模拟按下鼠标左键
//              //if (i % 3 == 0)
//              //      simulate_key(fd_kbd, KEY_A);  //模拟按下键盘A键
//             //模拟鼠标相对上次x和y轴相应移动10个像素
//              //simulate_mouse(fd_mouse, 10, 10);                  
//              sleep(3);
//     }
//     close(fd_kbd);
//     close(fd_mouse);
// }



#include <stdio.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// http://blog.chinaunix.net/uid-20666855-id-4905405.html
int main()
{
    int fd;
    struct input_event event;

    fd = open("/dev/input/event3", O_RDONLY); //打开输入文件根据实际情况
    if(fd <= 0)
    {
            printf("open device error\n");
            return 0;
    }

    while(1)
    {
            if(read(fd, &event, sizeof(event)) == sizeof(event))
            {
                if (event.type == EV_KEY)
                    if (event.value == 0 || event.value == 1)
                    {
                            printf("key %d %s\n", event.code, (event.value) ? "Pressed" : "Released");
                            if(event.code == KEY_ESC)
                                    break;
                    }
            }
    }
    close(fd);
    return 0;
}
