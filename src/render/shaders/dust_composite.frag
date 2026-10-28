#version 450
layout(location=0) out vec4 outColor;
layout(set=2,binding=0) uniform sampler2D scene;
layout(set=2,binding=1) uniform sampler2D volume;
layout(set=2,binding=2) uniform sampler2D sceneDepth;
layout(std140,set=3,binding=0) uniform DustCompositeUniforms {vec4 size;};
void main(){
 vec2 uv=gl_FragCoord.xy*size.xy;vec4 base=texture(scene,uv);
 float depth=texture(sceneDepth,uv).r;
 vec2 pixel=uv*vec2(textureSize(volume,0))-.5;vec2 corner=floor(pixel);vec2 f=fract(pixel);
 vec4 sum=vec4(0);float weight=0;
 for(int y=0;y<2;++y)for(int x=0;x<2;++x){
  vec2 tap=(corner+vec2(x,y)+.5)/vec2(textureSize(volume,0));
  float difference=abs(texture(sceneDepth,tap).r-depth);
  float w=(x==0?1.-f.x:f.x)*(y==0?1.-f.y:f.y)*exp(-difference*1000.);
  sum+=texture(volume,tap)*w;weight+=w;
 }
 vec4 cloud=weight>.001?sum/weight:vec4(0,0,0,1);
 outColor=vec4(cloud.rgb+cloud.a*base.rgb,base.a);
}
