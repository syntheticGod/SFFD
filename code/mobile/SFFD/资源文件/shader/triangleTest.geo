#version 310 es
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

out vec4 geo_color;


float result_1_upbound=5;

void main() {  
    vec4 point_a=gl_in[0].gl_Position;
	vec4 point_b=gl_in[1].gl_Position;
	vec4 point_c=gl_in[2].gl_Position;
	float length_ab=sqrt((point_a.x-point_b.x)*(point_a.x-point_b.x)+(point_a.y-point_b.y)*(point_a.y-point_b.y)+(point_a.z-point_b.z)*(point_a.z-point_b.z));
	float length_ac=sqrt((point_a.x-point_c.x)*(point_a.x-point_c.x)+(point_a.y-point_c.y)*(point_a.y-point_c.y)+(point_a.z-point_c.z)*(point_a.z-point_c.z));
	float length_bc=sqrt((point_c.x-point_b.x)*(point_c.x-point_b.x)+(point_c.y-point_b.y)*(point_c.y-point_b.y)+(point_c.z-point_b.z)*(point_c.z-point_b.z));
	float p=(length_ab+length_ac+length_bc)/2.0f;
	float result_1=length_ab*length_ac*length_bc/(4.0f*(p-length_ab)*(p-length_ac)*(p-length_bc));
    if(result_1<=result_1_upbound){//在蓝色域上插值
	     float result_normilazed=2.0f/result_1;
		 geo_color=vec4(((1.0f-result_normilazed)*vec3(1.0f,1.0f,1.0f)+result_normilazed*vec3(0.0f,0.0f,1.0f)),1.0f);
		}
	else{//不好的三角形在红绿色区域上区分
	    float Radius=length_ab*length_ac*length_bc/(4.0f*sqrt(p*(p-length_ab)*(p-length_ac)*(p-length_bc)));
		float result_2=2*Radius/(length_ab+length_ac+length_bc);
		//float result_normilazed=0.5f/result_2;
		float result_normilazed=result_2;
		//geo_color=vec4(((1.0f-result_normilazed)*vec3(1.0f,0.0f,0.0f)+result_normilazed*vec3(0.0f,1.0f,0.0f)),1.0f);
		geo_color=vec4(((1.0f-result_normilazed)*vec3(0.0f,1.0f,0.0f)+result_normilazed*vec3(1.0f,0.0f,0.0f)),1.0f);
	}
	
	      
    gl_Position =gl_in[0].gl_Position;   
    EmitVertex();   
    gl_Position =gl_in[1].gl_Position; 
    EmitVertex();
    gl_Position =gl_in[2].gl_Position; 
    EmitVertex();   
    EndPrimitive();   
}