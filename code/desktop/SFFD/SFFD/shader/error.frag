#version 460 core

in float error_out;

uniform float beginError;
uniform float middleError;
uniform float endError;
uniform vec3 beginColor;
uniform vec3 middleColor;
uniform vec3 endColor;

out vec4 color;

void main()
{
   vec3 resultColor=vec3(0,0,0);
   if(error_out<=beginError)
        resultColor=beginColor;
   else if(error_out>=endError)
		resultColor=endColor;
   else{
      if(error_out>middleError){
	    float f=(error_out-middleError)/(endError-middleError);
		resultColor=f*endColor+(1.0f-f)*middleColor;
	  }
	  else{
	    float f=(error_out-beginError)/(middleError-beginError);
		resultColor=f*middleColor+(1.0f-f)*beginColor;
	  }
	}
		
	color=vec4(resultColor,1.0f);
}