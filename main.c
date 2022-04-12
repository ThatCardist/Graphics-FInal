#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stdbool.h>
#ifdef USEGLEW
#include <GL/glew.h>
#endif
//  OpenGL with prototypes for glext
#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#include <GLUT/glut.h>
// Tell Xcode IDE to not gripe about OpenGL deprecation
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#else
#include <GL/glut.h>
#endif
//  Default resolution
//  For Retina displays compile with -DRES=2
#ifndef RES
#define RES 1
#endif

//variables for the world and the viewer
int th=10;          //  Azimuth of view angle
int ph=10;          //  Elevation of view angle
int mode = 0;     //define 0 = First person, 1 = item
int fov = 55;     //field of view
double w = 1;      // used for zoom
double dim = 10.0; //dimension of world
double asp = 1.0;  //used for aspect ratio
int item = 0; //item to display
int lw = 1; //length and width of fence in item mode
//movement bools for fpv mode
bool forward = false;
bool backward = false;
bool left = false;
bool right = false;
//vars for where fpv camera looks
double pitch = 0.0;
double yaw = 0.0;
double fpvEx = 2.0; //eye x position in fpv
double fpvEz = 2.0; //eye y position in fpv
//remove instances of shininess and change to shiny, or just make for each object
int light = 1;
int shininess;  // Shininess (power of two)
float shiny;  // Shininess (value)
unsigned int texture[14]; // Texture names
int worldMade = false;
unsigned int structurePlaces[36];


struct Cloud{double orbPlace[6]; double orbScale[6];};
int numOfClouds = 0;
struct Cloud cloudArr[10];

int zh =  90;  // Light azimuth
int num       =   10;  // Number of quads
int inf       =   0;  // Infinite distance light
int smooth    =   1;  // Smooth/Flat shading
int local     =   0;  // Local Viewer Model
int emission  =   0;  // Emission intensity (%)
int ambient   =   0;  // Ambient intensity (%)
int diffuse   = 100;  // Diffuse intensity (%)
int specular  =   0;  // Specular intensity (%)
int shininess =   0;  // Shininess (power of two)
float ylight = 6;
float distance = 10;
float yellow[] = {1.0,1.0,0.0,1.0};
int branchesMade = 0;
float branchAngles[3][3];


//define Cos and Sin in degrees
#define Cos(x) (cos((x)*3.14159265/180))
#define Sin(x) (sin((x)*3.14159265/180))

/*
 *  Convenience routine to output raster text
 *  Use VARARGS to make this more flexible
 *///from lecture
#define LEN 8192  //  Maximum length of text string
void Print(const char* format , ...)
{
   char    buf[LEN];
   char*   ch=buf;
   va_list args;
   //  Turn the parameters into a character string
   va_start(args,format);
   vsnprintf(buf,LEN,format,args);
   va_end(args);
   //  Display the characters one at a time at the current raster position
   while (*ch)
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,*ch++);
}
/*
 *  Check for OpenGL errors
 *///from lecture
void ErrCheck(const char* where)
{
   int err = glGetError();
   if (err) fprintf(stderr,"ERROR: %s [%s]\n",gluErrorString(err),where);
}
/*
 *  Print message to stderr and exit
 *///from lecture
void Fatal(const char* format , ...)
{
   va_list args;
   va_start(args,format);
   vfprintf(stderr,format,args);
   va_end(args);
   exit(1);
}

//random float generator from here...
//https://stackoverflow.com/a/33059025
double randfrom(double min, double max)
{
    double range = (max - min);
    double div = RAND_MAX / range;
    return min + (rand() / div);
}
//loadtexbmp (I couldnt get the makefile to work so put it in here for window)
//
//  Load texture from BMP file
//

//
//  Reverse n bytes
//
static void Reverse(void* x,const int n)
{
   int k;
   char* ch = (char*)x;
   for (k=0;k<n/2;k++)
   {
      char tmp = ch[k];
      ch[k] = ch[n-1-k];
      ch[n-1-k] = tmp;
   }
}

//
//  Load texture from BMP file
//
unsigned int LoadTexBMP(const char* file)
{
   unsigned int   texture;    // Texture name
   FILE*          f;          // File pointer
   unsigned short magic;      // Image magic
   unsigned int   dx,dy,size; // Image dimensions
   unsigned short nbp,bpp;    // Planes and bits per pixel
   unsigned char* image;      // Image data
   unsigned int   off;        // Image offset
   unsigned int   k;          // Counter
   unsigned int   max;        // Maximum texture dimensions

   //  Open file
   f = fopen(file,"rb");
   if (!f) Fatal("Cannot open file %s\n",file);
   //  Check image magic
   if (fread(&magic,2,1,f)!=1) Fatal("Cannot read magic from %s\n",file);
   if (magic!=0x4D42 && magic!=0x424D) Fatal("Image magic not BMP in %s\n",file);
   //  Read header
   if (fseek(f,8,SEEK_CUR) || fread(&off,4,1,f)!=1 ||
       fseek(f,4,SEEK_CUR) || fread(&dx,4,1,f)!=1 || fread(&dy,4,1,f)!=1 ||
       fread(&nbp,2,1,f)!=1 || fread(&bpp,2,1,f)!=1 || fread(&k,4,1,f)!=1)
     Fatal("Cannot read header from %s\n",file);
   //  Reverse bytes on big endian hardware (detected by backwards magic)
   if (magic==0x424D)
   {
      Reverse(&off,4);
      Reverse(&dx,4);
      Reverse(&dy,4);
      Reverse(&nbp,2);
      Reverse(&bpp,2);
      Reverse(&k,4);
   }
   //  Check image parameters
   glGetIntegerv(GL_MAX_TEXTURE_SIZE,(int*)&max);
   if (dx<1 || dx>max) Fatal("%s image width %d out of range 1-%d\n",file,dx,max);
   if (dy<1 || dy>max) Fatal("%s image height %d out of range 1-%d\n",file,dy,max);
   if (nbp!=1)  Fatal("%s bit planes is not 1: %d\n",file,nbp);
   if (bpp!=24) Fatal("%s bits per pixel is not 24: %d\n",file,bpp);
   if (k!=0)    Fatal("%s compressed files not supported\n",file);
#ifndef GL_VERSION_2_0
   //  OpenGL 2.0 lifts the restriction that texture size must be a power of two
   for (k=1;k<dx;k*=2);
   if (k!=dx) Fatal("%s image width not a power of two: %d\n",file,dx);
   for (k=1;k<dy;k*=2);
   if (k!=dy) Fatal("%s image height not a power of two: %d\n",file,dy);
#endif

   //  Allocate image memory
   size = 3*dx*dy;
   image = (unsigned char*) malloc(size);
   if (!image) Fatal("Cannot allocate %d bytes of memory for image %s\n",size,file);
   //  Seek to and read image
   if (fseek(f,off,SEEK_SET) || fread(image,size,1,f)!=1) Fatal("Error reading data from image %s\n",file);
   fclose(f);
   //  Reverse colors (BGR -> RGB)
   for (k=0;k<size;k+=3)
   {
      unsigned char temp = image[k];
      image[k]   = image[k+2];
      image[k+2] = temp;
   }

   //  Sanity check
   ErrCheck("LoadTexBMP");
   //  Generate 2D texture
   glGenTextures(1,&texture);
   glBindTexture(GL_TEXTURE_2D,texture);
   //  Copy image
   glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,dx,dy,0,GL_RGB,GL_UNSIGNED_BYTE,image);
   if (glGetError()) Fatal("Error in glTexImage2D %s %dx%d\n",file,dx,dy);
   //  Scale linearly when image size doesn't match
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

   //  Free image memory
   free(image);
   //  Return texture name
   return texture;
}
//define a structure for each light
struct Light{
  int distance;  // Light distance
  int local;  // Local Viewer Model
  int emission;  // Emission intensity (%)
  int ambient;  // Ambient intensity (%)
  int diffuse;  // Diffuse intensity (%)
  int specular;  // Specular intensity (%)
  int Zh;  // Light azimuth
  float position[4];
  float color[3];
};

