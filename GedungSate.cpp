#include <iostream>
#include "stdlib.h"
#include "gl/glut.h"
#include "math.h"
#include "imageloader.h"
#include "vec3f.h"

using namespace std;

int w=600, h=600, z=10;
int a=0 , b=0, c=0, d=0;
int x1=0, y2=0, sudut=0, z1=0;
float skalaX=1, skalaY=1, skalaZ=1;
int cx, cy;


const GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
const GLfloat light_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 1.0f };

const GLfloat light_ambient2[] = { 0.3f, 0.3f, 0.3f, 0.0f };
const GLfloat light_diffuse2[] = { 0.3f, 0.3f, 0.3f, 0.0f };

const GLfloat mat_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 70.0f };


class Terrain {
	private:
		int w; //Width
		int l; //Length
		float** hs; //Heights
		Vec3f** normals;
		bool computedNormals; //Whether normals is up-to-date
	public:
		Terrain(int w2, int l2) {
			w = w2;
			l = l2;
			
			hs = new float*[l];
			for(int i = 0; i < l; i++) {
				hs[i] = new float[w];
			}
			
			normals = new Vec3f*[l];
			for(int i = 0; i < l; i++) {
				normals[i] = new Vec3f[w];
			}
			
			computedNormals = false;
		}
		
		~Terrain() {
			for(int i = 0; i < l; i++) {
				delete[] hs[i];
			}
			delete[] hs;
			
			for(int i = 0; i < l; i++) {
				delete[] normals[i];
			}
			delete[] normals;
		}
		
		int width() {
			return w;
		}
		
		int length() {
			return l;
		}
		
		//Sets the height at (x, z) to y
		void setHeight(int x, int z, float y) {
			hs[z][x] = y;
			computedNormals = false;
		}
		
		//Returns the height at (x, z)
		float getHeight(int x, int z) {
			return hs[z][x];
		}
		
		//Computes the normals, if they haven't been computed yet
		void computeNormals() {
			if (computedNormals) {
				return;
			}
			
			//Compute the rough version of the normals
			Vec3f** normals2 = new Vec3f*[l];
			for(int i = 0; i < l; i++) {
				normals2[i] = new Vec3f[w];
			}
			
			for(int z = 0; z < l; z++) {
				for(int x = 0; x < w; x++) {
					Vec3f sum(0.0f, 0.0f, 0.0f);
					
					Vec3f out;
					if (z > 0) {
						out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
					}
					Vec3f in;
					if (z < l - 1) {
						in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
					}
					Vec3f left;
					if (x > 0) {
						left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
					}
					Vec3f right;
					if (x < w - 1) {
						right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
					}
					
					if (x > 0 && z > 0) {
						sum += out.cross(left).normalize();
					}
					if (x > 0 && z < l - 1) {
						sum += left.cross(in).normalize();
					}
					if (x < w - 1 && z < l - 1) {
						sum += in.cross(right).normalize();
					}
					if (x < w - 1 && z > 0) {
						sum += right.cross(out).normalize();
					}
					
					normals2[z][x] = sum;
				}
			}
			
			//Smooth out the normals
			const float FALLOUT_RATIO = 0.5f;
			for(int z = 0; z < l; z++) {
				for(int x = 0; x < w; x++) {
					Vec3f sum = normals2[z][x];
					
					if (x > 0) {
						sum += normals2[z][x - 1] * FALLOUT_RATIO;
					}
					if (x < w - 1) {
						sum += normals2[z][x + 1] * FALLOUT_RATIO;
					}
					if (z > 0) {
						sum += normals2[z - 1][x] * FALLOUT_RATIO;
					}
					if (z < l - 1) {
						sum += normals2[z + 1][x] * FALLOUT_RATIO;
					}
					
					if (sum.magnitude() == 0) {
						sum = Vec3f(0.0f, 1.0f, 0.0f);
					}
					normals[z][x] = sum;
				}
			}
			
			for(int i = 0; i < l; i++) {
				delete[] normals2[i];
			}
			delete[] normals2;
			
			computedNormals = true;
		}
		
		//Returns the normal at (x, z)
		Vec3f getNormal(int x, int z) {
			if (!computedNormals) {
				computeNormals();
			}
			return normals[z][x];
		}
};

