#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <math.h>

/* CONSTANTS =============================================================== */

#define TFT_CS      5        // Chip select
#define TFT_DC      16       // Data/Command
#define TFT_RST     17       // Reset (can be -1 if connected to ESP32 reset)
#define SERIALDISP  115200   // Serial location of display
#define WHITE       0xFFFF
#define BLACK       0x0000
#define BLUE        0x00FF
#define WIDTH       240
#define HEIGHT      320
#define MAXBULLETS  50

/* STRUCTURES ============================================================== */

struct vec2_t {
  double x;
  double y;
};

// cursor/spaceship //
/*       /\         */
/*      /__\        */
struct ship_t {
  vec2_t tip;
  vec2_t centre;
  vec2_t rFin;
  vec2_t lFin;
  int dirDeg; // direction 
};

//    asteroid      //
/*      ,__,        */
/*      |__|        */
struct asteroid_t {
  vec2_t tL;
  vec2_t tR;
  vec2_t bL;
  vec2_t bR;
  double slope; // slope of graph determining direction
};

//     bullet       //
/*       --         */
struct bullet_t {
  vec2_t front;
  vec2_t rear;
  double slope; // slope of graph determining direction
};

// array of all live bullets with count integrated
struct bulletList_t 
{
  bullet_t** worldBullets;
  int count;
};

/* FUNCTION DECLARATIONS =================================================== */

void initialiseScreen         ();
void drawLineV                (int x0, int y0, int x1, int y1, int colour);
void drawLineH                (int x0, int y0, int x1, int y1, int colour);
void drawLineXY               (int x0, int y0, int x1, int y1, int colour);
void drawLineVec              (vec2_t p1, vec2_t p2, int colour);
void rotMatCenter             (vec2_t* point, float rad, vec2_t* centre);
ship_t* buildShip             (vec2_t centre);
void drawShip                 (ship_t* ship, int colour);
void rotateShip               (ship_t* ship, int deg);
bulletList_t* buildBulletList (int arraySize);
void shootBullet              (bullet_t* worldBullets, vec2_t centre, int deg);

// initialise display screen in program
Adafruit_ST7789 dis = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);


/* SETUP/MAIN ============================================================== */

void setup() 
{
  // initialises screen 
  initialiseScreen();

  // finds centre of screen and stores ar variable
  vec2_t centreOfScreen;
  centreOfScreen.x = WIDTH/2;
  centreOfScreen.y = HEIGHT/2;
  
  // creates a pointer to a ship
  ship_t* ship = buildShip(centreOfScreen);
  drawShip(ship, BLUE);

  // inititialises the world bullets array (all values set to null)
  bulletList_t* worldBullets = buildBulletList(MAXBULLETS);
}

// loops until power off
void loop() 
{

}


/* FUNCTIONS =============================================================== */

// Initialises the Adafruit display
void initialiseScreen()
{
  Serial.begin(SERIALDISP);
  dis.init(WIDTH, HEIGHT);
  dis.fillScreen(BLACK);
}

// allocates memory for an array of bullet_t pointers and returns address
bulletList_t* buildBulletList(int arraySize)
{
  // allocate memory to array
  bulletList_t* bulletList = (bulletList_t*) malloc(sizeof(bulletList_t));

  // check if memory allocated sucessfully
  if (bulletList == NULL)
  {
    dis.printf("worldBullets array malloc fail!");
    return NULL;
  }

  // set bullet count to zero
  bulletList->count = 0;
  // set all values to NULL
  for (int i = 0; i < arraySize; i++)
  {
    bulletList->worldBullets[i] = NULL;
  }

  return bulletList;
}

// shoots a bullet from a centre point in a direction (degrees)
void shootBullet(bullet_t* worldBullets, vec2_t centre, int deg)
{

}

// creates a ship at a given location for its centre
ship_t* buildShip(vec2_t centre)
{
  ship_t* ship = (ship_t*) malloc(sizeof(ship_t));

  ship->centre = centre;
  ship->tip.x = centre.x; ship->tip.y = centre.y + 10;
  ship->lFin.x = centre.x - 5; ship->lFin.y = centre.y - 5;
  ship->rFin.x = centre.x + 5; ship->rFin.y = centre.y - 5;
  ship->dirDeg = 180; // pointing down

  return ship;
}