void drawCamera(double x, double y, double z, double s, double r, double lensLength, double lensWid){
  //set lighting data
  float white[] = {1,1,1,1};
  float black[] = {0,0,0,1};
  glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
  glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
  glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,black);

  double wid = lensWid;
  glPushMatrix();
  //do translations
  glTranslated(x,y,z);
  glRotated(r,0,1,0);
  glScaled(s,s,s );
  //enable Textures
  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  //draw base
  glBegin(GL_POLYGON);
  glColor3f(1, 1, 1);
  glNormal3f(0.5, 0, .25);
  glVertex3f(0.0, 0.0, 0.0);
  glVertex3f(1.5, 0.0, 0.0);
  glVertex3f(1.625, 0.0, 0.125);
  glVertex3f(1.625, 0.0, .375);
  glVertex3f(1.5, 0.0, 0.5);
  glVertex3f(0.0, 0.0, 0.5);
  glVertex3f(-0.125, 0.0, .375);
  glVertex3f(-0.125, 0.0, 0.125);
  glVertex3f(0.0, 0.0, 0.0);
  glEnd();
  //draw top
  glBindTexture(GL_TEXTURE_2D, texture[7]);
  glBegin(GL_POLYGON);
  glColor3f(1, 1, 1);
  glNormal3f(0.5, 1, 0.25);
  glTexCoord2f(.07,0);glVertex3f(0.0, 1.0, 0.0);
  glTexCoord2f(.85,0);glVertex3f(1.5, 1.0, 0.0);
  glTexCoord2f(1,.25);glVertex3f(1.625, 1.0, 0.125);
  glTexCoord2f(1,.75);glVertex3f(1.625, 1.0, .375);
  glTexCoord2f(.85,1);glVertex3f(1.5, 1.0, 0.5);
  glTexCoord2f(.07,1);glVertex3f(0.0, 1.0, 0.5);
  glTexCoord2f(0,.75);glVertex3f(-0.125, 1.0, .375);
  glTexCoord2f(0,.25);glVertex3f(-0.125, 1.0, 0.125);
  glTexCoord2f(0.07,0);glVertex3f(0.0, 1.0, 0.0);
  glEnd();
  //draw sides
  glBindTexture(GL_TEXTURE_2D, texture[2]);
  //back
  glBegin(GL_QUADS);
  glColor3f(1, 1, 1);
  glNormal3f(0, 0, 1);
  glTexCoord2f(0,0);glVertex3d(0.0, 0.0, 0.0);
  glTexCoord2f(1,0);glVertex3d(0.0, 1.0, 0.0);
  glTexCoord2f(1,1);glVertex3d(1.5, 1.0, 0.0);
  glTexCoord2f(0,1);glVertex3d(1.5, 0.0, 0.0);
  glEnd();
  //right back
  glBegin(GL_QUADS);
  glNormal3f(0, -1, -1);
  glTexCoord2f(0,0);glVertex3d(1.5, 1.0, 0.0);
  glTexCoord2f(1,0);glVertex3d(1.625, 1.0, 0.125);
  glTexCoord2f(1,1);glVertex3d(1.625, 0.0, 0.125);
  glTexCoord2f(0,1);glVertex3d(1.5, 0.0, 0.0);
  glEnd();
  //right mid
  glBegin(GL_QUADS);
  glNormal3f(1,0,0);
  glTexCoord2f(0,0);glVertex3d(1.625, 0.0, 0.125);
  glTexCoord2f(1,0);glVertex3d(1.625, 0.0, 0.375);
  glTexCoord2f(1,1);glVertex3d(1.625, 1.0, .375);
  glTexCoord2f(0,1);glVertex3d(1.625, 1.0, .125);
  glEnd();
  //right front
  glBegin(GL_QUADS);
  glNormal3f(0, -1, 1);
  glTexCoord2f(0,0);glVertex3d(1.625, 1.0, .375);
  glTexCoord2f(1,0);glVertex3d(1.625, 0.0, .375);
  glTexCoord2f(1,1);glVertex3d(1.5, 0.0, 0.5);
  glTexCoord2f(0,1);glVertex3d(1.5, 1.0, 0.5);
  glEnd();
  //front
  glBegin(GL_QUADS);
  glNormal3f(0, 0, 1);
  glTexCoord2f(0,0);glVertex3d(1.5, 0.0, 0.5);
  glTexCoord2f(1,0);glVertex3d(1.5, 1.0, 0.5);
  glTexCoord2f(1,1);glVertex3d(0.0, 1.0, 0.5);
  glTexCoord2f(0,1);glVertex3d(0.0, 0.0, 0.5);
  glEnd();
  //left front
  glBegin(GL_QUADS);
  glNormal3f(-1, 1, 0);
  glTexCoord2f(0,0);glVertex3d(0.0, 1.0, 0.5);
  glTexCoord2f(1,0);glVertex3d(0.0, 0.0, 0.5);
  glTexCoord2f(1,1);glVertex3d(-0.125, 0.0, 0.375);
  glTexCoord2f(0,1);glVertex3d(-0.125, 1.0, 0.375);
  glEnd();
  //left mid
  glBegin(GL_QUADS);
  glNormal3f(-1, 0, 0);
  glTexCoord2f(0,0);glVertex3d(-0.125, 0.0, 0.375);
  glTexCoord2f(1,0);glVertex3d(-0.125, 1.0, 0.375);
  glTexCoord2f(1,1);glVertex3d(-0.125, 1.0, 0.125);
  glTexCoord2f(0,1);glVertex3d(-0.125, 0.0, 0.125);
  glEnd();
  //left back
  glBegin(GL_QUADS);
  glNormal3f(0, -1, -1);
  glTexCoord2f(0,0);glVertex3d(-0.125, 1.0, 0.125);
  glTexCoord2f(1,0);glVertex3d(-0.125, 0.0, 0.125);
  glTexCoord2f(1,1);glVertex3d(0.0, 0.0, 0.0);
  glTexCoord2f(0,1);glVertex3d(0.0, 1.0, 0.0);
  glEnd();

  //draw eyepiece
  glBegin(GL_QUADS);
  glBindTexture(GL_TEXTURE_2D, texture[5]);
  glColor3f(1, 1, 1);
  //right of eyepiece
  glNormal3f(1,0,0);
  glTexCoord2f(0,0);glVertex3d(0.9, 1.0, 0.0);
  glTexCoord2f(1,0);glVertex3d(0.9, 1.25, 0.0);
  glTexCoord2f(1,1);glVertex3d(0.9, 1.05, 0.5);
  glTexCoord2f(0,1);glVertex3d(0.9, 1.0, 0.5);
  glEnd();
  //left of eyepiece
  glBegin(GL_QUADS);
  glNormal3f(-1,0,0);
  glTexCoord2f(0,0);glVertex3d(0.45, 1.0, 0.0);
  glTexCoord2f(1,0);glVertex3d(0.45, 1.25, 0.0);
  glTexCoord2f(1,1);glVertex3d(0.45, 1.05, 0.5);
  glTexCoord2f(0,1);glVertex3d(0.45, 1.0, 0.5);
  glEnd();
  //front
  glBegin(GL_QUADS);
  glNormal3f(0, 0, 1);
  glTexCoord2f(0,0);glVertex3d(0.45, 1.0, 0.5);
  glTexCoord2f(1,0);glVertex3d(0.45, 1.05, 0.5);
  glTexCoord2f(1,1);glVertex3d(0.9, 1.05, 0.5);
  glTexCoord2f(0,1);glVertex3d(0.9, 1.0, 0.5);
  glEnd();
  //top of eyepiece
  glBegin(GL_QUADS);
  glNormal3f(0,1,1);
  glTexCoord2f(0,0);glVertex3d(0.9, 1.05, 0.5);
  glTexCoord2f(1,0);glVertex3d(0.9, 1.25, 0.0);
  glTexCoord2f(1,1);glVertex3d(0.45, 1.25, 0.0);
  glTexCoord2f(0,1);glVertex3d(0.45, 1.05, 0.5);
  glEnd();
  //back of eyepiece
  glBindTexture(GL_TEXTURE_2D, texture[4]);
  glBegin(GL_QUADS);
  glNormal3f(0, 0, 1);
  glTexCoord2f(1,0);glVertex3d(0.9, 1.00, 0.0);
  glTexCoord2f(1,.35);glVertex3d(0.9, 1.25, 0.0);
  glTexCoord2f(0,.35);glVertex3d(0.45, 1.25, 0.0);
  glTexCoord2f(0,0);glVertex3d(0.45, 1.00, 0.0);
  glEnd();

  //draw lens
  glTranslated(0.65, 0.5, 0.05);
  glRotated(-90, 0, 1, 0);
  glColor3f(1, 1, 1);
  //lens barrel
  glBindTexture(GL_TEXTURE_2D, texture[6]);
  glBegin(GL_QUAD_STRIP);
  for (int th=0;th<=360;th+=15)
  {
    glBindTexture(GL_TEXTURE_2D, texture[6]);
    glNormal3d(0, wid*Cos(th), wid*Sin(th));
    glTexCoord3f(wid*Sin(th), 0, wid*Cos(th));
    glVertex3d(0, wid*Cos(th), wid*Sin(th));
    glTexCoord3f(wid*Sin(th), 1, wid*Cos(th));
    glVertex3d(lensLength, wid*Cos(th), wid*Sin(th));
  }
  glEnd();
  //lens front glass

  glBindTexture(GL_TEXTURE_2D, texture[1]);
  glColor3f(1, 1, 1);
  glBegin(GL_TRIANGLE_FAN);
  glNormal3f(lensLength, 0, 0);
  //add glass texture
  glTexCoord2f(0,0);
  glVertex3d(lensLength, 0.0, 0.0);
  for (int th=0;th<=360;th+=15){
    glTexCoord2f(1/4*Cos(th)+wid,1/4*Sin(th)+wid);
    glVertex3d(lensLength,wid*Cos(th),wid*Sin(th));
  }
  glEnd();
  glDisable(GL_TEXTURE_2D);
  glPopMatrix();
  glWindowPos2i(10, 10);
}