GLuint loadTexture(Image* image) {
	GLuint textureId;
	glGenTextures(1, &textureId); //Make room for our texture
	glBindTexture(GL_TEXTURE_2D, textureId); //Tell OpenGL which texture to edit
	//Map the image to the texture
	glTexImage2D(GL_TEXTURE_2D,                //Always GL_TEXTURE_2D
				 0,                            //0 for now
				 GL_RGB,                       //Format OpenGL uses for image
				 image->width, image->height,  //Width and height
				 0,                            //The border of the image
				 GL_RGB, //GL_RGB, because pixels are stored in RGB format
				 GL_UNSIGNED_BYTE, //GL_UNSIGNED_BYTE, because pixels are stored
				                   //as unsigned numbers
				 image->pixels);               //The actual pixel data
	return textureId; //Returns the id of the texture
}

GLuint _textureId;

//Loads a terrain from a heightmap.  The heights of the terrain range from
//-height / 2 to height / 2.
Terrain* loadTerrain(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for(int y = 0; y < image->height; y++) {
		for(int x = 0; x < image->width; x++) {
			unsigned char color =
				(unsigned char)image->pixels[3 * (y * image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);
		}
	}
	
	delete image;
	t->computeNormals();
	return t;
}

float _angle = 60.0f;
Terrain* _terrain;

void cleanup() {
	delete _terrain;
}


void myKeyboard(unsigned char key, int x, int y){
 if (key =='a') z+=5;
 else if (key == 'd') z-=5;
 else if (key == 'w') {
  x1=0;
  y2=-1;
  z1=0;
  sudut+=-10;
 }
 else if (key == 's') {
  y2=1;
  x1=0;
  z1=0;
  sudut+=-10;
 }
 else if (key == 'q') {
  y2=0;
  x1=0;
  z1=1;
  sudut+=-10;
 }
 else if (key=='z'){
  a+=-1;
  d+=1;
  b=0;
  c=0;
  ;
      }
      else if (key=='c'){
           d+=-1;
  a+=1;
  b=0;
  c=0;
  ;
      }
}

void init(){

 glShadeModel(GL_SMOOTH);
  glShadeModel(GL_SMOOTH);
   glShadeModel(GL_SMOOTH);
   	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
 glClearColor(0.0f,0.0f,0.0f,0.0f);
 glClearDepth(1.0f);
 glEnable(GL_DEPTH_TEST);
 glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);


   Image* image = loadBMP("wall.bmp");
_textureId = loadTexture(image);
	delete image;

 glEnable(GL_LIGHTING);
 glEnable(GL_LIGHT0);
 return;

}



