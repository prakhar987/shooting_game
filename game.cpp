
#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SOIL/SOIL.h>
#include <FTGL/ftgl.h>
#include <unistd.h>
#include <SOIL.h>
using namespace std;

// DEFINATIONS
#define  MAX_SEGMENTS  15000
#define  G             9.8
#define  DEFAULT_LAUNCH_VELOCITY 10
#define  GROUND_FRICTION 2.35
// GLOBAL VARIABLES
int fireball_act=0;
int circle_key=0; int rec_key=0;
float cannon_angle=0;
float u= DEFAULT_LAUNCH_VELOCITY; // initial velocity
float camera_angle=0;
// GAME VARIABLES
int hit=0;

GLfloat u_xn = -13.0f;
GLfloat u_xp = 13.0f;
GLfloat u_yn = -6.0f;
GLfloat u_yp = 6.0f;

// Structures
struct VAO {
	GLuint VertexArrayID;
	GLuint VertexBuffer;
	GLuint ColorBuffer;
	GLuint TextureBuffer;
	GLuint TextureID;

	GLenum PrimitiveMode; // GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_LINE_STRIP_ADJACENCY, GL_LINES_ADJACENCY, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES, GL_TRIANGLE_STRIP_ADJACENCY and GL_TRIANGLES_ADJACENCY
	GLenum FillMode; // GL_FILL, GL_LINE
	int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID; // For use with normal shader
	GLuint TexMatrixID; // For use with texture shader
} Matrices;


struct FTGLFont {
	FTFont* font;
	GLuint fontMatrixID;
	GLuint fontColorID;
} GL3Font;

GLuint programID, fontProgramID, textureProgramID;

struct Mobile {
       float theta,x0,y0;double t0;
       float x1,y1;
       float v,radius,mass,u_init; // v is y component of velocity
       VAO *vao;
} mobile[20];

// ARRAYS
//VAO *circle[10];
VAO *rectangle[10];

void gravity (int key)
{ 
  if ( (mobile[key].y1-mobile[key].radius)<= -4.5 && mobile[key].u_init==0 && (mobile[key].v>-0.1)&&(mobile[key].v<0.1))
    {   //mobile[key].u_init=0;
    	return;
    }
  float x=0,y=0;double t;
  float theta = mobile[key].theta;
  float u     = mobile[key].u_init;
  t           = glfwGetTime()-mobile[key].t0;
  x=u*cos((theta*M_PI)/180.0f)*t;
  y=u*sin((theta*M_PI)/180.0f)*t-0.5*G*t*t;
  mobile[key].v=u*sin((theta*M_PI)/180.0f)-G*t;
  mobile[key].x1= mobile[key].x0+x;
  if((mobile[key].y0+y)>=-4.5)
  mobile[key].y1= mobile[key].y0+y;
}

void hit_ground(int key)
{ 
  float parameter = -4.5 + mobile[key].radius;
  if (mobile[key].y1 <= parameter ) //&& abs(mobile[key].v)>0)
   {  
    float v,v1,hit_angle,x;
    float theta=mobile[key].theta;
    float t_hit=glfwGetTime();
    float u=mobile[key].u_init;
    
    x=u*cos((theta*M_PI)/180.0f)*(t_hit- mobile[key].t0 );
    mobile[key].x0=x+mobile[key].x0;
    mobile[key].y0=parameter;
    mobile[key].t0=glfwGetTime();
    if(key!=2)
    	mobile[key].u_init=mobile[key].v;
    if(mobile[key].u_init>0)
     mobile[key].u_init-=GROUND_FRICTION;
    if(mobile[key].u_init<0)
     mobile[key].u_init+=GROUND_FRICTION;	
    if(mobile[key].u_init<1.5 && mobile[key].u_init>-1.5)
      { mobile[key].u_init=0;mobile[key].v=0;mobile[key].theta=90;}
    //if(abs(mobile[key].v)<0.5)
      //mobile[key].v=0;	
   }
}

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	cout << "Compiling shader : " <<  vertex_file_path << endl;
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage( max(InfoLogLength, int(1)) );
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	cout << VertexShaderErrorMessage.data() << endl;

	// Compile Fragment Shader
	cout << "Compiling shader : " << fragment_file_path << endl;
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage( max(InfoLogLength, int(1)) );
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	cout << FragmentShaderErrorMessage.data() << endl;

	// Link the program
	cout << "Linking program" << endl;
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	cout << ProgramErrorMessage.data() << endl;

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
 {
    fprintf(stderr, "Error: %s\n", description);
 }

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

