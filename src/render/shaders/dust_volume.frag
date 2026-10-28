#version 450
// Opt-in experimental world-density body, no native performance claim.
// World-space density, not camera-facing sheets. Fixed32steps / maximum16lobes.
layout(location=0) out vec4 outColor;

layout(set=2,binding=0) uniform sampler2D sceneDepth;
layout(std140,set=3,binding=0) uniform DustVolumeUniforms {
    mat4 inverseViewProjection;
    vec4 cameraAndCount; // xyz world camera; w active lobe count0..16
    vec4 targetAndDensity; // reciprocal target width/height, density, max distance
    vec4 centres[16]; // xyz world centre, radius
    vec4 colours[16]; // linear-ish tint RGB, opacity multiplier
};
vec3 world(vec2 uv,float z) {
    vec4 p=inverseViewProjection*vec4(uv*2.-1.,z,1.);
    return p.xyz/p.w;
}
// Smooth world-space noise. No frame time or camera basis in density.
float hash(vec3 p) { return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453); }
float noise(vec3 p) {
    vec3 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);
    return mix(mix(mix(hash(i),hash(i+vec3(1,0,0)),f.x),mix(hash(i+vec3(0,1,0)),hash(i+vec3(1,1,0)),f.x),f.y),
               mix(mix(hash(i+vec3(0,0,1)),hash(i+vec3(1,0,1)),f.x),mix(hash(i+vec3(0,1,1)),hash(i+vec3(1,1,1)),f.x),f.y),f.z);
}
void main() {
    vec2 uv=gl_FragCoord.xy*targetAndDensity.xy;
    int count=clamp(int(cameraAndCount.w),0,16);
    if(count==0 || targetAndDensity.z<=0.) {outColor=vec4(0,0,0,1);return;}
    vec3 origin=cameraAndCount.xyz;
    vec3 dir=normalize(world(uv,1.)-origin);
    float geometryDistance=length(world(uv,texture(sceneDepth,uv).r)-origin);
    float start=targetAndDensity.w,finish=0.;
    for(int i=0;i<count;++i){
        vec3 q=origin-centres[i].xyz;float b=dot(q,dir);
        float disc=b*b-dot(q,q)+centres[i].w*centres[i].w;
        if(disc>0.) {float root=sqrt(disc);start=min(start,max(0.,-b-root));finish=max(finish,-b+root);}
    }
    finish=min(finish,min(geometryDistance,targetAndDensity.w));
    if(finish<=start){outColor=vec4(0,0,0,1);return;}
    float stepSize=(finish-start)/32.;float trans=1.;vec3 light=vec3(0.);
    for(int step=0;step<32;++step){
        float distance=start+(float(step)+.5)*stepSize;vec3 p=origin+dir*distance;
        float density=0.;vec3 tint=vec3(0.);
        for(int i=0;i<count;++i){
            vec3 q=(p-centres[i].xyz)/max(centres[i].w,.001);
            float fall=max(0.,1.-dot(q,q));
            float d=fall*fall*colours[i].w*(.4+.6*noise(p*2.1));
            density+=d;tint+=colours[i].rgb*d;
        }
        if(density>0.){
            // Soft fade at actual opaque terrain depth, not a hard card edge.
            float soft=clamp((geometryDistance-distance)/.25,0.,1.);
            float a=1.-exp(-density*targetAndDensity.z*stepSize*soft);
            light+=trans*a*tint/density;trans*=1.-a;
            if(trans<.015)break;
        }
    }
    outColor=vec4(light,trans);
}
