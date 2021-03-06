#include "HeadersAndDefines.h"
#include "WorldDrawer.h"
#include "camera.h"

#include "Point2d.h"
#include "Mesh.h"
#include "CustomObject3D.h"
#include "Cube.h"
#include "Light.h"

bool WorldDrawer::animation = true;
bool WorldDrawer::keyStates[256];
bool WorldDrawer::keySpecialStates[256];

unsigned int WorldDrawer::tick = 0;
Camera WorldDrawer::cameraDynamic(MODE_TPS);
Camera WorldDrawer::cameraOnBoard(MODE_FPS);

CustomObject3D *WorldDrawer::aircraft;
Object3D *WorldDrawer::gameBox;
Mesh *WorldDrawer::aircraftMesh;					// Mesh for aircraft
Mesh *WorldDrawer::asteroidMesh;					// Mesh for asteroid
Mesh *WorldDrawer::wrenchMesh;						// Mesh for wrench

std::vector<Asteroid *> WorldDrawer::asteroids;		// Asteroid objects
std::vector<Asteroid *> WorldDrawer::repair;		// Shield repairing tools

Light *WorldDrawer::light_o1;						// Omnidirectional light
Light *WorldDrawer::light_o2;						// Omnidirectional light2
Light *WorldDrawer::light_s1;						// Spot light1
Light *WorldDrawer::light_s2;						// Spot light2

int WorldDrawer::selectedObject = 0;				// Selected object
int WorldDrawer::selectedIndex = -1;				// Selected index

int WorldDrawer::cameraType = Dynamic;				// Default camera mode is Dynamic

Shield *WorldDrawer::shield;						// Aircraft shield
bool WorldDrawer::gameInPlay = true;				// Game is in play

//add
void WorldDrawer::init(){

	// Initialize vector arrays
	Vector3D::arr = new float[3];
	Vector4D::arr = new float[4];

	aircraftMesh = new Mesh();
	aircraftMesh->Init("aircraft.off");

	asteroidMesh = new Mesh();
	//asteroidMesh->Init("asteroid.off");
	asteroidMesh->Init("asteroid_small.off");

	wrenchMesh = new Mesh();
	wrenchMesh->Init("wrench.off");

	std::cerr << "Aircraft radius " << aircraftMesh->radius << " center: " << aircraftMesh->center << "\n";
	std::cerr << "Asteroid radius " << asteroidMesh->radius << " center: " << asteroidMesh->center << "\n";
	std::cerr << "Wrench radius " << wrenchMesh->radius << " center: " << wrenchMesh->center << "\n";

	aircraft = new CustomObject3D(aircraftMesh, AIRCRAFT);

	shield = new Shield(0.9f, aircraft);

	// Create asteroids
	for (int i = 0; i < NUM_ASTEROIDS; ++i)
	{
		if (i == 0)
			asteroids.push_back(new Asteroid(asteroidMesh, ASTEROID, true));
		else
			asteroids.push_back(new Asteroid(asteroidMesh, ASTEROID, false));

		float rand_scale = genRandomFloat(0.2f, 1.f);
		asteroids[i]->SetScale(new Vector3D(rand_scale, rand_scale, rand_scale));
		asteroids[i]->SetColor(new Vector3D(0.5f, 0.5f, 0.5f));
		asteroids[i]->SetDiffuse(new Vector4D(0.25f, 0.25f, 0.3f, 1.f));

		// Move
		Vector3D randPos = WorldDrawer::genRandomPosition(-PLANE_SIZE/2, PLANE_SIZE/2, -PLANE_SIZE/2, PLANE_SIZE/2);
		asteroids[i]->SetPosition(new Vector3D(randPos));
		asteroids[i]->moveStep = genRandomFloat(0.3f, 0.9f);

		asteroids[i]->angleStep = Vector3D(genRandomFloat(0.5f, 1.f), genRandomFloat(0.5f, 1.f), genRandomFloat(0.5f, 1.f));
	}

	// Create repairing tools
	for (int i = 0; i < NUM_WRENCHES; ++i)
	{
		if (i == 0)
			repair.push_back(new Asteroid(wrenchMesh, WRENCH, true));
		else
			repair.push_back(new Asteroid(wrenchMesh, WRENCH, false));

		float scale = 10.f;
		repair[i]->SetScale(new Vector3D(scale, scale, scale));
		repair[i]->SetColor(new Vector3D(0.9f, 0.1f, 0.f));
		repair[i]->SetDiffuse(new Vector4D(0.9f, 0.1f, 0.f, 1.f));

		// Move
		Vector3D randPos = WorldDrawer::genRandomPosition(-PLANE_SIZE/2, PLANE_SIZE/2, -PLANE_SIZE/2, PLANE_SIZE/2);
		repair[i]->SetPosition(new Vector3D(randPos));
		repair[i]->moveStep = genRandomFloat(0.3f, 0.9f);

		repair[i]->angleStep = Vector3D(genRandomFloat(0.5f, 1.f), genRandomFloat(0.5f, 1.f), genRandomFloat(0.5f, 1.f));
	}

	// Inits objects on scene
	initScene();

	gameBox = new Object3D(GlutCube);
	gameBox->Wireframe = true;
	gameBox->SetColor(new Vector3D(0.8f, 0.8f, 0.8f));
	gameBox->SetScale(new Vector3D(PLANE_SIZE, PLANE_SIZE, PLANE_SIZE));

	// Ambiental lights
	light_o1 = new Light();
	// Init
	light_o1->SetPosition(new Vector3D(-PLANE_SIZE / 2 + 1.f, PLANE_SIZE / 2 - 1.f, -PLANE_SIZE / 2 + 1.f));
	light_o1->setDiffuse(Vector4D(1.f, 1.f, 0.f, 1.f));
	light_o1->setAmbient(Vector4D(0.1f, 0.02f, 0.05f, 1.f));
	light_o1->setSpecular(Vector4D(0.f, 1.f, 0.f, 1.f));

	light_o2 = new Light();
	light_o2->SetPosition(new Vector3D(PLANE_SIZE / 2 - 1.f, -PLANE_SIZE / 2 + 1.f, PLANE_SIZE / 2 - 1.f));
	light_o2->setDiffuse(Vector4D(0.f, 1.f, 1.f, 1.f));
	light_o2->setAmbient(Vector4D(0.05f, 0.02f, 0.1f, 1.f));
	light_o2->setSpecular(Vector4D(0.f, 0.f, 1.f, 1.f));

	tick = glutGet(GLUT_ELAPSED_TIME);
}