void drawGround(){
  //enalble textures
  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glColor3f(.5, .25, .25);
  glBindTexture(GL_TEXTURE_2D, texture[0]);
  //taken from wall example
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(1,1);
  double mul = num/dim;
  glPushMatrix();
  glTranslated(-dim/2,0,-dim/2);
  glColor3f(1.0,1.0,1.0);
  glBegin(GL_QUADS);
  glNormal3f(0,0,1);
  for (int i=0;i<num * 5;i++){
     for (int j=0;j<num * 5;j++){
        glTexCoord2d(mul*(i+0),mul*(j+0)); glVertex3d( i ,0, j );
        glTexCoord2d(mul*(i+1),mul*(j+0)); glVertex3d(i+1,0, j );
        glTexCoord2d(mul*(i+1),mul*(j+1)); glVertex3d(i+1,0, j+1);
        glTexCoord2d(mul*(i+0),mul*(j+1)); glVertex3d( i ,0, j+1);
     }
   }
  glEnd();
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_POLYGON_OFFSET_FILL);
  glPopMatrix();
}

void staticCamera(){
   //  Screen edge
   //  Save transform attributes (Matrix Mode and Enabled Modes)
   glPushAttrib(GL_TRANSFORM_BIT|GL_ENABLE_BIT);
   //  Save projection matrix and set unit transform
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();
   glOrtho(-asp,+asp,-1,1,-1,1);
   //  Save model view matrix and set to indentity
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();
   drawCamera(-1.5,-1.15,1,.75,0,1,1);
   //  Reset model view matrix
   glPopMatrix();
   //  Reset projection matrix
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   //  Pop transform attributes (Matrix Mode and Enabled Modes)
   glPopAttrib();
}

//Draw epic background for you to take pictures of
static void drawBackground(){
  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glColor3f(.1,.1,1);
  glBindTexture(GL_TEXTURE_2D, texture[3]);
  //backSide
  glBegin(GL_QUADS);
  glColor3f(1,1,1);
  glNormal3f(dim, dim, 0);
  glTexCoord2f(0,0);glVertex3d(-dim, 0, -dim);
  glTexCoord2f(1,0);glVertex3d(dim, 0, -dim);
  glTexCoord2f(1,1);glVertex3d(dim, dim, -dim);
  glTexCoord2f(0,1);glVertex3d(-dim, dim, -dim);
  glEnd();
  //right side
  glBegin(GL_QUADS);
  glNormal3f(0, dim, dim);
  glTexCoord2f(0,0);glVertex3d(dim, 0, -dim);
  glTexCoord2f(-1,0);glVertex3d(dim, 0, dim);
  glTexCoord2f(-1,1);glVertex3d(dim, dim, dim);
  glTexCoord2f(0,1);glVertex3d(dim, dim, -dim);
  glEnd();
  //front sides
  glBegin(GL_QUADS);
  glNormal3f(-dim, -dim, 0);
  glTexCoord2f(0,0);glVertex3d(dim, 0, dim);
  glTexCoord2f(1,0);glVertex3d(-dim, 0, dim);
  glTexCoord2f(1,1);glVertex3d(-dim, dim, dim);
  glTexCoord2f(0,1);glVertex3d(dim, dim, dim);
  glEnd();
  //left side
  glBegin(GL_QUADS);
  glNormal3f(0, -dim, -dim);
  glTexCoord2f(0,0);glVertex3d(-dim, 0, dim);
  glTexCoord2f(-1,0);glVertex3d(-dim, 0, -dim);
  glTexCoord2f(-1,1);glVertex3d(-dim, dim, -dim);
  glTexCoord2f(0,1);glVertex3d(-dim, dim, dim);
  glEnd();
  glDisable(GL_TEXTURE_2D);
}
 //from lecture
