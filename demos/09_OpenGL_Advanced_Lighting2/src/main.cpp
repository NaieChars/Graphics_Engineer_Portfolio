#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <STB_IMAGE/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "SHADER.h"
#include "CAMERA.h"

#include <iostream>
#include <vector>
#include <map>


#define STB_IMAGE_IMPLEMENTATION
#include <STB_IMAGE/stb_image.h>

// Callback function---------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
unsigned int loadCubemap(std::vector<std::string> faces);
void RenderScene(Shader& shader);
void renderCube();
void renderQuad();
void SetupFramebuffers();
void RenderShadowPass(Shader& simpleDepthShader, Shader& shader, float far_plane);
void RenderScenePass(Shader& shader, Shader& lightShader, glm::mat4 view, glm::mat4 projection, float far_plane);
void ResolveMSAA();
void RenderScreen(Shader& screenShader);
void RenderLight(Shader& lightShader, glm::mat4& view, glm::mat4& projection);
void renderPlane();
void RenderFloor(Shader& shader);
void RenderCubes(Shader& shader);
void RenderBloomPass(Shader& filterShader, Shader& gaussShader, Shader& upsampleShader);

// 全局变量----------------------------------------------
unsigned int floorTexture;
unsigned int cubeTexture;
unsigned int cubeNormalMap;
unsigned int defaultNormalMap;

unsigned int planeVAO, planeVBO;
unsigned int quadVAO = 0;
unsigned int quadVBO;
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;

unsigned int framebuffer;
unsigned int texture;
unsigned int rbo;

unsigned int intermediateFBO;
unsigned int screenTexture;

unsigned int depthMapFBO;
unsigned int depthMap;

const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

float exposure = 0.5f;
float bloomStrength = 0.1f;
const int BLOOM_LEVEL = 5;
unsigned int blurFBO[BLOOM_LEVEL + 1];
unsigned int blurColorBuffer1[BLOOM_LEVEL + 1];
unsigned int blurColorBuffer2[BLOOM_LEVEL + 1];
unsigned int blurColorBuffer3[BLOOM_LEVEL + 1];

unsigned int cubeSpecularMap;  // 用来存 _s 贴图
unsigned int defaultSpecularMap;
//---------------------------------------------------------
// 光源位置
glm::vec3 lightPos = glm::vec3(0.0f, 1.0f, 0.0f);

// 窗口设置
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
bool blinn = false;
bool blinnKeyPressed = false;


