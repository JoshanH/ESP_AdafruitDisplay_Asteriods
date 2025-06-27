#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <math.h>

/* CONSTANTS =============================================================== */

#define TFT_CS        5        // Chip select
#define TFT_DC        16       // Data/Command
#define TFT_RST       17       // Reset (can be -1 if connected to ESP32 reset)
#define SERIALDISP    115200   // Serial location of display

#define MAXBULLETS    35       // max number of bullets on screen at once
#define BULLETSPEED   7        // pixels per tick
#define SHOTCOOLDOWN  2        // number of ticks before next shot allowed  

#define ASTEROIDSPEED 2        // pixels per tick

#define ROTSTEP       15       // degrees of ship rotation per button press

#define TICKSPEED     100      // tick speed in milliseconds

#define WIDTH         240
#define HEIGHT        320

#define LEFTBUTTON    35
#define RIGHTBUTTON   34
#define SHOOTBUTTON   33
#define DEBUGSWITCH   21

#define WHITE         0xFFFF
#define BLACK         0x0000
#define BLUE          0x00FF
#define RED           0xF800 
#define CREAM         0xFDB6

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
/*      \_-`        */
struct asteroid_t {
  vec2_t tL;
  vec2_t tR;
  vec2_t mL;
  vec2_t mR;
  vec2_t bL;
  vec2_t bR;
  vec2_t cL;
  vec2_t cR;
  float deg; // angle in degrees
  vec2_t centre;
  asteroid_t* next; // for use in world asteroid list
};

// linked list of all live asteroids
struct asteroidList_t
{
  asteroid_t* head;
};

//     bullet       //
/*       --         */
struct bullet_t {
  vec2_t head;
  vec2_t tail;
  float deg; // angle in degrees
  bullet_t* next; // for use in world bullet list
};

// linked list of all live bullets
struct bulletList_t 
{
  bullet_t* head;
};

/* FUNCTION DECLARATIONS =================================================== */

void initialiseScreen         ();
void initialiseControls       ();

void drawLineV                (int x0, int y0, int x1, int y1, int colour);
void drawLineH                (int x0, int y0, int x1, int y1, int colour);
void drawLineXY               (int x0, int y0, int x1, int y1, int colour);
void drawLineVec              (vec2_t p1, vec2_t p2, int colour);
void scaleVec2FromCentre      (vec2_t* point, vec2_t* centre, float scalar);
float distanceBetweenVec2     (vec2_t p1, vec2_t p2);

void rotMatCenter             (vec2_t* point, float rad, vec2_t* centre);

ship_t* buildShip             (vec2_t centre);
void drawShip                 (ship_t* ship, int colour);
void rotateShip               (ship_t* ship, int deg);

bulletList_t* buildBulletList ();
void shootBullet              (bulletList_t* bulletsList, ship_t* ship);
void drawBullet               (bullet_t* bullet, int colour);
void moveBullet               (bullet_t* bullet, bulletList_t* bulletsList);
void freeBullet               (bullet_t* bullet, bulletList_t* bulletsList);
bool bulletCollision          (bullet_t* bullet, bulletList_t* bulletsList);
void updateWorldBullets       (bulletList_t* bulletsList);

asteroidList_t* buildAsteroidList ();
void calculateVec2OfAsteroid      (asteroid_t* asteroid);
void spawnAsteroid                (asteroidList_t* asteroidList);
void drawAsteroid                 (asteroid_t* asteroid, int colour);
void rotateAsteroid               (asteroid_t* asteroid, float rad);
void moveAsteroid                 (asteroid_t* asteroid);
void updateWorldAsteroids         (asteroidList_t* asteroidList);
bool checkCollisionVec2Ast        (vec2_t point, asteroid_t* asteroid);
void handleAsteroidCollision      (asteroidList_t* asteroidList, 
                                   bulletList_t* bulletList, ship_t* ship);