static void Vertex(double th,double ph){
   double x = Sin(th)*Cos(ph);
   double y = Cos(th)*Cos(ph);
   double z =         Sin(ph);
   //  For a sphere at the origin, the position
   //  and normal vectors are the same
   glNormal3d(x,y,z);
   glVertex3d(x,y,z);
}
//from lecture
static void ball(double x,double y,double z,double sx, double sy, double sz){
   //  Save transformation
   glPushMatrix();
   //  Offset, scale and rotate
   glTranslated(x,y,z);
   glScaled(sx,sy,sz);
   //  White ball with yellow specular
   float yellow[]   = {1.0,1.0,0.0,1.0};
   float Emission[] = {0.0,0.0,0.01*0,1.0};
   //glColor3f(1,1,1);
   glMaterialf(GL_FRONT,GL_SHININESS,shiny);
   glMaterialfv(GL_FRONT,GL_SPECULAR,yellow);
   glMaterialfv(GL_FRONT,GL_EMISSION,Emission);
   //  Bands of latitude
   for (int ph=-90;ph<90;ph+=10)
   {
      glBegin(GL_QUAD_STRIP);
      for (int th=0;th<=360;th+=20)//maybe change back to inc like in hw6
      {
         Vertex(th,ph);
         Vertex(th,ph+10);
      }
      glEnd();
   }
   //  Undo transofrmations
   glPopMatrix();
}

void drawSkyscraper(int segments){
  //tall building with antenea on top
  glPushMatrix();
  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glBindTexture(GL_TEXTURE_2D, texture[10]);
  for(int i = 0; i < segments; i += 1){
    glPushMatrix();
    glRotatef(i %360, 0, 1, 0);
    glTranslated(0, (double)i/10, 0);
    for(int j = 0; j < 360; j+=60){
      glPushMatrix();
      glRotated(j, 0,1,0);
      glBegin(GL_QUADS);
        glNormal3d(.433, .05, .75);
        glTexCoord2d(0,0);glVertex3d(1,0,0);
        glTexCoord2d(.5, 0);glVertex3d(.5, 0, .866);
        glTexCoord2d(.5, .2);glVertex3d(.5, .1, .866);
        glTexCoord2d(0, .2);glVertex3d(1, .1, 0);
      glEnd();
      glPopMatrix();
    }
    glPopMatrix();
  }
  glDisable(GL_TEXTURE_2D);
  glPopMatrix();
}

void drawPicket(double x, double z, double r){
  glPushMatrix();
  glTranslated(x,0,z);
  glRotated(r, 0, 1,0);
  glScaled(2.5, 2.5, 2.5);
  glColor3f(1, 1, 1);
  //first picket
  //front face
  glBegin(GL_POLYGON);
    glNormal3f(0, 0, 1);
    glVertex3d(0, 0, 0);
    glVertex3d(0.2, 0, 0);
    glVertex3d(0.2, 0.4, 0);
    glVertex3d(0.1, 0.5, 0);
    glVertex3d(0, 0.4, 0);
  glEnd();
  //back face
  glBegin(GL_POLYGON);
    glNormal3f(0, 0, -1);
    glVertex3d(0, 0, -.05);
    glVertex3d(0.2, 0, -.05);
    glVertex3d(0.2, 0.4, -.05);
    glVertex3d(0.1, 0.5, -.05);
    glVertex3d(0, 0.4, -.05);
  glEnd();
  //drawing sides starting at bottom left and going around clockwise
  glBegin(GL_QUADS);
    glNormal3f(-1, 0, 0);
    glVertex3d(0, 0, 0);
    glVertex3d(0, 0, -.05);
    glVertex3d(0, .4, -.05);
    glVertex3d(0, .4, 0);
  glEnd();
  glBegin(GL_QUADS);
    glNormal3f(-1, 1,0);
    glVertex3d(0, .4, 0);
    glVertex3d(0, .4, -.05);
    glVertex3d(.1, .5, -.05);
    glVertex3d(0.1, .5, 0);
  glEnd();
  glBegin(GL_QUADS);
    glNormal3f(1, 1,0);
    glVertex3d(0.1, .5, -.05);
    glVertex3d(0.1, .5, 0);
    glVertex3d(.2, .4, 0);
    glVertex3d(.2, .4, -.05);
  // glEnd();
  glBegin(GL_QUADS);
    glNormal3f(1, 0, 0);
    glVertex3d(0.2, .4, 0);
    glVertex3d(0.2, .4, -.05);
    glVertex3d(.2, 0, -.05);
    glVertex3d(.2, 0, 0);
  glEnd();
  glPopMatrix();
}

void drawFence(int length, int width){
  //white picket fence that repeats for a certian length and width
  //should go around house
  glPushMatrix();
  glScaled(.5,.5,.5);
  //glTranslated(-length-.25, 0, 0);
  //length
  double x = 2*length;
  for(double i = 0.0; i < length; i++){
    drawPicket(i, 0, 0);
    //drawBeam(x, y, r);
    //drawBeam(x, y, r);
    drawPicket(i, width, 0);
  }
  glPopMatrix();
  glPushMatrix();
  glScaled(.5,.5,.5);
  //glTranslated(0, 0, -width);
  double y = .25*width;
  //width
  for(double i = 0.0; i < width; i++){
    drawPicket(0, i, 270);
    //drawBeam(x, y, r);
    //drawBeam(x, y, r);
    drawPicket(length, i, 270);
  }
  glPopMatrix();
}