// Init scene
void WorldDrawer::initScene()
{
	gameInPlay = true;
	aircraft->SetScale(new Vector3D(25.f, 25.f, 25.f));
	aircraft->SetColor(new Vector3D(1.f, 0.f, 0.f));
	aircraft->SetDiffuse(new Vector4D(1.f, 0.f, 0.f, 1.f));
	aircraft->SetRotation(new Vector3D(-90.f, 0.f, -90.f));
	aircraft->SetPosition(new Vector3D(0.f, 0.f, 0.f));

	// Set up camera
	cameraDynamic.init();

	if (cameraDynamic.mode == MODE_TPS)
	{
		cameraDynamic.position = Vector3D(0, 0, distanceToTPSTarget);
	}
	else if (cameraDynamic.mode == MODE_TOP)
	{
		cameraDynamic.position = Vector3D(0, 0, distanceToTop);
		cameraDynamic.rotateTPS_OX(float(M_PI), distanceToTop);
	}
	cameraDynamic.position += Vector3D(0, 0, 260);

	cameraOnBoard.init();
	if (cameraOnBoard.mode == MODE_TPS)
	{
		cameraOnBoard.position = Vector3D(15, 0, 0);
		cameraOnBoard.forward = Vector3D(-1, 0, 0);
		cameraOnBoard.right = Vector3D(0, 0, -1);
		cameraOnBoard.up = Vector3D(0, 1, 0);
	}
	else if (cameraOnBoard.mode == MODE_FPS)
	{
		cameraOnBoard.position = Vector3D(-3, 0, 0);
		cameraOnBoard.forward = Vector3D(-1, 0, 0);
		cameraOnBoard.right = Vector3D(0, 0, -1);
		cameraOnBoard.up = Vector3D(0, 1, 0);
	}

	selectedObject = -1;
	for (unsigned int i = 0; i < asteroids.size(); ++i)
	{
		asteroids[i]->Deselect();
	}

	shield->setAlpha(0.9f);
	cameraType = Dynamic;

	// Spot lights
	light_s1 = new Light();
	// Init
	light_s1->SetLightType(IlluminationType::Spot);
	float radius = aircraft->getRadius();
	Vector3D aircraftPos = aircraft->GetPosition();
	light_s1->SetPosition(&(aircraftPos + Vector3D(radius / 3, -radius / 4, -radius / 3)));

	light_s2 = new Light();
	light_s2->SetLightType(IlluminationType::Spot);
	light_s2->SetPosition(&(aircraftPos + Vector3D(radius / 3, -radius / 4, radius / 3)));
}

