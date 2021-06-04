
#include <RTL.h>
#include <LPC17xx.H>                    /* LPC17xx definitions                */
#include "GLCD.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>\

/* JoyStick  ------------------------------------------------------------------*/
#define JOYSTICK_SELECT      0x01               
#define JOYSTICK_LEFT        0x08               
#define JOYSTICK_UP          0x10               
#define JOYSTICK_RIGHT       0x20               
#define JOYSTICK_DOWN        0x40
#define JOYSTICK_MASK        0x79  

#define UP 1
#define RIGHT 2
#define DOWN 3
#define LEFT 4

/* Settings  ------------------------------------------------------------------*/

#define SCREEN_PART_WIDTH 10
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 160
#define MAP_SIZE 9
#define FOV 3.141592 / 3.0
#define DEPTH 16.0f
#define SPEED 0.5f
#define BOUNDARY 0.08f
#define CEILING_RATIO 1.5f

#define CHAR_WALL '#'
#define CHAR_BLANK ' '
#define CHAR_END '$'

#define SEED 0  // Random seed for generating a map

static unsigned char GIMP_IMAGE_pixel_data[SCREEN_HEIGHT * SCREEN_PART_WIDTH * 2];

char map[MAP_SIZE][MAP_SIZE];

float player_x = 1.5f;
float player_y = 1.5f;
float player_angle = 3.141592 / 4;
float speed = SPEED;
float field_of_view = FOV;
float depth = DEPTH;
float ray_angle;
float distance_to_wall;
float eye_x;
float eye_y;
float test_x;
float test_y;
float boundary = BOUNDARY;
	
uint32_t joystick_val = 0;

void init_maze(){
	int i, j;
	int x=1, y=1;
	
	for(i=0; i<MAP_SIZE; i++){
		for(j=0; j<MAP_SIZE; j++){ 
			map[j][i] = CHAR_WALL;
		}
	}

	srand(SEED); // put random number.
	
	map[x][y] = CHAR_BLANK;

	for(i=0; i<1000; i++){
		int dir = rand()%4+1;
		if(dir==UP){
		
			if(y<=1){
			
				continue;
			}
		
			if(map[x][y-2] == CHAR_WALL){ 
			
				y--; 
				map[x][y]=CHAR_BLANK;
				
				y--; 
				map[x][y]=CHAR_BLANK;
			
			}else{ 
			
				y-=2;
			
			}
		
		}
		
		if(dir==LEFT){
		
			if(x<=1) {
			
				continue; 
			}
			
			if(map[x-2][y] == CHAR_WALL){ 
			
				x--; 
				map[x][y]=CHAR_BLANK;
				
				x--; 
				map[x][y]=CHAR_BLANK;
			
			}else{ 
			
				x-=2;
			
			}
		
		}
	
		if(dir==RIGHT){
		
			if(x>=MAP_SIZE-2){
			
				continue;
			}
			
			if(map[x+2][y] == CHAR_WALL){
			
				x++; 
				map[x][y]=CHAR_BLANK;
				
				x++; 
				map[x][y]=CHAR_BLANK;
			
			}else{
			
				x+=2;
			
			}
		
		}
		
		if(dir==DOWN){
		
			if(y>=MAP_SIZE-2) {
			
				continue;
			}
			
			if(map[x][y+2] == CHAR_WALL){ 
				
				y++; 
				map[x][y]=CHAR_BLANK;
				
				y++; 
				map[x][y]=CHAR_BLANK;
			
			}else{ 
				y+=2;
			}
		
		}
	}
	
	map[MAP_SIZE-2][MAP_SIZE-2] = CHAR_END;
}
	
	
	