void renderScene(void){
 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 	glClearColor(0.0, 1.8, 5.0, 0.0);

 glLoadIdentity();
 glTranslatef(10,0,z-80);
 glRotatef(sudut,x1,y2,z1);
 glScalef(skalaX, skalaY, skalaZ);
 
    glPushMatrix();
 	float scale = 80.0f / max(_terrain->width() - 1, _terrain->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float)(_terrain->width() - 1) / 2,
				 0.0f,
				 -(float)(_terrain->length() - 1) / 2);
	 glEnable(GL_COLOR_MATERIAL);
	glColor3f(0.1f, 0.9f, 0.0f);
	for(int z = 0; z < _terrain->length() - 1; z++) {
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);
		for(int x = 0; x < _terrain->width(); x++) {
			Vec3f normal = _terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, _terrain->getHeight(x, z), z);
			normal = _terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, _terrain->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	} glDisable(GL_COLOR_MATERIAL);
	 glPopMatrix();
 
 

 //tembok
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
 glRotatef(50,0,1,0);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);

	
  //tembok2
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,-0.9,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
   //tembok2
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,-0.9,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
    //tembok2
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,-0.9,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 

 
 
   //tembok tangga kanan
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(1,0.7,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
 
    //tembok4 panjang kanan
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(2,-0.9,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
     //tembok4 panjang kanan
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(3,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
  
 
 
     //tembok tangga kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-7,1,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
     //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-2,-0.9,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
     //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-3,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
      //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
       //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
        //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);

       //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
        //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-3,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
         //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(20,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
  
         //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
       //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
       //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
       //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
       //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
        //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,-2,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
         //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
  
         //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
          //tembok panjang kiri
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
 
           //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(12,5,1.3);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 
           //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 
           //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 
           //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,0,-2.6);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 
           //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 
  
           //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(-1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 
           //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(-4,-3,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 
 
            //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(-2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 
             //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(-2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 
             //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(-2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 
             //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(-2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 
 
            //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(17,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 
             //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
             //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
             //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
             //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 
              //Jendela atas
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(0,-2,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 
              //Jendela bawah
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(0,-0.5,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(-2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 
 
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(-2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 

 
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(-2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 

 
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
glColor3f(0.3,0.0,0.0);
  glTranslatef(-2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL);
 

//pintu
 
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(-4,0.5,0);
 glRotatef(360,360,360,360);
 glutSolidCube(3);
 glDisable(GL_COLOR_MATERIAL);
 
//pintu
 
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.8,0,0.2);
  glTranslatef(0,1,0);
 glRotatef(360,360,360,360);
 glutSolidCube(2);
 glDisable(GL_COLOR_MATERIAL); 
 
  
//pintu
 
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,-1.5,-1);
 glRotatef(360,360,360,360);
 glutSolidCube(2);
 glDisable(GL_COLOR_MATERIAL); 

//jendela bawah kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(-5,0,1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 

//jendela bawah kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(-2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 

//jendela bawah kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(-2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 

//jendela bawah kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(-2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 

//jendela bawah kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(-2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 

//jendela bawah kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,0.5,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 
 
 
 //tembok kanan bawah
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(2,0,-6);
 glRotatef(360,360,360,360);
 glutSolidCube(2.5);
 glDisable(GL_COLOR_MATERIAL); 
 
  //tembok kanan bawah
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,0,-1);
 glRotatef(360,360,360,360);
 glutSolidCube(2.5);
 glDisable(GL_COLOR_MATERIAL); 
 
   //tembok kanan bawah
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,0,-1);
 glRotatef(360,360,360,360);
 glutSolidCube(2.5);
 glDisable(GL_COLOR_MATERIAL); 
 
   //tembok kanan bawah
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,0,-1);
 glRotatef(360,360,360,360);
 glutSolidCube(2.5);
 glDisable(GL_COLOR_MATERIAL); 
 
 
   //tembok kanan bawah
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,0,-1);
 glRotatef(360,360,360,360);
 glutSolidCube(2.5);
 glDisable(GL_COLOR_MATERIAL); 
 
 
   //tembok kanan bawah
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,0,-1);
 glRotatef(360,360,360,360);
 glutSolidCube(2.5);
 glDisable(GL_COLOR_MATERIAL); 
 
   //tembok kanan bawah
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,0,-1);
 glRotatef(360,360,360,360);
 glutSolidCube(2.5);
 glDisable(GL_COLOR_MATERIAL); 
 
 //jendela bangunan kiri
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(-0.5,0.7,-1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 
 
 
  //jendela bangunan kiri
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(1,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 
 
 
  
  //jendela bangunan kiri
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0.5,0,1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 
 
 //jendela bangunan kiri
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,0,1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 
 
 
  //jendela bangunan kiri
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,0,1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 
 
 
 
  //jendela bangunan kiri
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,0,1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 
 
 
 
  //jendela bangunan kiri
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,0,1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 
 
 
  //jendela bangunan kiri
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,0,1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 
 
 
 
  //jendela bangunan kiri
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,0,1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 
 
  //pintu bangunan kiri
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,-1,-3);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 
 
 //pintu bangunan kiri
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,-0.3,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 
 
  //tembok bangunan kanan
  	
	
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(22,0.5,4);
 glRotatef(360,360,360,360);
 glutSolidCube(2.5);
 glDisable(GL_COLOR_MATERIAL); 
 
   //tembok bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,0,-1);
 glRotatef(360,360,360,360);
 glutSolidCube(2.5);
 glDisable(GL_COLOR_MATERIAL); 
 
   //tembok bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,0,-1);
 glRotatef(360,360,360,360);
 glutSolidCube(2.5);
 glDisable(GL_COLOR_MATERIAL); 
 
   //tembok bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,0,-1);
 glRotatef(360,360,360,360);
 glutSolidCube(2.5);
 glDisable(GL_COLOR_MATERIAL); 
 
    //tembok bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,0,-1);
 glRotatef(360,360,360,360);
 glutSolidCube(2.5);
 glDisable(GL_COLOR_MATERIAL); 
 
    //tembok bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,0,-1);
 glRotatef(360,360,360,360);
 glutSolidCube(2.5);
 glDisable(GL_COLOR_MATERIAL); 
 
    //tembok bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(2,2,2);
  glTranslatef(0,0,-1);
 glRotatef(360,360,360,360);
 glutSolidCube(2.5);
 glDisable(GL_COLOR_MATERIAL); 
 
     //jendela bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(-0.7,0.7,-1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 
 	
      //jendela bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(1.2,0,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 

 
 
       //jendela bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(-1.5,0,1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 


       //jendela bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,0,1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 
 
        //jendela bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,0,1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 


       //jendela bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,0,1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 


       //jendela bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,0,1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 


       //jendela bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,0,1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 


       //jendela bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,0,1);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 

       //pintu bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,-1,-4);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 

       //pintu bangunan kanan
  
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.3,0.0,0.0);
  glTranslatef(0,-0.6,0);
 glRotatef(360,360,360,360);
 glutSolidCube(0.6);
 glDisable(GL_COLOR_MATERIAL); 
 
 // Draw ground
 glEnable(GL_COLOR_MATERIAL);
	glColor3f(0.0f, 0.0f, 0.0f);
	 glTranslatef(-11,0,0);
	glBegin(GL_QUADS);
		glVertex3f(	-3.0f, -0.5f, -7.0f);
		glVertex3f(-3.0f, -0.5f,  12.0f);
		glVertex3f( 3.0f, -0.5f,  12.0f);
		glVertex3f( 3.0f, -0.5f, -7.0f);
	glEnd();
  glDisable(GL_COLOR_MATERIAL); 
  
  //jalan