// Is called in glut main loop by the system on idle
void WorldDrawer::onIdle(){	//per frame
	keyOperations();			// Operations for buffered keys
	mouseRotations();

	updateLight();

	if(animation){
		// Do nothing here
		angle = angle+1;

		if(angle > 360) angle = angle-360;

		for (unsigned int i = 0; i < asteroids.size(); ++i)
		{
			Vector3D pos = asteroids[i]->GetPosition();
			pos.x += asteroids[i]->moveStep;

			if (pos.x >= PLANE_SIZE / 2)
			{
				pos = genRandomPosition(-PLANE_SIZE / 2, PLANE_SIZE / 2, -PLANE_SIZE / 2, PLANE_SIZE / 2);
			}
			asteroids[i]->SetPosition(new Vector3D(pos));

			Vector3D rot = asteroids[i]->GetRotation();
			rot += asteroids[i]->angleStep;
			asteroids[i]->SetRotation(new Vector3D(rot));
		}

		for (unsigned int i = 0; i < repair.size(); ++i)
		{
			Vector3D pos = repair[i]->GetPosition();
			pos.x += repair[i]->moveStep;

			if (pos.x >= PLANE_SIZE / 2)
			{
				pos = genRandomPosition(-PLANE_SIZE / 2, PLANE_SIZE / 2, -PLANE_SIZE / 2, PLANE_SIZE / 2);
			}
			repair[i]->SetPosition(new Vector3D(pos));

			Vector3D rot = repair[i]->GetRotation();
			rot += repair[i]->angleStep;
			repair[i]->SetRotation(new Vector3D(rot));
		}

		// Collision logic
		collision();

	}
}