void freeAsteroid                 (asteroid_t* asteroid, 
                                   asteroidList_t* asteroidList);

void gameOver                 (ship_t* ship);

void checkMoveInput           (ship_t* ship);
void checkShootInput          (ship_t* ship, bulletList_t* bulletsList, 
                              int* shootCooldown);

/* DEBUG FUNCTION DECLARATIONS =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int debugWorldBulletNum       (bulletList_t* list);
int debugWorldAsteroidNum     (asteroidList_t* list);
void printDebugValues         (bulletList_t* bulList, ship_t* ship, 
                               asteroidList_t* astList, int level);




// initialise display screen in program
Adafruit_ST7789 dis = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);


/* SETUP/MAIN ============================================================== */

void setup() 
{
  // initialises screen and control buttons
  initialiseScreen();
  initialiseControls();

  // finds centre of screen and stores ar variable
  vec2_t centreOfScreen;
  centreOfScreen.x = WIDTH/2;
  centreOfScreen.y = HEIGHT/2;
  
  // creates a pointer to a ship
  ship_t* ship = buildShip(centreOfScreen);
  drawShip(ship, BLUE);

  // inititialises the world bullets array (all values set to null)
  bulletList_t* worldBulletList = buildBulletList();

  // used to check if bullet shot this tick
  int shootCooldown = SHOTCOOLDOWN;

  asteroidList_t* worldAsteroidList = buildAsteroidList();

  int tickCounter = 0;
  int level = 1;

  /* MAIN GAME LOOP */
  while(1){ 

    // increment shot cooldown timer
    if (shootCooldown != 0) { 
      shootCooldown--; 
    }

    dis.fillScreen(BLACK);
    checkMoveInput(ship);
    checkShootInput(ship, worldBulletList, &shootCooldown);
    drawShip(ship, BLUE);
    updateWorldBullets(worldBulletList);
    handleAsteroidCollision(worldAsteroidList, worldBulletList, ship);

    if ((tickCounter % (30 - level)) == 0)
    {
      spawnAsteroid(worldAsteroidList);
    }

    updateWorldAsteroids(worldAsteroidList);

    // print debug screen only if activated
    if (digitalRead(DEBUGSWITCH) == LOW) 
    { 
      printDebugValues(worldBulletList, ship, worldAsteroidList, level); 
    }

    if (level < 10 && tickCounter % 100 == 1)
    {
      level++;
    }

    tickCounter++;
    delay(TICKSPEED);
  }
}

// loops until power off
void loop() 
{

}


/* DEBUG FUNCTIONS ========================================================= */

// prints a debug screen onto display
void printDebugValues(bulletList_t* bulList, ship_t* ship, 
                      asteroidList_t* astList, int level)
{
  dis.setCursor(0,0);
  dis.setTextColor(WHITE);

  dis.print("Ship Direction Degrees: ");
  dis.println(ship->dirDeg);

  dis.print("World Bullet Count: ");
  dis.println(debugWorldBulletNum(bulList));

  dis.print("World Asteroid Count: ");
  dis.println(debugWorldAsteroidNum(astList));

  dis.print("Game Level: ");
  dis.println(level);
}

// get length of bullet linked list
int debugWorldBulletNum(bulletList_t* list)
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

int debugWorldAsteroidNum(asteroidList_t* list)
{
  asteroid_t* current = list->head;
  int count = 0;
  
  while(current != NULL)
  {
    current = current->next;
    count++;
  }

  return count;
}

/* FUNCTIONS =============================================================== */