void drawHouse(){
  //house should be kinda basic
  //4 walls and roof and some windows and doors
  glColor3f(1,1,1);
  glPushMatrix();
  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glBindTexture(GL_TEXTURE_2D, texture[9]);
  //backSide
  glBegin(GL_QUADS);
    glNormal3f(0, 0, -1);
    glTexCoord2d(0,0);glVertex3d(0,0,0);
    glTexCoord2d(4,0);glVertex3d(4, 0, 0);
    glTexCoord2d(4,2);glVertex3d(4, 2, 0);
    glTexCoord2d(0,2);glVertex3d(0, 2, 0);
  glEnd();
  //front
  glBegin(GL_QUADS);
  glNormal3f(0, 0, +1);
  glTexCoord2d(0,0);glVertex3d(0,0,3);
  glTexCoord2d(4,0);glVertex3d(4,0,3);
  glTexCoord2d(4,2);glVertex3d(4, 2, 3);
  glTexCoord2d(0,2);glVertex3d(0, 2, 3);
  glEnd();
  //right
  glColor3f(1,1,1);
  glBegin(GL_POLYGON);
  glNormal3f(1,0, 0);
  glTexCoord2d(0,0);glVertex3d(4, 0, 0);
  glTexCoord2d(2,0);glVertex3d(4, 0, 3);
  glTexCoord2d(2,2);glVertex3d(4, 2, 3);
  glTexCoord2d(1,3);glVertex3d(4, 3, 1.5);
  glTexCoord2d(0,2);glVertex3d(4, 2, 0);
  glEnd();
  //left
  glColor3f(1,1,1);
  glBegin(GL_POLYGON);
  glNormal3f(-1,0 ,0);
  glTexCoord2d(0,0);glVertex3d(0, 0, 0);
  glTexCoord2d(2,0);glVertex3d(0, 0, 3);
  glTexCoord2d(2,2);glVertex3d(0, 2, 3);
  glTexCoord2d(1,3);glVertex3d(0, 3, 1.5);
  glTexCoord2d(0,2);glVertex3d(0, 2, 0);
  glEnd();
  //door
  glBegin(GL_QUADS);
  glEnd();
  //roof front
  glBindTexture(GL_TEXTURE_2D, texture[12]);
  glBegin(GL_QUADS);
  glNormal3f(0,1 ,-1 );
  glTexCoord2d(0,0);glVertex3d(0, 2, 0);
  glTexCoord2d(2,0);glVertex3d(4,2, 0);
  glTexCoord2d(2,1);glVertex3d(4, 3, 1.5);
  glTexCoord2d(0,1);glVertex3d(0, 3, 1.5);
  glEnd();
  //roof back
  glBegin(GL_QUADS);
  glNormal3f(0, 1, 1);
  glTexCoord2d(0,0);glVertex3d(4, 2, 3);
  glTexCoord2d(2,0);glVertex3d(0, 2, 3);
  glTexCoord2d(2,1);glVertex3d(0, 3, 1.5);
  glTexCoord2d(0,1);glVertex3d(4, 3, 1.5);
  glEnd();
  //chimney
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(1,1);
  glColor3f(1,1,1);
  glBindTexture(GL_TEXTURE_2D, texture[9]);
  //back
  glBegin(GL_QUADS);
  glNormal3f(0, 0, -1);
  glTexCoord2d(0,0);glVertex3d(0, 2, 1);
  glTexCoord2d(1,0);glVertex3d(1,2,1);
  glTexCoord2d(1,2);glVertex3d(1, 4, 1);
  glTexCoord2d(0,2);glVertex3d(0, 4, 1);
  glEnd();
  //front
  glBegin(GL_QUADS);
  glNormal3f(0, 0, 1);
  glTexCoord2d(0,0);glVertex3d(0, 2, 2);
  glTexCoord2d(1,0);glVertex3d(1,2,2);
  glTexCoord2d(1,2);glVertex3d(1, 4, 2);
  glTexCoord2d(0,2);glVertex3d(0, 4, 2);
  glEnd();
  //left
  glBegin(GL_QUADS);
  glNormal3f(-1, 0, 0);
  glTexCoord2d(0,0);glVertex3d(0, 2, 1);
  glTexCoord2d(1,0);glVertex3d(0, 2, 2);
  glTexCoord2d(1,2);glVertex3d(0, 4, 2);
  glTexCoord2d(0,2);glVertex3d(0, 4, 1);
  glEnd();
  //right
  glBegin(GL_QUADS);
  glNormal3f(1, 0, 0);
  glTexCoord2d(0,0);glVertex3d(1, 2, 1);
  glTexCoord2d(1,0);glVertex3d(1, 2, 2);
  glTexCoord2d(1,2);glVertex3d(1, 4, 2);
  glTexCoord2d(0,2);glVertex3d(1, 4, 1);
  glEnd();
  glDisable(GL_POLYGON_OFFSET_FILL);
  //chimney top
  glBegin(GL_QUADS);
  glNormal3f(.5, 4, 1.5);
  glVertex3d(0, 4, 1);
  glVertex3d(1, 4, 1);
  glVertex3d(1, 4, 2);
  glVertex3d(0, 4, 2);
  glEnd();
  glDisable(GL_TEXTURE_2D);
  glPopMatrix();
}

