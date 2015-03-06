/* Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* File for "Terrain" lesson of the OpenGL tutorial on
 * www.videotutorialsrock.com
 */



#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "imageloader.h"
#include "vec3f.h"
#define PI 3.14159265

using namespace std;
float theta = 0;
// Camera direction
float lx = 0.0, ly = 1.0; // camera points initially along y-axis
float angle = 0.0; // angle of rotation for the camera direction
float deltaAngle = 0.0; // additional angle change when dragging

// Mouse drag control
int isDragging = 0; // true when dragging
int xDragStart = 0; // records the x-coordinate when dragging starts



//Represents a terrain, by storing a set of heights and normals at 2D locations
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
            srand(time(NULL));
            h = h * (rand()%10) / 10.0 + (rand()%2) / 2.0;
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

class Top {
    private:
        int camera;
    public:
        int fired;
        float velx, vely, velz;
        float x, y, z;
        Top();
        void drawTop();
        void setPosition(float vx, float vy, float vz) {
            x = vx;
            y = vy;
            z = vz; 
        }
        void setCameraPosition(int tmp) {
            camera = tmp;
        }
        int getCameraPosition() {
            return camera;
        }
};

Top::Top() {
    x = 5;
    y = 0;
    z = 58;
    camera = 1;
    velx = vely = velz = 0;
    fired = 0;
}

void Top::drawTop() {

    y = _terrain->getHeight(x, z);
    glPushMatrix();
    glTranslatef(x, y, z);
    Vec3f N = _terrain->getNormal(x, z);
    float rotation_angle = acos(N[1] / (sqrt(N[0] * N[0] + N[1] * N[1] + N[2] * N[2])));
    glRotatef(-rotation_angle / PI * 180,
            -N[2] / (sqrt(N[0] * N[0] + N[1] * N[1] + N[2] * N[2])),
            0,
            N[0] / (sqrt(N[0] * N[0] + N[1] * N[1] + N[2] * N[2])));
    glTranslatef(0, 4.5, 0);
    glRotatef(theta, 0, 1, 0);
    glRotatef(90, 1, 0, 0);
    glColor3f(0.0, 0.0, 0.0);
    glScalef(5.0, 5.0, 5.0);
    glPushMatrix();
    glutSolidTorus(0.2, 0.5 , 25 , 30);

    GLUquadricObj *quadratic;
    quadratic = gluNewQuadric();
    glColor3f(1.0, 1.0, 1.0);
    gluCylinder(quadratic,0.70f,0.001f,0.9f,32,32);
    glTranslatef(0.0, 0.0, -0.5);
    gluCylinder(quadratic,0.001f,0.55f,0.3f,32,32);

    glPopMatrix();
    glPopMatrix();

}

Top T;

class Extras {
    private:
        float r;
        float tx, ty, tz;
        float speed;
    public:
        int power;
        int score;
        float theta, start_time;
        float pointerX, pointerY, pointerZ;
        Extras(float, float, float);
        void drawTarget();
        void changePointerDirection(int);
        void drawLine(float, float, float, float, float, float, float);
        void showScoreBoard();
        float getSpeedFactor() {
            return speed;
        }

        float getTargetX() {
            return tx;
        }

        float getTargetY() {
            return ty;
        }

        float getTargetZ() {
            return tz;
        }
};

Extras::Extras(float Ax, float Ay, float Az) {
    tx = Ax;
    ty = Ay;
    tz = Az;
    r = 80;
    score = 10;
    power = 0;
    theta = 0;
    speed = 0.1;
    pointerY = 0;
    pointerX = T.x - r * cos(PI - theta);
    pointerZ = T.z - r * sin(PI - theta);
}

void Extras::showScoreBoard() {
    glPushMatrix();
    glTranslatef(-60, 20, -3);
    glColor3f(1.0, 0.5, 0.2 );
    glRasterPos2f(-3.2, 1.7);
    char s[] = "Scoreboard";
    int i;
    for(i = 0 ; s[i] ; i++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, s[i]);
    glTranslatef(4, -5, 0);
    glColor3f(0.8, 1.0, 0.7);
    glRasterPos2f(-3.0, 1.5);
    char str[10];
    sprintf(str, "%d", score);
    for(i = 0 ; str[i] ; i++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, str[i]);

    glTranslatef(160, 5, 0);
    glColor3f(0.6, 0.95, 1.0 );
    glRasterPos2f(2.35, 1.25);
    char st[] = "Power";
    for(i = 0 ; st[i] ; i++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, st[i]);

    glPopMatrix();
}

