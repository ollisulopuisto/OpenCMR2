#version 450
layout(location=0) out vec4 outColor;
layout(set=2,binding=0) uniform sampler2D scene;
layout(set=2,binding=1) uniform sampler2D depthTex;
layout(set=2,binding=2) uniform sampler2D cloudTransmission;
layout(std140,set=3,binding=0) uniform SunGlareUniforms {
 vec4 sun; // top-origin normalized x,y, strength0..1, aspect
};
void main(){
 vec2 size=vec2(textureSize(scene,0)),uv=gl_FragCoord.xy/size;
 vec4 s=texture(scene,uv);
 // Depth at and around the projected actual stage sun gates the whole effect.
 // Edge pixels outside the frame are never treated as unoccluded light.
 float visibility=0.;
 for(int i=0;i<5;i++){
  vec2 q=sun.xy+vec2(i==1?2.:i==2?-2.:0.,i==3?2.:i==4?-2.:0.)/size;
  if(all(greaterThanEqual(q,vec2(0)))&&all(lessThanEqual(q,vec2(1))))
   visibility+=smoothstep(.995,.9999,texture(depthTex,q).r)/5.;
 }
 vec2 delta=(uv-sun.xy)*vec2(sun.w,1.);
 float r=length(delta),halo=exp(-r*r/ .008);
 // Tiny local bright-pixel spread, not global bloom or a white screen.
 vec3 bloom=vec3(0);
 for(int i=0;i<4;i++){
  vec2 off=vec2(i==0?3.:i==1?-3.:0.,i==2?3.:i==3?-3.:0.)/size;
  vec3 c=texture(scene,clamp(uv+off,vec2(0),vec2(1))).rgb;
  bloom+=max(c-vec3(.75),vec3(0))*.25;
 }
 float strength=clamp(sun.z,0.,1.)*visibility*texture(cloudTransmission,clamp(sun.xy,vec2(0),vec2(1))).a;
 // Slow eye response slightly lowers gain (max4%), retaining dark contrast.
 // Local halo max6%, warm veil max0.6%. Cap total brightening at8/255.
 float gain=1.-.04*strength;
 vec3 lit=s.rgb*gain+(vec3(1.,.93,.78)*(.006+.06*halo)+bloom*.06*halo)*strength;
 vec3 result=min(lit,s.rgb+vec3(8./255.));
 outColor=vec4(clamp(result,vec3(0),vec3(1)),s.a);
}