// displays game over message and runs infinite loop
void gameOver(ship_t* ship)
{
  // clear screen
  dis.fillScreen(BLACK);

  while(1)
  {
    rotateShip(ship, 10);    
    drawShip(ship, RED);

    dis.setCursor(0, HEIGHT / 4);
    dis.setTextColor(RED);
    dis.setTextSize(3);
    dis.print("GAME OVER!");

    dis.setCursor(0, (HEIGHT / 4) + 20);
    dis.setTextColor(WHITE);
    dis.setTextSize(1);
    dis.println(" ");
    dis.println(" ");
    dis.println(" ");
    dis.println("You were killed by an Asteroid!");
    dis.println(" ");
    dis.println("Your pilot has ejected.... Retry?");

    // clear ship
    drawShip(ship, BLACK);
  }
}

// runs through list of asteroids and bullets to determine if an asteroid 
// collision with ship or bullet has occurred
void handleAsteroidCollision(asteroidList_t* asteroidList, 
                             bulletList_t* bulletList, ship_t* ship)
{
  /* checks each asteroid in world's asteroid list for collisions */

  asteroid_t* currentAsteroid = asteroidList->head;

  while(currentAsteroid != NULL)
  {
    /* less costly ship collision check performed first */
    if (checkCollisionVec2Ast(ship->centre, currentAsteroid))
    {
      gameOver(ship);
    }

    /* check if asteroid too far off screen */
    if (currentAsteroid->centre.x > (WIDTH + 20)  ||
        currentAsteroid->centre.y > (HEIGHT + 20) ||
        currentAsteroid->centre.x < -20           ||
        currentAsteroid->centre.y < -20             )
    {
      freeAsteroid(currentAsteroid, asteroidList);
      return;
    }

    /* check each bullet in world's bullet list for a collision */
    bullet_t* currentBullet = bulletList->head;
    while(currentBullet != NULL)
    {
      // if collsiion between bullet and asteroid
      if (checkCollisionVec2Ast(currentBullet->head, currentAsteroid))
      {
        freeAsteroid(currentAsteroid, asteroidList);
        freeBullet(currentBullet, bulletList);
        return;
      }

      currentBullet = currentBullet->next;
    }
    currentAsteroid = currentAsteroid->next;
  }
  
  /* no asteroid collisions occurred */
  return;
}

// removes an asteroid from memory and from linked list
void freeAsteroid(asteroid_t* asteroid, asteroidList_t* asteroidList)
{
  /* check if the asteroid is the head of the list */
  if (asteroidList->head == asteroid)
  {
    asteroidList->head = asteroid->next;
    free(asteroid);
    return;
  }

  asteroid_t* current = asteroidList->head;

  /* Search for the asteroid in the list */
  while (current->next != NULL && current->next != asteroid)
  {
    current = current->next;
  }

  // Asteroid not found in list
  if (current->next == NULL)
  {
    dis.printf("Attempted to free non-existent asteroid!\n");
    return;
  }

  current->next = asteroid->next;
  /* asteroid is freed from memory */
  free(asteroid);
}

// check if there is a collision between an input vector and bullet
bool checkCollisionVec2Ast(vec2_t point, asteroid_t* asteroid)
{
  /* calculate distance between asteroid and bullet */
  float distance = distanceBetweenVec2(point, asteroid->centre);

  /* check if distance of bullet and centre of asteroid is less than or 
     equal to the size of the asteroid */
  if (distance <= distanceBetweenVec2(asteroid->centre, asteroid->mL))
  {
    return true;
  }

  /* bullet not in contact with asteroid */
  return false; 
}

void updateWorldAsteroids(asteroidList_t* asteroidList)
{
  /* check if world asteroid list is not empty */
  if (asteroidList->head != NULL)
  {
    /* goes through list and updates all asteroids */
    asteroid_t* current = asteroidList->head;
    while(current != NULL)
    {
      moveAsteroid(current);
      current = current->next;
    }
  }
}

asteroidList_t* buildAsteroidList()
{
  // allocate memory to array
  asteroidList_t* asteroidList = (asteroidList_t*) malloc(sizeof(asteroidList_t));

  // check if memory allocated sucessfully
  if (asteroidList == NULL)
  {
    dis.printf("worldAsteroids array malloc fail!");
    return NULL;
  }

  // sets head bullet pointer to NULL
  asteroidList->head = NULL;

  return asteroidList;
}