void Extras::drawTarget() {

    float right_top_height = _terrain->getHeight(tx, tz);
    glTranslatef(tx, right_top_height + 5, tz);
    glRotatef(-45, 0, 1, 0);
    glColor3f(1.0, 0.0, 0.0);
    glutSolidTorus(0.8, 4.5, 25, 30);
    glColor3f(1.0, 1.0, 1.0);
    glutSolidTorus(0.8, 4, 25, 30);
    glColor3f(1.0, 0.0, 0.0);
    glutSolidTorus(0.8, 3, 25, 30);
    glColor3f(1.0, 1.0, 1.0);
    glutSolidTorus(0.8, 2, 25, 30);
    glColor3f(1.0, 0.0, 0.0);
    glutSolidTorus(0.8, 0.8, 25, 30);
    glPopMatrix();

}

void Extras::changePointerDirection(int flag) {
    if(flag == 1)
        theta -= 0.01;
    else
        theta += 0.01;

    pointerX = T.x - r * cos(PI + theta);
    pointerZ = T.z - r * sin(PI + theta);
}

void Extras::drawLine(float ax, float ay, float az, float bx, float by, float bz, float width) {

    glLineWidth(width);
    glBegin(GL_LINES);
    glVertex3f(ax, ay, az);
    glVertex3f(bx, by, bz);
    glEnd();

}

Extras E(59, 0, 0);

void topupdate(int val) {

    int prevx = 1, prevz = 1;
    // Check previous direction of the striker prevx(x direction) +/- 1 and same for prevy
    if(T.velx < 0)
        prevx = -1;
    if(T.velz < 0)
        prevz = -1;

    float tmptheta;
    float acceleration = -0.0005;
    // Apply friction only when the striker is moving
    if(T.velx && T.velz) {
        tmptheta = -atan(T.velz / T.velx) * 180 / PI;
        if(T.velx < 0)
            acceleration = -acceleration;
        //cout << T.velx <<"accx" << acceleration << endl;
        T.velx = T.velx + acceleration * cos(tmptheta * PI / 180) * 10;
        acceleration = -0.0005;
        if(T.velz < 0)
            acceleration = -acceleration;
        //cout << T.velz << "accz" << acceleration << endl;
        T.velz = T.velz + acceleration * sin(tmptheta * PI / 180) * 10;
    }

    if(!T.velx && !T.vely && T.fired) {
        T.x = 2;
        T.y = 0;
        T.z = 58;
        T.fired = 0;
        E.power = 0;
        E.score -= 1;
    }
    // Make velocity 0 if the friction has increased than the objects velocity
    if((T.velx < 0 && prevx == 1) || (T.velz < 0 && prevz == 1) || (T.velx > 0 && prevx == -1) || (T.velz > 0 && prevz == -1)) {
        T.velx = 0;
        T.velz = 0;
        E.power = 0;
    }
    
    if((abs(E.getTargetX() - T.x) < 3) && (abs(E.getTargetZ() - T.z) < 3)) {
        E.score += 10;
        T.x = 2;
        T.y = 0;
        T.z = 58;
        T.velx = 0; 
        T.vely = 0; 
        T.velz = 0;
        T.fired = 0;
        E.power = 0;
    }
    
    float finalx = T.x + T.velx;
    float finalz = T.z + T.velz;
    if(finalx >= 60) {
        finalx = 59;
        T.velx = T.vely = T.velz = 0;
    }
    if(finalx <= 0) {
        finalx = 0;
        T.velx= T.vely = T.velz = 0;
    }
    if(finalz >= 60) {
        finalz = 59;
        T.velx = T.vely = T.velz = 0;
    }
    if(finalz <= 0) {
        T.velx = T.vely = T.velz = 0;
        finalz = 0;
    }
    T.setPosition(finalx, T.y, finalz);
    glutTimerFunc(10, topupdate, 0);
}

void handleKeypress(unsigned char key, int x, int y) {
    float speed, m, tmptheta;
    switch (key) {
        case 'a':
            E.changePointerDirection(1);
            break;
        case 'c':
            E.changePointerDirection(2);
            break;
        case '1':
            // Player View
            T.setCameraPosition(1);
            break;
        case '2':
            // Top View
            T.setCameraPosition(2);
            break;
        case '3':
            // Overhead View
            T.setCameraPosition(3);
            break;
        case '4':
            // Helicopter View
            T.setCameraPosition(4);
            break;
        case '5':
            // Follow View
            T.setCameraPosition(5);
            break;
        case 32:
            m = (E.pointerZ - T.z) / (E.pointerX - T.x);
            tmptheta = atan(m) * 180 / PI;
            speed = E.getSpeedFactor();
            T.velx = E.power * speed / sqrt(1 + m * m); 
            T.velz = E.power * speed * m / sqrt(1 + m * m); 
            T.fired = 1;
            /*
            if((tmptheta < 0 && E.pointerZ > 0) || (tmptheta > 0 && E.pointerZ < 0)) {
                T.velx = -T.velx;
                T.velz = -T.velz;
            }
            */
            glutTimerFunc(10, topupdate, 0);
            break;
        case 27: //Escape key
            cleanup();
            exit(0);
            break;
    }
}

