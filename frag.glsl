#version 330 core

in vec2 iuv;
out vec3 color;

uniform float itime;
uniform vec2 ires;
uniform vec2 imouse;
uniform int param;



vec2 cxMult(vec2 a,vec2 b){
    return vec2(a.x*b.x-a.y*b.y,a.x*b.y+a.y*b.x);
}

vec2 cxPow(vec2 a,vec2 b){
    float len=length(a);
    float theta=b.x*atan(a.y/a.x);
    float phi=b.y*log(len);
    return vec2(cos(theta)*cos(phi)-sin(theta)*sin(phi),
                cos(theta)*sin(phi)+sin(theta)*cos(phi))
                *pow(len,b.x)*exp(-b.y*atan(a.y/a.x));
}

vec2 iexp(vec2 a){
    return exp(a.x)*vec2(cos(a.y),sin(a.y));
}

vec3 hsb2rgb(in vec3 c){
    vec3 rgb=
    clamp(
       abs(
           mod(
               c.x*6.0+vec3(0.0,4.0,2.0),6.0
            )
           -3.0
        )
       -1.0,0.0,1.0
    );
    rgb=rgb*rgb*(3.0-2.0*rgb);
    return c.z*mix(vec3(1.0),rgb,c.y);
}

vec3 colorMap(float a){
    return hsb2rgb(vec3(a,1.,1.)*2.*(sin(1.*itime)+1.5));
}
    

vec3 mandelbrot(vec2 c){
    vec2 z=vec2(c);
    for (int i=0;i<300;i++){
        z=cxPow(z,vec2(2.,0.))+c;
    }
    return colorMap(length(z));
}

vec3 julia(vec2 c){
    vec2 z=vec2(c);
    float r=.05;
    for (int i=0;i<100;i++){
        z=cxPow(z,vec2(2.,0.))+.79*vec2(cos(r*itime),sin(r*itime));
    }
    return colorMap(length(z));
}

vec3 zeta(vec2 s){
    vec2 z=vec2(0.,0.);
    for (int i=1;i<100;i++){
        vec2 a=cxPow(vec2(float(i),0.),s);
        z+=vec2(a.x,-a.y)/(pow(a.x,2.)+pow(a.y,2.));
    }
    return colorMap(1000.*length(z));
}

vec3 lambertW(vec2 zeta){
    vec2 z = cxPow(vec2(exp(1.),0.),cxMult(zeta,cxPow(vec2(exp(1.),0.),-zeta)));
    vec2 a=vec2(0.);
    for (int i=0;i<100;i++){
        a=cxPow(z,a);
    }
    return colorMap(length(a));
}

vec3 powertower(vec2 c){
    vec2 z=c;
    for (int i=0;i<100;i++){
        z=cxPow(c,z);
    }
    return colorMap(length(z));
}

vec2 getuv(){
    vec2 uv=iuv;
    uv.x*=ires.x/ires.y;
    return uv;
}

vec2 pix2scr(vec2 v){
    v/=ires;
    v*=2;
    v-=1;
    v.y*=-1;
    v.x*=ires.x/ires.y;
    return v;
}

void main(){
    vec2 uv=getuv();
    
    switch(param){
        case 1:
            color=mandelbrot(uv);
            break;
        case 2:
            color=powertower(uv*2);
            break;
        case 3:
            color=lambertW(uv*2);
            break;
        case 4:
            color=julia(uv*2);
            break;
        default:
            color=julia(uv*2);
            break;
    }
        
    color+=10000*vec3(step(length(uv-pix2scr(imouse)),.05));
    
}