void moveAsteroid(asteroid_t* asteroid)
{
  /* degrees converted to radians */
  float rad = asteroid->deg * (M_PI / 180);

  /* unit vector in direction of asteroid approach degree scaled */
  /* by asteroid speed                                           */
  float dx = cos(rad) * ASTEROIDSPEED;
  float dy = sin(rad) * ASTEROIDSPEED;

  /* all vectors in asteroid updated */
  asteroid->tL.x -= dx;
  asteroid->tL.y -= dy;

  asteroid->tR.x -= dx;
  asteroid->tR.y -= dy;

  asteroid->mL.x -= dx;
  asteroid->mL.y -= dy;

  asteroid->mR.x -= dx;
  asteroid->mR.y -= dy;

  asteroid->bL.x -= dx;
  asteroid->bL.y -= dy;

  asteroid->bR.x -= dx;
  asteroid->bR.y -= dy;

  asteroid->cL.x -= dx;
  asteroid->cL.y -= dy;

  asteroid->cR.x -= dx;
  asteroid->cR.y -= dy;

  asteroid->centre.x -= dx;
  asteroid->centre.y -= dy;

  drawAsteroid(asteroid, CREAM);
}

// spawns a new asteroid in radnom off screen position and adds it to 
// the world's asteroid list
void spawnAsteroid(asteroidList_t* asteroidList)
{
  /* calculate centre of screen */
  vec2_t centreOfScreen;
  centreOfScreen.x = WIDTH / 2;
  centreOfScreen.y = HEIGHT / 2;

  /* allocate memory to new asteroid */
  
  asteroid_t* newAsteroid = (asteroid_t*) malloc(sizeof(asteroid_t));

  /* check if memory allocated successfully */

  if (newAsteroid == NULL)
  {
    dis.printf("newAsteroid malloc fail!");
    return;
  }

  /* add new asteroid to world's asteroid list */

  // if linked list not empty
  if (asteroidList->head != NULL){
    asteroid_t* nextAsteroid = asteroidList->head;

    // search for end bullet of list
    while(nextAsteroid->next != NULL)
    {
      nextAsteroid = nextAsteroid->next;
    }

    // attach new bullet to end of list
    nextAsteroid->next = newAsteroid;
  } 
    else // list is empty
  {
    asteroidList->head = newAsteroid;
  }

  /* assign random approaching degree and spawn point */

  // generates a random integer
  int randomValue = esp_random();

  // restricts random integer to range [0, 360)
  int deg = randomValue % 360;

  /* push completely off screen */

  // convert to radians
  float rad = deg * (M_PI / 180);
  // scale unit vector in direction of deg to puch asteroid off screen
  // scaled from centre to offscreen+10 to ensure not drawn on screen slightly
  newAsteroid->centre.x = centreOfScreen.x 
                          + (cos(rad) * (max((HEIGHT / 2), (WIDTH / 2)) + 10));
  newAsteroid->centre.y = centreOfScreen.y 
                          + (sin(rad) * (max((HEIGHT / 2), (WIDTH / 2)) + 10));

  /* calculate positions of vectors of new asteroid */

  calculateVec2OfAsteroid(newAsteroid);

  /* scale asteroid by random amount */
  float scalar = (randomValue % 3) + 1;
  scaleVec2FromCentre(&newAsteroid->tL, &newAsteroid->centre, scalar);
  scaleVec2FromCentre(&newAsteroid->tR, &newAsteroid->centre, scalar);
  scaleVec2FromCentre(&newAsteroid->mL, &newAsteroid->centre, scalar);
  scaleVec2FromCentre(&newAsteroid->mR, &newAsteroid->centre, scalar);
  scaleVec2FromCentre(&newAsteroid->bL, &newAsteroid->centre, scalar);
  scaleVec2FromCentre(&newAsteroid->bR, &newAsteroid->centre, scalar);
  scaleVec2FromCentre(&newAsteroid->cL, &newAsteroid->centre, scalar);
  scaleVec2FromCentre(&newAsteroid->cR, &newAsteroid->centre, scalar);

  // set asteroid aproach degree
  newAsteroid->deg = deg;

  /* rotate asteroid random amount */
  int rotDeg = esp_random() % 360; // new random degree
  int rotRad = deg * (M_PI / 180);

  // rotate all vectors
  rotateAsteroid(newAsteroid, rad);

  // set new bullets list pointer to null
  newAsteroid->next = NULL;

  /* astroid off screen, no need to draw until move */
}

