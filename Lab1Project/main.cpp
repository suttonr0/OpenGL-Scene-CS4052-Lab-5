// To remove fopen error message
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_MATH_DEFINES // for M_PI

//Some Windows Headers (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "maths_funcs.h"

// Assimp includes
#include <assimp/cimport.h> // C importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.

// STB Image loader
// https://github.com/nothings/stb/blob/master/stb_image.h
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


/*----------------------------------------------------------------------------
                MESHES TO LOAD
  ----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
#define MESH_NAME1 "C:/.Trinity 4/CS4052 Computer Graphics/Lab 5/Lab 5 Code/Lab1Project/BasicTree.dae"
#define MESH_NAME2 "C:/.Trinity 4/CS4052 Computer Graphics/Lab 5/Lab 5 Code/Lab1Project/UVSnowman.obj"
#define MESH_NAME3 "C:/.Trinity 4/CS4052 Computer Graphics/Lab 5/Lab 5 Code/Lab1Project/basicplane.dae"
/*----------------------------------------------------------------------------
				TEXTURES TO LOAD
----------------------------------------------------------------------------*/

// Textures need to be larger than ???x??? and in png or jpeg format
#define SNOW_TEXTURE "C:/.Trinity 4/CS4052 Computer Graphics/Lab 5/Lab 5 Code/Lab1Project/SnowmanTexture.png"
#define GROUND_TEXTURE "C:/.Trinity 4/CS4052 Computer Graphics/Lab 5/Lab 5 Code/Lab1Project/SnowTexture.png"

/*----------------------------------------------------------------------------
  ----------------------------------------------------------------------------*/




std::vector<float> g_vp, g_vn, g_vt;
int tree_vertex_count = 0;  // Global variable to store the number of vertices in the tree object mesh
int snowman_vertex_count = 0;  // Global variable to store the number of vertices in the snowman mesh
int ground_vertex_count = 0;  // Global variable to store the number of vertices in the ground mesh


// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;
GLuint shaderProgramID;

unsigned int mesh_vao = 0;

GLuint treeID = 1;
GLuint snowman_ID = 2;
GLuint ground_ID = 3;

GLuint tree_tex_ID = 1;
GLuint snowman_tex_ID = 2;
GLuint ground_tex_ID = 3;


int width = 800;
int height = 600;

GLuint loc1, loc2, loc3;
GLfloat rotate_y = 0.0f;
GLfloat translate_y = 0.0f;
GLfloat camerarotationy = 0.0f;

vec3 cameraPosition = vec3(0, 2, -15); 
vec3 cameraDirection = vec3(0.0f, 0.0f, 1.0f); // start direction depends on camerarotationy, not this vector
vec3 cameraUpVector = vec3(0.0f, 1.0f, 0.0f);


#pragma region MESH LOADING
/*----------------------------------------------------------------------------
                   MESH LOADING FUNCTION
  ----------------------------------------------------------------------------*/

bool load_mesh (const char* file_name) {
  const aiScene* scene = aiImportFile (file_name, aiProcess_Triangulate); // TRIANGLES!
  if (!scene) {
    fprintf (stderr, "ERROR: reading mesh %s\n", file_name);
    return false;
  }
  printf ("  %i animations\n", scene->mNumAnimations);
  printf ("  %i cameras\n", scene->mNumCameras);
  printf ("  %i lights\n", scene->mNumLights);
  printf ("  %i materials\n", scene->mNumMaterials);
  printf ("  %i meshes\n", scene->mNumMeshes);
  printf ("  %i textures\n", scene->mNumTextures);
  
  for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
	  const aiMesh* mesh = scene->mMeshes[m_i];
	  printf("    %i vertices in mesh\n", mesh->mNumVertices);

	  // Clear from previous mesh
	  g_vp.clear();
	  g_vn.clear();
	  g_vt.clear();

	  if (file_name == MESH_NAME1) {
		  printf("found tree\n");
		  tree_vertex_count = mesh->mNumVertices;  // Count number of vertices for later drawing
	  }
	  else if (file_name == MESH_NAME2) {
		  snowman_vertex_count = mesh->mNumVertices;
		  printf("found snowman\n");
	  }
	  else if (file_name == MESH_NAME3) {
		  ground_vertex_count = mesh->mNumVertices;
		  printf("found ground plane\n");
	  }
	  else
		  printf("MESH NOT FOUND!!");

    for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
      if (mesh->HasPositions ()) {
        const aiVector3D* vp = &(mesh->mVertices[v_i]);
        //printf ("      vp %i (%f,%f,%f)\n", v_i, vp->x, vp->y, vp->z);
        g_vp.push_back (vp->x);
        g_vp.push_back (vp->y);
        g_vp.push_back (vp->z);
      }
      if (mesh->HasNormals ()) {
        const aiVector3D* vn = &(mesh->mNormals[v_i]);
        // printf ("      vn %i (%f,%f,%f)\n", v_i, vn->x, vn->y, vn->z);
        // THESE VERTEX NORMALS ARE INCORRECT FOR PHONG SHADING
		g_vn.push_back (vn->x);
        g_vn.push_back (vn->y);
        g_vn.push_back (vn->z);
      }
      if (mesh->HasTextureCoords (0)) {
        const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
        // printf ("      vt %i (%f,%f)\n", v_i, vt->x, vt->y);
        g_vt.push_back (vt->x);
        g_vt.push_back (vt->y);
      }
      if (mesh->HasTangentsAndBitangents ()) {
        // NB: could store/print tangents here
      }
    }
	printf("      vt size: %i\n", g_vt.size());
  }
  
  aiReleaseImport (scene);
  return true;
}

