#include "Render.h"

#include <sstream>
#include <iostream>

#include <windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>

#include "MyOGL.h"

#include "Camera.h"
#include "Light.h"
#include "Primitives.h"

#include "GUItextRectangle.h"

bool textureMode = false;
bool lightMode = true;

//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	double camDist;
	//углы поворота камеры
	double fi1, fi2;


	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 15;
		fi1 = 1;
		fi2 = 1;
	}


	//считает позицию камеры, исходя из углов поворота, вызывается движком
	void SetUpCamera()
	{
		//отвечает за поворот камеры мышкой
		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist * cos(fi2) * cos(fi1),
			camDist * cos(fi2) * sin(fi1),
			camDist * sin(fi2));

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		//функция настройки камеры
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
	}



}  camera;   //создаем объект камеры


//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 3);
	}


	//рисует сферу и линии под источником света, вызывается движком
	void  DrawLightGhismo()
	{
		glDisable(GL_LIGHTING);


		glColor3d(0.9, 0.8, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale * 0.08;
		s.Show();

		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//линия от источника света до окружности
			glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale * 1.5;
			c.Show();
		}

	}

	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}


} light;  //создаем источник света




//старые координаты мыши
int mouseX = 0, mouseY = 0;

void mouseEvent(OpenGL* ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01 * dx;
		camera.fi2 += -0.01 * dy;
	}


	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y);

		double z = light.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k * r.direction.X() + r.origin.X();
		y = k * r.direction.Y() + r.origin.Y();

		light.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		light.pos = light.pos + Vector3(0, 0, 0.02 * dy);
	}


}

void mouseWheelEvent(OpenGL* ogl, int delta)
{

	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01 * delta;

}

void keyDownEvent(OpenGL* ogl, int key)
{
	if (key == 'L')
	{
		lightMode = !lightMode;
	}

	if (key == 'T')
	{
		textureMode = !textureMode;
	}

	if (key == 'R')
	{
		camera.fi1 = 1;
		camera.fi2 = 1;
		camera.camDist = 15;

		light.pos = Vector3(1, 1, 3);
	}

	if (key == 'F')
	{
		light.pos = camera.pos;
	}
}

void keyUpEvent(OpenGL* ogl, int key)
{

}



GLuint texId;

//выполняется перед первым рендером
void initRender(OpenGL* ogl)
{
	//настройка текстур

	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);


	//массив трехбайтных элементов  (R G B)
	RGBTRIPLE* texarray;

	//массив символов, (высота*ширина*4      4, потомучто   выше, мы указали использовать по 4 байта на пиксель текстуры - R G B A)
	char* texCharArray;
	int texW, texH;
	OpenGL::LoadBMP("texture.bmp", &texW, &texH, &texarray);
	OpenGL::RGBtoChar(texarray, texW, texH, &texCharArray);



	//генерируем ИД для текстуры
	glGenTextures(1, &texId);
	//биндим айдишник, все что будет происходить с текстурой, будте происходить по этому ИД
	glBindTexture(GL_TEXTURE_2D, texId);

	//загружаем текстуру в видеопямять, в оперативке нам больше  она не нужна
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, texCharArray);

	//отчистка памяти
	free(texCharArray);
	free(texarray);

	//наводим шмон
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	ogl->mainLight = &light;

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH);


	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	camera.fi1 = -1.3;
	camera.fi2 = 0.8;
}

struct Vector {
	float x;
	float y;
	float z;
};

Vector FindNormal(const double(&p1)[3], const double(&p2)[3], const double(&p3)[3])
{
	Vector v1, v2, normal;

	// Вычисляем векторы треугольника
	v1.x = p2[0] - p1[0];
	v1.y = p2[1] - p1[1];
	v1.z = p2[2] - p1[2];

	v2.x = p3[0] - p1[0];
	v2.y = p3[1] - p1[1];
	v2.z = p3[2] - p1[2];

	// Вычисляем векторное произведение
	normal.x = v1.y * v2.z - v1.z * v2.y;
	normal.y = -(v1.x * v2.z - v1.z * v2.x);
	normal.z = v1.x * v2.y - v1.y * v2.x;

	// Нормализуем вектор
	float length = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
	normal.x /= length;
	normal.y /= length;
	normal.z /= length;
	glNormal3d(normal.x, normal.y, normal.z);
	return normal;
}