// aplies 2D rotational matrix to an asteroid for a set number of radians
void rotateAsteroid(asteroid_t* asteroid, float rad)
{
  rotMatCenter(&asteroid->tL, rad, &asteroid->centre);
  rotMatCenter(&asteroid->tR, rad, &asteroid->centre);
  rotMatCenter(&asteroid->mL, rad, &asteroid->centre);
  rotMatCenter(&asteroid->mR, rad, &asteroid->centre);
  rotMatCenter(&asteroid->bL, rad, &asteroid->centre);
  rotMatCenter(&asteroid->bR, rad, &asteroid->centre);
  rotMatCenter(&asteroid->cL, rad, &asteroid->centre);
  rotMatCenter(&asteroid->cR, rad, &asteroid->centre);
}

// draws lines between all vectors of an input asteroid with input colour
void drawAsteroid(asteroid_t* asteroid, int colour)
{
  drawLineVec(asteroid->tL, asteroid->tR, colour);
  drawLineVec(asteroid->tR, asteroid->mR, colour);
  drawLineVec(asteroid->mR, asteroid->bR, colour);
  drawLineVec(asteroid->bR, asteroid->cR, colour);
  drawLineVec(asteroid->cR, asteroid->cL, colour);
  drawLineVec(asteroid->cL, asteroid->bL, colour);
  drawLineVec(asteroid->bL, asteroid->mL, colour);
  drawLineVec(asteroid->mL, asteroid->tL, colour);
}

// calculate vector posotions of asteroid from centre
// purely for abstraction
void calculateVec2OfAsteroid(asteroid_t* asteroid)
{
  asteroid->tL.x = asteroid->centre.x - 4;
  asteroid->tL.y = asteroid->centre.y + 10;

  asteroid->tR.x = asteroid->centre.x + 6;
  asteroid->tR.y = asteroid->centre.y + 10;

  asteroid->mL.x = asteroid->centre.x - 10;
  asteroid->mL.y = asteroid->centre.y + 2;

  asteroid->mR.x = asteroid->centre.x + 10;
  asteroid->mR.y = asteroid->centre.y + 2;

  asteroid->bL.x = asteroid->centre.x - 8;
  asteroid->bL.y = asteroid->centre.y - 6;

  asteroid->bR.x = asteroid->centre.x + 6;
  asteroid->bR.y = asteroid->centre.y - 8;

  asteroid->cL.x = asteroid->centre.x - 4;
  asteroid->cL.y = asteroid->centre.y - 6;

  asteroid->cR.x = asteroid->centre.x;
  asteroid->cR.y = asteroid->centre.y - 10;
}

// Initialises the Adafruit display
void initialiseScreen()
{
  Serial.begin(SERIALDISP);
  dis.init(WIDTH, HEIGHT);
  dis.fillScreen(BLACK);
}

void initialiseControls()
{
  pinMode(LEFTBUTTON, INPUT);
  pinMode(RIGHTBUTTON, INPUT);
  pinMode(SHOOTBUTTON, INPUT);
  pinMode(DEBUGSWITCH, INPUT);
}