void drawHome(){
  glPushMatrix();
  glPushMatrix();
  glTranslated(.5, 0, 2);
  glScaled(.75, .75, .75);
  drawHouse();
  glPopMatrix();
  drawFence(10, 15);
  glPopMatrix();
}
//fix light
void drawLampPost(){
  //need to add lighting for every lamp
  //should be made from cyllinders, and pyramid on top
  //maybe add transparent frosted glass for light to shine through
  glPushMatrix();
  //draw post part
  //glRotated();
  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glBindTexture(GL_TEXTURE_2D, texture[8]);
  glColor3f(1,1,1);
  glBegin(GL_QUAD_STRIP);
  //draw main pole
  for (int th=0;th<=360;th+=15){
    glNormal3d(.1*Cos(th),0 ,1*Sin(th));
    glTexCoord3d(0, .1*Sin(th), .1*Cos(th));
    glVertex3d(.1*Cos(th), 0, .1*Sin(th));
    glTexCoord3d(1, .1*Sin(th), .1*Cos(th));
    glVertex3d(.1*Cos(th), 2, .1*Sin(th));
  }
  glEnd();
  //draw base at top of pole
  glBegin(GL_QUADS);
    glNormal3d(0,2,0);
    glTexCoord2d(0, 0);glVertex3d(-.2, 2, .2);
    glTexCoord2d(.2, 0);glVertex3d(.2, 2, .2);
    glTexCoord2d(.2, 1);glVertex3d(.2, 2, -.2);
    glTexCoord2d(0, 1);glVertex3d(-.2, 2, -.2);
  glEnd();
  //draw quads for corners
  glBegin(GL_QUADS);
    glNormal3d(0, 0, 1);
    glTexCoord2d(0, 0);glVertex3d(.1, 2, .2);
    glTexCoord2d(.2, 0);glVertex3d(.2, 2, .2);
    glTexCoord2d(.2, 1);glVertex3d(.2, 2.4, .2);
    glTexCoord2d(0, 1);glVertex3d(.1, 2.4, .2);
  glEnd();
  glBegin(GL_QUADS);
    glNormal3d(1, 0, 0);
    glTexCoord2d(0, 0);glVertex3d(.2, 2, .2);
    glTexCoord2d(.2, 0);glVertex3d(.2, 2, .1);
    glTexCoord2d(.2, 1);glVertex3d(.2, 2.4, .1);
    glTexCoord2d(0, 1);glVertex3d(.2, 2.4, .2);
  glEnd();
  glBegin(GL_QUADS);
    glNormal3d(0,0,-1);
    glTexCoord2d(0, 0);glVertex3d(.1, 2, -.2);
    glTexCoord2d(.2, 0);glVertex3d(.2, 2, -.2);
    glTexCoord2d(.2, 1);glVertex3d(.2, 2.4,-.2);
    glTexCoord2d(0, 1);glVertex3d(.1, 2.4,-.2);
  glEnd();
  glBegin(GL_QUADS);
    glNormal3d(1, 0, 0);
    glTexCoord2d(0, 0);glVertex3d(.2, 2, -.2);
    glTexCoord2d(.2, 0);glVertex3d(.2, 2, -.1);
    glTexCoord2d(.2, 1);glVertex3d(.2, 2.4,-.1);
    glTexCoord2d(0, 1);glVertex3d(.2, 2.4,-.2);
  glEnd();
  glBegin(GL_QUADS);
    glNormal3d(0, 0, -1);
    glTexCoord2d(0, 0);glVertex3d(-.1, 2,-.2);
    glTexCoord2d(.2, 0);glVertex3d(-.2, 2,-.2);
    glTexCoord2d(.2, 1);glVertex3d(-.2, 2.4,-.2);
    glTexCoord2d(0, 1);glVertex3d(-.1, 2.4,-.2);
  glEnd();
  glBegin(GL_QUADS);
    glNormal3d(-1, 0, 0);
    glTexCoord2d(0, 0);glVertex3d(-.2, 2, -.2);
    glTexCoord2d(.2, 0);glVertex3d(-.2, 2, -.1);
    glTexCoord2d(.2, 1);glVertex3d(-.2, 2.4,-.1);
    glTexCoord2d(0, 1);glVertex3d(-.2, 2.4,-.2);
  glEnd();
  glBegin(GL_QUADS);
    glNormal3d(-1, 0, 0);
    glTexCoord2d(0, 0);glVertex3d(-.1, 2, .2);
    glTexCoord2d(.2, 0);glVertex3d(-.2, 2, .2);
    glTexCoord2d(.2, 1);glVertex3d(-.2, 2.4, .2);
    glTexCoord2d(0, 1);glVertex3d(-.1, 2.4, .2);
  glEnd();
  glBegin(GL_QUADS);
    glNormal3d(-1, 0, 0);
    glTexCoord2d(0, 0);glVertex3d(-.2, 2, .2);
    glTexCoord2d(.2, 0);glVertex3d(-.2, 2, .1);
    glTexCoord2d(.2, 1);glVertex3d(-.2, 2.4, .1);
    glTexCoord2d(0, 1);glVertex3d(-.2, 2.4, .2);
  glEnd();
  //draw pyramid for top
    glBegin(GL_POLYGON);
      glNormal3d(0, 1, 1);
      glVertex3d(-.2,2.4, .2);
      glVertex3d(.2, 2.4, .2);
      glVertex3d(0,2.5,0);
    glEnd();
    glBegin(GL_POLYGON);
      glNormal3d(1, 1, 0);
      glVertex3d(.2,2.4, .2);
      glVertex3d(.2, 2.4, -.2);
      glVertex3d(0,2.5,0);
    glEnd();
    glBegin(GL_POLYGON);
      glNormal3d(0, 1, -1);
      glVertex3d(.2,2.4, -.2);
      glVertex3d(-.2, 2.4, -.2);
      glVertex3d(0,2.5,0);
    glEnd();
    glBegin(GL_POLYGON);
      glNormal3d(-1, 1, 0);
      glVertex3d(-.2,2.4, -.2);
      glVertex3d(-.2, 2.4, .2);
      glVertex3d(0,2.5,0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();


}

//do reasearch with random clouds
void generateClouds(int numClouds){
  struct Cloud newCloud;
  for(int i = 0; i < numClouds; i++){
    double cloudPlace[3];
    double cloudScale[3];
    for(int j = 0; j < 6; j++){
      cloudScale[0] = randfrom(.1, 2);
      cloudScale[1] = randfrom(.1, 2);
      cloudScale[2] = randfrom(.1, 2);
      cloudPlace[0] = randfrom(0, 1);
      cloudPlace[1] = randfrom(0, 1);
      cloudPlace[2] = randfrom(0, 1);
      newCloud.orbPlace[j] = *cloudPlace;
      newCloud.orbScale[j] = *cloudScale;
    }
    cloudArr[i] = newCloud;
  }
}

void drawCloud(){
  //make out of spheres and oblong spheres? mess with trnasparency,
  //maybe just do a transparent texture
  if(numOfClouds == 0){
    numOfClouds = rand() % 5;
    generateClouds(5);
  }
  glPushMatrix();
  glColor3f(1, 1, 1);
  struct Cloud c;
  //for loop to generate each cloud
  for(int i = 0; i < numOfClouds; i++){
    //double placex = (rand() / rand()) % 5
    //for loop to generate orbs
    c = cloudArr[i];
    for(int j = 0; j < 6; j++){
      ball(c.orbPlace[0], c.orbPlace[1], c.orbPlace[2], c.orbScale[0], c.orbScale[1], c.orbScale[2]);
    }
  }
  glPopMatrix();
}

void drawTree(){
  //tree with branches from cyllinders and maybe make a leaf obj
  //could do leaves from spheres with branches, make have different color
  glPushMatrix();
  glColor3f(.7, .3, .3);
  //main trunk
  //glBindTexture(GL_TEXTURE_2D, texture[])
  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glBindTexture(GL_TEXTURE_2D, texture[13]);
  glBegin(GL_QUAD_STRIP);
  for (int th=0;th<=360;th+=10)
  {
    glNormal3d(.25*Cos(th),0 ,1*Sin(th));
    glTexCoord3d(0, .25*Sin(th), .25*Cos(th));
    glVertex3d(.25*Cos(th), 0, .25*Sin(th));
    glTexCoord3d(1, .25*Sin(th), .25*Cos(th));
    glVertex3d(.25*Cos(th), 2, .25*Sin(th));
  }
  glEnd();
  glColor3f(.85, .55, .06);
  glTranslated(0, 1, 0);
  ball(0, 1.75, 0, 1 ,1 , 1);

  //first set of branches
  if(branchesMade == 0){
    for(int i = 0; i < 3; i++){
        branchAngles[i][0] = randfrom(15, 60);
        branchAngles[i][1] = 0;
        branchAngles[i][2] = randfrom(15, 60);
    }
    branchesMade = 1;
  }
  for(int i = 0; i < 3; i++){
    glColor3f(.7, .3, .3);
    glPushMatrix();
    glTranslated(0, 1.25, 0);
    glRotated(120*i, 0, 1, 0);
    glRotated(branchAngles[i][0], 1, 0, 0);
    //glRotated(branchAngles[i][2], 0, 0, 1);

    glBegin(GL_QUAD_STRIP);
    for (int th=0;th<=360;th+=10)
    {
      glNormal3d(.1*Cos(th),0 ,1*Sin(th));
      glTexCoord2f(0, 0);
      glVertex3d(.1*Cos(th), 0, .1*Sin(th));
      glTexCoord2f(1, 0);
      glVertex3d(.1*Cos(th), 1, .1*Sin(th));
    }
    glEnd();
    glColor3f(.85, .55, .06);
    glTranslated(0, 1, 0);
    ball(0, 0, 0, .5 , .5 , .5);

    glPopMatrix();
  }
  //second set of branches
  //leaves
  glDisable(GL_TEXTURE_2D);
  glPopMatrix();
}

void drawCar(){
  //basic car, 4 wheels, maybe find an obj cuz i cant even draw a car on paper
}



void drawScene(){
  if(worldMade == false){
    for(int i = 0; i < 36; i++){
      structurePlaces[i] = rand() % 4;
    }
    worldMade = true;
  }
  glPushMatrix();
    glPushMatrix();
      glTranslated(-14, 0, -14);
      drawGround();
    glPopMatrix();
    for(int i = 0; i < 36; i += 6){
      for(int j = 0; j < 6; j++){
        glPushMatrix();
        glTranslated(-8 , 0, -6);
        int loc = i + j;
        glColor3f(1, 1, 1);
        glTranslated(i, 0, j * 4);
        if(structurePlaces[loc] == 0){
          //glScaled(.5,.5,.5);
          drawSkyscraper(100);
        }else if(structurePlaces[loc] == 1){
          glScaled(.5, .5, .5);
          drawTree();
        }else if(structurePlaces[loc] == 2){
          glScaled(.5, .5, .5);
          drawLampPost();
        }else if(structurePlaces[loc] == 3){
          glScaled(.5,.5,.5);
          drawHome();
        }
        glPopMatrix();
      }
    }
    //place clouds in the sky
    glTranslated((Sin(glutGet(GLUT_ELAPSED_TIME) / 1000.0)*10) - 7, 10, 3);
    drawCloud();
    glTranslated((Cos(glutGet(GLUT_ELAPSED_TIME) / 500.0)*10) + 2, 10, -2);
    drawCloud();
    glTranslated((Cos(glutGet(GLUT_ELAPSED_TIME) / 500.0)*10)-3, 10, 7);
    drawCloud();
    glTranslated((Cos(glutGet(GLUT_ELAPSED_TIME) / 500.0)*10)+ 7, 10, -6);
    drawCloud();
    // drawCloud();
    // drawCloud();
    // drawCloud();
    // drawCloud();
  glPopMatrix();
}


//function to calculate eye direction
//modified from thepentamollisproject
void camera(){
  //calculate direction to move camera based on where camera is facing
  if(forward == true){
    fpvEx += Cos(th + 90);
    fpvEz -= Sin(th + 90);
  }
  if(backward == true){
    fpvEx += Cos(th + 270);
    fpvEz -= Sin(th + 270);
  }
  if(left == true){
    fpvEx += Cos(th + 180);
    fpvEz -= Sin(th + 180);
  }
  if(right == true){
    fpvEx += Cos(th);
    fpvEz -= Sin(th);
  }
  if(ph >= 70.0){
    ph = 70;
  }
  if(ph <= -70.0){
    ph = -70;
  }
  glRotatef(-ph, 1.0, 0.0, 0.0);//rotate along x axis
  glRotatef(-th, 0.0, 1.0, 0.0);//rotate along y axis
  glTranslatef(-fpvEx/10, -.25, -fpvEz/10);
}

//from lecture
void drawAxes(){
  const double len=2.0;  //  Length of axes
  glBegin(GL_LINES);
  glVertex3d(0.0,0.0,0.0);
  glVertex3d(len,0.0,0.0);
  glVertex3d(0.0,0.0,0.0);
  glVertex3d(0.0,len,0.0);
  glVertex3d(0.0,0.0,0.0);
  glVertex3d(0.0,0.0,len);
  glEnd();
  //  Label axes
  glRasterPos3d(len,0.0,0.0);
  Print("X");
  glRasterPos3d(0.0,len,0.0);
  Print("Y");
  glRasterPos3d(0.0,0.0,len);
  Print("Z");
}

void Project(){
  //  Tell OpenGL we want to manipulate the projection matrix
  glMatrixMode(GL_PROJECTION);
  //  Undo previous transformations
  glLoadIdentity();
  //  Perspective transformation
  if (mode == 0){
     gluPerspective(fov,asp,0.1,4*dim);
  }else{//object mode
    glOrtho(-asp*dim,+asp*dim, -dim,+dim, -dim,+dim);
  }
  //  Switch to manipulating the model matrix
  glMatrixMode(GL_MODELVIEW);
  //  Undo previous transformations
  glLoadIdentity();
}

void display(){
   //  Erase the window and the depth buffer
   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
   //set background glColor
   glClearColor(.3, .6, .9, 1.0);
   //  Enable Z-buffering in OpenGL
   glEnable(GL_DEPTH_TEST);
   //  Undo previous transformations
   glLoadIdentity();
   if(mode == 0){// First person mode
     camera();
     //  White
     glColor3f(1,1,1);
     //  Five pixels from the lower left corner of the window
     glWindowPos2i(5,5);
     //  Print the text string
     Print("Camera position=%d,%d , %d",fpvEx,fpvEz, mode);
     //drawBackground();
     staticCamera();
     drawScene();
     glPushMatrix();
       glTranslated(5, 0, 5);
       glScaled(2,2,2);
       drawBackground();
     glPopMatrix();
   }
   //view items by itself
   else{
     glScaled(w, w, w);
     glRotatef(ph,1,0,0);
     glRotatef(th,0,1,0);
     drawAxes();
     if(item == 0){
       drawCamera(0.0,0.0,0.0,2 * w,0.0, 1.0, .3);
     }else if(item == 1){
       drawLampPost();
     }else if(item == 2){
       if(numOfClouds == 0){
         generateClouds(1);
       }
       drawCloud(0,0,0);
     }else if(item == 3){
       drawFence(lw, lw);
     }else if(item == 4){
       drawHome();
     }else if(item == 5){
       drawSkyscraper(100);
     }else if(item == 6){
       drawTree();
     }
     //  White
     glColor3f(1,1,1);
     //  Five pixels from the lower left corner of the window
     glWindowPos2i(5,5);
     //  Print the text string
     Print("Angle=%d,%d , %d, %d",th,ph, mode, item);
    }
    // struct Light mainLight = {
    //   0, 0, 1, 10, 50, 0, 90, {5*Cos(90), 1,5*Sin(90),1.0}, {1, .8, .8}
    // };
    //makeLights(mainLight);
   //  Render the scene
   //ErrCheck("display");
   if (light)
   {
      //  Translate intensity to color vectors
      float Ambient[]   = {0.01*25 ,0.01*25 ,0.01*25 ,1.0};
      float Diffuse[]   = {0.01*100 ,0.01*100 ,0.01*100 ,1.0};
      float Specular[]  = {0.01*specular,0.01*specular,0.01*specular,1.0};
      //  Light position
      float Position[]  = {distance*Cos(zh),ylight,distance*Sin(zh),1.0};
      //  Draw light position as ball (still no lighting here)
      glColor3f(1,1,1);
      ball(Position[0],Position[1],Position[2] , 0.1,.1,.1);
      //  OpenGL should normalize normal vectors
      glEnable(GL_NORMALIZE);
      //  Enable lighting
      glEnable(GL_LIGHTING);
      //  Location of viewer for specular calculations
      glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,local);
      //  glColor sets ambient and diffuse color materials
      glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
      glEnable(GL_COLOR_MATERIAL);
      //  Enable light 0
      glEnable(GL_LIGHT0);
      //  Set ambient, diffuse, specular components and position of light 0
      glLightfv(GL_LIGHT0,GL_AMBIENT ,Ambient);
      glLightfv(GL_LIGHT0,GL_DIFFUSE ,Diffuse);
      glLightfv(GL_LIGHT0,GL_SPECULAR,Specular);
      glLightfv(GL_LIGHT0,GL_POSITION,Position);
   }
   else
      glDisable(GL_LIGHTING);
   glFlush();
   glutSwapBuffers();
}


