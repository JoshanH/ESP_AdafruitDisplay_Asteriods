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

#define MAXBULLETS  35       // max number of bullets on screen at once
#define BULLETSPEED 5        // pixels per tick

#define WIDTH       240
#define HEIGHT      320

#define WHITE       0xFFFF
#define BLACK       0x0000
#define BLUE        0x00FF
#define RED         0xF800 

/* STRUCTURES ============================================================== */

struct vec2_t {
  float x;
  float y;
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
  float deg; // angle in degrees
};

//     bullet       //
/*       --         */
struct bullet_t {
  vec2_t head;
  vec2_t tail;
  float deg; // angle in degrees
  bullet_t* next; // for use in world bullet list
};

// array of all live bullets with count integrated
struct bulletList_t 
{
  bullet_t* head;
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
bulletList_t* buildBulletList ();
void shootBullet              (bulletList_t* worldBullets, vec2_t centre, int deg);
void drawBullet               (bullet_t* bullet, int colour);
void moveBullet               (bullet_t* bullet, bulletList_t* bulletsList);
void freeBullet               (bullet_t* bullet, bulletList_t* bulletsList);
bool bulletCollision          (bullet_t* bullet, bulletList_t* bulletsList);

int debugListLength(bulletList_t* list);

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
  bulletList_t* worldBulletList = buildBulletList();

  // testing segment == START
  shootBullet(worldBulletList, ship->centre, ship->dirDeg);
  while(1){
    dis.fillScreen(BLACK);

    if (worldBulletList->head != NULL){
      moveBullet(worldBulletList->head, worldBulletList);
    }

    rotateShip(ship, 10);
    drawShip(ship, BLUE);
    dis.setCursor(0,0);
    dis.printf("World Bullet Count: %d", debugListLength(worldBulletList));
    delay(100);
  }
  // testing segment == END
}

// loops until power off
void loop() 
{

}

int debugListLength(bulletList_t* list)
{
  bullet_t* current = list->head;
  int count = 0;
  
  while(current != NULL)
  {
    current = current->next;
    count++;
  }

  return count;
}


/* FUNCTIONS =============================================================== */

// Initialises the Adafruit display
void initialiseScreen()
{
  Serial.begin(SERIALDISP);
  dis.init(WIDTH, HEIGHT);
  dis.fillScreen(BLACK);
}

// checks if a bullet has collided with the edge of the screen or an asteroid
bool bulletCollision(bullet_t* bullet, bulletList_t* bulletsList)
{
  /* check if bullet out of bounds of display */
  if (bullet->head.x > WIDTH || bullet->head.y > HEIGHT)
  {
    /* free bullet from memory and remove from list */
    freeBullet(bullet, bulletsList);

    /* confirm a collision occured */
    return true;
  }

  /* no collision */
  return false;
}

void freeBullet(bullet_t* bullet, bulletList_t* bulletsList)
{

  /* checks if bullet is head in list */
  if (bulletsList->head == bullet)
  {
    bulletsList->head = bullet->next;
    free(bullet);
    return;
  }

  bullet_t* current = bulletsList->head;

  /* search for input bullet in bullet list */
  while(current->next != NULL && current->next != bullet)
  {
    current = current->next;
  }

  // bullet not in list
  if (current->next == NULL)
  {
    dis.printf("attempted to free nonexistant bullet!");
    return;
  }

  current->next = bullet->next;
  /* bullet is freed from memory */
  free(bullet);
}

// updates the bullet's movement and draws bullet in updates location
void moveBullet(bullet_t* bullet, bulletList_t* bulletsList)
{
  /* degrees coonvereted to radians */
  float rad = bullet->deg * (M_PI / 180);

  /* unit vector in direction of input degree scaled by bullet speed */
  float dx = cos(rad) * BULLETSPEED;
  float dy = -1 * sin(rad) * BULLETSPEED;

  /* bullet head and tail vectors updated */
  bullet->head.x += dx; 
  bullet->head.y += dy;
  bullet->tail.x += dx;
  bullet->tail.y += dy;

  /* check if bullet has collision */
  if (bulletCollision(bullet, bulletsList))
  {
    return;
  }

  /* bullet drawn */
  drawBullet(bullet, RED);
}