// 摄像机
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// 跟踪时间差
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// main=====================================================================================
int main()
{
	// glfw: initialize and configure
	//-------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4); // GLFW会自动创建一个每像素4个子采样点的深度和样本缓冲

	// glfw window creation
	//---------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LeranOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Fail to creat GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);


	// 告诉GWLF隐藏光标，并捕捉它
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Fail to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);
	// 深度测试函数，默认是GL_LESS比较运算符
	glDepthFunc(GL_LESS);

	// 启用混合
	glEnable(GL_BLEND);
	// 混合函数，设置源因子和目标因子
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// 启用面剔除

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); 
	glFrontFace(GL_CCW); 

	// opengl 自动 gamma
	//glEnable(GL_FRAMEBUFFER_SRGB);

	// 启用多重采样
	glEnable(GL_MULTISAMPLE);

	// 创建并编译着色器
	// ------------------------------------------------------------------------------------------------------
	Shader shader("resources/shaders/cube_shader.vs", "resources/shaders/cube_shader.fs");
	Shader screenShader("resources/shaders/screenShader.vs", "resources/shaders/screenShader.fs");
	Shader lightShader("resources/shaders/lighting_cube.vs", "resources/shaders/lighting_cube.fs");
	Shader simpleDepthShader(
		"resources/shaders/shadow_mapping_depth.vs",
		"resources/shaders/shadow_mapping_depth.fs",
		"resources/shaders/shadow_mapping_depth.gs"
	);
	Shader filterShader("resources/shaders/screenShader.vs", "resources/shaders/filter.fs");
	Shader gaussShader("resources/shaders/screenShader.vs", "resources/shaders/gauss.fs");
	Shader upsampleShader("resources/shaders/screenShader.vs", "resources/shaders/upsample.fs");
	Shader blendShader("resources/shaders/screenShader.vs", "resources/shaders/blend.fs");

	// 加载纹理---------------------------------------------------------------------------------------------------
	cubeTexture = loadTexture("resources/textures/crying_obsidian.png");
	floorTexture = loadTexture("resources/textures/wood.png");
	cubeNormalMap = loadTexture("resources/textures/crying_obsidian_n.png");
	cubeSpecularMap = loadTexture("resources/textures/crying_obsidian_s.png");

	shader.use();
	shader.setInt("material.albedo", 0);
	shader.setInt("material.normalMap", 2);
	shader.setInt("material.specularMap", 3);

	screenShader.use();
	screenShader.setInt("screenTexture", 0);

	// 加载帧缓冲-------------------------------------------------------------------------------------------------
	SetupFramebuffers();

	//临时默认法线贴图=========
	// 创建一个 1x1 的默认法线贴图 (0.5, 0.5, 1.0) → 解码后是 (0,0,1)
	unsigned char defaultNormal[] = { 128, 128, 255 };
	glGenTextures(1, &defaultNormalMap);
	glBindTexture(GL_TEXTURE_2D, defaultNormalMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, defaultNormal);
	//========================
	// main 里加，创建一个 1x1 的全黑纹理
	unsigned char blackPixel[] = { 0, 0, 0 };
	glGenTextures(1, &defaultSpecularMap);
	glBindTexture(GL_TEXTURE_2D, defaultSpecularMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, blackPixel);
	//======================
	
	// 渲染循环
	//-----------------------------------------
	//-----------------------------------------
	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		/*
		float radius = 3.0f;  // 旋转半径
		float speed = 2.0f;   // 旋转速度
		float angle = glfwGetTime() * speed;

		lightPos = glm::vec3(
			cos(angle) * radius,  // X
			0.3f,                  // Y
			sin(angle) * radius   // Z
		);
		*/

		shader.use();
		shader.setBool("blinn", blinn);

		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
			(float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();

		float far_plane = 20.0f;  // ← 只需要 far_plane，不再需要 lightSpaceMatrix

		RenderShadowPass(simpleDepthShader, shader, far_plane);  // ← 改成传 far_plane

		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		RenderScenePass(shader, lightShader, view, projection, far_plane);  // ← 改成传 far_plane

		ResolveMSAA();

		RenderBloomPass(filterShader, gaussShader, upsampleShader);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		RenderScreen(blendShader);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &quadVAO);
	glDeleteVertexArrays(1, &planeVAO);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteBuffers(1, &quadVBO);
	glDeleteBuffers(1, &planeVBO);
	glDeleteRenderbuffers(1, &rbo);
	glDeleteFramebuffers(1, &framebuffer);


	// glfw: terminate, clearing all previously allocated GLFW resources
	//------------------------------------------------------------------
	glfwTerminate();
	return 0;
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// render相关函数------------------------------------------------------
// 渲染地面
void RenderFloor(Shader& shader)
{
	shader.setFloat("material.shininess", 32.0f);
	shader.setVec3("light.specular", 0.3f, 0.3f, 0.3f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, floorTexture);
	// 地面暂时的纯蓝色法线贴图============
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, defaultNormalMap);
	// 地面暂时加上黑色纹理（自发光）
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, defaultSpecularMap);
	shader.setInt("material.specularMap", 3);
	shader.setInt("material.normalMap", 2);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	shader.setMat4("model", model);

	renderPlane();
}


void renderPlane()
{
	if (planeVAO == 0)
	{
		float planeVertices[] = {
			// positions              // normals          // texcoords    // tangents
			-10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,  1.0f, 0.0f, 0.0f,
			-10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
			 10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,  1.0f, 0.0f, 0.0f,
			-10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,  1.0f, 0.0f, 0.0f,
			 10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,  1.0f, 0.0f, 0.0f,
			 10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f,  1.0f, 0.0f, 0.0f,
		};

		// plane VAO
		glGenVertexArrays(1, &planeVAO);
		glGenBuffers(1, &planeVBO);
		glBindVertexArray(planeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
		glBindVertexArray(0);
	}

	// render
	glBindVertexArray(planeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}


// 渲染所有立方体
void RenderCubes(Shader& shader)
{
	shader.setFloat("material.shininess", 500.0f);
	shader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cubeTexture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, cubeNormalMap);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, cubeSpecularMap);
	

	glm::mat4 model;

	// cube 1
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
	shader.setMat4("model", model);

	renderCube();

	// cube 2
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
	shader.setMat4("model", model);

	renderCube();
}


