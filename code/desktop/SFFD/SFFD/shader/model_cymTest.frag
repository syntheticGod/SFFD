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
uniform sampler2D texture_diffuse0;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_diffuse2;
uniform sampler2D texture_specular0;
uniform sampler2D texture_specular1;
uniform sampler2D texture_specular2;


vec3 tnorm; 

out vec4 FragColor;

struct PointLightSource
{
	vec4 position, ambient, diffuse, specular;
	float constantAttenuation, linearAttenuation, quadraticAttenuation;
};


struct Material
{
	vec4 ka, kd, ks;
};
Material Mtl;//采用默认值

void PointLight(const vec3 eye, const vec3 ecPosition3, const vec3 normal,
				inout vec4 ambient, inout vec4 diffuse, inout vec4 specular)
{
    PointLightSource PointLightSource0=PointLightSource(vec4(light.position,1.0f),vec4(light.ambient,1.0f),
	vec4(light.diffuse,1.0f),vec4(light.specular,1.0f),1.0f,0.0f,0.0f);

	vec3 VP = vec3(PointLightSource0.position) - ecPosition3;	// 片元到光源的向量
	float d = length(VP);										// 片元和光源的距离
	VP = normalize(VP);
	float attenuation = 1.0 / (PointLightSource0.constantAttenuation +
						 PointLightSource0.linearAttenuation * d +
						 PointLightSource0.quadraticAttenuation * d * d);// 衰减因子
// 方法1:
	float nDotHV = max(0.0, dot(reflect(-VP, normal), eye));
// 方法2:
	//vec3 halfVector = normalize(VP + eye);		// direction of maximum highlights
	//float nDotHV = max(0.0, dot(normal, halfVector));

	float nDotVP = max(0.0, dot(normal, VP));	// normal.light
	float pf = 0.0;								// power factor
	if (nDotVP > 0.0)
		pf = pow(nDotHV, 10.0);

	ambient += PointLightSource0.ambient * attenuation;
	diffuse += PointLightSource0.diffuse * nDotVP * attenuation;
	specular += PointLightSource0.specular * pf * attenuation;
}

void main()
{
    tnorm=FragNormal;

    vec4 ka=vec4(0.2f,0.2f,0.2f,1.0f);
	vec4 kd=vec4(1.0f,1.0f,1.0f,1.0f);
	vec4 ks=vec4(0.8f,0.8f,0.8f,1.0f);
	Mtl=Material(ka,kd,ks);
	
	vec4 amb = vec4(0.0), diff = vec4(0.0), spec = vec4(0.0);
	PointLight(viewPos, FragPos, normalize(tnorm), amb, diff, spec);
	FragColor = amb * Mtl.ka + diff * Mtl.kd;
	vec4 SecondaryColor = spec * Mtl.ks;
	FragColor = clamp(FragColor, 0.0, 1.0);


	FragColor *= texture(texture_diffuse0, TextCoord);
	
	FragColor += SecondaryColor;
	FragColor = clamp(FragColor, 0.0, 1.0);

	
}

