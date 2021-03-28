#version 460 core

in vec3 FragPos;
in vec2 TextCoord;
in vec3 FragNormal;

// ��Դ���Խṹ��
struct LightAttr
{
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;	// ˥������
	float linear;   // ˥��һ��ϵ��
	float quadratic; // ˥������ϵ��
};

uniform LightAttr light;
uniform vec3 viewPos;
uniform sampler2D texture_diffuse0;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_diffuse2;
uniform sampler2D texture_specular0;
uniform sampler2D texture_specular1;
uniform sampler2D texture_specular2;
uniform bool draw_wireframe;
out vec4 color;

void main()
{
    if(TextCoord==vec2(-1.0f,-1.0f))
	    color=vec4(0.0f,0.0f,0.0f , 1.0f);
	else{

	// ������ɷ�
	float	ambientStrength = 0.06f;
	vec3	ambient = ambientStrength*light.ambient * vec3(texture(texture_diffuse0, TextCoord));

	// �������ɷ� ��ʱ��Ҫ���߷���Ϊָ���Դ
	float	diffceStrength = 1.0f;
	vec3	lightDir = normalize(light.position - FragPos);
	vec3	normal = normalize(FragNormal);
	float	diffFactor = max(dot(lightDir, normal), 0.0);
	vec3	diffuse = diffceStrength*diffFactor * light.diffuse * vec3(texture(texture_diffuse0, TextCoord));

	// ���淴��ɷ� ��ʱ��Ҫ���߷���Ϊ�ɹ�Դָ��
	float	specularStrength = 0.5f;
	vec3	reflectDir = normalize(reflect(-lightDir, normal));
	vec3	viewDir = normalize(viewPos - FragPos);
	float	specFactor = pow(max(dot(reflectDir, viewDir), 0.0), 10.0f);
	vec3	specular = specularStrength*specFactor * light.specular * vec3(texture(texture_specular0, TextCoord));

	// ����˥������
	float distance = length(light.position - FragPos); // ����������ϵ�м������
	float attenuation = 1.0f / (light.constant 
			+ light.linear * distance
			+ light.quadratic * distance * distance);

	vec3	result = (ambient+ diffuse+specular ) * attenuation;
	color	= vec4(result , 1.0f);

	if(draw_wireframe)
	    color=vec4(0.0f,0.0f,0.0f , 1.0f);
	}
		
}