void MakePrism()
{
	double h = 10;
	double a = 0;
	double r1 = 5;
	double r2 = 0.5 * sqrt(61);
	double b = 0.01;	// b'шка это кароче сдвиг на угол, т.е. величина сегмента окружности, можно че хош поставить, чем меньше - тем больше сегментов => выше точность построения
	double A[] = { 0, 0, 0 };
	double B[] = { 0, -8, 0 };
	double C[] = { 4, -9, 0 };
	double D[] = { 5, -17, 0 };
	double E[] = { 11, -12, 0 };
	double F[] = { 7, -8, 0 };
	double G[] = { 10, -4, 0 };
	double H[] = { 6, -6, 0 };
	double Z[] = { 2,-4,0 };
	double A1[] = { 0, 0, h };
	double B1[] = { 0, -8, h };
	double C1[] = { 4, -9, h };
	double D1[] = { 5, -17, h };
	double E1[] = { 11, -12, h };
	double F1[] = { 7, -8, h };
	double G1[] = { 10, -4, h };
	double H1[] = { 6, -6, h };
	double Z1[] = { 2,-4,h };
	double p1[] = { 0,0,0 };
	double p2[] = { 0,0,0 };
	double p3[] = { 0,0,0 };

	FindNormal(H, Z, C);
	glBegin(GL_TRIANGLES);
	glColor3d(0.949, 0.941, 0.62);
	glVertex3dv(H);
	glVertex3dv(Z);
	glVertex3dv(C);
	glEnd();

	FindNormal(H, C, F);
	glBegin(GL_TRIANGLES);
	glColor3d(0.949, 0.941, 0.62);
	glVertex3dv(H);
	glVertex3dv(C);
	glVertex3dv(F);

	glVertex3dv(F);
	glVertex3dv(G);
	glVertex3dv(H);

	glVertex3dv(E);
	glVertex3dv(F);
	glVertex3dv(D);

	glVertex3dv(F);
	glVertex3dv(C);
	glVertex3dv(D);
	glEnd();

	FindNormal(H1, Z1, C1);
	glBegin(GL_TRIANGLES);
	glColor3d(0.949, 0.941, 0.62);
	glVertex3dv(H1);
	glVertex3dv(Z1);
	glVertex3dv(C1);
	glEnd();

	FindNormal(B, C, C1);
	glBegin(GL_POLYGON);
	glColor4d(0.949, 0.341, 0.62, 1);
	glVertex3dv(B);
	glVertex3dv(C);
	glVertex3dv(C1);
	glVertex3dv(B1);
	glEnd();

	FindNormal(C, D, D1);
	glBegin(GL_POLYGON);
	glColor4d(0.349, 0.941, 0.62, 1);
	glVertex3dv(C);
	glVertex3dv(D);
	glVertex3dv(D1);
	glVertex3dv(C1);
	glEnd();

	FindNormal(E, F, F1);
	glBegin(GL_POLYGON);
	glColor4d(0.949, 0.941, 0.32, 1);
	glVertex3dv(E);
	glVertex3dv(F);
	glVertex3dv(F1);
	glVertex3dv(E1);
	glEnd();

	FindNormal(F, G, G1);
	glBegin(GL_POLYGON);
	glColor4d(0.349, 0.341, 0.62, 1);
	glVertex3dv(F);
	glVertex3dv(G);
	glVertex3dv(G1);
	glVertex3dv(F1);
	glEnd();

	FindNormal(G, H, H1);
	glBegin(GL_POLYGON);
	glColor4d(0.349, 0.941, 0.32, 1);
	glVertex3dv(G);
	glVertex3dv(H);
	glVertex3dv(H1);
	glVertex3dv(G1);
	glEnd();

	p1[0] = 1.115;
	p1[1] = -1.115;
	p1[2] = 0;
	p2[0] = 1.115;
	p2[1] = 1.115;
	p2[2] = h;

	FindNormal(H, p1, p2);
	glBegin(GL_POLYGON);
	glColor4d(0.3, 0.6, 0.3, 1);
	glVertex3dv(H);
	glVertex3d(1.115, -1.115, 0);
	glVertex3d(1.115, -1.115, h);
	glVertex3dv(H1);
	glEnd();

	glPushMatrix();
	glTranslated(-3, -4, 0);	//тут кароче сдвиг на центр окружности впуплой, чтобы мозги не ебать
	for (a = 36.87; a < 90;) //бляяя кароче епта))) кароче бля пон да, по рисунку кароче посчитал кусочек сектора 36.87, забей
	{
		p1[0] = 9;
		p1[1] = -2;
		p1[2] = 0;
		p2[0] = r1 * sin(a * PI / 180);
		p2[1] = r1 * cos(a * PI / 180);
		p3[0] = r1 * sin((a + b) * PI / 180);
		p3[1] = r1 * cos((a + b) * PI / 180);
		FindNormal(p1, p2, p3);
		glBegin(GL_TRIANGLE_FAN);
		glColor3d(0.3, 0.6, 0.3);
		glVertex3d(9, -2, 0);	// бля это кароче точка H в новых координатах после смещения
		glVertex3d(r1 * sin(a * PI / 180), r1 * cos(a * PI / 180), 0);	//мне повезло, у меня будто пятерка радиус четко вписывается я хз
		glVertex3d(r1 * sin((a + b) * PI / 180), r1 * cos((a + b) * PI / 180), 0);
		glEnd();

		p1[0] = 9;
		p1[1] = -2;
		p1[2] = 10;
		p2[2] = 10;
		p3[2] = 10;
		FindNormal(p1, p2, p3);
		glBegin(GL_TRIANGLE_FAN);
		glColor3d(0.3, 0.6, 0.3);
		glVertex3d(9, -2, 10);// бля это кароче точка H в новых координатах после смещения  еще и сверху
		glVertex3d(r1 * sin(a * PI / 180), r1 * cos(a * PI / 180), h);
		glVertex3d(r1 * sin((a + b) * PI / 180), r1 * cos((a + b) * PI / 180), h);
		glEnd();

		p1[0] = r1 * sin(a * PI / 180);
		p1[1] = r1 * cos(a * PI / 180);
		p1[2] = h;
		p2[0] = r1 * sin((a + b) * PI / 180);
		p2[1] = r1 * cos((a + b) * PI / 180);
		p2[2] = h;
		p3[0] = r1 * sin((a + b) * PI / 180);
		p3[1] = r1 * cos((a + b) * PI / 180);
		p3[2] = 0;
		FindNormal(p2, p1, p3);
		glBegin(GL_POLYGON);
		glVertex3d(r1 * sin(a * PI / 180), r1 * cos(a * PI / 180), h);
		glVertex3d(r1 * sin((a + b) * PI / 180), r1 * cos((a + b) * PI / 180), h);
		glVertex3d(r1 * sin((a + b) * PI / 180), r1 * cos((a + b) * PI / 180), 0);
		glVertex3d(r1 * sin(a * PI / 180), r1 * cos(a * PI / 180), 0);
		glEnd();
		a += b;
	}

	for (a = 90; a < 90 + 53.13;)	// тут еще один кусочек 53.13
	{
		p1[0] = 7;
		p1[1] = -5;
		p1[2] = 0;
		p2[0] = r1 * sin(a * PI / 180);
		p2[1] = r1 * cos(a * PI / 180);
		p3[0] = r1 * sin((a + b) * PI / 180);
		p3[1] = r1 * cos((a + b) * PI / 180);
		FindNormal(p1, p2, p3);
		glBegin(GL_TRIANGLE_FAN);
		glColor3d(0.3, 0.6, 0.3);
		glVertex3d(7, -5, 0);// бля это кароче точка C в новых координатах после смещения
		glVertex3d(r1 * sin(a * PI / 180), r1 * cos(a * PI / 180), 0);
		glVertex3d(r1 * sin((a + b) * PI / 180), r1 * cos((a + b) * PI / 180), 0);
		glEnd();

		p1[0] = 7;
		p1[1] = -5;
		p1[2] = h;
		p2[2] = h;
		p3[2] = h;
		FindNormal(p1, p2, p3);
		glBegin(GL_TRIANGLE_FAN);
		glColor3d(0.3, 0.6, 0.3);
		glVertex3d(7, -5, h);// бля это кароче точка C в новых координатах после смещения еще и сверху
		glVertex3d(r1 * sin(a * PI / 180), r1 * cos(a * PI / 180), h);
		glVertex3d(r1 * sin((a + b) * PI / 180), r1 * cos((a + b) * PI / 180), h);
		glEnd();

		p1[0] = r1 * sin(a * PI / 180);
		p1[1] = r1 * cos(a * PI / 180);
		p1[2] = h;
		p2[0] = r1 * sin((a + b) * PI / 180);
		p2[1] = r1 * cos((a + b) * PI / 180);
		p2[2] = h;
		p3[0] = r1 * sin((a + b) * PI / 180);
		p3[1] = r1 * cos((a + b) * PI / 180);
		p3[2] = 0;
		FindNormal(p2, p1, p3);
		glBegin(GL_POLYGON);
		glVertex3d(r1 * sin(a * PI / 180), r1 * cos(a * PI / 180), h);
		glVertex3d(r1 * sin((a + b) * PI / 180), r1 * cos((a + b) * PI / 180), h);
		glVertex3d(r1 * sin((a + b) * PI / 180), r1 * cos((a + b) * PI / 180), 0);
		glVertex3d(r1 * sin(a * PI / 180), r1 * cos(a * PI / 180), 0);
		glEnd();
		a += b;
	}
	glPopMatrix(); //выпуплость построена !!!


	glPushMatrix();
	glTranslated(8, -14.5, 0);	//тут кароче сдвиг на центр окружности выпуклой, чтобы мозги не ебать
	for (a = 50.19; a < 180 + 50.19;) //тут  50.19 - это угол поворота полуокружности (индивидуально по варику посчитать)
	{
		p1[0] = 0;
		p1[1] = 0;
		p1[2] = 0;
		p2[0] = r2 * sin(a * PI / 180);
		p2[1] = r2 * cos(a * PI / 180);
		p2[2] = 0;
		p3[0] = r2 * sin((a + b) * PI / 180);
		p3[1] = r2 * cos((a + b) * PI / 180);
		p3[2] = 0;
		FindNormal(p2, p1, p3);
		glBegin(GL_TRIANGLE_FAN);
		glColor3d(0.3, 0.6, 0.3);
		glVertex3d(0, 0, 0);
		glVertex3d(r2 * sin(a * PI / 180), r2 * cos(a * PI / 180), 0); // sqrt(61) - диаметр окружности (то же самое)
		glVertex3d(r2 * sin((a + b) * PI / 180), r2 * cos((a + b) * PI / 180), 0);
		glEnd();
		p1[2] = h;
		p2[2] = h;
		p3[2] = h;
		FindNormal(p2, p1, p3);
		glBegin(GL_TRIANGLE_FAN);
		glColor3d(0.3, 0.6, 0.3);
		glVertex3d(0, 0, 10);
		glVertex3d(r2 * sin(a * PI / 180), r2 * cos(a * PI / 180), h);
		glVertex3d(r2 * sin((a + b) * PI / 180), r2 * cos((a + b) * PI / 180), h);
		glEnd();

		p1[0] = r2 * sin(a * PI / 180);
		p1[1] = r2 * cos(a * PI / 180);
		p1[2] = h;
		p2[0] = r2 * sin((a + b) * PI / 180);
		p2[1] = r2 * cos((a + b) * PI / 180);
		p2[2] = h;
		p3[0] = r2 * sin((a + b) * PI / 180);
		p3[1] = r2 * cos((a + b) * PI / 180);
		p3[2] = 0;
		FindNormal(p1, p2, p3);
		glBegin(GL_POLYGON);
		glVertex3d(r2 * sin(a * PI / 180), r2 * cos(a * PI / 180), 10);
		glVertex3d(r2 * sin((a + b) * PI / 180), r2 * cos((a + b) * PI / 180), h);
		glVertex3d(r2 * sin((a + b) * PI / 180), r2 * cos((a + b) * PI / 180), 0);
		glVertex3d(r2 * sin(a * PI / 180), r2 * cos(a * PI / 180), 0);
		glEnd();
		a += b;
	}
	glPopMatrix();

	glBegin(GL_TRIANGLES);
	glColor4d(1, 0.21, 0.62, 0.5);
	glVertex3dv(H1);
	glVertex3dv(C1);
	glVertex3dv(F1);

	glVertex3dv(F1);
	glVertex3dv(G1);
	glVertex3dv(H1);

	glVertex3dv(E1);
	glVertex3dv(F1);
	glVertex3dv(D1);

	glVertex3dv(F1);
	glVertex3dv(C1);
	glVertex3dv(D1);
	glEnd();
}