/*
 *  GLUT calls this routine when an arrow key is pressed
 */
 //change only fpv mode
void special(int key,int x,int y)
{
   //  Right arrow key - increase angle by 5 degrees
   if (key == GLUT_KEY_RIGHT) th -= 5;
   //  Left arrow key - decrease angle by 5 degrees
   else if (key == GLUT_KEY_LEFT) th += 5;
   //  Up arrow key - increase elevation by 5 degrees
   else if (key == GLUT_KEY_UP)
      ph += 5;
   //  Down arrow key - decrease elevation by 5 degrees
   else if (key == GLUT_KEY_DOWN)
      ph -= 5;
   //  Keep angles to +/-360 degrees
   th %= 360;
   ph %= 360;
     //  Tell GLUT it is necessary to redisplay the scene
   Project();
   glutPostRedisplay();
}

/*
 *  GLUT calls this routine when a key is pressed
 */
//change since only fpv mode
void key(unsigned char ch,int x,int y)
{
   //  Exit on ESC
  if (ch == 27) exit(0);
   //  Reset view angle
  else if (ch == '0') th = ph = 0;
  else if(ch == '-' || ch == '_') w -= .1; //zoom out
  else if(ch == '+' || ch == '=') w += .1; //zoom in
  else if(ch == 'm' || ch == 'M') mode = (mode + 1) % 2;//change mode of screen
  else if(ch == 'w' || ch == 'W') forward = true;
  else if(ch == 'a' || ch == 'A') left = true;
  else if(ch == 's' || ch == 'S')backward = true;
  else if(ch == 'd' || ch == 'D')right = true;
  else if(ch == 'i')item = (item + 1) % 7; //change item in item view
  else if(ch == ' ')generateClouds(1);
  else if(ch == 'l')light = (light + 1) % 2;
  else if(ch == '[')ylight -= .1;
  else if(ch == ']')ylight += .1;
  else if(ch == '.')distance += .1;
  else if(ch == ',')distance -= .1;
  else if(ch == 'p' && lw > 0) lw -= 1;
  else if(ch == 'P') lw += 1;
  else if(ch == 'r'){numOfClouds = 0; worldMade = false;}


  //  Tell GLUT it is necessary to redisplay the scene
  Project();
  glutPostRedisplay();
}