// draws lines between the appropriate points of a spaceship
void drawShip(ship_t* ship, int colour)
{
  drawLineVec(ship->tip, ship->lFin, colour);
  drawLineVec(ship->tip, ship->rFin, colour);
  drawLineVec(ship->centre, ship->lFin, colour);
  drawLineVec(ship->centre, ship->rFin, colour);
}

// rotates a ship_t an input degree around its centre
void rotateShip(ship_t* ship, int deg)
{
  // convert from degrees to radians
  float rad = deg * (M_PI / 180);

  // apply rotational matrix to ship coordinates
  rotMatCenter(&ship->tip, rad, &ship->centre);
  rotMatCenter(&ship->lFin, rad, &ship->centre);
  rotMatCenter(&ship->rFin, rad, &ship->centre);

  // update ship stored angle (degrees)
  ship->dirDeg += deg;
  ship->dirDeg %= 360; // keep degrees below 360
}

// applies 2D rotational matrix on vec2_t for input radians 
// around an input centre coorinate
void rotMatCenter(vec2_t* point, float rad, vec2_t* centre) 
{
  // Translate point to origin
  double x = point->x - centre->x;
  double y = point->y - centre->y;

  // Rotate
  // R(x) = [cos(x) -sin(x)]
  //        [sin(x)  cos(x)]
  double x_rot = cos(rad) * x - sin(rad) * y;
  double y_rot = sin(rad) * x + cos(rad) * y;

  // Translate back to center
  point->x = x_rot + centre->x;
  point->y = y_rot + centre->y;
}

// draws a line between two vec2_t
// (purely for abstraction)
void drawLineVec(vec2_t p1, vec2_t p2, int colour)
{
  drawLineXY((int)round(p1.x), (int)round(p1.y), 
             (int)round(p2.x), (int)round(p2.y), colour);
}


// LINE DRAWING CODE FROM YOUTUBER https://www.youtube.com/watch?v=CceepU1vIKo
// Slightly modified by myself and translated from Python to C by ChatGPT
void drawLineXY(int x0, int y0, int x1, int y1, int colour)
{
  if (abs(x1-x0) > abs(y1-y0))
  {
    drawLineH(x0, y0, x1, y1, colour);
  } 
    else 
  {
    drawLineV(x0, y0, x1, y1, colour);
  }
}

void drawLineV(int x0, int y0, int x1, int y1, int colour) {
    if (y0 > y1) {
        // Swap x0 and x1
        int temp = x0; x0 = x1; x1 = temp;
        // Swap y0 and y1
        temp = y0; y0 = y1; y1 = temp;
    }

    int dx = x1 - x0;
    int dy = y1 - y0;

    int dir;
    if (dx < 0)
    {
      dir = -1;
    } 
      else
    {
      dir = 1;
    }

    dx = dx * dir; 

    if (dy != 0) {
        int x = x0;
        int p = 2 * dx - dy;

        for (int i = 0; i <= dy; i++) {
            dis.drawPixel(x, y0 + i, colour); 
            if (p >= 0) {
                x += dir;
                p -= 2 * dy;
            }
            p += 2 * dx;
        }
    }
}

void drawLineH(int x0, int y0, int x1, int y1, int colour) {
    if (x0 > x1) {
        // Swap x0 and x1
        int temp = x0; x0 = x1; x1 = temp;
        // Swap y0 and y1
        temp = y0; y0 = y1; y1 = temp;
    }

    int dx = x1 - x0;
    int dy = y1 - y0;

    int dir;
    if (dy < 0)
    {
      dir = -1;
    } 
      else
    {
      dir = 1;
    }
    dy = dy * dir;

    if (dx != 0) {
        int y = y0;
        int p = 2 * dy - dx;

        for (int i = 0; i <= dx; i++) {
            dis.drawPixel(x0 + i, y, colour);
            if (p >= 0) {
                y += dir;
                p -= 2 * dx;
            }
            p += 2 * dy;
        }
    }
}