glm::vec3 getRGBfromHue (int hue)
{
	float intp;
	float fracp = modff(hue/60.0, &intp);
	float x = 1.0 - abs((float)((int)intp%2)+fracp-1.0);

	if (hue < 60)
		return glm::vec3(1,x,0);
	else if (hue < 120)
		return glm::vec3(x,1,0);
	else if (hue < 180)
		return glm::vec3(0,1,x);
	else if (hue < 240)
		return glm::vec3(0,x,1);
	else if (hue < 300)
		return glm::vec3(x,0,1);
	else
		return glm::vec3(1,0,x);
}

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
						  0,                  // attribute 0. Vertices
						  3,                  // size (x,y,z)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
						  1,                  // attribute 1. Color
						  3,                  // size (r,g,b)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
	GLfloat* color_buffer_data = new GLfloat [3*numVertices];
	for (int i=0; i<numVertices; i++) {
		color_buffer_data [3*i] = red;
		color_buffer_data [3*i + 1] = green;
		color_buffer_data [3*i + 2] = blue;
	}

	return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

struct VAO* create3DTexturedObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* texture_buffer_data, GLuint textureID, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;
	vao->TextureID = textureID;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->TextureBuffer));  // VBO - textures

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
						  0,                  // attribute 0. Vertices
						  3,                  // size (x,y,z)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	glBindBuffer (GL_ARRAY_BUFFER, vao->TextureBuffer); // Bind the VBO textures
	glBufferData (GL_ARRAY_BUFFER, 2*numVertices*sizeof(GLfloat), texture_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
						  2,                  // attribute 2. Textures
						  2,                  // size (s,t)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	return vao;
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Enable Vertex Attribute 1 - Color
	glEnableVertexAttribArray(1);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

void draw3DTexturedObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Bind Textures using texture units
	glBindTexture(GL_TEXTURE_2D, vao->TextureID);

	// Enable Vertex Attribute 2 - Texture
	glEnableVertexAttribArray(2);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->TextureBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle

	// Unbind Textures to be safe
	glBindTexture(GL_TEXTURE_2D, 0);
}
/* Create an OpenGL Texture from an image */
GLuint createTexture (const char* filename)
{
	GLuint TextureID;
	// Generate Texture Buffer
	glGenTextures(1, &TextureID);
	// All upcoming GL_TEXTURE_2D operations now have effect on our texture buffer
	glBindTexture(GL_TEXTURE_2D, TextureID);
	// Set our texture parameters
	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Set texture filtering (interpolation)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Load image and create OpenGL texture
	int twidth, theight;
	unsigned char* image = SOIL_load_image(filename, &twidth, &theight, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, twidth, theight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D); // Generate MipMaps to use
	SOIL_free_image_data(image); // Free the data read from file after creating opengl texture
	glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess it up

	return TextureID;
}
/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = -1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;

void updateProjection()
 {
    Matrices.projection = glm::ortho(u_xn, u_xp, u_yn, u_yp, 0.1f, 500.0f);
 }

void zoomin()
{
  u_xn += 0.3f;
  u_xp -= 0.3f;
  u_yn += 0.15f;
  u_yp -= 0.15f;
}

