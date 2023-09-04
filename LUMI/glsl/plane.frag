#version 330 core

#define NUM_SAMPLES 50
#define NUM_RINGS 10
#define SHADOW_MAP_SIZE 2048.0
#define FILTER_RADIUS 15.0
#define FRUSTUM_SIZE 400.0
#define NEAR_PLANE 0.1
#define LIGHT_WORLD_SIZE 5.0
#define LIGHT_SIZE_UV (LIGHT_WORLD_SIZE / FRUSTUM_SIZE)
#define EPS 0.001
#define PI 3.141592653589793
#define PI2 6.283185307179586

out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 viewPos;


vec3 evalDiffuseColor(vec2 texcoords)
{
    float scale = 2;
    float pattern = float((mod(texcoords.x * scale, 1) > 0.5f) ^^ (mod(texcoords.y * scale, 1) > 0.5f));
    return vec3(0.525f, 0.525f, 0.525f) * pattern + vec3(0.423f, 0.423f, 0.423) * (1.0f - pattern);
}

highp float rand_1to1(highp float x) { 
	// -1 -1
	return fract(sin(x)*10000.0);
}

highp float rand_2to1(vec2 uv) { 
  	// 0 - 1
	const highp float a = 12.9898, b = 78.233, c = 43758.5453;
	highp float dt = dot(uv.xy, vec2(a,b)), sn = mod(dt, PI);
	return fract(sin(sn) * c);
}

vec2 poissonDisk[NUM_SAMPLES];

void poissonDiskSamples(const in vec2 randomSeed) {

	float ANGLE_STEP = PI2 * float(NUM_RINGS) / float(NUM_SAMPLES);
	float INV_NUM_SAMPLES = 1.0 / float( NUM_SAMPLES );

	float angle = rand_2to1(randomSeed) * PI2;
	float radius = INV_NUM_SAMPLES;
	float radiusStep = radius;

	for(int i = 0; i < NUM_SAMPLES; i++) {
		poissonDisk[i] = vec2(cos(angle), sin(angle)) * pow(radius, 0.75);
		radius += radiusStep;
		angle += ANGLE_STEP;
	}
}

void uniformDiskSamples(const in vec2 randomSeed) {

	float randNum = rand_2to1(randomSeed);
	float sampleX = rand_1to1(randNum) ;
	float sampleY = rand_1to1(sampleX) ;

	float angle = sampleX * PI2;
	float radius = sqrt(sampleY);

	for( int i = 0; i < NUM_SAMPLES; i ++ ) {
		poissonDisk[i] = vec2(radius * cos(angle) , radius * sin(angle));

		sampleX = rand_1to1(sampleY) ;
		sampleY = rand_1to1(sampleX) ;

		angle = sampleX * PI2;
		radius = sqrt(sampleY);
	}
}

float findBlocker(vec2 uv, float zReceiver) {
	int blockerNum = 0;
	float blockerDepth = 0.0;
	float posZFromLight = fs_in.FragPosLightSpace.z;
	float searchRadius = LIGHT_SIZE_UV * (posZFromLight - NEAR_PLANE) / posZFromLight;
	poissonDiskSamples(uv);
	for (int i = 0; i <	NUM_SAMPLES; ++i) {
		float shadowDepth = texture2D(shadowMap, uv + poissonDisk[i] * searchRadius).r;
		if (zReceiver > shadowDepth) {
			++blockerNum;
			blockerDepth += shadowDepth;
		}
	}
	return (blockerNum != 0) ? blockerDepth / float(blockerNum) : -1.0;
}

float getBias(float c, float filterRadiusUV)
{
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
	float fragSize = (1.0 + ceil(filterRadiusUV)) * (FRUSTUM_SIZE / SHADOW_MAP_SIZE / 2.0);
	// return max(fragSize * (1.0 - dot(normal, lightDir)) * c, 0.001);
    return max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
}

float ShadowCalculation(vec3 shadowCoord, float biasC, float filterRadiusUV)
{
    // 检查是否超出远平面
    if(shadowCoord.z > 1.0) return 1.0;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, shadowCoord.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = shadowCoord.z;
    // 消除阴影Ance
    float bias = getBias(biasC, filterRadiusUV);
    // check whether current frag pos is in shadow
    return (currentDepth - bias > closestDepth) ? 0.0 : 1.0;
}


float PCF(vec3 shadowCoord, float biasC, float filterRadiusUV)
{
    float shadow = 0.0;
    poissonDiskSamples(shadowCoord.xy);
	for (int i = 0; i < NUM_SAMPLES; ++i) {
		vec2 offset = poissonDisk[i] * filterRadiusUV;
		vec3 coord = shadowCoord + vec3(offset, 0.0);
		shadow += ShadowCalculation(coord, biasC, filterRadiusUV);
	}
	return shadow / float(NUM_SAMPLES);
}

float PCSS(vec3 shadowCoord, float biasC)
{
	float zReceiver = shadowCoord.z;
	// STEP 1: avgblocker depth
	float avgBlockerDepth = findBlocker(shadowCoord.xy, zReceiver);
	
	if (avgBlockerDepth < -EPS) {
		return 1.0;
	}
	
	// STEP 2: penumbra size
	float penumbra = LIGHT_SIZE_UV * (zReceiver - avgBlockerDepth) / avgBlockerDepth;
	float filterRadiusUV = penumbra;

	// STEP 3: filtering
	return PCF(shadowCoord, biasC, filterRadiusUV);
}


void main()
{           
    vec3 color = evalDiffuseColor(fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightColor = vec3(0.5);
    // ambient
    vec3 ambient = 0.3 * lightColor;
    // diffuse
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;    

    // calculate shadow
    vec3 shadowCoord = fs_in.FragPosLightSpace.xyz / fs_in.FragPosLightSpace.w;
    shadowCoord = shadowCoord * 0.5 + 0.5;

    // ShadowMap
    // float shadow = ShadowCalculation(shadowCoord, 0.2, 0.0);              
    
    // PCF
	// float shadow = PCF(shadowCoord, 0.2, FILTER_RADIUS / SHADOW_MAP_SIZE);

	// PCSS
	float shadow = PCSS(shadowCoord, 0.2);

    vec3 lighting = (ambient + shadow * (diffuse + specular)) * color;    
    FragColor = vec4(lighting, 1.0);
}