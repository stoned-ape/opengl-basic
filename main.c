#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>


#ifdef __APPLE__

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>
#include <simd/simd.h>
typedef simd_float2 vec2;
typedef simd_float3 vec3;
typedef simd_float4 vec4;

#else

typedef struct{
    float x,y;
}vec2;
typedef struct{
    float x,y,z;
}vec3;
typedef struct{
    float x,y,z,w;
}vec4;

#endif

void printv1(float v){ printf("%f \n",v);}
void printv2(vec2  v){ printf("[ %f , %f ]\n",v.x,v.y);}
void printv3(vec3  v){ printf("[ %f , %f , %f ]\n",v.x,v.y,v.z);}
void printv4(vec4  v){ printf("[ %f , %f , %f , %f ]\n",v.x,v.y,v.z,v.w);}

int ret;
#define SYSCALL(call) \
ret=call; \
if(ret==-1){ \
    printf("syscall error: (%s) in function %s at line %d of file %s\n", \
        strerror(errno),__func__,__LINE__,__FILE__); \
    printf("->  SYSCALL(%s); \n",#call); \
    exit(errno); \
}


typedef struct{
    int program_id;
    int itime_id;
    float itime;
    int ires_id;
    vec2 ires;
    int imouse_id;
    vec2 imouse;
    int param_id;
    int param;
    bool leftmousedown;
    bool rightmousedown;
    bool keydown[255];
}uniforms;



char *filemap(char*,int*);
int compile_shader(char*,bool);
int makeprogram(char*,char*);
float itime();
void key_callback(GLFWwindow*,int,int,int,int);
void mouse_callback(GLFWwindow*,int,int,int);
void uniforms_init(uniforms*,int,int);
void uniforms_set(uniforms*);
void print_fps();
GLFWwindow *setup_window(int,int);
void setup_vertecies();

GLFWwindow *window;
uniforms uni;

char *filemap(char *fname,int *n){
    int fd=SYSCALL(open(fname,O_RDWR));
    int len=SYSCALL(lseek(fd,0,SEEK_END));
    char *v=mmap(NULL,len+1,PROT_READ,MAP_SHARED,fd,0);
    assert(v!=NULL);
    SYSCALL(close(fd));
    if(n) *n=len;
    return v;
}


int compile_shader(char *fname,bool vert){
    int len;
    char *code=filemap(fname,&len);
    int shader=glCreateShader(vert?GL_VERTEX_SHADER:GL_FRAGMENT_SHADER);
    assert(shader);
    glShaderSource(shader,1,(const GLchar *const*)&code,NULL);
    SYSCALL(munmap(code,len+1));
    glCompileShader(shader);
    int res;
    glGetShaderiv(shader,GL_COMPILE_STATUS,&res);
    if(res!=1){
        printf("compiler error in %s \n",fname);
        char infoLog[1024];
        glGetShaderInfoLog(shader,1024,NULL,infoLog);
        puts(infoLog);
        exit(3);
    }
    return shader;
}

int makeprogram(char *vert,char *frag){
    int vs_id=compile_shader(vert,true);
    int fs_id=compile_shader(frag,false);
    int program_id=glCreateProgram();
    glAttachShader(program_id,vs_id);
    glAttachShader(program_id,fs_id);
    glLinkProgram(program_id);
    int res;
    glGetProgramiv(program_id,GL_LINK_STATUS,&res);
    if(res!=1){
        printf("linker error \n");
        char infolog[1024];
        glGetProgramInfoLog(program_id,1024,NULL,infolog);
        puts(infolog);
        exit(4);
    }
    return program_id;
}

float itime(){
    struct timeval tp;
    SYSCALL(gettimeofday(&tp,NULL));
    return (tp.tv_sec%(60*60*24))+tp.tv_usec/1E6;
}


//key: character that correstponds to the key that was pressed
//mods: assigns a number to modifier keys like shift, control, and option
//action: GLFW_PRESS=1, GLFW_RELEASE=0 or GLFW_REPEAT=2
void key_callback(GLFWwindow* window,int key,int scancode,int action,int mods){
    (void)window;
    (void)scancode;
    (void)mods;
    switch(action){
        case GLFW_PRESS:   uni.keydown[key]=true ;break;
        case GLFW_REPEAT:  uni.keydown[key]=true ;break;
        case GLFW_RELEASE: uni.keydown[key]=false;break;
    }
}

void mouse_callback(GLFWwindow* window,int button,int action,int mods){
    (void)window;
    (void)mods;
    bool *mouse=button?&uni.rightmousedown:&uni.leftmousedown;
    switch(action){
        case GLFW_PRESS:   *mouse=true ;break;
        case GLFW_RELEASE: *mouse=false;break;
    }
}



void uniforms_init(uniforms *this,int program_id,int param){
    for(int i=0;i<255;i++) this->keydown[i]=false;
    glfwSetKeyCallback(window,key_callback);
    glfwSetMouseButtonCallback(window,mouse_callback);
    this->program_id=program_id;
    this->param=param;
    this->itime_id =glGetUniformLocation(program_id,"itime");
    this->ires_id  =glGetUniformLocation(program_id,"ires");
    this->imouse_id=glGetUniformLocation(program_id,"imouse");
    this->param_id =glGetUniformLocation(program_id,"param");
}

void uniforms_set(uniforms *this){
    glUniform1i(this->param_id,this->param);
    
    this->itime=itime();
    glUniform1f(this->itime_id,this->itime);
    
    int width,height;
    glfwGetFramebufferSize(window,&width,&height);
    this->ires.x=width;
    this->ires.y=height;
    glUniform2f(this->ires_id,width,height);
    
    double xpos,ypos;
    glfwGetCursorPos(window,&xpos,&ypos);
    this->imouse.x=xpos;
    this->imouse.y=ypos;
    glUniform2f(this->imouse_id,xpos,ypos);
}

void print_fps(){
    static float timer;
    float delta=itime()-timer;
    timer+=delta;
    printf("\rfps = %f ",1/delta);
    fflush(stdout);
}


GLFWwindow *setup_window(int w,int h){
    if(!glfwInit()) exit(1);
    glfwWindowHint(GLFW_SAMPLES,4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *win=glfwCreateWindow(w,h,".____.",NULL,NULL);
    if(!win) exit(2);
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);
    return win;
}

void setup_vertecies(){
    GLuint vtxarray_id;
    glGenVertexArrays(1, &vtxarray_id);
    glBindVertexArray(vtxarray_id);
    static vec4 vtx_data[]={
        {-1,-1,0,0},
        { 1,-1,0,0},
        {-1, 1,0,0},
        { 1, 1,0,0}
    };
    GLuint vtxbuff_id;
    glGenBuffers(1,&vtxbuff_id);
    glBindBuffer(GL_ARRAY_BUFFER,vtxbuff_id);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vtx_data),vtx_data,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER,vtxbuff_id);
    glVertexAttribPointer(0,4,GL_FLOAT,false,0,NULL);
}






int main(int argc,char **argv){
    
    int param=1;
    if(argc>1){
        if(!strcmp(argv[1],"-m")) param=1;
        if(!strcmp(argv[1],"-p")) param=2;
        if(!strcmp(argv[1],"-l")) param=3;
        if(!strcmp(argv[1],"-j")) param=4;
    }
    
    
    window=setup_window(320,240);
    
    int program_id=makeprogram("vert.glsl","frag.glsl");
    
    setup_vertecies();
    
    
    uniforms_init(&uni,program_id,param);
    glUseProgram(program_id);
    while(!glfwWindowShouldClose(window)){
        glClear(GL_COLOR_BUFFER_BIT);
        
        uniforms_set(&uni);
        glDrawArrays(GL_TRIANGLES,0,3);
        glDrawArrays(GL_TRIANGLES,1,3);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
        print_fps();
    }
}


