void zoomout()
{
  u_xn -= 0.3f;
  u_xp += 0.3f;
  u_yn -= 0.15f;
  u_yp += 0.15f;
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
 {
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0,0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(-13.0f, 13.0f, -6.0f, 6.0f, 0.1f, 500.0f);
}
void panleft()
 {
   camera_angle-=0.5;
   //reshapeWindow();
 }

void panright()
{
  camera_angle+=0.5;
   // reshapeWindow();
}


/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
 {
     // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_D:
                //cannon_angle-=4;
                break;
            case GLFW_KEY_A:
                //cannon_angle+=4;
                break;
            case GLFW_KEY_X:
                // do something ..
                break;
            default:
                break;
        }
    }
    else if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_F:
              if(u <= 1.5*DEFAULT_LAUNCH_VELOCITY)
                u+=1;
                break;
            case GLFW_KEY_S:
              if(u>=7)
                u-=1;
                break;
            case GLFW_KEY_B:
                cannon_angle-=5;
                break;
            case GLFW_KEY_A:
                cannon_angle+=5;
                break;
            case GLFW_KEY_SPACE: // Act Fireball
             mobile[2].x0= -12.3 + cos((cannon_angle*M_PI)/180.0f)*1.25; // launch from centre of cannon
             mobile[2].y0= -4.0  + sin((cannon_angle*M_PI)/180.0f)*1.25;
             mobile[2].t0=glfwGetTime();
             mobile[2].theta= cannon_angle; 
             mobile[2].u_init=u;mobile[key].v=0;
             mobile[2].mass=0.32;
             fireball_act=1;
                break;
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            case GLFW_KEY_UP:
              zoomin();
              updateProjection();
              break;
            case GLFW_KEY_DOWN:
              zoomout();
              updateProjection();
              break;
            case GLFW_KEY_LEFT:
              panleft();
              //updateProjection();
              break;
            case GLFW_KEY_RIGHT:
              panright();
              //updateProjection();
              break;
            default:
              break;


        }
    }
       // else if(action == GLFW_REPEAT)
    //{
      //switch(key)
      //{

      //}
    //}
 }

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{   
    double xpos, ypos;
    double result;
    float window_width = 1300;
    float window_height = 600;
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_PRESS)
                {
                    glfwGetCursorPos(window, &xpos, &ypos);
                    ypos = window_height - ypos;
                    result=atan(ypos/xpos);
                    ypos =(ypos/600.0)*12;
                    xpos =(xpos/1300.0)*26;
                    if(xpos>=0&&xpos<=13) xpos=-13+xpos;
                    if(xpos>13)    xpos=xpos-13.0;
                    if(ypos>=0&&ypos<=6) ypos=-6+ypos;
                    if(ypos>6)ypos=ypos-6.0; 
                    result = (result*180.0) / M_PI;
                    cannon_angle=result;
                    mobile[2].theta= cannon_angle;       
                    //if(xpos < window_width/4)
                     if(xpos >= window_width/4 && xpos < window_width/2) u=12.5;
                     else if(xpos >= window_width/2 && xpos < window_width*3/4.0f) u=15;
                     else u=17.5;


              mobile[2].x0= -12.3 + cos((cannon_angle*M_PI)/180.0f)*1.25; // launch from centre of cannon
              mobile[2].y0= -4.0  + sin((cannon_angle*M_PI)/180.0f)*1.25;
              mobile[2].u_init=u;
              mobile[2].t0=glfwGetTime();
              mobile[2].v=0;
              mobile[2].mass=0.32;
              fireball_act=1;
                }
                break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_PRESS) {
                /*
                glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, 1);
                glfwGetCursorPos(window, &xpos, &ypos);
                if((float)xpos > prevX)panCameraRight();
                else if((float)xpos < prevX)panCameraLeft();
                prevX = (float)xpos;
                */
            //    panFlag = true;
 
            }
            else if(action == GLFW_RELEASE){
                //glfwSetInputMode(window, GLFW_CURSOR_NORMAL, 1);
                  //  prevX = 0.0f;
                    //panFlag = false;
            }
            break;
        default:
            break;
    }
}



VAO *triangle,*rect_text;

