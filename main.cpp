#include<GL/glew.h>
#include"glfw3.h"

#include<iostream>

#include"saveBMP.h"

void check_(int line) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) { 
        std::cout << "Error on line " << line << ": " << error << std::endl;
    }
}
#define check() check_(__LINE__)

static void checkShader(GLuint shader) {
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE) {
        GLint logSize;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
        GLchar* errorLog = new GLchar[logSize+1];
        glGetShaderInfoLog(shader, logSize, NULL, errorLog);
        printf("Compile failed in shader: %s \n", errorLog);
        delete errorLog;
    }
    else std::cout << "It's fine\n";
}
                            
int main() {
    int width = 1920, height = 1080;
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(width, height, "", NULL, NULL);
    if (!window) return (glfwTerminate(), -1);
    glfwMakeContextCurrent(window);
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cout << "GLEW error: %s\n" << glewGetErrorString(err) << '\n';     
        glfwTerminate();
        return -1;
    }

    if(false) {
          GLint n=0; 
  glGetIntegerv(GL_NUM_EXTENSIONS, &n); 
for (GLint i=0; i<n; i++) 
{ 
  const char* extension = 
    (const char*)glGetStringi(GL_EXTENSIONS, i);
  printf("Ext %d: %s\n", i, extension); 
}
    }

    auto const tCount = 1;
    GLuint textures[tCount];
    GLuint const screenshotColor_it{0};
    GLuint const &screenshotColor_t{ textures[screenshotColor_it] };
    GLuint screenshot_fb;

    glGenTextures(tCount, &textures[0]);

{ //screenshot framebuffer
		glActiveTexture(GL_TEXTURE0 + screenshotColor_it);
		glBindTexture(GL_TEXTURE_2D, screenshotColor_t);
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		  
		glDeleteFramebuffers(1, &screenshot_fb);
		glGenFramebuffers(1, &screenshot_fb);
		glBindFramebuffer(GL_FRAMEBUFFER, screenshot_fb);
		  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenshotColor_t, 0);
		  
		  GLenum status;
		  if ((status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
		  	fprintf(stderr, "screenshot framebuffer: error %u", status);
		  }
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
    
    GLuint prog = glCreateProgram();
    
    GLuint shader_vert = glCreateShader(GL_VERTEX_SHADER);
    GLuint shader_frag = glCreateShader(GL_FRAGMENT_SHADER);
    
    char const *source_vert = R"(#version 130 
        #extension GL_ARB_shading_language_420pack : require
        out vec2 uv;

        void main() {
            const vec2 verts[] = {vec2(-1),vec2(1, -1),vec2(-1, 1),vec2(1)};
            gl_Position = vec4( verts[gl_VertexID], 0, 1 );
            uv = verts[gl_VertexID];
        }
    )";
    char const *source_frag = R"(#version 130
        #extension GL_ARB_shader_storage_buffer_object : require
        #extension GL_ARB_shader_bit_encoding : require

        coherent volatile buffer Buffer {
            uvec4 d;
        };
        
        uniform uint param = 0u;
        in vec2 uv;
        out vec4 color;

        const float pi = 3.14159265359;
        const vec2 rotation = vec2(0.75 * pi, 0.2 * pi);
        
        vec3 rightDir_() {
            return vec3(cos(rotation.x), 0, sin(rotation.x));
        }
        vec3 topDir_() {
            return vec3(-sin(rotation.x) * sin(rotation.y), cos(rotation.y), cos(rotation.x) * sin(rotation.y));
        }
        vec3 forwardDir_() {
            return cross(topDir_(), rightDir_());
        }

        void main() {
            vec3 topDir = topDir_();
            vec3 forwardDir = forwardDir_();
            vec3 rightDir = rightDir_();
            float htF = tan(1.57079632679 * 0.5);
            float projX = 1.0 / htF * 512.0 / 512.0, projY = 1.0 / htF;

            vec3 rayDir_ = rightDir * uv.x / projX + topDir * uv.y / projY + forwardDir;
            vec3 rayDir = normalize(rayDir_);


            d[1] = floatBitsToUint(rayDir.y);
            d.w = floatBitsToUint(rayDir.x + rayDir.z * param);
            d[2] = floatBitsToUint(rayDir.z);
            d[0] = floatBitsToUint(rayDir.x);

            for(int i = 0; i < 10; i++) {
                if(d.x >= 25u && param != 5u && param != 0u) {
                    atomicAdd(
                        d.y, 
                        d.w * param * 38u
                    );
                }
                else if(d.w >= 37u && param > 50u) {
                    atomicAdd(
                        d.x, 
                        d.w * param * 3u
                    );
                }
                else if(param > 1u && param < 10u && d.w >= 890u) atomicAdd(d.z, param * 2u);
                else atomicAdd(d.w, 36u * param);
            }
            vec3 res = uintBitsToFloat(uvec3(d.x, d.y, d.z));
            mat3 matrix;
            matrix[0] = normalize(vec3(167, 107, 9) / 255.0); //vec3(255, 230, 0) / 255.0;
            matrix[1] = normalize(vec3(0, 125, 146));// / 255.0; //vec3(21, 188, 235) / 255.0;
            matrix[2] = normalize(vec3(217, 9, 44));// / 255.0;//cross(matrix[0], matrix[1]);
            
            color = vec4(matrix * res, 1.0);
        }
    )";
    
    glShaderSource(shader_vert, 1, &source_vert, nullptr); 
    glCompileShader(shader_vert);
    std::cout << "Shader 1: ";
    checkShader(shader_vert);
    glShaderSource(shader_frag, 1, &source_frag, nullptr); 
    glCompileShader(shader_frag);
    std::cout << "Shader 2: ";
    checkShader(shader_frag);
    
    glAttachShader(prog, shader_vert);
    glAttachShader(prog, shader_frag);
    
    glLinkProgram(prog);
    GLint progStatus;
    glGetProgramiv(prog, GL_LINK_STATUS, &progStatus);
    std::cout << "Program status: " << progStatus << '\n';
    {
        int length;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &length);
        GLchar *msg = new GLchar[length + 1];
        msg[0] = '\0';
        glGetProgramInfoLog(prog, length, &length, msg);
        std::cout << "Program error:\n" << msg;
        delete[] msg;
    }
    
    glValidateProgram(prog);
    GLint progValidate;
    glGetProgramiv(prog, GL_VALIDATE_STATUS, &progValidate);
    std::cout << "Program valid: " << progValidate << '\n';
    //check();
    
    glUseProgram(prog);
    check();

    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 32, nullptr, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
    
    bool takeScreenshot = false;
    int frameIndex = 0;
    while(!takeScreenshot) {
        glBindFramebuffer(GL_FRAMEBUFFER, screenshot_fb);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);
		glfwPollEvents();

        //if(takeScreenshot = glfwWindowShouldClose(window)) {
		if(takeScreenshot = (frameIndex++) == 1) {
			static uint8_t *data{ 0 };
			static int size{ 0 };
			if(size < 3 * width * height) {
				delete[] data;
				data = new uint8_t[3 * width * height];
			}
			
			glFinish();
			glActiveTexture(GL_TEXTURE0 + screenshotColor_it);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
			
			generateBitmapImage(data, height, width, "screenshot.bmp");
			
			glBindFramebuffer(GL_READ_FRAMEBUFFER, screenshot_fb);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBlitFramebuffer(	
				0, 0, width, height,
                0, 0, width, height,
				GL_COLOR_BUFFER_BIT, GL_NEAREST 
			);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
    }
    
    return 0;
}