// All key events are processed here
void WorldDrawer::keyOperations()
{
	if (keyStates[KEY_ESC])			// On Escape, program exits
		glutExit();

	float rotateStep = 0.04f;
	float moveStep = 3.f;

	Camera *camera = NULL;
	if (cameraType == Dynamic)
	{
		camera = &cameraDynamic;
	}
	else if (cameraType == OnBoard)
	{
		camera = &cameraOnBoard;
	}

	if (camera)
	{
		if (camera->mode == MODE_FPS)
		{
			if (keySpecialStates[KEY_UP])			// Rotate FPS up
			{
				camera->rotateFPS_OX(-rotateStep);
			}
			if (keySpecialStates[KEY_DOWN])			// Rotate FPS down
			{
				camera->rotateFPS_OX(rotateStep);
			}
			if (keySpecialStates[KEY_LEFT])			// Rotate FPS left
			{
				camera->rotateFPS_OY(-rotateStep);
			}
			if (keySpecialStates[KEY_RIGHT])		// Rotate FPS right
			{
				camera->rotateFPS_OY(rotateStep);
			}
		}
		else if (camera->mode == MODE_TPS)
		{
			if (keySpecialStates[KEY_UP])					// Rotate TPS up
			{
				camera->rotateTPS_OX(-rotateStep, distanceToTPSTarget);
			}
			if (keySpecialStates[KEY_DOWN])					// Rotate TPS down
			{
				camera->rotateTPS_OX(rotateStep, distanceToTPSTarget);
			}
			if (keySpecialStates[KEY_LEFT])					// Rotate TPS left
			{
				camera->rotateTPS_OY(-rotateStep, distanceToTPSTarget);
			}
			if (keySpecialStates[KEY_RIGHT])				// Rotate TPS right
			{
				camera->rotateTPS_OY(rotateStep, distanceToTPSTarget);
			}
		}
		else if (camera->mode == MODE_TOP)
		{
			if (keySpecialStates[KEY_LEFT])					// Rotate left
			{
				camera->rotateTPS_OY(-rotateStep, distanceToTop);
			}
			if (keySpecialStates[KEY_RIGHT])				// Rotate right
			{
				camera->rotateTPS_OY(rotateStep, distanceToTop);
			}
		}
	}

	float eyeDistanceStep = 1.f;

	if (camera)
	{
		// Zoom closer
		if (keyStates['['])
		{
			eyeDistance -= eyeDistanceStep;		// Move closer to the viewer
			if (camera->mode == MODE_TPS && distanceToTPSTarget - zoomSensivity > 0)
			{
				distanceToTPSTarget -= zoomSensivity;
				camera->translate_ForwardFree(zoomSensivity);
			}
			else if (camera->mode == MODE_TOP && distanceToTop - zoomSensivity * 20 > 0)
			{
				distanceToTop -= zoomSensivity * 20;
				camera->translate_ForwardFree(zoomSensivity * 20);
			}
		}
		if (keyStates[']'])
		{
			eyeDistance += eyeDistanceStep;		// Move farther from the viewer
			if (camera->mode == MODE_TPS)
			{
				distanceToTPSTarget += zoomSensivity;
				camera->translate_ForwardFree(-zoomSensivity);
			}
			else if (camera->mode == MODE_TOP)
			{
				distanceToTop += zoomSensivity * 20;
				camera->translate_ForwardFree(-zoomSensivity * 20);
			}
		}
	}

	if (keyStates['w'])						// Move forward
	{
		if (cameraType == Dynamic)
			cameraDynamic.translate_ForwardFree(moveStep);

		// Check collision
	}
	if (keyStates['s'])						// Move backwards
	{
		if (cameraType == Dynamic)
			cameraDynamic.translate_ForwardFree(-moveStep);

		// Check collision
	}
	if (keyStates['a'])						// Move left
	{
		if (cameraType == Dynamic)
			cameraDynamic.translate_RightFree(-moveStep);

		// Check collision
	}
	if (keyStates['d'])						// Move right
	{
		if (cameraType == Dynamic)
			cameraDynamic.translate_RightFree(moveStep);

		// Check collision
	}

	float aircraftStep = 0.9f;
	if (keyStates['8'])						// Move aircraft up
	{
		aircraft->SetPosition(&(aircraft->GetPosition() + Vector3D(0.f, aircraftStep, 0.f)));
		cameraOnBoard.position += Vector3D(0.f, aircraftStep, 0.f);
	}
	if (keyStates['5'])						// Move aircraft down
	{
		aircraft->SetPosition(&(aircraft->GetPosition() + Vector3D(0.f, -aircraftStep, 0.f)));
		cameraOnBoard.position += Vector3D(0.f, -aircraftStep, 0.f);
	}
	if (keyStates['4'])						// Move aircraft left
	{
		aircraft->SetPosition(&(aircraft->GetPosition() + Vector3D(0.f, 0.f, aircraftStep)));
		cameraOnBoard.position += Vector3D(0.f, 0.f, aircraftStep);
	}
	if (keyStates['6'])						// Move aircraft right
	{
		aircraft->SetPosition(&(aircraft->GetPosition() + Vector3D(0.f, 0.f, -aircraftStep)));
		cameraOnBoard.position += Vector3D(0.f, 0.f, -aircraftStep);
	}
	if (keyStates['9'])						// Move aircraft forward
	{
		aircraft->SetPosition(&(aircraft->GetPosition() + Vector3D(-aircraftStep, 0.f, 0.f)));
		cameraOnBoard.position += Vector3D(-aircraftStep, 0.f, 0.f);
	}
	if (keyStates['7'])						// Move aircraft backward
	{
		aircraft->SetPosition(&(aircraft->GetPosition() + Vector3D(aircraftStep, 0.f, 0.f)));
		cameraOnBoard.position += Vector3D(aircraftStep, 0.f, 0.f);
	}
}