void renderCube()
{
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// positions          // normals     // texcoords   // tangents
			// 后面 (normal 0,0,-1, tangent -1,0,0)
			-0.5f,-0.5f,-0.5f,  0.0f,0.0f,-1.0f,  0.0f,0.0f,  -1.0f,0.0f,0.0f,
			 0.5f, 0.5f,-0.5f,  0.0f,0.0f,-1.0f,  1.0f,1.0f,  -1.0f,0.0f,0.0f,
			 0.5f,-0.5f,-0.5f,  0.0f,0.0f,-1.0f,  1.0f,0.0f,  -1.0f,0.0f,0.0f,
			 0.5f, 0.5f,-0.5f,  0.0f,0.0f,-1.0f,  1.0f,1.0f,  -1.0f,0.0f,0.0f,
			-0.5f,-0.5f,-0.5f,  0.0f,0.0f,-1.0f,  0.0f,0.0f,  -1.0f,0.0f,0.0f,
			-0.5f, 0.5f,-0.5f,  0.0f,0.0f,-1.0f,  0.0f,1.0f,  -1.0f,0.0f,0.0f,
			// 前面 (normal 0,0,1, tangent 1,0,0)
			-0.5f,-0.5f, 0.5f,  0.0f,0.0f, 1.0f,  0.0f,0.0f,   1.0f,0.0f,0.0f,
			 0.5f,-0.5f, 0.5f,  0.0f,0.0f, 1.0f,  1.0f,0.0f,   1.0f,0.0f,0.0f,
			 0.5f, 0.5f, 0.5f,  0.0f,0.0f, 1.0f,  1.0f,1.0f,   1.0f,0.0f,0.0f,
			 0.5f, 0.5f, 0.5f,  0.0f,0.0f, 1.0f,  1.0f,1.0f,   1.0f,0.0f,0.0f,
			-0.5f, 0.5f, 0.5f,  0.0f,0.0f, 1.0f,  0.0f,1.0f,   1.0f,0.0f,0.0f,
			-0.5f,-0.5f, 0.5f,  0.0f,0.0f, 1.0f,  0.0f,0.0f,   1.0f,0.0f,0.0f,
			// 左面 (normal -1,0,0, tangent 0,0,-1)
			-0.5f, 0.5f, 0.5f, -1.0f,0.0f,0.0f,  1.0f,0.0f,   0.0f,0.0f,-1.0f,
			-0.5f, 0.5f,-0.5f, -1.0f,0.0f,0.0f,  1.0f,1.0f,   0.0f,0.0f,-1.0f,
			-0.5f,-0.5f,-0.5f, -1.0f,0.0f,0.0f,  0.0f,1.0f,   0.0f,0.0f,-1.0f,
			-0.5f,-0.5f,-0.5f, -1.0f,0.0f,0.0f,  0.0f,1.0f,   0.0f,0.0f,-1.0f,
			-0.5f,-0.5f, 0.5f, -1.0f,0.0f,0.0f,  0.0f,0.0f,   0.0f,0.0f,-1.0f,
			-0.5f, 0.5f, 0.5f, -1.0f,0.0f,0.0f,  1.0f,0.0f,   0.0f,0.0f,-1.0f,
			// 右面 (normal 1,0,0, tangent 0,0,1)
			 0.5f, 0.5f, 0.5f,  1.0f,0.0f,0.0f,  1.0f,0.0f,   0.0f,0.0f, 1.0f,
			 0.5f,-0.5f,-0.5f,  1.0f,0.0f,0.0f,  0.0f,1.0f,   0.0f,0.0f, 1.0f,
			 0.5f, 0.5f,-0.5f,  1.0f,0.0f,0.0f,  1.0f,1.0f,   0.0f,0.0f, 1.0f,
			 0.5f,-0.5f,-0.5f,  1.0f,0.0f,0.0f,  0.0f,1.0f,   0.0f,0.0f, 1.0f,
			 0.5f, 0.5f, 0.5f,  1.0f,0.0f,0.0f,  1.0f,0.0f,   0.0f,0.0f, 1.0f,
			 0.5f,-0.5f, 0.5f,  1.0f,0.0f,0.0f,  0.0f,0.0f,   0.0f,0.0f, 1.0f,
			 // 下面 (normal 0,-1,0, tangent 1,0,0)
			 -0.5f,-0.5f,-0.5f,  0.0f,-1.0f,0.0f,  0.0f,1.0f,   1.0f,0.0f,0.0f,
			  0.5f,-0.5f,-0.5f,  0.0f,-1.0f,0.0f,  1.0f,1.0f,   1.0f,0.0f,0.0f,
			  0.5f,-0.5f, 0.5f,  0.0f,-1.0f,0.0f,  1.0f,0.0f,   1.0f,0.0f,0.0f,
			  0.5f,-0.5f, 0.5f,  0.0f,-1.0f,0.0f,  1.0f,0.0f,   1.0f,0.0f,0.0f,
			 -0.5f,-0.5f, 0.5f,  0.0f,-1.0f,0.0f,  0.0f,0.0f,   1.0f,0.0f,0.0f,
			 -0.5f,-0.5f,-0.5f,  0.0f,-1.0f,0.0f,  0.0f,1.0f,   1.0f,0.0f,0.0f,
			 // 上面 (normal 0,1,0, tangent 1,0,0)
			 -0.5f, 0.5f,-0.5f,  0.0f,1.0f,0.0f,  0.0f,1.0f,   1.0f,0.0f,0.0f,
			  0.5f, 0.5f, 0.5f,  0.0f,1.0f,0.0f,  1.0f,0.0f,   1.0f,0.0f,0.0f,
			  0.5f, 0.5f,-0.5f,  0.0f,1.0f,0.0f,  1.0f,1.0f,   1.0f,0.0f,0.0f,
			  0.5f, 0.5f, 0.5f,  0.0f,1.0f,0.0f,  1.0f,0.0f,   1.0f,0.0f,0.0f,
			 -0.5f, 0.5f,-0.5f,  0.0f,1.0f,0.0f,  0.0f,1.0f,   1.0f,0.0f,0.0f,
			 -0.5f, 0.5f, 0.5f,  0.0f,1.0f,0.0f,  0.0f,0.0f,   1.0f,0.0f,0.0f,
		};

		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glBindVertexArray(cubeVAO);

		// location 0: position
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
		// location 1: normal
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
		// location 2: texcoord
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
		// location 3: tangent
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}