void arrowKeys(int key, int x, int y) {
    switch(key) {
        case GLUT_KEY_UP:
            if(E.power < 9)
                E.power++;
            break;
        case GLUT_KEY_DOWN:
            if(E.power > 0)
                E.power--;
            break;
        case GLUT_KEY_LEFT:
            if(T.x > 0)
                T.x -= 1;
            break;
        case GLUT_KEY_RIGHT:
            if(T.x < 59)
                T.x += 1;
            break;
    }
}

void initRendering() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
}

void handleResize(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 1.0, 200.0);
}

void drawScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    int tmpcam = T.getCameraPosition();
    switch(tmpcam) {
        case 1:
            glTranslatef(0, 0, -10);
            glRotatef(30, 1.0f, 0.0f, 0.0f);
            break;
        case 2:

            gluLookAt(T.x / 5, T.y / 5, T.z / 5,
                    0, 0, 0,
                    //E.getTargetX() / 5-10, E.getTargetY() / 5 , E.getTargetZ() / 5,
                    0, 1, 0);
            break;
        case 3:
            gluLookAt(0.4, 10, 0.4,
                    0.3, 0, 0.3,
                    0, 1, 0);
            break;
        default:
            break;
    }

    GLfloat ambientColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);

    GLfloat lightColor0[] = {0.6f, 0.6f, 0.6f, 1.0f};
    GLfloat lightPos0[] = {-0.5f, 0.8f, 0.1f, 0.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);

    float scale = 5.0f / max(_terrain->width() - 1, _terrain->length() - 1);
    glScalef(scale, scale, scale);
    glTranslatef(-(float)(_terrain->width() - 1) / 2,
            0.0f,
            -(float)(_terrain->length() - 1) / 2);

    glColor3f(1.3f, 0.9f, 0.0f);
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
    }

    // Draw Top on terrain
    glPushMatrix();
    T.drawTop();
    glPopMatrix();


    // Draw Target
    glPushMatrix();
    E.drawTarget();
    glPopMatrix();

    if(!T.fired) {
        // Draw Pointer
        glPushMatrix();
        glColor3f(0.0, 0.0, 0.0);
        E.drawLine(T.x, T.y, T.z, E.pointerX, E.pointerY, E.pointerZ, 3);
        glPopMatrix();
    }
    
    glPushMatrix();

    glRotatef(90, 1, 0, 0);
    glTranslatef(150, -50, 110);
    for(int i=0;i<E.power;i++) {
        if(i / 3 == 0)
            glColor3f(0.0, 1.0, 0.0);
        else if(i / 3 == 1)
            glColor3f(1.0, 1.0, 0.0);
        else
            glColor3f(1.0, 0.0, 0.0);
        glTranslatef(0, 0, -10);
        glutSolidTorus(i + 5, 1, 25, 30);
    }
    glPopMatrix();

    glPushMatrix();
    E.showScoreBoard();
    glPopMatrix();

    glutSwapBuffers();
}

void update(int value) {
    _angle += 1.0f;
    if (_angle > 360) {
        _angle -= 360;
    }
    theta += 1;
    glutPostRedisplay();
    glutTimerFunc(25, update, 0);
}

void mouseMove(int x, int y)
{
    if (isDragging) { // only when dragging
        // update the change in angle
        deltaAngle = (x - xDragStart) * 0.005;

        // camera's direction is set to angle + deltaAngle
        lx = -sin(angle + deltaAngle);
        ly = cos(angle + deltaAngle);
    }
}

void mouseButton(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) { // left mouse button pressed
            isDragging = 1; // start dragging
            xDragStart = x; // save x where button first pressed
        }
        else  { /* (state = GLUT_UP) */
            angle += deltaAngle; // update camera turning angle
            isDragging = 0; // no longer dragging
        }
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutCreateWindow("Top shoot");
    glutFullScreen();

    initRendering();

    _terrain = loadTerrain("heightmap.bmp", 20);

    glutDisplayFunc(drawScene);
    glutKeyboardFunc(handleKeypress);
    glutMouseFunc(mouseButton); // process mouse button push/release
    glutMotionFunc(mouseMove); // process mouse dragging motion

    glutSpecialFunc(arrowKeys);
    glutReshapeFunc(handleResize);
    glutTimerFunc(25, update, 0);

    glutMainLoop();
    return 0;
}