// Callback function for mouse actions
void WorldDrawer::mouseCallbackFunction(int button, int state, int x, int y)
{
	mousePosX = float (x);
	mousePosY = float (y);
	if (button == MOUSE_LEFT)			// Buffer left clicks
	{
		if (state == GLUT_DOWN)
			mouseLeftState = true;
		else if (state == GLUT_UP)
			mouseLeftState = false;
	}
	else if (button == MOUSE_RIGHT)		// Buffer right clicks
	{
		if (state == GLUT_DOWN)
		{
			selectedObject = 0;

			pick(x,y);

			if (cameraType != OnAsteroid && selectedObject > 0 && selectedObject <= (int)asteroids.size())
			{
				asteroids[selectedObject - 1]->Select();
			}
			std::cerr << "selected object " << selectedObject << "\n";


			mouseRightState = true;
		}
		else if (state == GLUT_UP)
		{
			if (cameraType != OnAsteroid)
			{
				if (selectedObject > 0 && selectedObject <= (int)asteroids.size())
				{
					asteroids[selectedObject - 1]->Deselect();
					Vector3D pos = genRandomPosition(-PLANE_SIZE / 2, PLANE_SIZE / 2, -PLANE_SIZE / 2, PLANE_SIZE / 2);
					asteroids[selectedObject - 1]->SetPosition(new Vector3D(pos));
				}
				selectedObject = 0;
			}

			mouseRightState = false;
		}
	}
}

// Callback for mouse movement
void WorldDrawer::mouseMotionCallbackFunction(int x, int y)
{
	float eyeDistanceStep = 0.2f;
	if (mouseLeftState == true)			// Make rotation if left is clicked and moved mouse
	{
		viewAngleX += (y - mousePosY) / mouseSensivity;
		viewAngleY += (x - mousePosX) / mouseSensivity;
		mousePosX = float(x);
		mousePosY = float(y);
	}
	if (mouseRightState == true)		// Make zoom in/out if right is clicked and moved
	{
		eyeDistance -= (y - mousePosY) * eyeDistanceStep;
		mousePosY = float (y);
	}
}

// Execute mouse rotations
void WorldDrawer::mouseRotations()
{
	Camera *camera = NULL;
	if (cameraType == Dynamic)
		camera = &cameraDynamic;
	else if (cameraType == OnBoard)
		camera = &cameraOnBoard;

	if (camera)
	{
		if (camera->mode == MODE_FPS)
		{
			camera->rotateFPS_OX(viewAngleX);
			camera->rotateFPS_OY(viewAngleY);
			viewAngleX = viewAngleY = 0.f;
		}
		else if (camera->mode == MODE_TPS)
		{
			camera->rotateTPS_OX(viewAngleX, distanceToTPSTarget);
			camera->rotateTPS_OY(viewAngleY, distanceToTPSTarget);
			viewAngleX = viewAngleY = 0.f;
		}
	}
}

// Callback for mouse scroll (wheel)
void WorldDrawer::mouseWheelCallbackFunction(int wheel, int direction, int x, int y)
{
	float eyeDistanceStep = 2.f;
	eyeDistance += eyeDistanceStep * (-direction);
	Camera *camera = NULL;
	if (cameraType == Dynamic)
		camera = &cameraDynamic;
	else if (cameraType == OnBoard)
		camera = &cameraOnBoard;

	if (camera)
	{
		if (camera->mode == MODE_TPS && distanceToTPSTarget + zoomSensivity * 20 * (-direction) > 0)
		{
			distanceToTPSTarget += zoomSensivity * 20 * (-direction);
			camera->translate_ForwardFree(-zoomSensivity * 20 * (-direction));
		}
		else if (camera->mode == MODE_TOP && distanceToTop + zoomSensivity * 50 * (-direction) > 0)
		{
			distanceToTop += zoomSensivity * 50 * (-direction);
			camera->translate_ForwardFree(-zoomSensivity * 50 * (-direction));
		}
	}
}

