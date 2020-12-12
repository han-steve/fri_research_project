/*  Copyright © 2018, Roboti LLC

    This file is licensed under the MuJoCo Resource License (the "License").
    You may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        https://www.roboti.us/resourcelicense.txt
*/


#include "mujoco.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <random>
#include <chrono>


// select EGL, OSMESA or GLFW
#if defined(MJ_EGL)
    #include <EGL/egl.h>
#elif defined(MJ_OSMESA)
    #include <GL/osmesa.h>
    OSMesaContext ctx;
    unsigned char buffer[10000000];
#else
    #include "glfw3.h"
#endif


//-------------------------------- global data ------------------------------------------

// MuJoCo model and data
mjModel* m = 0;
mjData* d = 0;

// MuJoCo visualization
mjvScene scn;
mjvCamera cam;
mjvOption opt;
mjrContext con;


//-------------------------------- utility functions ------------------------------------

// load model, init simulation and rendering
void initMuJoCo(const char* filename)
{
    // activate
    mj_activate("mjkey.txt");

    // load and compile
    char error[1000] = "Could not load binary model";
    if( strlen(filename)>4 && !strcmp(filename+strlen(filename)-4, ".mjb") )
        m = mj_loadModel(filename, 0);
    else
        m = mj_loadXML(filename, 0, error, 1000);
    if( !m )
        mju_error_s("Load model error: %s", error);

    // make data, run one computation to initialize all fields
    d = mj_makeData(m);
    mj_forward(m, d);

    // initialize visualization data structures
    mjv_defaultCamera(&cam);
    mjv_defaultOption(&opt);
    mjv_defaultScene(&scn);
    mjr_defaultContext(&con);

    // create scene and context
    mjv_makeScene(m, &scn, 2000);
    mjr_makeContext(m, &con, 200);

    // center and scale view
    cam.lookat[0] = m->stat.center[0] + .2;
    cam.lookat[1] = m->stat.center[1];
    cam.lookat[2] = m->stat.center[2];
    cam.distance = .8 * m->stat.extent;
}


// deallocate everything and deactivate
void closeMuJoCo(void)
{
    mj_deleteData(d);
    mj_deleteModel(m);
    mjr_freeContext(&con);
    mjv_freeScene(&scn);
    mj_deactivate();
}


// create OpenGL context/window
void initOpenGL(void)
{
    //------------------------ EGL
#if defined(MJ_EGL)
    // desired config
    const EGLint configAttribs[] ={
        EGL_RED_SIZE,           8,
        EGL_GREEN_SIZE,         8,
        EGL_BLUE_SIZE,          8,
        EGL_ALPHA_SIZE,         8,
        EGL_DEPTH_SIZE,         24,
        EGL_STENCIL_SIZE,       8,
        EGL_COLOR_BUFFER_TYPE,  EGL_RGB_BUFFER,
        EGL_SURFACE_TYPE,       EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE,    EGL_OPENGL_BIT,
        EGL_NONE
    };

    // get default display
    EGLDisplay eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if( eglDpy==EGL_NO_DISPLAY )
        mju_error_i("Could not get EGL display, error 0x%x\n", eglGetError());

    // initialize
    EGLint major, minor;
    if( eglInitialize(eglDpy, &major, &minor)!=EGL_TRUE )
        mju_error_i("Could not initialize EGL, error 0x%x\n", eglGetError());

    // choose config
    EGLint numConfigs;
    EGLConfig eglCfg;
    if( eglChooseConfig(eglDpy, configAttribs, &eglCfg, 1, &numConfigs)!=EGL_TRUE )
        mju_error_i("Could not choose EGL config, error 0x%x\n", eglGetError());

    // bind OpenGL API
    if( eglBindAPI(EGL_OPENGL_API)!=EGL_TRUE )
        mju_error_i("Could not bind EGL OpenGL API, error 0x%x\n", eglGetError());

    // create context
    EGLContext eglCtx = eglCreateContext(eglDpy, eglCfg, EGL_NO_CONTEXT, NULL);
    if( eglCtx==EGL_NO_CONTEXT )
        mju_error_i("Could not create EGL context, error 0x%x\n", eglGetError());

    // make context current, no surface (let OpenGL handle FBO)
    if( eglMakeCurrent(eglDpy, EGL_NO_SURFACE, EGL_NO_SURFACE, eglCtx)!=EGL_TRUE )
        mju_error_i("Could not make EGL context current, error 0x%x\n", eglGetError());

    //------------------------ OSMESA
#elif defined(MJ_OSMESA)
    // create context
    ctx = OSMesaCreateContextExt(GL_RGBA, 24, 8, 8, 0);
    if( !ctx )
        mju_error("OSMesa context creation failed");

    // make current
    if( !OSMesaMakeCurrent(ctx, buffer, GL_UNSIGNED_BYTE, 800, 800) )
        mju_error("OSMesa make current failed");

    //------------------------ GLFW
#else
    // init GLFW
    if( !glfwInit() )
        mju_error("Could not initialize GLFW");

    // create invisible window, single-buffered
    glfwWindowHint(GLFW_VISIBLE, 0);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(800, 800, "Invisible window", NULL, NULL);
    if( !window )
        mju_error("Could not create GLFW window");

    // make context current
    glfwMakeContextCurrent(window);
#endif
}