void RenderScene(Shader& shader)
{
	RenderFloor(shader);
	RenderCubes(shader);
}


void RenderLight(Shader& lightShader, glm::mat4& view, glm::mat4& projection)
{
	lightShader.use();

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, lightPos);
	model = glm::scale(model, glm::vec3(0.2f));

	lightShader.setMat4("model", model);
	lightShader.setMat4("view", view);
	lightShader.setMat4("projection", projection);

	renderCube();
}



// 后期处理-----------------------------------------------------------
void SetupFramebuffers()
{
	// 帧缓冲---------------------------
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	// 创建纹理附件（MSAA采样）
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);

	glTexImage2DMultisample(
		GL_TEXTURE_2D_MULTISAMPLE,
		4,
		GL_RGB16F,
		800,
		600,
		GL_TRUE
	);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D_MULTISAMPLE,
		texture,
		0
	);

	// 深度FBO
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorageMultisample(
		GL_RENDERBUFFER,
		4,
		GL_DEPTH24_STENCIL8,
		800,
		600
	);

	glFramebufferRenderbuffer(
		GL_FRAMEBUFFER,
		GL_DEPTH_STENCIL_ATTACHMENT,
		GL_RENDERBUFFER,
		rbo
	);

	// ----------------------------------
	// resolve FBO
	glGenFramebuffers(1, &intermediateFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);

	glGenTextures(1, &screenTexture);
	glBindTexture(GL_TEXTURE_2D, screenTexture);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGB16F,
		800,
		600,
		0,
		GL_RGB,
		GL_FLOAT,
		NULL
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D,
		screenTexture,
		0
	);

	// ----------------------------------
	// shadow map
	glGenFramebuffers(1, &depthMapFBO);
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, depthMap);

	for (unsigned int i = 0; i < 6; i++)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
			SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	
	// Bloom FBO
	glGenFramebuffers(BLOOM_LEVEL + 1, blurFBO);

	for (int i = 0; i < BLOOM_LEVEL + 1; i++)
	{
		int scale = 1 << i;
		glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[i]);

		// attachment0：存当前层模糊结果
		unsigned int tex1;
		glGenTextures(1, &tex1);
		glBindTexture(GL_TEXTURE_2D, tex1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH / scale, SCR_HEIGHT / scale, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex1, 0);
		blurColorBuffer1[i] = tex1;

		if (i != 0)
		{
			unsigned int tex2, tex3;

			glGenTextures(1, &tex2);
			glBindTexture(GL_TEXTURE_2D, tex2);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH / scale, SCR_HEIGHT / scale, 0, GL_RGB, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, tex2, 0);
			blurColorBuffer2[i] = tex2;

			glGenTextures(1, &tex3);
			glBindTexture(GL_TEXTURE_2D, tex3);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH / scale, SCR_HEIGHT / scale, 0, GL_RGB, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, tex3, 0);
			blurColorBuffer3[i] = tex3;
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Bloom FBO[" << i << "] not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// 渲染阴影
void RenderShadowPass(Shader& simpleDepthShader, Shader& shader, float far_plane)
{
	glm::mat4 shadowProj = glm::perspective(
		glm::radians(90.0f), 1.0f, 0.1f, far_plane);

	std::vector<glm::mat4> shadowMatrices = {
		shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1,0,0), glm::vec3(0,-1,0)),
		shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1,0,0), glm::vec3(0,-1,0)),
		shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0, 1,0), glm::vec3(0,0, 1)),
		shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0,-1,0), glm::vec3(0,0,-1)),
		shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0,0, 1), glm::vec3(0,-1,0)),
		shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0,0,-1), glm::vec3(0,-1,0)),
	};

	glDisable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_CULL_FACE);

	simpleDepthShader.use();
	for (int i = 0; i < 6; i++)
		simpleDepthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowMatrices[i]);
	simpleDepthShader.setVec3("lightPos", lightPos);
	simpleDepthShader.setFloat("far_plane", far_plane);

	RenderScene(simpleDepthShader);

	glEnable(GL_CULL_FACE);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// 泛光
