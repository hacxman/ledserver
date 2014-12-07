#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>

#include "clk.h"
#include "gpio.h"
#include "dma.h"
#include "pwm.h"

#include "ws2811.h"


#define ARRAY_SIZE(stuff)                        (sizeof(stuff) / sizeof(stuff[0]))

#define TARGET_FREQ                              WS2811_TARGET_FREQ
#define GPIO_PIN                                 18
#define DMA                                      5

#define WIDTH                                    1
#define HEIGHT                                   240
#define LED_COUNT                                (WIDTH * HEIGHT)

int wheel(int pos) {
  if (pos < 85) { return ((pos*3)<<16)+((255-pos*3)<<8)+0; }
  else if (pos < 170) { pos -= 85; return ((255-pos*3)<<16)+0+((pos*3)); }
  else { pos -= 170; return 0+((pos*3)<<8)+((255-pos*3)); }
}

ws2811_t ledstring =
{
    .freq = TARGET_FREQ,
    .dmanum = DMA,
    .channel =
    {
        [0] =
        {
            .gpionum = GPIO_PIN,
            .count = LED_COUNT,
            .invert = 0,
        },
        [1] =
        {
            .gpionum = 0,
            .count = 0,
            .invert = 0,
        },
    },
};

ws2811_led_t matrix[WIDTH][HEIGHT];
void matrix_render(void)
{
    int x, y;

    for (x = 0; x < WIDTH; x++)
    {
        for (y = 0; y < HEIGHT; y++)
        {
            ledstring.channel[0].leds[(y * WIDTH) + x] = matrix[x][y];
        }
    }
}

static void ctrl_c_handler(int signum)
{
    ws2811_fini(&ledstring);
}

static void setup_handlers(void)
{
    struct sigaction sa =
    {
        .sa_handler = ctrl_c_handler,
    };

    sigaction(SIGKILL, &sa, NULL);
}

int main(int argc, char *argv[])
{
    int ret = 0;
    setup_handlers();

    if (ws2811_init(&ledstring))
    {
        return -1;
    }

    char *line = NULL;
    size_t llen;
    int i,j=0;
    enum {
      MODE_RAINBOW,
      MODE_ARRAY,
      MODE_COUNT
    };
    int mode = MODE_ARRAY;
    while (1)
    {
        getline(&line, &llen, stdin);
	if (line) {
        if (strncmp("quit", line, 4) == 0) {
          printf("quit\n");
          exit(0);
        } else if (strncmp("rainbow", line, 7) == 0) {
          mode = MODE_RAINBOW;
        } else if (strncmp("array", line, 5) == 0) {
          mode = MODE_ARRAY;
        } else if (strncmp("count", line, 5) == 0) {
          mode = MODE_COUNT;
        }}

        switch (mode) {
        case MODE_RAINBOW: {
          printf("r\n");
          j = atoi(line);
          //matrix_render();
          for (i = 0; i < LED_COUNT; i++) {
            ledstring.channel[0].leds[i] = wheel((i*256/LED_COUNT+j) % 256);
          }
          break;
        };
        case MODE_ARRAY: {
          printf("a\n");
          int t = 0;
          for (t = 0; t < LED_COUNT; t++) {
            fread(&ledstring.channel[0].leds[t], 3, 1, stdin);
          }
          break;
        };
        case MODE_COUNT: {
          printf("c\n");
          int t = 0;
          int n = atoi(line);
          for (t = 0; t < LED_COUNT; t++) {
            ledstring.channel[0].leds[t] = t < n ? 0xffffff : 0;
          }
          break;
        };
        };

        if (ws2811_render(&ledstring))
        {
            ret = -1;
            break;
        }

        // 15 frames /sec
        //usleep(1000000 / 250);
    }

    ws2811_fini(&ledstring);

    return ret;
}


