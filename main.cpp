// compute shaders tutorial
// Dr Anton Gerdelan <gerdela@scss.tcd.ie>
// Trinity College Dublin, Ireland
// 26 Feb 2016. latest v 2 Mar 2016

#include <string>
#include <fstream>
#include <iostream>
#include <cstring>
#include <chrono>
#include "gl_utils.h"

// Reader for shaders
std::string readFile(const char *filePath) {
    std::string content;
    std::ifstream fileStream(filePath, std::ios::in);

    if(!fileStream.is_open()) {
        std::cerr << "Could not read file " << filePath << ". File does not exist." << std::endl;
        return "";
    }

    std::string line;
    while(!fileStream.eof()) {
        std::getline(fileStream, line);
        content.append(line + "\n");
    }

    fileStream.close();
    return content;
}

// Shader data struct for compute shader
struct shader_data_t
{
    float light[3] = {-5.0, 5.0, 0.0};
    float camera[3] = {-1.0, 0.0, -1.0};
    float fov = 45.0;
    float front[3] = {1.0, 0.0, 1.0};
    float up[3] = {0.0, 1.0, 0.0};
} shader_data;

double clockToMilliseconds(clock_t ticks){
    // units/(units/time) => time (seconds) * 1000 = milliseconds
    return (ticks/(double)CLOCKS_PER_SEC)*1000.0;
}

void processInput()
{

}


auto startTime = std::chrono::high_resolution_clock::now();;
unsigned int frames = 0;

int main()
{
    (start_gl()); // just starts a 4.3 GL context+window

    // set up shaders and geometry for full-screen quad
    // moved code to gl_utils.cpp
    GLuint quad_vao = create_quad_vao();
    GLuint quad_program = create_quad_program();

    // Get compute shader
    std::string s = readFile("raymarch.comp");
    const char *compute_shader = s.c_str();

    GLuint ray_program = 0;
    // create the compute shader
    GLuint ray_shader = glCreateShader( GL_COMPUTE_SHADER );
    {
        glShaderSource( ray_shader, 1, &compute_shader, nullptr );
        glCompileShader( ray_shader );
        ( check_shader_errors( ray_shader ) ); // code moved to gl_utils.cpp
        ray_program = glCreateProgram();
        glAttachShader( ray_program, ray_shader );
        glLinkProgram( ray_program );
        ( check_program_errors( ray_program ) ); // code moved to gl_utils.cpp
    }

    // texture handle and dimensions
    GLuint tex_output = 0;
    int tex_w = WINDOW_W, tex_h = WINDOW_H;
    { // create the texture
        glGenTextures( 1, &tex_output );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, tex_output );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        // linear allows us to scale the window up retaining reasonable quality
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        // same internal format as compute shader input
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT,
                      nullptr );
        // bind to image unit so can write to specific pixels from the shader
        glBindImageTexture( 0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F );
    }

    { // query up the workgroups
        int work_grp_size[3], work_grp_inv;
        // maximum global work group (total work in a dispatch)
        glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_size[0] );
        glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_size[1] );
        glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_size[2] );
        printf( "max global (total) work group size x:%i y:%i z:%i\n", work_grp_size[0],
                work_grp_size[1], work_grp_size[2] );
        // maximum local work group (one shader's slice)
        glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0] );
        glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1] );
        glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2] );
        printf( "max local (in one shader) work group sizes x:%i y:%i z:%i\n",
                work_grp_size[0], work_grp_size[1], work_grp_size[2] );
        // maximum compute shader invocations (x * y * z)
        glGetIntegerv( GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv );
        printf( "max computer shader invocations %i\n", work_grp_inv );
    }

    startTime = std::chrono::high_resolution_clock::now();
    while ( !glfwWindowShouldClose( window ) ) { // drawing loop
        auto beginFrame = std::chrono::high_resolution_clock::now();

        // Input stuff
        int state = glfwGetKey(window, GLFW_KEY_E);
        if (state == GLFW_PRESS)
        {
            std::cout << "E" << std::endl;
        }

        // Rendering
		{
		    // Use ray program
			glUseProgram( ray_program );
            glUniform3fv(glGetUniformLocation(ray_program, "camera"), 1, shader_data.camera);
            glUniform3fv(glGetUniformLocation(ray_program, "light"), 1, shader_data.light);
            glUniform3fv(glGetUniformLocation(ray_program, "up"), 1, shader_data.up);
            glUniform3fv(glGetUniformLocation(ray_program, "front"), 1, shader_data.front);
            glUniform1f(glGetUniformLocation(ray_program, "fov"), shader_data.fov);

            // launch compute shaders
			glDispatchCompute( (GLuint)tex_w, (GLuint)tex_h, 1 );
		}

		// prevent sampling before all writes to image are done
		glMemoryBarrier( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT );

		glClear( GL_COLOR_BUFFER_BIT );
		glUseProgram( quad_program );
		glBindVertexArray( quad_vao );
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, tex_output );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

		glfwPollEvents();
		if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_ESCAPE ) ) {
			glfwSetWindowShouldClose( window, 1 );
		}
		glfwSwapBuffers( window );

		// End time of frame
        auto endFrame = std::chrono::high_resolution_clock::now();;
        frames++;
//        std::cout << endFrame - beginFrame << std::endl;
        if (frames%10 == 0)
        {
            double seconds = std::chrono::duration_cast<std::chrono::seconds>(endFrame - startTime).count();
            double fps = frames / seconds;
            std::cout << "Seconds: " << seconds << std::endl;
            std::cout << "FPS: " << fps << std::endl;
        }
//        deltaTime += endFrame - beginFrame;
//        frames ++;
//        //if you really want FPS
//        if( clockToMilliseconds(deltaTime)>1000.0){ //every second
//            frameRate = (double)frames*0.5 +  frameRate*0.5; //more stable
//            frames = 0;
//            deltaTime -= CLOCKS_PER_SEC;
//            averageFrameTimeMilliseconds  = 1000.0/(frameRate==0?0.001:frameRate);
//
//            std::cout << "FrameTime was:" << averageFrameTimeMilliseconds << std::endl;
//        }
	}

	stop_gl(); // stop glfw, close window
	return 0;
}