// Switches the camera mode
void WorldDrawer::switchCameraMode(int mode, Camera &camera)
{
	if (camera.mode == MODE_FPS && mode == MODE_TPS)
	{
		camera.rotateFPS_OX(float(ANGLE_LIMIT * 2));
		camera.rotateFPS_OX(float(-ANGLE_LIMIT + camera.angleTpsX));
		camera.translate_ForwardFree(-distanceToTPSTarget);
	}
	else if (camera.mode == MODE_FPS && mode == MODE_TOP)
	{
		camera.rotateFPS_OX(float(ANGLE_LIMIT * 2));
		camera.translate_ForwardFree(-distanceToTop);
	}
	else if (camera.mode == MODE_TPS && mode == MODE_FPS)
	{
		camera.angleTpsX = camera.getAngleX();
		camera.translate_ForwardFree(distanceToTPSTarget);
		camera.rotateFPS_OX(float(ANGLE_LIMIT * 2));
		camera.rotateFPS_OX(float(-ANGLE_LIMIT));
	}
	else if (camera.mode == MODE_TPS && mode == MODE_TOP)
	{
		camera.angleTpsX = camera.getAngleX();
		camera.translate_ForwardFree(distanceToTPSTarget);
		camera.rotateFPS_OX(float(ANGLE_LIMIT * 2));
		camera.translate_ForwardFree(-distanceToTop);
	}
	else if (camera.mode == MODE_TOP && mode == MODE_FPS)
	{
		camera.translate_ForwardFree(distanceToTop);
		camera.rotateFPS_OX(float(-ANGLE_LIMIT));
	}
	else if (camera.mode == MODE_TOP && mode == MODE_TPS)
	{
		camera.translate_ForwardFree(distanceToTop);
		camera.rotateFPS_OX(float(-ANGLE_LIMIT + camera.angleTpsX));
		camera.translate_ForwardFree(-distanceToTPSTarget);
	}
	camera.mode = mode;
}

// Gets the position of the player in world space
Vector3D WorldDrawer::getPlayerPosition()
{
	Camera *camera = NULL;
	if (cameraType == Dynamic)
		camera = &cameraDynamic;

	if (camera)
	{
		if (camera->mode == MODE_FPS)
			return camera->position;
		else
		{
			int mode = camera->mode;
			switchCameraMode(MODE_FPS, *camera);
			Vector3D temp(camera->position);
			switchCameraMode(mode, *camera);
			return temp;
		}
	}
	return Vector3D(0, 0, 0);
}

// Random helper
float WorldDrawer::genRandomFloat(float min, float max)
{
	return ((float(rand()) / float(RAND_MAX)) * (max - min)) + min;
}

// Random Asteroid position
Vector3D WorldDrawer::genRandomPosition(float minY, float maxY, float minZ, float maxZ)
{
	float x = genRandomFloat(-PLANE_SIZE /2, 0.f);
	float y = genRandomFloat(minY, maxY);
	float z = genRandomFloat(minZ, maxZ);
	return Vector3D(-PLANE_SIZE /2, y, z);
}

// functia care proceseaza hitrecordurile pentru a vedea daca s-a click pe un obiect din scena
void WorldDrawer::processhits (GLint hits, GLuint buffer[])
{
	int i;
	GLuint names, *ptr, minZ,*ptrNames, numberOfNames;

	// pointer la inceputul bufferului ce contine hit recordurile
	ptr = (GLuint *) buffer;
	// se doreste selectarea obiectului cel mai aproape de observator
	minZ = 0xffffffff;
	for (i = 0; i < hits; i++) 
	{
		// numarul de nume numele asociate din stiva de nume
		names = *ptr;
		ptr++;
		// Z-ul asociat hitului - se retine 
		if (*ptr < minZ) {
			numberOfNames = names;
			minZ = *ptr;
			// primul nume asociat obiectului
			ptrNames = ptr+2;
		}

		// salt la urmatorul hitrecord
		ptr += names+2;
	}

	// identificatorul asociat obiectului
	ptr = ptrNames;

	selectedObject = *ptr;
}