#pragma endregion MESH LOADING

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS

// Create a NULL-terminated string by reading the provided file
char* readShaderSource(const char* shaderFile) {   
    FILE* fp = fopen(shaderFile, "rb"); //!->Why does binary flag "RB" work and not "R"... wierd msvc thing?

    if ( fp == NULL ) { return NULL; }

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);
    char* buf = new char[size + 1];
    fread(buf, 1, size, fp);
    buf[size] = '\0';

    fclose(fp);

    return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }
	const char* pShaderSource = readShaderSource( pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
    glCompileShader(ShaderObj);
    GLint success;
	// check for shader related errors using glGetShaderiv
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }
	// Attach the compiled shader object to the program object
    glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
    shaderProgramID = glCreateProgram();
    if (shaderProgramID == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }

	//// Change to /simple...Shader.txt for basic test lighting

	// Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgramID, "C:/.Trinity 4/CS4052 Computer Graphics/Lab 5/Lab 5 Code/Lab1Project/Shaders/phongVertexShader.txt", GL_VERTEX_SHADER);
	printf("loaded vertex shader\n");
	AddShader(shaderProgramID, "C:/.Trinity 4/CS4052 Computer Graphics/Lab 5/Lab 5 Code/Lab1Project/Shaders/phongFragmentShader.txt", GL_FRAGMENT_SHADER);
	printf("loaded fragment shader\n");
    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };
	// After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
    glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS


// make g_vp,... local but return them afterwards
// Pass Mesh_Name to the generateObjectBufferMesh and get it to return the vao
void generateObjectBufferMesh(GLuint &vao, const char* meshname, int &count ) {
/*----------------------------------------------------------------------------
                   LOAD MESH HERE AND COPY INTO BUFFERS
  ----------------------------------------------------------------------------*/

	//Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
	//Might be an idea to do a check for that before generating and binding the buffer.

	load_mesh (meshname);
	unsigned int vp_vbo = 0;
	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
	loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

	// vbos are temporary
	glGenBuffers (1, &vp_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
	glBufferData (GL_ARRAY_BUFFER, count * 3 * sizeof (float), &g_vp[0], GL_STATIC_DRAW);
	unsigned int vn_vbo = 0;
	glGenBuffers (1, &vn_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
	glBufferData (GL_ARRAY_BUFFER, count * 3 * sizeof (float), &g_vn[0], GL_STATIC_DRAW);

//	This is for texture coordinates which you don't currently need, so I have commented it out
	unsigned int vt_vbo = 0;
	glGenBuffers(1, &vt_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
	glBufferData(GL_ARRAY_BUFFER, count * 2 * sizeof(float), &g_vt[0], GL_STATIC_DRAW);
	
	// unsigned int vao;  // Vertex array object (Effectively ID of mesh)
	glGenVertexArrays(1, &vao);
	glBindVertexArray (vao);

	glEnableVertexAttribArray (loc1);
	glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer (loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray (loc2);
	glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer (loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);  

//	This is for texture coordinates which you don't currently need, so I have commented it out
	glEnableVertexAttribArray(loc3);
	glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
	glVertexAttribPointer(loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	
}


#pragma endregion VBO_FUNCTIONS

// --------------------------------------------------------
// https://open.gl/textures
// --------------------------------------------------------
void loadTextures(GLuint& tex, const char* file_name) {
	int img_width, img_height, n;

	// STB image loader
	unsigned char* loaded_image = stbi_load(file_name, &img_width, &img_height, &n, STBI_rgb);

	glGenTextures(1, &tex);
	glActiveTexture(GL_TEXTURE0);  // Specifies which texture unit a texture object is bound to with glBindTexture
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_width, img_height, 0, GL_RGB, 
		GL_UNSIGNED_BYTE, loaded_image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Type of interpolation used
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Type of interpolation used
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // repeat across x coordinate if texture too small 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  // repeat across y coordinate if texture too small 
}


void display(){

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable (GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor (0.2f, 0.5f, 0.7f, 1.0f); // Specify the clear colors used to clear the color buffer
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram (shaderProgramID);


	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation (shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation (shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation (shaderProgramID, "proj");
	int texture_location = glGetUniformLocation(shaderProgramID, "texture_for_shader");
	
	cameraDirection.v[0] = sin(camerarotationy);
	cameraDirection.v[2] = cos(camerarotationy);
	mat4 view = look_at(cameraPosition, cameraPosition + cameraDirection, cameraUpVector);
	mat4 persp_proj = perspective(45.0, (float)width/(float)height, 0.1, 100.0);

	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);


	// SNOWMAN 1 ----------------------
	mat4 snowman_matrix = identity_mat4();
	snowman_matrix = translate(snowman_matrix, vec3(2.0, 0.0, 0.0));
	//snowman_matrix = rotate_x_deg(snowman_matrix, -90);

	// Texturing
	glBindTexture(GL_TEXTURE_2D, snowman_tex_ID);
	glUniform1i(texture_location, 0);

	// Bind vertices
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, snowman_matrix.m);
	glBindVertexArray(snowman_ID);
	glDrawArrays(GL_TRIANGLES, 0, snowman_vertex_count);

	// TREE 1 ---------------------
	mat4 tree_matrix = identity_mat4();
	tree_matrix = rotate_x_deg(tree_matrix, -90);
	tree_matrix = translate(tree_matrix, vec3(0, -1, -10));
	tree_matrix = translate(tree_matrix, vec3(0, translate_y, 0));

	// Texturing
	glBindTexture(GL_TEXTURE_2D, tree_tex_ID);
	glUniform1i(texture_location, 0);

	// update uniforms & draw
	glUniformMatrix4fv (matrix_location, 1, GL_FALSE, tree_matrix.m);
	glBindVertexArray(treeID);
	glDrawArrays (GL_TRIANGLES, 0, tree_vertex_count);
	
	// TREE 2 ---------------------
	mat4 tree_matrix2 = identity_mat4();
	tree_matrix2 = rotate_x_deg(tree_matrix2, -90);
	tree_matrix2 = translate(tree_matrix2, vec3(-4, -1, 0));
	tree_matrix2 = translate(tree_matrix2, vec3(0, translate_y, 0));

	// Texturing
	glBindTexture(GL_TEXTURE_2D, tree_tex_ID);
	glUniform1i(texture_location, 0);

	// update uniforms & drawly 
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, tree_matrix2.m);
	glBindVertexArray(treeID);
	glDrawArrays(GL_TRIANGLES, 0, tree_vertex_count);
	
	// GROUND 1 ------------------------
	mat4 ground_matrix = identity_mat4();
	ground_matrix = rotate_x_deg(ground_matrix, -90);
	ground_matrix = scale(ground_matrix, vec3(10.0, 0.0, 10.0));
	// Texturing
	glBindTexture(GL_TEXTURE_2D, ground_tex_ID);
	glUniform1i(texture_location, 0);

	// Bind vertices
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, ground_matrix.m);
	glBindVertexArray(ground_ID);
	glDrawArrays(GL_TRIANGLES, 0, ground_vertex_count);

	glutSwapBuffers();
}


void updateScene() {	

	// Placeholder code, if you want to work with framerate
	// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static DWORD  last_time = 0;
	DWORD  curr_time = timeGetTime();
	float  delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
		delta = 0.03f;
	last_time = curr_time;

	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	// load mesh into a vertex buffer array
	generateObjectBufferMesh(treeID, MESH_NAME1, tree_vertex_count);
	generateObjectBufferMesh(snowman_ID, MESH_NAME2, snowman_vertex_count);
	//generateObjectBufferMesh(ground_ID, MESH_NAME3, ground_vertex_count);

	loadTextures(snowman_tex_ID, SNOW_TEXTURE);
	loadTextures(tree_tex_ID, SNOW_TEXTURE);
	loadTextures(ground_tex_ID, GROUND_TEXTURE);
}

// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {

	if(key=='i'){
		translate_y = translate_y + 0.1;
	}
	if (key == 'w') {
		cameraPosition = cameraPosition + cameraDirection;  // Move forward in direction of camera
	}
	if (key == 's') {
		cameraPosition = cameraPosition - cameraDirection;  // Move backward in direction of camera
	}
	if (key == 'a') {
		// Move left relative to camera direction (in direction 90 deg left of camera direction) 
		cameraPosition.v[0] = cameraPosition.v[0] + cameraDirection.v[2];  // x1' = x1 + y2
		cameraPosition.v[2] = cameraPosition.v[2] - cameraDirection.v[0];  // y1' = y1 - x2
	}
	if (key == 'd') {
		// Move right relative to camera direction (in direction 90 deg right of camera direction) 
		cameraPosition.v[0] = cameraPosition.v[0] - cameraDirection.v[2];  // x1' = x1 - y2
		cameraPosition.v[2] = cameraPosition.v[2] + cameraDirection.v[0];  // y1' = y1 + x2
	}
	if (key == 'q') {
		// Rotate counter-clockwise about y-axis (turn left)
		camerarotationy += 0.015f;
	}
	if (key == 'e') {
		// Rotate clockwise aboutengl y-axis (turn right)
		camerarotationy -= 0.015f;
	}
	display();
}

int main(int argc, char** argv){

	// Set up the window
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(width, height);
    glutCreateWindow("Hello Triangle");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);

	 // A call to glewInit() must be done after glut is initialized!
    GLenum res = glewInit();
	// Check for any errors
    if (res != GLEW_OK) {
      fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
      return 1;
    }
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
    return 0;
}











