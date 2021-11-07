#version 330 core

layout(location=0) in vec3 vtx;
out vec2 iuv;


void main(){
    iuv=vtx.xy;
    gl_Position=vec4(vtx,1.);
}