// checks if shoot button pressed and shoots if shot cooldown expired
void checkShootInput(ship_t* ship, bulletList_t* bulletsList, 
                          int* shootCooldown)
{
  if (digitalRead(SHOOTBUTTON) == LOW && *shootCooldown == 0)
  {
    shootBullet(bulletsList, ship);
    *shootCooldown = SHOTCOOLDOWN;
    return;
  }

  /* shoot button not pressed */
  return;
}

// checks which movement button has been pressed and applies 
// rotation function accordingly
void checkMoveInput(ship_t* ship)
{
  /* checks which movement button has been pressed */
  if (digitalRead(LEFTBUTTON) == LOW)
  {
    rotateShip(ship, (-1 * ROTSTEP)); // negative for left rotation
    return;
  }

  if (digitalRead(RIGHTBUTTON) == LOW)
  {
     rotateShip(ship, ROTSTEP); // positive for right rotation
    return;
  }

  /* neither button pressed */
  return;
}

void updateWorldBullets(bulletList_t* bulletsList)
{
  /* check if world bullet list is not empty */
  if (bulletsList->head != NULL)
  {
    /* goes through list and updates all bullets */
    bullet_t* current = bulletsList->head;
    while(current != NULL)
    {
      moveBullet(current, bulletsList);
      current = current->next;
    }
  }

  /* exit function */
  return; 
}

// checks if a bullet has collided with the edge of the screen or an asteroid
bool bulletCollision(bullet_t* bullet, bulletList_t* bulletsList)
{
  /* check if bullet out of bounds of display */
  if (bullet->head.x > WIDTH || bullet->head.y > HEIGHT || 
      bullet->head.x < 0     || bullet->head.y < 0        )
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

  /* checks if bullet is head of list */
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

// updates the bullet's movement and draws bullet in updates location,
// also handles collision
void moveBullet(bullet_t* bullet, bulletList_t* bulletsList)
{
  /* degrees coonvereted to radians */
  float rad = bullet->deg * (M_PI / 180);

  /* unit vector in direction of input degree scaled by bullet speed */
  float dx = cos(rad) * BULLETSPEED;
  float dy = sin(rad) * BULLETSPEED;

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
void shootBullet(bulletList_t* bulletsList, ship_t* ship)
{
  vec2_t centre;
  int deg;

  /* fill location and direction varibes with ship data */
  centre.x = ship->centre.x;
  centre.y = ship->centre.y;
  deg = ship->dirDeg;

  /* allocate memory to new bullet */

  bullet_t* newBullet = (bullet_t*) malloc(sizeof(bullet_t));

  /* check if memory allocated successfully */

  if (newBullet == NULL)
  {
    dis.printf("newBullet malloc fail!");
    return;
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
  float rad = deg * (M_PI / 180);
  newBullet->head.x = centre.x + (cos(rad) * BULLETSPEED);
  newBullet->head.y = centre.y + (sin(rad) * BULLETSPEED);

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
  ship->dirDeg = 90; // pointing down on display, which has y axis reversed

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

// vector is scaled from its centre by an input scalar
void scaleVec2FromCentre(vec2_t* point, vec2_t* centre, float scalar) 
{
    point->x = centre->x + (point->x - centre->x) * scalar;
    point->y = centre->y + (point->y - centre->y) * scalar;
}

// draws a line between two vec2_t
// (purely for abstraction)
void drawLineVec(vec2_t p1, vec2_t p2, int colour)
{
  drawLineXY((int)round(p1.x), (int)round(p1.y), 
             (int)round(p2.x), (int)round(p2.y), colour);
}

// gets the euclidean distance between two points
// distance = sqrt( (x1 - x2)^2 + (y1 - y2)^2) )
float distanceBetweenVec2(vec2_t p1, vec2_t p2)
{
  float dx = p1.x - p2.x;
  float dy = p1.y - p2.y;
  return sqrt((dx * dx) + (dy * dy));
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