#include "WorldDrawer.h"
#include "Vector4D.h"
#include "CustomObject3D.h"

bool WorldDrawer::mouseLeftState = false;		// Left click not pressed
bool WorldDrawer::mouseRightState = false;	// Right click not pressed
float WorldDrawer::mousePosX = 0.f;			// Mouse position X coordinate
float WorldDrawer::mousePosY = 0.f;			// Mouse position Y coordinate
float WorldDrawer::viewAngleX = 0.f;			// ViewAngle on OX
float WorldDrawer::viewAngleY = 0.f;			// ViewAngle on OY
float WorldDrawer::viewAngleTpsX = 0.f;			// ViewAngle on OX for TPS camera
float WorldDrawer::viewAngleTpsY = 0.f;			// ViewAngle on OY for TPS camera
float WorldDrawer::eyeDistance = 0.f;			// Distance from viewer
float WorldDrawer::distanceToTPSTarget = 10.f;	// Distance to target for TPS camera
float WorldDrawer::distanceToTop = 50.f;	// Distance to top Camera
float WorldDrawer::mouseSensivity = 100.f;
float WorldDrawer::zoomSensivity = 0.1f;

int WorldDrawer::mainWindow = 0;				// Main window ID
float WorldDrawer::angle = 0;

float *Vector3D::arr;
float *Vector4D::arr;


WorldDrawer::WorldDrawer
	(int argc, char **argv, int windowWidth, int windowHeight,
	int windowStartX, int windowStartY, std::string windowName)
{
	//init
	glutInit(&argc, argv);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);	// This will not close the program for multiWindowClose

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glEnable(GLUT_MULTISAMPLE);							// Enable multisampling

	glutInitWindowSize(windowWidth,windowHeight);		// Window size
	glutInitWindowPosition(windowStartX,windowStartY);	// Window position
	mainWindow = glutCreateWindow(windowName.c_str());

	// Display, reshape and idle callbacks
	glutDisplayFunc(displayCallbackFunction);
	glutReshapeFunc(reshapeCallbackFunction);
	glutIdleFunc(idleCallbackFunction);

	// Keyboard callbacks
	glutKeyboardFunc(keyDownCallbackFunction);
	glutKeyboardUpFunc(keyUpCallbackFunction);
	glutSpecialFunc(specialKeyDownCallbackFunction);
	glutSpecialUpFunc(specialKeyUpCallbackFunction);

	// Mouse callbacks
	glutMouseFunc(mouseCallbackFunction);
	glutMotionFunc(mouseMotionCallbackFunction);
	glutMouseWheelFunc(mouseWheelCallbackFunction);

	// Background color
	glClearColor(0.098f, 0.09f, 0.157f, 1.f);

	//zbuff
	glEnable(GL_DEPTH_TEST);

	// Enable and init lighting
	glEnable(GL_LIGHTING);
	// Track material ambient and diffuse from surface color, call it before glEnable(GL_COLOR_MATERIAL)
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
	

	// Set up light colors (ambient, diffuse, specular)
    GLfloat lightKa[] = {.2f, .2f, .2f, 1.0f};  // Ambient light
    GLfloat lightKd[] = {.7f, .7f, .7f, 1.0f};  // Diffuse light
    GLfloat lightKs[] = {1, 1, 1, 1};           // Specular light
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightKa);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightKd);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightKs);

    // position the light
    float lightPos[4] = {0, 0, 20, 1}; // positional light
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    glEnable(GL_LIGHT0);  // MUST enable each light source after configuration

}


WorldDrawer::~WorldDrawer()
{
}

void WorldDrawer::idleCallbackFunction(){
	
	unsigned int diff = getTimeDifference();
	// Limit to 50 frames per second
	if (diff >= 20)
	{
		tick += diff;

		onIdle();

		//redisplay
		glutPostRedisplay();
	}
}

// Returns difference of time since last update
unsigned int WorldDrawer::getTimeDifference()
{
	unsigned int newTick = getTime();
	return newTick - tick;
}

// Gets current time
unsigned int WorldDrawer::getTime()
{
	return glutGet(GLUT_ELAPSED_TIME);
}

void WorldDrawer::reshapeCallbackFunction(int w, int h){
	glViewport(0,0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float aspect = (float)w/(float)h;
	gluPerspective(45.0f, aspect, 0.2f, 600.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void WorldDrawer::displayCallbackFunction(){
	//Render objects
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//setup view
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Render the camera
	camera.render();

	// Activate omnidirectional light
	//light_o->Render();

	// Draw game box
	gameBox->Draw();

	drawAxis();

	// Cube and square for test
	//glColor3f(0.5f, 0.8f, 0.03f);
	//glutSolidCube(3);

	aircraft->Draw();
	//glColor4f(1.f, 0.f, 0.f, 0.7f);
	//cub1->Draw();

	std::cerr << "camera pos: " << camera.position << "\n";

	// Player
	if (camera.mode == MODE_TPS)
	{
		glPushMatrix();
		Vector3D pos(camera.position + camera.forward * distanceToTPSTarget);
		glTranslatef(pos.x, pos.y, pos.z);
		glRotatef(float(-camera.getAngleY() * 180 / M_PI) + 180, 0.f, 1.f, 0.f);
		
		glColor3f(0.f, 1.f, 0.f);
		glutSolidCone(PLAYER_RADIUS / 2, PLAYER_RADIUS, 100, 10);
		glColor3f(0.f, 0.9f, 0.f);
		glutSolidSphere(PLAYER_RADIUS / 2, 100, 10);

		glPopMatrix();
	}
	else if (camera.mode == MODE_TOP)	
	{
		glPushMatrix();
		Vector3D pos(camera.position + camera.forward * distanceToTop);
		glTranslatef(pos.x, pos.y, pos.z);
		glRotatef(float(-camera.getAngleY() * 180 / M_PI) + 180, 0.f, 1.f, 0.f);
		
		glColor3f(0.f, 1.f, 0.f);
	
		glutSolidCone(PLAYER_RADIUS / 2, PLAYER_RADIUS, 100, 10);
		glColor3f(0.f, 0.9f, 0.f);
		glutSolidSphere(PLAYER_RADIUS / 2, 100, 10);
		
		glPopMatrix();
	}
	//swap buffers
	glutSwapBuffers();
}

// Buffer keys on press
void WorldDrawer::keyDownCallbackFunction(unsigned char key, int posx, int posy)
{
	if (key == KEY_SPACE)
		animation = !animation;
	keyStates[key] = true;

	if (key == '1')						// Switch to FPS camera
		switchCameraMode(MODE_FPS);
	if (key == '2')						// Switch to TPS camera
		switchCameraMode(MODE_TPS);
	if (key == '3')						// Switch to TOP camera
		switchCameraMode(MODE_TOP);
	else if (key == 'r')						// Resets game
	{
		int mode = camera.mode;
		switchCameraMode(MODE_FPS);
		init();
		switchCameraMode(mode);
	}
}

// Unbuffer keys on release
void WorldDrawer::keyUpCallbackFunction(unsigned char key, int posx, int posy)
{
	keyStates[key] = false;
}

// Buffer special keys on press
void WorldDrawer::specialKeyDownCallbackFunction(int key, int posx, int posy)
{
	keySpecialStates[key] = true;
}

// Unbuffer special keys on release
void WorldDrawer::specialKeyUpCallbackFunction(int key, int posx, int posy)
{
	keySpecialStates[key] = false;
}

void WorldDrawer::run(){
	glutMainLoop();
}