void keyboard_up(unsigned char ch,int x,int y){
  if(ch == 'w' || ch == 'W') forward = false;
  else if(ch == 'a' || ch == 'A') left = false;
  else if(ch == 's' || ch == 'S') backward = false;
  else if(ch == 'd' || ch == 'D') right = false;
  Project();
}

/*
 *  GLUT calls this routine when the window is resized
 *///from lecture
void reshape(int width,int height)
{
  //  Ratio of the width to the height of the window
  asp = (height>0) ? (double)width/height : 1;
  //  Set the viewport to the entire window
  glViewport(0,0, RES*width,RES*height);
  //  Set projection
  Project();
}



/*
 *  GLUT calls this routine when there is nothing else to do
 *///from lecture
void idle()
{
   double t = glutGet(GLUT_ELAPSED_TIME)/1000.0;
   zh = fmod(90*t,360);
   glutPostRedisplay();
}

/*
 *  Start up GLUT and tell it what to do
 *///from lecture
int main(int argc,char* argv[])
{
   //  Initialize GLUT and process user parameters
   glutInit(&argc,argv);
   //  Request double buffered, true color window with Z buffering at 600x600
   glutInitWindowSize(800,450);
   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
   //  Create the window
   glutCreateWindow("Wesley Lang final Project");
#ifdef USEGLEW
   //  Initialize GLEW
   if (glewInit()!=GLEW_OK) Fatal("Error initializing GLEW\n");
#endif
   //  Tell GLUT to call "idle" when there is nothing else to do
   glutIdleFunc(idle);
   //  Tell GLUT to call "display" when the scene should be drawn
   glutDisplayFunc(display);
   //  Tell GLUT to call "reshape" when the window is resized
   glutReshapeFunc(reshape);
   //  Tell GLUT to call "special" when an arrow key is pressed
   glutSpecialFunc(special);
   //  Tell GLUT to call "key" when a key is pressed
   glutKeyboardFunc(key);
   //tell GLUT when a key is released
   glutKeyboardUpFunc(keyboard_up);
   //pass the Textures
   texture[0] = LoadTexBMP("textures/grass.bmp");//img from-> https://www.google.com/url?sa=i&url=http%3A%2F%2Fwww.readingorders.net%2Fseamless-desert-sand-texture%2F&psig=AOvVaw0axTrpEalqxD6n6sRPVIs2&ust=1634147144758000&source=images&cd=vfe&ved=0CAwQjhxqFwoTCNjwy_G2xfMCFQAAAAAdAAAAABAD
   texture[1] = LoadTexBMP("textures/glass.bmp");//img from-> https://www.istockphoto.com/photo/glass-background-texture-gm1060207626-283390489?utm_source=unsplash&utm_medium=affiliate&utm_campaign=srp_photos_top&utm_content=https%3A%2F%2Funsplash.com%2Fs%2Fphotos%2Fglass-texture&utm_term=glass%20texture%3A%3Asearch-aggressive-affiliates-v1%3Ab
   texture[2] = LoadTexBMP("textures/leather.bmp"); //img from -> https://www.google.com/url?sa=i&url=https%3A%2F%2Fwww.sketchuptextureclub.com%2Ftextures%2Fmaterials%2Fleather&psig=AOvVaw3ODXj946h3sXTrdr05D-KY&ust=1633985815111000&source=images&cd=vfe&ved=0CAwQjhxqFwoTCMiJ_9LdwPMCFQAAAAAdAAAAABAD
   texture[3] = LoadTexBMP("textures/mountains.bmp");//my own image
   texture[4] = LoadTexBMP("textures/eyepiece.bmp");//i created
   texture[5] = LoadTexBMP("textures/generalTexture.bmp");//i created
   texture[6] = LoadTexBMP("textures/lensbarell.bmp");//i created
   texture[7] = LoadTexBMP("textures/cameraTop.bmp");//i created
   texture[8] = LoadTexBMP("textures/metal.bmp");//i created from this https://seamless-pixels.blogspot.com/2012/09/free-seamless-metal-textures_28.html
   texture[9] = LoadTexBMP("textures/brick.bmp"); //from here https://3docean.net/item/brick-texture-natural/28517658
   texture[10] = LoadTexBMP("textures/building.bmp");//from here https://www.shutterstock.com/image-photo/glass-building-skyscraper-texture-seamless-1307976139
   texture[11] = LoadTexBMP("textures/cloud.bmp"); //from here https://www.shutterstock.com/image-illustration/background-seamless-texture-sky-clouds-web-238274254
   texture[12] = LoadTexBMP("textures/shingles.bmp"); //from here https://www.shutterstock.com/image-photo/roof-shingles-background-texture-567551566
   texture[13] = LoadTexBMP("textures/bark.bmp"); //from here https://pixabay.com/photos/bark-wood-tree-seamless-texture-1517215/

   //texture[8] = loadtexbmp("textures/grass.bmp")
   //  Pass control to GLUT so it can interact with the user
   glutMainLoop();
   return 0;
}
