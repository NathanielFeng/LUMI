#define M_PI 3.1415926535
#define STB_IMAGE_IMPLEMENTATION
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image/stb_image.h>

#include <iostream>
#include "shader.h"
#include "camera.h"
#include "model.h"
#include "skybox.h"
#include "prefab.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void renderQuad();

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;
unsigned int planeVAO;

int main()
{
	// ��ʼ��������GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// ʹ��GLFW�������ڶ���
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LUMI", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window!" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	// ��ʼ��GLAD��GLAD���ڹ���OpenGL�ĺ���ָ��
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD!" << std::endl;
		return -1;
	}

	// ����OpenGLѡ��
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEPTH_TEST);

	// �߿�ģʽ
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	// ������պ�
	/*
	* �� opengl �У����� 2D �������ݵ�ʱ��data ����ʼλ��Ӧ����
	* ͼƬ�����½����أ���������������� 0,0����Ȼ����������ɨ�衣���ǣ�cubemap ȴ�Ǹ����⣬
	* ��˵ cubemap ��������һ������ RenderMan ��Լ���������� cubemap �� face ��ʱ��
	* data ����ʼλ��Ӧ����ͼƬ�����Ͻ����أ�Ȼ����������ɨ�裩��
	*/
	stbi_set_flip_vertically_on_load(false);
	Skybox skybox("images/skybox/");

	// ����ģ��
	// stbi_set_flip_vertically_on_load(true);
	Model sponzaModel("models/sponza/sponza.obj");
	Model cubeModel("models/cube.obj");

	// ��������� Shader ����
	Shader sponzaShader("glsl/sponza.vert", "glsl/sponza.frag");
	Shader cubeShader("glsl/cube.vert", "glsl/cube.frag");
	Shader skyboxShader("glsl/skybox.vert", "glsl/skybox.frag");
	Shader depthShader("glsl/shadow_mapping_depth.vert", "glsl/shadow_mapping_depth.frag");
	Shader debugShader("glsl/debug_quad_depth.vert", "glsl/debug_quad_depth.frag");


	// �������ͼ FBO
	const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);
	// create depth texture
	unsigned int depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ����shader
	sponzaShader.use();
	sponzaShader.setInt("shadowMap", 1);
	debugShader.use();
	debugShader.setInt("depthMap", 0);

	// ��Դλ��
	glm::vec3 lightPos(10.7f, 10.3f, 1.6f);


	// ��Ⱦѭ��
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// ��������
		processInput(window);

		// �����ɫ����Ȼ���
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// ��Ⱦ���ͼ
		glm::mat4 lightProjection, lightView;
		glm::mat4 lightSpaceMatrix;
		float near_plane = 0.01f, far_plane = 9.0f;
		lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);//glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, near_plane, far_plane);
		lightView = glm::lookAt(lightPos, glm::vec3(-10.6f, 0.0f, 0.0f), glm::vec3(0.0, 1.0, 0.0));
		//std::cout << "camera.Position = (" << camera.Position.x << ", "
		//								   << camera.Position.y << ", "
		//								   << camera.Position.z << ")\n";
		//std::cout << "camera.Front = (" << camera.Front.x << ", "
		//								   << camera.Front.y << ", "
		//								   << camera.Front.z << ")\n";
		lightSpaceMatrix = lightProjection * lightView;
		depthShader.use();
		glm::mat4 model = glm::mat4(1.0f);
		depthShader.setMat4("model", model);
		depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		sponzaModel.draw(depthShader);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		// ��Ⱦ����
		sponzaShader.use();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
		sponzaShader.setMat4("projection", projection);
		sponzaShader.setMat4("view", view);
		sponzaShader.setMat4("model", model);
		sponzaShader.setVec3("viewPos", camera.Position);
		sponzaShader.setVec3("lightPos", lightPos);
		sponzaShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

		sponzaModel.draw(sponzaShader);

		//cubeShader.use();
		//model = glm::mat4(1.0f);
		//model = glm::translate(model, lightPos + glm::vec3(0.0, 2.0, 0.0));
		//model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
		//cubeShader.setMat4("projection", projection);
		//cubeShader.setMat4("view", view);
		//cubeShader.setMat4("model", model);
		//
		//cubeModel.draw(cubeShader);


		// debug ���ͼ
		//debugShader.use();
		//debugShader.setFloat("near_plane", near_plane);
		//debugShader.setFloat("far_plane", far_plane);
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, depthMap);
		//renderQuad();

		// ��Ⱦ��պ�
		//skyboxShader.use();
		//view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // �Ƴ�view�����λ�Ʋ���
		//skyboxShader.setMat4("view", view);
		//skyboxShader.setMat4("projection", projection);
		//skybox.draw(skyboxShader);


		glfwSwapBuffers(window);		// ������ɫ����
		glfwPollEvents();		// �����û�д���ʲô�¼�(����������롢����ƶ���)
	}

	glfwTerminate();		// ��ȷ�ͷ�/ɾ��֮ǰ�ķ����������Դ
	return 0;
}


// renderQuad() renders a 1x1 XY quad in NDC
unsigned int quadVAO = 0;
unsigned int quadVBO;
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


/*
*  �������뺯����ѯ��GLFW��ǰ֡�Ƿ��м�����/�ͷŲ�������Ӧ
*/
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::DOWN, deltaTime);
}


/*
*  ֡�����С�ص������������ڴ�С�����ı�ʱGLFW����ô˺���
*/
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}


/*
*  ���ص�������������ƶ�ʱGLFW����ô˺���
*/
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
	float yoffset = lastY - ypos; // ��תy��
	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}


/*
*  ���ֻص��������������ֹ���ʱGLFW���ô˺���
*/
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}


/*
* ����Ԥ����
* 
* 	// ����һϵ�������ģ�;���
	unsigned int amount = 3000;
	glm::mat4* modelMatrices;
	modelMatrices = new glm::mat4[amount];
	srand(static_cast<unsigned int>(glfwGetTime())); // ��ʼ���������
	for (unsigned int i = 0; i < amount; i++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		float radius = 15.0f;

		float scale = static_cast<float>((rand() % 50) / 100.0 + 0.5);
		model = glm::scale(model, glm::vec3(scale));

		float rotAngle = static_cast<float>((rand() % 360));
		model = glm::rotate(model, rotAngle, glm::vec3(0, 1, 0));

		float rand1 = rand() % (101) / (float)(101);
		float rand2 = rand() % (101) / (float)(101);
		float x = radius * sqrt(rand1) * cos(2 * M_PI * rand2);
		float z = radius * sqrt(rand1) * sin(2 * M_PI * rand2);
		model = glm::translate(model, glm::vec3(x, 0, z));

		modelMatrices[i] = model;
	}

	// ����Ԥ����		
	Prefab grassPrefab("models/grass/grass.obj", modelMatrices, amount);

	Shader grassShader("glsl/grass.vert", "glsl/grass.frag");
*/