void Render(OpenGL* ogl)
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);

	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	if (lightMode)
		glEnable(GL_LIGHTING);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	//настройка материала
	GLfloat amb[] = { 0.4, 0.2, 0.1, 1.0 };
	GLfloat dif[] = { 0.4, 0.45, 0.2, 1.0 };
	GLfloat spec[] = { 0.9, 0.8, 0.3, 1.0 };
	GLfloat sh = 1.0f * 256;


	//цвет фонового отражения материала
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//цвет рассеянного отражения материала
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//цвет зеркального отражения материала
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	//степень в формуле зеркального отражения (коэффициент блеска). Допускаются значения в интервале [0; 128].
	glMaterialf(GL_FRONT, GL_SHININESS, sh);

	//чтоб было красиво, без квадратиков (сглаживание освещения)
	glShadeModel(GL_SMOOTH);
	//===================================
	//Прогать тут  
	MakePrism();

	//===================================
   //Сообщение вверху экрана


	glMatrixMode(GL_PROJECTION);	//Делаем активной матрицу проекций. 
	//(всек матричные операции, будут ее видоизменять.)
	glPushMatrix();   //сохраняем текущую матрицу проецирования (которая описывает перспективную проекцию) в стек 				    
	glLoadIdentity();	  //Загружаем единичную матрицу
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);	 //врубаем режим ортогональной проекции

	glMatrixMode(GL_MODELVIEW);		//переключаемся на модел-вью матрицу
	glPushMatrix();			  //сохраняем текущую матрицу в стек (положение камеры, фактически)
	glLoadIdentity();		  //сбрасываем ее в дефолт

	glDisable(GL_LIGHTING);



	GuiTextRectangle rec;		   //классик моего авторства для удобной работы с рендером текста.
	rec.setSize(300, 150);
	rec.setPosition(10, ogl->getHeight() - 150 - 10);


	std::stringstream ss;
	ss << "T - вкл/выкл текстур" << std::endl;
	ss << "L - вкл/выкл освещение" << std::endl;
	ss << "F - Свет из камеры" << std::endl;
	ss << "G - двигать свет по горизонтали" << std::endl;
	ss << "G+ЛКМ двигать свет по вертекали" << std::endl;
	ss << "Коорд. света: (" << light.pos.X() << ", " << light.pos.Y() << ", " << light.pos.Z() << ")" << std::endl;
	ss << "Коорд. камеры: (" << camera.pos.X() << ", " << camera.pos.Y() << ", " << camera.pos.Z() << ")" << std::endl;
	ss << "Параметры камеры: R=" << camera.camDist << ", fi1=" << camera.fi1 << ", fi2=" << camera.fi2 << std::endl;

	rec.setText(ss.str().c_str());
	rec.Draw();

	glMatrixMode(GL_PROJECTION);	  //восстанавливаем матрицы проекции и модел-вью обратьно из стека.
	glPopMatrix();


	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

}