void RenderBloomPass(Shader& filterShader, Shader& gaussShader, Shader& upsampleShader)
{
	// Step1: 提取亮部到 blurFBO[0] attachment0
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[0]);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT);
	filterShader.use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, screenTexture);  // intermediateFBO 的输出
	filterShader.setInt("colorBuffer", 0);
	renderQuad();

	// Step2: 下采样 + 高斯模糊
	gaussShader.use();
	for (int i = 1; i < BLOOM_LEVEL + 1; i++)
	{
		int scale = 1 << i;
		glViewport(0, 0, SCR_WIDTH / scale, SCR_HEIGHT / scale);
		glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[i]);

		// 下采样
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glClear(GL_COLOR_BUFFER_BIT);
		gaussShader.setInt("isy", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, blurColorBuffer1[i - 1]);
		gaussShader.setInt("colorBuffer", 0);
		renderQuad();

		// 垂直模糊
		glDrawBuffer(GL_COLOR_ATTACHMENT1);
		glClear(GL_COLOR_BUFFER_BIT);
		gaussShader.setInt("isy", 1);
		glBindTexture(GL_TEXTURE_2D, blurColorBuffer1[i]);
		renderQuad();

		// 水平模糊
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glClear(GL_COLOR_BUFFER_BIT);
		gaussShader.setInt("isy", 0);
		glBindTexture(GL_TEXTURE_2D, blurColorBuffer2[i]);
		renderQuad();
	}

	// Step3: 上采样叠加
	// 最高层先做一次模糊
	int topScale = 1 << BLOOM_LEVEL;
	glViewport(0, 0, SCR_WIDTH / topScale, SCR_HEIGHT / topScale);
	glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[BLOOM_LEVEL]);
	gaussShader.use();

	glDrawBuffer(GL_COLOR_ATTACHMENT1);
	gaussShader.setInt("isy", 1);
	glBindTexture(GL_TEXTURE_2D, blurColorBuffer1[BLOOM_LEVEL]);
	renderQuad();

	glDrawBuffer(GL_COLOR_ATTACHMENT2);
	gaussShader.setInt("isy", 0);
	glBindTexture(GL_TEXTURE_2D, blurColorBuffer2[BLOOM_LEVEL]);
	renderQuad();

	// 逐层上采样
	for (int i = BLOOM_LEVEL - 1; i > 0; i--)
	{
		int scale = 1 << i;
		glViewport(0, 0, SCR_WIDTH / scale, SCR_HEIGHT / scale);
		glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[i]);
		gaussShader.use();

		glDrawBuffer(GL_COLOR_ATTACHMENT1);
		gaussShader.setInt("isy", 1);
		glBindTexture(GL_TEXTURE_2D, blurColorBuffer1[i]);
		renderQuad();

		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		gaussShader.setInt("isy", 0);
		glBindTexture(GL_TEXTURE_2D, blurColorBuffer2[i]);
		renderQuad();

		// 上采样叠加
		upsampleShader.use();
		upsampleShader.setInt("currentLevel", i);
		upsampleShader.setFloat("baseWeight", 0.3f);
		upsampleShader.setFloat("growthRate", 0.05f);
		glDrawBuffer(GL_COLOR_ATTACHMENT2);
		glClear(GL_COLOR_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, blurColorBuffer1[i]);
		upsampleShader.setInt("downTex", 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, blurColorBuffer3[i + 1]);
		upsampleShader.setInt("preTex", 1);
		renderQuad();
	}
}