// Creates the triangle object used in this sample code
void createTriangle ()
{
  /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

  /* Define vertex array as used in glBegin (GL_TRIANGLES) */
  static const GLfloat vertex_buffer_data [] = {
    0, 1,0, // vertex 0
    -1,-1,0, // vertex 1
    1,-1,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 0
    0,1,0, // color 1
    0,0,1, // color 2
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
}
//////////////////////////////BASIC OBJECTS  ///////////////////////////////////////////////////////
void createCircle (int key,GLfloat red, GLfloat blue, GLfloat green,float radius,const char *c)
{ mobile[key].radius=radius;
  float theta;
  int numSegments=360;
  int degrees = 3 * 360 + 3;
    GLfloat vertex_buffer_data[MAX_SEGMENTS * 9];
    GLfloat  color_buffer_data[MAX_SEGMENTS * 9];

  for(int i = 0; i <= numSegments; i++)
  {
    theta = 2.0 * M_PI * float(i) / float(numSegments);
    vertex_buffer_data[9*i + 0] = 0;
    vertex_buffer_data[9*i + 1] = 0;
    vertex_buffer_data[9*i + 2] = 0;

    if(i == 0)
    {
      vertex_buffer_data[9*i + 3] = radius;
      vertex_buffer_data[9*i + 4] = 0;
      vertex_buffer_data[9*i + 5] = 0;
    }

    else
    {
      vertex_buffer_data[9*i + 3] = vertex_buffer_data[9*i - 3];
      vertex_buffer_data[9*i + 4] = vertex_buffer_data[9*i - 2];
      vertex_buffer_data[9*i + 5] = vertex_buffer_data[9*i - 1];
    }

    vertex_buffer_data[9*i + 6] = radius * cosf(theta);
    vertex_buffer_data[9*i + 7] = radius * sinf(theta);
    vertex_buffer_data[9*i + 8] = 0;

    color_buffer_data[9*i + 0] = red + 0.2f;
    color_buffer_data[9*i + 1] = green + 0.2f;
    color_buffer_data[9*i + 2] = blue + 0.2f;


    for(int j = 3; j < 9; j++)
    {
      //Adding color
      switch(j % 3)
      {
        case 0:
          color_buffer_data[9*i + j] = red;
          break;
        case 1:
          color_buffer_data[9*i + j] = green;
          break;
        case 2:
          color_buffer_data[9*i + j] = blue;
          break;
      }
    }
  }
  if (c==NULL)
    mobile[circle_key ++].vao =create3DObject(GL_TRIANGLES, degrees, vertex_buffer_data, color_buffer_data, GL_FILL);  
   else
  {
	glActiveTexture(GL_TEXTURE0);
	GLuint textureID = createTexture(c);
	// check for an error during the load process
	if(textureID == 0 )
		cout << "SOIL loading error: '" << SOIL_last_result() << "'" << endl;

	static const GLfloat texture_buffer_data [] = {
		0,1, // TexCoord 1 - bot left
		1,1, // TexCoord 2 - bot right
		1,0, // TexCoord 3 - top right

		1,0, // TexCoord 3 - top right
		0,0, // TexCoord 4 - top left
		0,1  // TexCoord 1 - bot left
	};

  mobile[circle_key ++].vao= create3DTexturedObject(GL_TRIANGLES, degrees,vertex_buffer_data, texture_buffer_data,textureID, GL_FILL);
  }

}

void init_circle(int key,float x1,float y1,float mass)
 {
   mobile[key].x1=x1;
   mobile[key].y1=y1;
   mobile[key].mass=mass;
 }

void movecircle(int key,float x,float y,float z)
{ glUseProgram (programID);
  glm::mat4 VP = Matrices.projection * Matrices.view;
  glm::mat4 MVP; 

  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateCircle = glm::translate (glm::vec3(x, y, z)); // glTranslatef
  glm::mat4 circleTransform = translateCircle;
  Matrices.model *= circleTransform; 
  MVP = VP * Matrices.model;

  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  glUseProgram (programID);
 
  draw3DObject(mobile[key].vao);
}

void move_Text_Circle(int key , float x, float y,float z)
{
	glUseProgram(textureProgramID);

  glm::mat4 VP = Matrices.projection * Matrices.view;
  glm::mat4 MVP; 
  Matrices.model = glm::mat4(1.0f);

   glm::mat4 translateCircle = glm::translate (glm::vec3(x, y, z)); // glTranslatef
  glm::mat4 circleTransform = translateCircle;
  Matrices.model *= circleTransform; 
  MVP = VP * Matrices.model;

  //  Don't change unless you are sure!!
   glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
   glUseProgram(textureProgramID);
  	// Set the texture sampler to access Texture0 memory
   glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);
   draw3DTexturedObject(mobile[key].vao);	
 
}