glPushMatrix();
glEnable(GL_COLOR_MATERIAL);		    
glColor3f(0.0f, 0.0f, 0.0f);
	 glTranslatef(0,-1.9,-20);
	glBegin(GL_QUADS);
		glVertex3f(	-22.0f, -0.5f, -5.0f);
		glVertex3f(-22.0f, -0.5f,  5.0f);
		glVertex3f( 22.0f, -0.5f,  5.0f);
		glVertex3f( 22.0f, -0.5f, -5.0f);
	glEnd();
	 glDisable(GL_COLOR_MATERIAL);
	
     
  glPushMatrix();
     glEnable(GL_COLOR_MATERIAL);		    
glColor3f(2.0f, 2.0f, 2.0f);
	 glTranslatef(-20,0.2,-1);
     glBegin(GL_QUADS);
     glVertex3f(0.0f, -0.5f, 0.0f);
     glVertex3f(0.0f, -0.5f,  1.0f);
     glVertex3f( 5.0f, -0.5f,  1.0f);
		glVertex3f(5.0f, -0.5f, 0.0f);
	glEnd();
 	glDisable(GL_COLOR_MATERIAL);
 	
 	 glEnable(GL_COLOR_MATERIAL);		    
glColor3f(2.0f, 2.0f, 2.0f);
	 glTranslatef(9,0,-0);
     glBegin(GL_QUADS);
     glVertex3f(0.0f, -0.5f, 0.0f);
     glVertex3f(0.0f, -0.5f,  1.0f);
     glVertex3f( 5.0f, -0.5f,  1.0f);
		glVertex3f(5.0f, -0.5f, 0.0f);
	glEnd();
 	glDisable(GL_COLOR_MATERIAL);
 	
 	glEnable(GL_COLOR_MATERIAL);		    
glColor3f(2.0f, 2.0f, 2.0f);
	 glTranslatef(9,0,-0);
     glBegin(GL_QUADS);
     glVertex3f(0.0f, -0.5f, 0.0f);
     glVertex3f(0.0f, -0.5f,  1.0f);
     glVertex3f( 5.0f, -0.5f,  1.0f);
		glVertex3f(5.0f, -0.5f, 0.0f);
	glEnd();
 	glDisable(GL_COLOR_MATERIAL);
 	
 	glEnable(GL_COLOR_MATERIAL);		    
glColor3f(2.0f, 2.0f, 2.0f);
	 glTranslatef(9,0,-0);
     glBegin(GL_QUADS);
     glVertex3f(0.0f, -0.5f, 0.0f);
     glVertex3f(0.0f, -0.5f,  1.0f);
     glVertex3f( 5.0f, -0.5f,  1.0f);
		glVertex3f(5.0f, -0.5f, 0.0f);
	glEnd();
 	glDisable(GL_COLOR_MATERIAL);
 	
 	glEnable(GL_COLOR_MATERIAL);		    
glColor3f(2.0f, 2.0f, 2.0f);
	 glTranslatef(9,0,-0);
     glBegin(GL_QUADS);
     glVertex3f(0.0f, -0.5f, 0.0f);
     glVertex3f(0.0f, -0.5f,  1.0f);
     glVertex3f( 5.0f, -0.5f,  1.0f);
		glVertex3f(5.0f, -0.5f, 0.0f);
	glEnd();
 	glDisable(GL_COLOR_MATERIAL);
glPopMatrix();

//mobil merah
	
 	glEnable(GL_COLOR_MATERIAL);		    
