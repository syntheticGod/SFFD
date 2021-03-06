#version 460 core

in vec3 FragPos;
in vec2 TextCoord;
in vec3 FragNormal;

// 光源属性结构体
struct LightAttr
{
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;	// 衰减常数
	float linear;   // 衰减一次系数
	float quadratic; // 衰减二次系数
};

uniform LightAttr light;
uniform vec3 viewPos;
uniform bool draw_wireframe;
out vec4 color;

void main()
{
    if(TextCoord==vec2(-1.0f,-1.0f))
	    color=vec4(0.0f,0.0f,0.0f , 1.0f);
	else{
	// 环境光成分
	float	ambientStrength = 0.06f;
	vec3	ambient = ambientStrength*light.ambient * vec3(1.0f,1.0f,1.0f);

	// 漫反射光成分 此时需要光线方向为指向光源
	float	diffceStrength = 1.0f;
	vec3	lightDir = normalize(light.position - FragPos);
	vec3	normal = normalize(FragNormal);
	float	diffFactor = max(dot(lightDir, normal), 0.0);
	vec3	diffuse = diffceStrength*diffFactor * light.diffuse * vec3(1.0f,1.0f,1.0f);

	// 镜面反射成分 此时需要光线方向为由光源指出
	float	specularStrength = 0.5f;
	vec3	reflectDir = normalize(reflect(-lightDir, normal));
	vec3	viewDir = normalize(viewPos - FragPos);
	float	specFactor = pow(max(dot(reflectDir, viewDir), 0.0), 10.0f);
	vec3	specular = specularStrength*specFactor * light.specular * vec3(1.0f,1.0f,1.0f);

	// 计算衰减因子
	float distance = length(light.position - FragPos); // 在世界坐标系中计算距离
	float attenuation = 1.0f / (light.constant 
			+ light.linear * distance
			+ light.quadratic * distance * distance);

	vec3	result = (ambient+diffuse+specular) * attenuation;
	color	= vec4(result , 1.0f);

	if(draw_wireframe)
	    color=vec4(0.0f,0.0f,0.0f , 1.0f);
	}
		
}