// 渲染真实场景
void RenderScenePass(Shader& shader, Shader& lightShader, glm::mat4 view, glm::mat4 projection, float far_plane)
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader.use();
	shader.setMat4("view", view);
	shader.setMat4("projection", projection);
	shader.setVec3("viewPos", camera.Position);
	shader.setVec3("light.position", lightPos);
	shader.setVec3("light.ambient", 0.05f, 0.05f, 0.05f);
	shader.setVec3("light.diffuse", 2.0f, 2.0f, 1.0f);
	shader.setFloat("light.constant", 1.0f);
	shader.setFloat("light.linear", 0.09f);
	shader.setFloat("light.quadratic", 0.032f);
	shader.setFloat("far_plane", far_plane);  // ← 传 far_plane

	// 紫色光源，放在 crying_obsidian 方块位置
	glm::vec3 purpleLightPos = glm::vec3(-1.0f, 0.0f, -1.0f);  // cube1 的位置
	shader.setVec3("light2.position", purpleLightPos);
	shader.setVec3("light2.ambient", 0.05f, 0.0f, 0.08f);   // 淡紫色环境光
	shader.setVec3("light2.diffuse", 0.8f, 0.0f, 1.5f);     // 紫色漫反射
	shader.setVec3("light2.specular", 0.05f, 0.0f, 0.08f);    // 紫色高光
	shader.setFloat("light2.constant", 1.0f);
	shader.setFloat("light2.linear", 0.5f);       // 衰减快一点，只影响周围
	shader.setFloat("light2.quadratic", 0.8f);


	shader.setInt("material.albedo", 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, depthMap);  // ← 改成 CUBE_MAP
	shader.setInt("shadowMap", 1);

	RenderScene(shader);
	RenderLight(lightShader, view, projection);
}

// MSAA->普通纹理
void ResolveMSAA()
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);

	glBlitFramebuffer(
		0, 0, 800, 600,
		0, 0, 800, 600,
		GL_COLOR_BUFFER_BIT,
		GL_NEAREST
	);
}

// screenShader
void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}


// 将帧缓冲纹理贴上
void RenderScreen(Shader& blendShader)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	blendShader.use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, screenTexture);
	blendShader.setInt("sceneColorBuffer", 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, blurColorBuffer3[1]);
	blendShader.setInt("bloomColorBuffer", 1);
	blendShader.setFloat("exposure", exposure);
	blendShader.setFloat("bloomStrength", bloomStrength);

	renderQuad();
}


// 处理输入
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	// 键盘输入处理
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);

	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !blinnKeyPressed)
	{
		blinn = !blinn;
		blinnKeyPressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
	{
		blinnKeyPressed = false;
	}

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		exposure = std::max(0.1f, exposure - 0.01f);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		exposure += 0.01f;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// 从文件中加载2D纹理的函数
unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		GLenum internalFormat;

		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
		{
			format = GL_RGB;
			internalFormat = GL_SRGB;
		}
		else if (nrComponents == 4)
		{
			format = GL_RGBA;
			internalFormat = GL_SRGB_ALPHA;
		}

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

// 加载天空盒函数
unsigned int loadCubemap(std::vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

