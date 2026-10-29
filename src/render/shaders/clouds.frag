#version 450
layout(location=0) out vec4 outColor;
layout(set=2,binding=0) uniform sampler2D scene;
layout(set=2,binding=1) uniform sampler2D depthTex;
layout(std140,set=3,binding=0) uniform CloudUniforms {
 mat4 inverseViewProjection; vec4 camera; vec4 layer; vec4 sun; vec4 target;
};
vec3 world(vec2 uv,float depth){vec4 p=inverseViewProjection*vec4(uv.x*2.-1.,1.-uv.y*2.,depth,1.);return p.xyz/p.w;}
float hash(vec3 p){return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453);}
float noise(vec3 p){vec3 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);
 return mix(mix(mix(hash(i),hash(i+vec3(1,0,0)),f.x),mix(hash(i+vec3(0,1,0)),hash(i+vec3(1,1,0)),f.x),f.y),
 mix(mix(hash(i+vec3(0,0,1)),hash(i+vec3(1,0,1)),f.x),mix(hash(i+vec3(0,1,1)),hash(i+vec3(1,1,1)),f.x),f.y),f.z);}
float density(vec3 p){
 float h=(p.y-layer.x)/(layer.y-layer.x);if(h<=0.||h>=1.)return 0.;
 vec3 q=(p-vec3(camera.w*2.,0.,camera.w*.65))*.008;
 // Exact early-out: the two finer octaves add at most .36, so skip them (and the
 // shape terms) when the base octave cannot reach the coverage threshold.
 float cut=1.-layer.z;
 float n=noise(q)*.64;
 if(n+.36<=cut)return 0.;
 n+=noise(q*2.07)*.26;
 if(n+.10<=cut)return 0.;
 n+=noise(q*4.13)*.10;
 float shape=smoothstep(0.,.18,h)*(1.-smoothstep(.65,1.,h));
 return max(0.,n-cut)*shape*layer.w;
}
void main(){
 vec2 uv=gl_FragCoord.xy*target.xy;vec4 base=texture(scene,uv);
 vec3 origin=camera.xyz,dir=normalize(world(uv,.9999)-origin);
 float rawDepth=texture(depthTex,uv).r;
 float limit=rawDepth>.9995?12000.:length(world(uv,rawDepth)-origin);
 float enter=0.,exit=limit;
 if(abs(dir.y)<.0001){if(origin.y<layer.x||origin.y>layer.y){outColor=vec4(base.rgb,1);return;}}
 else {float a=(layer.x-origin.y)/dir.y,b=(layer.y-origin.y)/dir.y;enter=max(0.,min(a,b));exit=min(exit,max(a,b));}
 if(exit<=enter){outColor=vec4(base.rgb,1);return;}
 float stepSize=(exit-enter)/48.;float trans=1.;vec3 light=vec3(0);vec3 sunDir=length(sun.xyz)>.0001?normalize(sun.xyz):vec3(0,1,0);
 for(int i=0;i<48;i++){
  vec3 p=origin+dir*(enter+(float(i)+.5)*stepSize);float d=density(p);if(d<=0.)continue;
  float optical=0.;for(int j=0;j<4;j++)optical+=density(p+sunDir*(float(j)+.5)*52.5)*52.5;
  float illumination=.3+.7*exp(-optical);float a=1.-exp(-d*stepSize);
  light+=trans*a*mix(vec3(.48,.55,.65),vec3(1.,.98,.92),illumination);trans*=1.-a;if(trans<.01)break;
 }
 // Alpha is explicit view-ray transmission, consumed separately by god rays.
 outColor=vec4(light+trans*base.rgb,trans);
}