void createRectangle (GLfloat red, GLfloat blue, GLfloat green,float length, float width,const char *c)
  {
  float angle = 0;
  float rotationspeed = 5.0f;  
  float minangle, maxangle;
  const int rectangleVertex = 6;
  int x=0,y=0;
  GLfloat vertex_buffer_data[3 * rectangleVertex];
  GLfloat  color_buffer_data[3 * rectangleVertex];
  minangle =   0;
  maxangle = 180;
    

  vertex_buffer_data [0] =            x;
  vertex_buffer_data [1] =            y;

  vertex_buffer_data [3] = x + length;
  vertex_buffer_data [4] =          y;

  vertex_buffer_data [6] =            x;
  vertex_buffer_data [7] =  y + width;


  vertex_buffer_data [9] =            x;
  vertex_buffer_data[10] =  y + width;

  vertex_buffer_data[12] = x + length;
  vertex_buffer_data[13] =  y + width;

  vertex_buffer_data[15] = x + length;
  vertex_buffer_data[16] =          y;

  vertex_buffer_data [2] = vertex_buffer_data [5] = vertex_buffer_data [8] = 0;
  vertex_buffer_data [11] = vertex_buffer_data [14] = vertex_buffer_data [17] = 0;


  for(int i = 0; i < rectangleVertex; i++)
  {
    color_buffer_data[i + 0] = red;
    color_buffer_data[i + 1] = green;
    color_buffer_data[i + 2] = blue;
  }
  if(c==NULL)
   rectangle[rec_key++] = create3DObject(GL_TRIANGLES, rectangleVertex, vertex_buffer_data, color_buffer_data, GL_FILL);
  else
  {
     // Load Textures
	// Enable Texture0 as current texture memory
	glActiveTexture(GL_TEXTURE0);
	// load an image file directly as a new OpenGL texture
	// GLuint texID = SOIL_load_OGL_texture ("beach.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS); // Buggy for OpenGL3
	GLuint textureID = createTexture(c);
	// check for an error during the load process
	if(textureID == 0 )
		cout << "SOIL loading error: '" << SOIL_last_result() << "'" << endl;
	// GL3 accepts only Triangles. Quads are not supported


	// Texture coordinates start with (0,0) at top left of the image to (1,1) at bot right
	static const GLfloat texture_buffer_data [] = {
		0,1, // TexCoord 1 - bot left
		1,1, // TexCoord 2 - bot right
		1,0, // TexCoord 3 - top right

		1,0, // TexCoord 3 - top right
		0,0, // TexCoord 4 - top left
		0,1  // TexCoord 1 - bot left
	};

	// create3DTexturedObject creates and returns a handle to a VAO that can be used later

  rectangle[rec_key++]= create3DTexturedObject(GL_TRIANGLES, 6, vertex_buffer_data, texture_buffer_data, textureID, GL_FILL);
  }
}


void moveRectangle(int key , float x, float y,float z,float rectangle_rotation)
{  glUseProgram (programID);

  glm::mat4 VP = Matrices.projection * Matrices.view;
  glm::mat4 MVP; 
  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRec = glm::translate (glm::vec3(x, y, z)); // glTranslatef
  glm::mat4 rotateRec = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRec * rotateRec); 
  MVP = VP * Matrices.model;

  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(rectangle[key]);
   
}

void move_Text_Rec(int key , float x, float y,float z,float rectangle_rotation)
{
	glUseProgram(textureProgramID);

  glm::mat4 VP = Matrices.projection * Matrices.view;
  glm::mat4 MVP; 
  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRec = glm::translate (glm::vec3(x, y, z)); // glTranslatef
  glm::mat4 rotateRec = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRec * rotateRec); 
  MVP = VP * Matrices.model;

  //  Don't change unless you are sure!!
   glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
   glUseProgram(textureProgramID);
  	// Set the texture sampler to access Texture0 memory
   glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);
   draw3DTexturedObject(rectangle[key]);	
 
}

float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;