// allocates memory for linked list of bullets
bulletList_t* buildBulletList()
{
  // allocate memory to array
  bulletList_t* bulletList = (bulletList_t*) malloc(sizeof(bulletList_t));

  // check if memory allocated sucessfully
  if (bulletList == NULL)
  {
    dis.printf("worldBullets array malloc fail!");
    return NULL;
  }

  // sets head bullet pointer to NULL
  bulletList->head = NULL;

  return bulletList;
}

// spawns a bullet from a centre point in a direction (degrees)
void shootBullet(bulletList_t* bulletsList, vec2_t centre, int deg)
{
  /* allocate memory to new bullet */

  bullet_t* newBullet = (bullet_t*) malloc(sizeof(bullet_t));

  /* check if memory allocated successfully */

  if (newBullet == NULL)
  {
    dis.printf("newBullet malloc fail!");
  }

  /* add new bullet to world's bullet list */

  // if linked list not empty
  if (bulletsList->head != NULL){
    bullet_t* nextBullet = bulletsList->head;

    // search for end bullet of list
    while(nextBullet->next != NULL)
    {
      nextBullet = nextBullet->next;
    }

    // attach new bullet to end of list
    nextBullet->next = newBullet;
  } 
    else // list is empty
  {
    bulletsList->head = newBullet;
  }

  /* calculate head and tail positions of new bullet */

  // set new bullets list pointer to null
  newBullet->next = NULL;

  // new bullets direction set
  newBullet->deg = deg;

  newBullet->tail = centre; // centre of ship usually

  // head of the bullet calculated by scaling unit vector in movement direction
  // (x,y) -> (x + dx, y + dy)
  // -sin(theta) is used as y axis is inverted on display
  float rad = deg * (M_PI / 180);
  newBullet->head.x = centre.x + (cos(rad) * BULLETSPEED);
  newBullet->head.y = centre.y + (-1 * sin(rad) * BULLETSPEED);

  // draws the new bullet
  drawBullet(newBullet, RED);
}

// draws a bullet on screen for a given colour
void drawBullet(bullet_t* bullet, int colour)
{
  drawLineVec(bullet->head, bullet->tail, colour);
}

// creates a ship at a given location for its centre
ship_t* buildShip(vec2_t centre)
{
  ship_t* ship = (ship_t*) malloc(sizeof(ship_t));

  /* Calculate ship vectors based on centre */
  ship->centre = centre;
  ship->tip.x = centre.x; ship->tip.y = centre.y + 10;
  ship->lFin.x = centre.x - 5; ship->lFin.y = centre.y - 5;
  ship->rFin.x = centre.x + 5; ship->rFin.y = centre.y - 5;
  ship->dirDeg = 270; // pointing down

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

  /* apply rotational matrix to ship coordinates */
  rotMatCenter(&ship->tip, rad, &ship->centre);
  rotMatCenter(&ship->lFin, rad, &ship->centre);
  rotMatCenter(&ship->rFin, rad, &ship->centre);

  /* update ship stored angle (degrees) */
  ship->dirDeg += deg;
  ship->dirDeg %= 360; // keep degrees below 360
}

// applies 2D rotational matrix on vec2_t for input radians 
// around an input centre coorinate
void rotMatCenter(vec2_t* point, float rad, vec2_t* centre) 
{
  // Translate point to origin
  float x = point->x - centre->x;
  float y = point->y - centre->y;

  /* Apply rotation matrix */
  // R(x) = [cos(x) -sin(x)]
  //        [sin(x)  cos(x)]
  float x_rot = cos(rad) * x - sin(rad) * y;
  float y_rot = sin(rad) * x + cos(rad) * y;

  /* Translate back to center */
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