void calculate_screen(int startX, int endX){
		int screen_width = SCREEN_WIDTH;
    int screen_height = SCREEN_HEIGHT;

    int map_height = MAP_SIZE;
    int map_width = MAP_SIZE;

    int ceiling;
    int floor;


    bool is_hit_wall;
    bool is_boundary;
    bool is_end;
		
		unsigned char shade;
		int x, y;
		
		for (x = startX; x < endX; x++)
        {
            // For each column, calculate the projected ray angle into world space. Fires ray each direction.
            ray_angle = (player_angle - field_of_view / 2.0f) + ((float)x / (float)screen_width) * field_of_view;
            distance_to_wall = 0;
            is_hit_wall = false;
            is_boundary = false;
            is_end = false;
            eye_x = sinf(ray_angle);
            eye_y = cosf(ray_angle);

            while (!is_hit_wall && distance_to_wall < depth)
            {
                distance_to_wall += 0.1f;
                test_x = player_x + eye_x * distance_to_wall;
                test_y = player_y + eye_y * distance_to_wall;

                if (test_x < 0 || test_x >= map_width || test_y < 0 || test_y >= map_height)
                { // Not hitted anything
                    is_hit_wall = true;
                    distance_to_wall = depth;
                }
                else
                {
                    if (map[(int)test_y][(int)test_x] == CHAR_WALL)
                    { // Hitted
                        is_hit_wall = true;

                        // Is boundary of the wall
                        if (((int)test_y != (int)(test_y + boundary) || (int)test_y != (int)(test_y - boundary)) && ((int)test_x != (int)(test_x + boundary) || (int)test_x != (int)(test_x - boundary)))
                            is_boundary = true;
                    }
                    if(map[(int)test_y][(int)test_x] == CHAR_END){
                        is_hit_wall = true;
                        is_end = true;
                    }
                }
            }

            ceiling = (float)(screen_height / CEILING_RATIO) - screen_height / ((float)distance_to_wall);
            floor = screen_height * 2 - ceiling;

            shade = 0xFF;

            if (distance_to_wall <= depth / 4.0f)
                shade = 0x18; // close
            else if (distance_to_wall <= depth / 3.0f)
                shade = 0x39;
            else if (distance_to_wall <= depth / 2.0f)
                shade = 0x7B;
            else if (distance_to_wall <= depth / 1.0f)
                shade = 0xDE; // far
            else
                shade = 0xFF;

            if (is_boundary)
                shade = 0x29;

            if (is_end)
                shade = 0x07;

            for (y = 0; y < screen_height * 2 ; y+= 2)
            {
                if (y <= ceiling)
                {
                  GIMP_IMAGE_pixel_data[y * SCREEN_PART_WIDTH + (x - startX) * 2] = 0x07;
									GIMP_IMAGE_pixel_data[y * SCREEN_PART_WIDTH + (x - startX) * 2 + 1] = 0xC3;
                }
                else if (y > ceiling && y <= floor)
                {
                  GIMP_IMAGE_pixel_data[y * SCREEN_PART_WIDTH + (x - startX) * 2] = shade;
									GIMP_IMAGE_pixel_data[y * SCREEN_PART_WIDTH + (x - startX) * 2 + 1] = shade;
                }
                else
                {
                    GIMP_IMAGE_pixel_data[y * SCREEN_PART_WIDTH+ (x - startX) * 2] = 0xB5;
									GIMP_IMAGE_pixel_data[y * SCREEN_PART_WIDTH+ (x - startX) * 2 + 1] = 0xB5;
                }
            }

        }
}

void check_joystick(){
	joystick_val = (LPC_GPIO1->FIOPIN >> 20) & JOYSTICK_MASK;
		
		if ((joystick_val & JOYSTICK_UP)     == 0) {  /* up     pressed means 0 */
				player_x = player_x + speed * sinf(player_angle);
            player_y = player_y + speed * cosf(player_angle);
            // Invalid move
            if (map[(int)player_y][(int)player_x] == CHAR_WALL)
            {
                player_x = player_x - speed * sinf(player_angle);
                player_y = player_y - speed * cosf(player_angle);
            }
			}
		
		if ((joystick_val & JOYSTICK_DOWN)     == 0) {  /* down     pressed means 0 */
				player_x = player_x - speed * sinf(player_angle);
            player_y = player_y - speed * cosf(player_angle);
            // Invalid move
            if (map[(int)player_y][(int)player_x] == CHAR_WALL)
            {
                player_x = player_x + speed * sinf(player_angle);
                player_y = player_y + speed * cosf(player_angle);
            }
		}
		
		if ((joystick_val & JOYSTICK_LEFT)     == 0) {  /* left     pressed means 0 */
				player_angle = player_angle - 0.25f;
		}
		
		if ((joystick_val & JOYSTICK_RIGHT)     == 0) {  /* right     pressed means 0 */
				player_angle = player_angle + 0.25f;
		}
}

int main (void)
{
	
	int i;
	
	SystemInit();                             /* Initialize the MCU clocks     */

	GLCD_init();                              /* Initialize the GLCD           */
	GLCD_clear(White);                        /* Clear the GLCD                */

	LPC_GPIO1->FIODIR   &= ~((1UL<<20)|(1UL<<23)|(1UL<<24)|(1UL<<25)|(1UL<<26));	/* JoyStick Init */
	
	init_maze();
	
	while(1){
		
		check_joystick();			// Check input
		
		for(i = 0; i < SCREEN_WIDTH; i += SCREEN_PART_WIDTH){ // draw partly for each screen part width determined.
			calculate_screen(i, i+SCREEN_PART_WIDTH);
			GLCD_bitmap(i, 40, SCREEN_PART_WIDTH , SCREEN_HEIGHT, GIMP_IMAGE_pixel_data);
		}
	}
}

/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/