void draw()
{
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  Matrices.view = glm::lookAt(glm::vec3(camera_angle+0,0,3), glm::vec3(camera_angle+0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
}


void drawFont(const char *c,float x,float y,float z,int color,int effect)
{
  glm::mat4 MVP;  // MVP = Projection * View * Model

  static int fontScale = 0;
  float fontScaleValue = 0.75;
  static int color_val=4;
  if(color==1)
   color_val = (color_val+1) % 360;
  glm::vec3 fontColor = getRGBfromHue (color_val);

  // Use font Shaders for next part of code
  glUseProgram(fontProgramID);
  //Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Transform the text
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateText = glm::translate(glm::vec3(x,y,0));
  glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
  Matrices.model *= (translateText * scaleText);
  MVP = Matrices.projection * Matrices.view * Matrices.model;
  // send font's MVP and font color to fond shaders
  glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
  glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

  // Render font
  GL3Font.font->Render(c);

  // font size and color changes
  if(effect==1)
   fontScale = (fontScale + 1) % 360;
}



GLFWwindow* initGLFW (int width, int height)
{
	GLFWwindow* window; // window desciptor/handle

	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(width, height, "SPACE SHOOTER", NULL, NULL);

	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	glfwSwapInterval( 1 );

	/* --- register callbacks with GLFW --- */

	/* Register function to handle window resizes */
	/* With Retina display on Mac OS X GLFW's FramebufferSize
	 is different from WindowSize */
	glfwSetFramebufferSizeCallback(window, reshapeWindow);
	glfwSetWindowSizeCallback(window, reshapeWindow);

	/* Register function to handle window close */
	glfwSetWindowCloseCallback(window, quit);

	/* Register function to handle keyboard input */
	glfwSetKeyCallback(window, keyboard);      // general keyboard input
	glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

	/* Register function to handle mouse click */
	glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

	return window;
}

// NEW initGL
/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
	

	// Create and compile our GLSL program from the texture shaders
	textureProgramID = LoadShaders( "TextureRender.vert", "TextureRender.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.TexMatrixID = glGetUniformLocation(textureProgramID, "MVP");


	/* Objects should be created before any other gl function and shaders */
	// Create the models
	//createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
	


	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL3.vert", "Sample_GL3.frag" ); // HALA
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

	reshapeWindow (window, width, height);

	// Background color of the scene
	glClearColor (0.0f, 0.6f, 0.1f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);
	// glEnable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Initialise FTGL stuff
	const char* fontfile = "arial.ttf";
	GL3Font.font = new FTExtrudeFont(fontfile); // 3D extrude style rendering

	if(GL3Font.font->Error())
	{
		cout << "Error: Could not load font `" << fontfile << "'" << endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Create and compile our GLSL program from the font shaders
	fontProgramID = LoadShaders( "fontrender.vert", "fontrender.frag" );
	GLint fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform;
	fontVertexCoordAttrib = glGetAttribLocation(fontProgramID, "vertexPosition");
	fontVertexNormalAttrib = glGetAttribLocation(fontProgramID, "vertexNormal");
	fontVertexOffsetUniform = glGetUniformLocation(fontProgramID, "pen");
	GL3Font.fontMatrixID = glGetUniformLocation(fontProgramID, "MVP");
	GL3Font.fontColorID = glGetUniformLocation(fontProgramID, "fontColor");

	GL3Font.font->ShaderLocations(fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform);
	GL3Font.font->FaceSize(1);
	GL3Font.font->Depth(0);
	GL3Font.font->Outset(0, 0);
	GL3Font.font->CharMap(ft_encoding_unicode);

	cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
	cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
	cout << "VERSION: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

}
  
// COLISSION SYSTEM
void resolveCollision (int key1, int key2)
{   

   float vx1,vy1,vx2,vy2;
   float theta=mobile[key1].theta;

   vx1=mobile[key1].u_init*cos((theta*M_PI)/180.0f);
   vy1=mobile[key1].v;
   vx2=mobile[key2].u_init*cos((theta*M_PI)/180.0f);;
   vy2=mobile[key2].v;
   float m1=mobile[key1].mass; 
   float m2=mobile[key2].mass;
 
   float cx,cy;

   cx=mobile[key1].x1-mobile[key2].x1;
   cy=mobile[key1].y1-mobile[key2].y1;
   float distance= sqrt((cx * cx ) + (cy * cy));
    //Unit vector in direction of collision
   float unitX, unitY;
   if(distance == 0.0f){
    unitX = 1.0f;
    unitY = 0.0f;
     }
   else{
    unitX = cx/distance;
    unitY = cy/distance;
       }

    
  // Component of velocity of Item first,second in
  // in direction of collision
  float firstInitComp =  vx1 * unitX + vy1 * unitY;
  float secondInitComp = vx2 * unitX + vy2 * unitY;


  float firstFinalComp = (firstInitComp*(m1 - m2) + 2*m2*secondInitComp)/(m1 + m2);
  float secondFinalComp = (secondInitComp*(m2 - m1) + 2*m1*firstInitComp)/(m1 + m2);


  float firstChange = firstFinalComp - firstInitComp;
  float secondChange = secondFinalComp - secondInitComp;
 

   vx1 += firstChange * unitX;
   vy1 += firstChange * unitY;

   vx2 += secondChange * unitX;
   vy2 += secondChange * unitY;
   
   //first.x += first.ux > 0 ? 1.0f : -1.0f;
  //first.y += first.uy > 0 ? 1.0f : -1.0f;

  //second.x -= second.ux > 0 ? 1.0f : -1.0f;
  //second.y -= second.uy > 0 ? 1.0f : -1.0f;
     float t1=atan(vy1/vx1);
     float t2=atan(vy2/vx2);
     float t=t1*t1;t1=sqrt(t);
      t=t2*t2;t2=sqrt(t);

     if( vy1<0)
        t1*=-1;
     if(vy2<0)
     	t2*=-1;

    mobile[key1].theta= (t1*180.0f)/M_PI;
    mobile[key2].theta= (t2*180.0f)/M_PI;

     mobile[key2].u_init=sqrt(vx2*vx2+vy2*vy2);
     mobile[key1].u_init=sqrt(vx1*vx1+vy1*vy1);
     
     mobile[key1].t0=glfwGetTime()-0.05;
     mobile[key2].t0=glfwGetTime()-0.05;

   mobile[key1].x0=mobile[key1].x1;
   mobile[key1].y0=mobile[key1].y1;
   mobile[key2].x0=mobile[key2].x1;
   mobile[key2].y0=mobile[key2].y1;
   
   gravity(2);
}
int  colliding(int key1 ,int key2 )
{
    float xd = mobile[key1].x1 - mobile[key2].x1;
    float yd = mobile[key1].y1 - mobile[key2].y1;
    float distSqr  = (xd * xd) + (yd * yd);
    float distance = sqrt(distSqr);
    float sumRadius = mobile[key1].radius + mobile[key2].radius;
    if (distance <= sumRadius)
        return 1;
    return 0;
}

void check_colission ()
 {
  for (int i = 2; i < circle_key; i++)  
  {  
    for (int j = i + 1; j < circle_key; j++)  
    {  
        if (colliding(i,j)==1)  
        {    
            if(i==2 && i<8 && j<8)

              hit=1;
            resolveCollision(i,j);
        }
    }
  }
 }
 
void init_targets(int key,float x0,float y0)
  {
   mobile[key].x0=x0;   
   mobile[key].y0=y0;   
   mobile[key].y1=mobile[key].y0;
   mobile[key].x1=mobile[key].x0;
   mobile[key].t0=glfwGetTime();
   mobile[key].theta=-90;
   mobile[key].u_init=-0.1;
  }

void create_bodies()
{ 
  
  createCircle(0,1,0,0,0.5,NULL);     init_circle(0,11,-4,500);           // CANNON WHEEL
  createCircle(1,0,1,0,0.25f,NULL);   init_circle(1,-12.3,-4,500);        // CANNON WHEEL
  char a[]="bird.png";
  createCircle(2,0.9,0.0,0.3,0.32f,&a[0]);                                 // FIREBALL

  createCircle(3,0.6,0.3,0.3,0.5,NULL);   init_circle(3,6.5,7,0.3);      // circle obstacle
  createCircle(4,0.3,0.6,0.3,0.25,NULL);   init_circle(4,8,7,0.1);      // circle obstacle
  createCircle(5,0.3,0.3,0.6,0.5,NULL);   init_circle(5,9,7,1.5);      // circle obstacle
  createCircle(6,0.6,0.6,0.3,0.75,NULL);   init_circle(6,10.5,7,15);      // circle obstacle
  createCircle(7,0.3,0.6,0.6,0.65,NULL);   init_circle(7,12,7,7.5);      // circle obstacle
  
  // DEATH
  createCircle(8,1,1,1,0.15,NULL);   init_circle(8,0,3,5000);   
  createCircle(9,1,1,1,0.15,NULL);   init_circle(9,0,0,5000); 
  createCircle(10,1,1,1,0.15,NULL);   init_circle(10,0,-3,5000);   
  createRectangle( 0, 1, 0,1.25, 0.37,NULL); // CANNON HEAD
  char b[]="ground.jpeg";
  createRectangle(0,1,0,160.0,1.5,&b[0]);   // GROUND
  char c[]="background.jpeg";
  createRectangle(0,0,0,36,12,&c[0]);   // image
  //createRectangle(0,0.5,0.3,1,1);    // OBSTACLE
  //createRectangle(0,0.5,0.3,1.5,1.5);    // OBSTACLE2

  init_targets(3,6.5,7);
  init_targets(4,8,7);
  init_targets(5,9,7);
  init_targets(6,10.5,7);
  init_targets(7,12,7);
  init_targets(8,0,3);
  init_targets(9,0,0);
  init_targets(10,0,-3);
}
void flash_screen()
{
	    char c[]="LIFE";
       // drawFont(&c[0],-3,0,0,0);
        sleep(3);

}
int main (int argc, char** argv)
{
  int width = 1300;
  int height = 600;
  GLFWwindow* window = initGLFW(width, height);
  create_bodies();
  initGL (window, width, height);
  
  float init_time=glfwGetTime();
  int   life=13;
  int   score=0; 
  float last_hit=glfwGetTime();
  float current_u_init=-0.1;
  //float death_time=1;
  //flash_screen();
  float death_speed=0.005;
    while (!glfwWindowShouldClose(window)) {
        draw();
        // DEATHS
        move_Text_Rec(2,-13,-6,0,0);   // Background
        move_Text_Rec(1,-13,-6,0,0);               // GROUND
        movecircle(8,mobile[8].x1,mobile[8].y1,0);
        movecircle(9,mobile[9].x1,mobile[9].y1,0);
        movecircle(10,mobile[10].x1,mobile[10].y1,0);
        moveRectangle(0,-12.3,-4,0,cannon_angle);  // CANNON head
        movecircle(0,-12.3,-4,0);                  // cannon wheel
        movecircle(1,-12.3,-4,0);                  // cannon wheel
        if(life>0){
        if (fireball_act==1 )  // FIREBALL INORBIT  
        {  
          gravity(2);
          hit_ground(2);
          move_Text_Circle(2,mobile[2].x1,mobile[2].y1,0);
        }

        

         	death_speed+=0.00001;
          mobile[8].y1+=death_speed;mobile[9].y1+=death_speed;mobile[10].y1+=death_speed;
          if(mobile[8].y1<-4.5)
          	mobile[8].y1=6;
          if(mobile[8].y1>6)
          	mobile[8].y1=-4.5;
                    if(mobile[9].y1<-4.5)
          	mobile[9].y1=6;
          if(mobile[9].y1>6)
          	mobile[9].y1=-4.5;
                    if(mobile[10].y1<-4.5)
          	mobile[10].y1=6;
          if(mobile[10].y1>6)
          	mobile[10].y1=-4.5;

         
        for(int i=3;i<=7;i++)
         { gravity(i);
           hit_ground(i);
           movecircle(i,mobile[i].x1,mobile[i].y1,0); //obstacle circle
           if( rand()%5==0 && mobile[i].u_init==0 || rand()%500  ==10 && mobile[i].theta!=-90 )
             {  if(i==3)
             	mobile[i].x0=6.5;
                if(i==4)
             	mobile[i].x0=8;
                if(i==5)
             	mobile[i].x0=9;
                if(i==6)
             	mobile[i].x0=10.5;               
                if(i==7)
             	mobile[i].x0=12;

             	mobile[i].y0=7;
             	mobile[i].t0=glfwGetTime();
             	mobile[i].x1=mobile[i].x0;mobile[i].y1=mobile[i].y0;
             	mobile[i].u_init=current_u_init;mobile[i].theta=-90;
             }
         } // Ball Making
        check_colission();
        if(hit==1)
        {   
        	score+=1;hit=0;
            char h[]="HIT!!";
        	drawFont(&h[0],-0.47,5.5,0,0,0);
        	last_hit=glfwGetTime();
        }
        if( glfwGetTime()-last_hit>6)
          {
        	life--;last_hit=glfwGetTime();
          }
 
          
        if ((int)glfwGetTime() % 10 == 0)
        	current_u_init+=0.01;
     

      // DISPLAY SECTION
        
        int time=(int)glfwGetTime()-(int)last_hit;
        time=5-time;
        if(time==-1)
        	time=0;
        char cc[20]=" ";
        cc[1]=char(48+time);
        drawFont(&cc[0],-0.42,5,0,1,0);

        char c[10]="LIFE:   ";
        c[6]=char(48+life);
        drawFont(&c[0],-12.5,5.5,0,1,0);

        char d[30]="SCORE: ";
        int num=score,counter=7;
        while(num!=0)
        { int r=num%10;
          d[counter++]=char(48+r);	
          num=num/10;
        }   

        drawFont(&d[0],-12.5,5,0,1,0);
     }
     if(life==0)
       {        
         char x[10]="GAME OVER";
         drawFont(&x[0],-0.5,0.5,0,1,1);
         char y[30]="SCORE: ";
         int num1=score,counter1=7;
         while(num1!=0)
         { int r1=num1%10;
          y[counter1++]=char(48+r1);	
          num1=num1/10;
         }   

         drawFont(&y[0],-0.5,-0.5,0,1,0);
              
       }
        glfwSwapBuffers(window);
        glfwPollEvents();
}
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