// close OpenGL context/window
void closeOpenGL(void)
{
    //------------------------ EGL
#if defined(MJ_EGL)
    // get current display
    EGLDisplay eglDpy = eglGetCurrentDisplay();
    if( eglDpy==EGL_NO_DISPLAY )
        return;

    // get current context
    EGLContext eglCtx = eglGetCurrentContext();

    // release context
    eglMakeCurrent(eglDpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    // destroy context if valid
    if( eglCtx!=EGL_NO_CONTEXT )
        eglDestroyContext(eglDpy, eglCtx);

    // terminate display
    eglTerminate(eglDpy);

    //------------------------ OSMESA
#elif defined(MJ_OSMESA)
    OSMesaDestroyContext(ctx);

    //------------------------ GLFW
#else
    // terminate GLFW (crashes with Linux NVidia drivers)
    #if defined(__APPLE__) || defined(_WIN32)
        glfwTerminate();
    #endif
#endif
}

// function for generating random line
double f(double x, double slope, bool goal) {
    double result = slope * (x - 1) + .5;
    if (goal) {
        // this is a vertical flip
        return -result;
    } else {
        return result;
    }
}


//-------------------------------- main function ----------------------------------------

int main(int argc, const char** argv)
{
    // check command-line arguments
    if( argc!=5 )
    {
        printf(" USAGE:  record modelfile duration fps num_repetition\n");
        return 0;
    }

    // parse numeric arguments
    double duration = 1.2, fps = 30, n = 1;
    sscanf(argv[2], "%lf", &duration);
    sscanf(argv[3], "%lf", &fps);
    sscanf(argv[4], "%lf", &n);

    // initialize OpenGL and MuJoCo
    initOpenGL();
    initMuJoCo(argv[1]);

    // set rendering to offscreen buffer
    mjr_setBuffer(mjFB_OFFSCREEN, &con);
    if( con.currentBuffer!=mjFB_OFFSCREEN )
        printf("Warning: offscreen rendering not supported, using default/window framebuffer\n");

    // get size of active renderbuffer
    mjrRect viewport =  mjr_maxViewport(&con);
    int W = viewport.width;
    int H = viewport.height;

    // generate n files for each goal
    for (int i = 0; i < n; i++) {
        // goal 0 or goal 1
        bool goal = i % 2;

        // allocate rgb and depth buffers
        unsigned char* rgb = (unsigned char*)malloc(3*W*H);
        float* depth = (float*)malloc(sizeof(float)*W*H);
        if( !rgb || !depth )
            mju_error("Could not allocate buffers");

        // create output rgb file
        std::string first = goal ? "1" : "0";
        printf("%d", i);
        std::string name = first + "_" + std::to_string(i/2) + ".out";
        FILE* fp = fopen(name.c_str(), "wb");
        if( !fp )
            mju_error("Could not open rgbfile for writing");

        // manipulate model to randomize things.
        int cue_id = mj_name2id(m, mjOBJ_BODY, "object0");
        int qposadr = -1, qveladr = -1;

        qposadr = m->jnt_qposadr[m->body_jntadr[cue_id]];
        qveladr = m->jnt_dofadr[m->body_jntadr[cue_id]];

        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

        std::default_random_engine generator(seed);
        std::uniform_real_distribution<double> distribution(-.25, .75);
        double slope = distribution(generator);
        printf("slope: %f\n", slope);

        std::uniform_real_distribution<double> distribution2(0, .8);
        double ball_x = distribution2(generator);
        double ball_y = f(ball_x, slope, goal);
        printf("ball_x: %f\n", ball_x);
        printf("ball_y: %f\n", ball_y);

        double cue_ball_x = -.9;
        double cue_ball_y = f(cue_ball_x, slope, goal);
        double velocity_x = 1;
        double velocity_y = velocity_x * (goal ? -slope : slope);
        printf("cue_ball_x: %f\n", cue_ball_x);
        printf("cue_ball_y: %f\n", cue_ball_y);
        printf("velocity_x: %f\n", velocity_x);
        printf("velocity_y: %f\n", velocity_y);

        d->qpos[qposadr] = cue_ball_x;
        d->qpos[qposadr + 1] = cue_ball_y;

        int ball_id = mj_name2id(m, mjOBJ_BODY, "object1");
        int qposadr_ball = m->jnt_qposadr[m->body_jntadr[ball_id]];
        d->qpos[qposadr_ball] = ball_x;
        d->qpos[qposadr_ball + 1] = ball_y;


        // main loop
        double frametime = 0;
        int framecount = 0;
        while( d->time<duration )
        {
            // set the velocity for every time step (whack)
            d->qvel[qveladr] = velocity_x;
            d->qvel[qveladr + 1] = velocity_y;

            // render new frame if it is time (or first frame)
            if( (d->time-frametime)>1/fps)
            {
                // update abstract scene
                mjv_updateScene(m, d, &opt, NULL, &cam, mjCAT_ALL, &scn);

                // render scene in offscreen buffer
                mjr_render(viewport, &scn, &con);

                // read rgb and depth buffers
                mjr_readPixels(rgb, depth, viewport, &con);

                // write rgb image to file
                fwrite(rgb, 3, W*H, fp);

                // print every 10 frames: '.' if ok, 'x' if OpenGL error
                if( ((framecount++)%10)==0 )
                {
                    if( mjr_getError() )
                        printf("x");
                    else
                        printf(".");
                }

                // save simulation time
                frametime = d->time;
            }

            // advance simulation
            mj_step(m, d);
        }
        printf("\n");

        // close file, free buffers
        fclose(fp);
        free(rgb);
        free(depth);
        // reset data
        mj_resetData(m, d);
    }


    // close MuJoCo and OpenGL
    closeMuJoCo();
    closeOpenGL();

    return 1;
}