glColor3f(1.0f, 0.0f, 0.0f);
	 glTranslatef(a-20,b-6.5,c-28);
     glutSolidCube(1.2);
 	glDisable(GL_COLOR_MATERIAL);
 		glPopMatrix();
 		
 //mobil biru
 glEnable(GL_COLOR_MATERIAL);		    
glColor3f(0.0f, 0.0f, 1.0f);
	 glTranslatef(d+20,b-7.5,c-34);
     glutSolidCube(1.2);
 	glDisable(GL_COLOR_MATERIAL);
 	glPopMatrix();
	  
//^^^^update bangunan dari sini^^^

//jendela
glPushMatrix();
glEnable(GL_COLOR_MATERIAL);
glColor3f(1.5,1.8,1.5);
glTranslatef(0.5,0.1,1.46);
glScalef(3,3,1);
glutSolidCube(0.01);
glDisable(GL_COLOR_MATERIAL);
glPopMatrix();

glPushMatrix();
glEnable(GL_COLOR_MATERIAL);
glColor3f(1.5,1.8,1);
glTranslatef(0.9,0.1,1.46);
glScalef(3,3,1);
glutSolidCube(0.01);
glDisable(GL_COLOR_MATERIAL);
glPopMatrix();

glPushMatrix();
glEnable(GL_COLOR_MATERIAL);
glColor3f(1.5,1.8,1);
glTranslatef(0.9,-0.3,1.46);
glScalef(3,3,1);
glutSolidCube(0.01);
glDisable(GL_COLOR_MATERIAL);
glPopMatrix();

glPushMatrix();
glEnable(GL_COLOR_MATERIAL);
glColor3f(1.5,1.8,1);
glTranslatef(0.5,-0.3,1.46);
glScalef(3,3,1);
glutSolidCube(0.01);
glDisable(GL_COLOR_MATERIAL);
glPopMatrix();



//atap
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.8,0,0);
 glRotatef(5,0,1,0);
 glTranslatef(0,1,0);
 glScalef(3,1.3,3);
 glutSolidOctahedron();
 glDisable(GL_COLOR_MATERIAL);
 glPopMatrix();
 
 //atap2
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.8,0,0);
 glRotatef(5,0,1,0);
 glTranslatef(0,1.5,0);
 glScalef(3,1.3,3);
 glutSolidOctahedron();
 glDisable(GL_COLOR_MATERIAL);
 glPopMatrix();
 
 //atap3
 glPushMatrix();
 glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.8,0,0);
 glRotatef(5,0,1,0);
 glTranslatef(0,2,0);
 glScalef(3,1.3,3);
 glutSolidOctahedron();
 glDisable(GL_COLOR_MATERIAL);
 glPopMatrix();
 
  //sate1

  glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.8,0,0);
 glRotatef(5,0,1,0);
 glTranslatef(0,3,0);
 glScalef(3,5.3,3);
  glutSolidSphere(0.1,12,16);
   glDisable(GL_COLOR_MATERIAL);
   
    //sate2

  glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.5,0,0);
 glRotatef(5,0,1,0);
 glTranslatef(0,0.05,0);
 glScalef(3,8,3);
  glutSolidSphere(0.02,12,12);
   glDisable(GL_COLOR_MATERIAL);

    //sate3

  glEnable(GL_COLOR_MATERIAL);
 glColor3f(0.5,0,0);
 glRotatef(5,0,1,0);
 glTranslatef(0,0.02,0);
 glScalef(3,7,3);
  glutSolidSphere(0.003,12,12);
   glDisable(GL_COLOR_MATERIAL);




 


 glutSwapBuffers();
}

void resize(int w1, int h1){
 glViewport(0,0,w1,h1);
 glMatrixMode(GL_PROJECTION);
 glLoadIdentity();
 gluPerspective(45.0,(float) w1/(float) h1, 1.0,300.0);
 glMatrixMode(GL_MODELVIEW); 
 glLoadIdentity();
}

void timer(int value){
 glutPostRedisplay();
 glutTimerFunc(50,timer,0);
}

main (int argc, char **argv){
 glutInit(&argc, argv);
 glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
 glutInitWindowPosition(140,60);
 glutInitWindowSize(1024,700);
 glutCreateWindow("Gedung Sate Jaman Doeloe");
 glutDisplayFunc(renderScene);
 
 _terrain = loadTerrain("heightmap.bmp",13);
 
 glutReshapeFunc(resize);
 glutKeyboardFunc(myKeyboard);
 glutTimerFunc(1,timer,0);
 init();
 
 
 glutMainLoop();
}