// functie ce realizeaza picking la pozitia la care s-a dat click cu mouse-ul
void WorldDrawer::pick(int x, int y)
{
	// buffer de selectie
	GLuint buffer[1024];

	// numar hituri
	GLint nhits;

	// coordonate viewport curent
	GLint	viewport[4];

	// se obtin coordonatele viewportului curent
	glGetIntegerv(GL_VIEWPORT, viewport);
	// se initializeaza si se seteaza bufferul de selectie
	memset(buffer,0x0,1024);
	glSelectBuffer(1024, buffer);

	// intrarea in modul de selectie
	glRenderMode(GL_SELECT);

	// salvare matrice de proiectie curenta
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	// se va randa doar intr-o zona din jurul cursorului mouseului de [1,1]
	glGetIntegerv(GL_VIEWPORT,viewport);
	gluPickMatrix(x,viewport[3]-y,1.0f,1.0f,viewport);

	gluPerspective(45,(viewport[2]-viewport[0])/(GLfloat) (viewport[3]-viewport[1]),0.1,1000);
	glMatrixMode(GL_MODELVIEW);

	// se "deseneaza" scena : de fapt nu se va desena nimic in framebuffer ci se va folosi bufferul de selectie
	shield->disable();
	drawScene();
	shield->enable();

	// restaurare matrice de proiectie initiala
	glMatrixMode(GL_PROJECTION);						
	glPopMatrix();				

	glMatrixMode(GL_MODELVIEW);
	// restaurarea modului de randare uzual si obtinerea numarului de hituri
	nhits=glRenderMode(GL_RENDER);	

	// procesare hituri
	if(nhits != 0)
		processhits(nhits,buffer);
	else
		selectedObject = 0;
}

// Draw 3D Scene
void WorldDrawer::drawScene()
{
	//setup view
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Render the camera
	if (cameraType == Dynamic)
		cameraDynamic.render();
	else if (cameraType == OnBoard)
		cameraOnBoard.render();
	else if (cameraType == OnAsteroid)
	{
		if (selectedObject > 0 && selectedObject <= (int)asteroids.size())
			asteroidCameraRender(asteroids[selectedObject - 1], &(aircraft->GetPosition()));
		else
			asteroidCameraRender(asteroids[0], &(aircraft->GetPosition()));

	}

	if (!gameInPlay)
	{
		glDisable(GL_LIGHTING);
		glLineWidth(1.0);
		output(-50,10,"You lost! Pres R to restart.");
		if (keyStates['r'])
		{
			initScene();
		}

		glEnable(GL_LIGHTING);
	}

	// Activate omnidirectional lights
	light_o1->Render();
	light_o2->Render();

	// Activate spot lights
	light_s1->Render();
	light_s2->Render();

	// Draw game box
	gameBox->Draw();

	drawAxis();

	// Draw aircraft
	aircraft->Draw();

	// Draw shield
	shield->Draw();

	for (unsigned int i = 0; i < asteroids.size(); ++i)
	{
		asteroids[i]->Deselect();
	}
	if (selectedObject > 0 && selectedObject <= (int)asteroids.size())
	{
		asteroids[selectedObject - 1]->Select();
		if (cameraType != OnAsteroid)
		{
			drawLaser(asteroids[selectedObject - 1]->GetPosition(), aircraft->GetPosition() + Vector3D(-aircraft->getRadius(), 0.f, 0.f));
		}
	}

	for (unsigned int i = 0; i < asteroids.size(); ++i)
	{
		glPushName(i + 1);
		asteroids[i]->Draw();
		glPopName();

		//asteroids[i]->Draw();
	}

	for (unsigned int i = 0; i < repair.size(); ++i)
	{
		repair[i]->Draw();
	}

	// Player
	if (cameraType == Dynamic)
	{
		if (cameraDynamic.mode == MODE_TPS)
		{
			glPushMatrix();
			Vector3D pos(cameraDynamic.position + cameraDynamic.forward * distanceToTPSTarget);
			glTranslatef(pos.x, pos.y, pos.z);
			glRotatef(float(-cameraDynamic.getAngleY() * 180 / M_PI) + 180, 0.f, 1.f, 0.f);

			glColor3f(0.f, 0.9f, 0.f);
			glutSolidSphere(PLAYER_RADIUS / 2, 100, 10);

			glPopMatrix();
		}
		else if (cameraDynamic.mode == MODE_TOP)	
		{
			glPushMatrix();
			Vector3D pos(cameraDynamic.position + cameraDynamic.forward * distanceToTop);
			glTranslatef(pos.x, pos.y, pos.z);
			glRotatef(float(-cameraDynamic.getAngleY() * 180 / M_PI) + 180, 0.f, 1.f, 0.f);

			glColor3f(0.f, 0.9f, 0.f);
			glutSolidSphere(PLAYER_RADIUS / 2, 100, 10);

			glPopMatrix();
		}
	}

	// Draw the sphere where the light comes from
	light_o1->Draw();
	light_o2->Draw();
	light_s1->Draw();
	light_s2->Draw();

	// Disable light
	light_o1->Disable();
	light_o2->Disable();
	light_s1->Disable();
	light_s2->Disable();
}

// Render Asteroid Camera
void WorldDrawer::asteroidCameraRender(Asteroid *asteroid, Vector3D *target)
{
	Vector3D position = asteroid->GetPosition();
	Vector3D forward = (*target - position).Normalize();
	Vector3D newPos = position + forward * asteroid->getRadius();

	gluLookAt(newPos.x, newPos.y, newPos.z, 
		target->x, target->y, target->z,
		0.f, 1.f, 0.f);
}

// Checks for collision between two objects
bool WorldDrawer::objectToObjectCollision(CustomObject3D *obj1, CustomObject3D *obj2)
{
	float radius1 = obj1->getRadius();
	float radius2 = obj2->getRadius();
	Vector3D pos1 = obj1->GetPosition();
	Vector3D pos2 = obj2->GetPosition();
	if (pos1.Distance(pos2) <= radius1 + radius2)
		return true;
	return false;
}

// Collision logic
void WorldDrawer::collision()
{
	for (unsigned int i = 0; i < asteroids.size(); ++i)
	{
		if (objectToObjectCollision(asteroids[i], aircraft))
		{
			std::cerr << "Asteroid number " << i << " collided with aircraft \n";
			Vector3D pos = genRandomPosition(-PLANE_SIZE / 2, PLANE_SIZE / 2, -PLANE_SIZE / 2, PLANE_SIZE / 2);
			asteroids[i]->SetPosition(new Vector3D(pos));

			if (shield->getAlpha() <= 0)
			{
				gameInPlay = false;
			}
			shield->decrementAlpha(SHIELD_STEP);

		}
	}
	for (unsigned int i = 0; i < repair.size(); ++i)
	{
		if (objectToObjectCollision(repair[i], aircraft))
		{
			std::cerr << "Wrench number " << i << " collided with aircraft \n";
			Vector3D pos = genRandomPosition(-PLANE_SIZE / 2, PLANE_SIZE / 2, -PLANE_SIZE / 2, PLANE_SIZE / 2);
			repair[i]->SetPosition(new Vector3D(pos));

			if (shield->getAlpha() < 0.8)
				shield->incrementAlpha(SHIELD_STEP);
		}
	}
}

// Update light position
void WorldDrawer::updateLight()
{
	float radius = aircraft->getRadius();
	Vector3D aircraftPos = aircraft->GetPosition();
	light_s1->SetPosition(&(aircraftPos + Vector3D(radius / 3, -radius / 4, -radius / 3)));
	light_s2->SetPosition(&(aircraftPos + Vector3D(radius / 3, -radius / 4, radius / 3)));
}

// Draw laser between two points
void WorldDrawer::drawLaser(Vector3D pos1, Vector3D pos2)
{
	glLineWidth(5);

	glBegin(GL_LINES);

	glColor3f(0, 1, 1);
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,(Vector4D(0.f, 1.f, 1.f, 1.f)).Array());
	glVertex3fv(pos1.Array());
	glVertex3fv(pos2.Array());
	glEnd();
}

// functie pentru output text
void WorldDrawer::output(GLfloat x, GLfloat y, char *format,...)
{
	va_list args;

	char buffer[1024],*p;

	va_start(args,format);

	vsprintf(buffer, format, args);

	va_end(args);

	glPushMatrix();

	glTranslatef(x,y,-15);

	glScalef(0.035, 0.035, 0.0); /* 0.1 to 0.001 as required */

	for (p = buffer; *p; p++)
		glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *p);

	glPopMatrix();
}

// Draw main axis
void WorldDrawer::drawAxis()
{
	float size = 100;

	glLineWidth(5);

	glBegin(GL_LINES);
	// Draw X
	glColor3f(1, 0, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(size, 0, 0);

	// Draw Y
	glColor3f(0, 1, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(0, size, 0);

	// Draw Z
	glColor3f(0, 0, 1);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, size);
	glEnd();
}

int main(int argc, char *argv[]){
	srand((unsigned int)time(0));

	WorldDrawer wd(argc, argv, 800, 600, 200, 200, std::string("Tema 4: SpaceEscape 2012"));
	wd.init();
	wd.run();

	return 0;
}