//=============================================================================
// MultiPassBloom.fx
// 1:1 Strict Translation from OGSFX to Direct3D 11 HLSL
//=============================================================================

// --- Uniform Parameters Mapping ---
cbuffer cbPostProcess : register(b0)
{
    float gBloomIntensity = 1.5f;
    float gGlowRadius     = 1.0f;
    float gThreshold      = 0.22f;
    float gSoftKnee       = 0.6f;
};

// --- Textures & Samplers Mapping ---
Texture2D gInputTex;
Texture2D gSourceMipTex; 

SamplerState gLinearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

SamplerState gSourceMipSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

// --- Input / Output Structures ---
struct VS_Input
{
    float3 inPosition : POSITION;
    float2 inUV       : TEXCOORD0;
};

struct VS_To_PS
{
    float4 gl_Position : SV_Position;
    float2 outUV        : TEXCOORD0;
};

// --- SHARED VERTEX SHADER ---
VS_To_PS VS_Main(VS_Input input)
{
    VS_To_PS output;
    output.gl_Position = float4(input.inPosition, 1.0f);
    output.outUV = input.inUV;
    return output;
}

// --- PASS 1: EXPONENTIAL SMOOTH-KNEE HIGHLIGHT ISOLATION ---
float3 ExtractBrightHQ(float3 color, float threshold, float softKnee)
{
    float luma = dot(color, float3(0.2126f, 0.7152f, 0.0722f));
    float knee = threshold * softKnee;
    float soft = luma - threshold + knee;
    soft = clamp(soft, 0.0f, 2.0f * knee);
    soft = (soft * soft) / (4.0f * knee + 0.00001f);
    float contribution = max(soft, luma - threshold);
    contribution /= max(luma, 0.00001f);
    return color * contribution;
}

float4 PS_ThresholdDownsample(VS_To_PS input) : SV_Target
{
    uint width, height;
    gInputTex.GetDimensions(width, height);
    float2 texelSize = float2(1.0f / (float)width, 1.0f / (float)height);
    
    float2 d = texelSize * 1.0f;
    float3 s1 = gInputTex.Sample(gLinearSampler, input.outUV + float2(-d.x, -d.y)).rgb;
    float3 s2 = gInputTex.Sample(gLinearSampler, input.outUV + float2( d.x, -d.y)).rgb;
    float3 s3 = gInputTex.Sample(gLinearSampler, input.outUV + float2(-d.x,  d.y)).rgb;
    float3 s4 = gInputTex.Sample(gLinearSampler, input.outUV + float2( d.x,  d.y)).rgb;
    float3 color = (s1 + s2 + s3 + s4) * 0.25f;
    
    float3 isolatedBright = ExtractBrightHQ(color, gThreshold, gSoftKnee);
    return float4(isolatedBright, 1.0f);
}

// --- PASS 2: CRISP DOWNSAMPLE ---
float4 PS_StandardDownsample(VS_To_PS input) : SV_Target
{
    uint width, height;
    gInputTex.GetDimensions(width, height);
    float2 texelSize = float2(1.0f / (float)width, 1.0f / (float)height);
    
    float2 d = texelSize * 1.0f;
    
    float3 target = gInputTex.Sample(gLinearSampler, input.outUV).rgb * 0.5f;
    target += gInputTex.Sample(gLinearSampler, input.outUV + float2(-d.x, -d.y)).rgb * 0.125f;
    target += gInputTex.Sample(gLinearSampler, input.outUV + float2( d.x, -d.y)).rgb * 0.125f;
    target += gInputTex.Sample(gLinearSampler, input.outUV + float2(-d.x,  d.y)).rgb * 0.125f;
    target += gInputTex.Sample(gLinearSampler, input.outUV + float2( d.x,  d.y)).rgb * 0.125f;
    
    return float4(target, 1.0f);
}

// --- PASS 3A: DEEP LAYER UPSAMPLE (MAXIMUM BLENDER BLEED RANGE) ---
float4 PS_UpsampleWide(VS_To_PS input) : SV_Target
{
    uint width, height;
    gInputTex.GetDimensions(width, height);
    float2 texelSize = float2(1.0f / (float)width, 1.0f / (float)height);
    
    float radiusScale = 1.35f;
    float2 dAxial = texelSize * radiusScale;
    float2 dDiag = texelSize * (radiusScale * 0.707106f);
    
    float3 upsampledGlow = gInputTex.Sample(gLinearSampler, input.outUV).rgb * 4.0f;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2(-dDiag.x, -dDiag.y)).rgb * 2.0f;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2( dDiag.x, -dDiag.y)).rgb * 2.0f;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2(-dDiag.x,  dDiag.y)).rgb * 2.0f;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2( dDiag.x,  dDiag.y)).rgb * 2.0f;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2(0.0f, -dAxial.y)).rgb;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2(0.0f,  dAxial.y)).rgb;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2(-dAxial.x, 0.0f)).rgb;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2( dAxial.x, 0.0f)).rgb;
    upsampledGlow /= 16.0f;
    
    float3 higherResLayer = gSourceMipTex.Sample(gSourceMipSampler, input.outUV).rgb;
    
    float weight = clamp((gGlowRadius / 30.0f) * 0.95f, 0.4f, 0.98f);
    return float4(lerp(higherResLayer, upsampledGlow, weight), 1.0f);
}

// --- PASS 3B: MID LAYER UPSAMPLE ---
float4 PS_UpsampleMedium(VS_To_PS input) : SV_Target
{
    uint width, height;
    gInputTex.GetDimensions(width, height);
    float2 texelSize = float2(1.0f / (float)width, 1.0f / (float)height);
    
    float radiusScale = 1.15f;
    float2 dAxial = texelSize * radiusScale;
    float2 dDiag = texelSize * (radiusScale * 0.707106f);
    
    float3 upsampledGlow = gInputTex.Sample(gLinearSampler, input.outUV).rgb * 4.0f;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2(-dDiag.x, -dDiag.y)).rgb * 2.0f;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2( dDiag.x, -dDiag.y)).rgb * 2.0f;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2(-dDiag.x,  dDiag.y)).rgb * 2.0f;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2( dDiag.x,  dDiag.y)).rgb * 2.0f;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2(0.0f, -dAxial.y)).rgb;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2(0.0f,  dAxial.y)).rgb;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2(-dAxial.x, 0.0f)).rgb;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2( dAxial.x, 0.0f)).rgb;
    upsampledGlow /= 16.0f;
    
    float3 higherResLayer = gSourceMipTex.Sample(gSourceMipSampler, input.outUV).rgb;
    
    float weight = clamp((gGlowRadius / 30.0f) * 0.85f, 0.3f, 0.88f);
    return float4(lerp(higherResLayer, upsampledGlow, weight), 1.0f);
}

// --- PASS 3C: HIGH LAYER UPSAMPLE ---
float4 PS_UpsampleTight(VS_To_PS input) : SV_Target
{
    uint width, height;
    gInputTex.GetDimensions(width, height);
    float2 texelSize = float2(1.0f / (float)width, 1.0f / (float)height);
    
    float radiusScale = 1.0f;
    float2 dAxial = texelSize * radiusScale;
    float2 dDiag = texelSize * (radiusScale * 0.707106f);
    
    float3 upsampledGlow = gInputTex.Sample(gLinearSampler, input.outUV).rgb * 4.0f;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2(-dDiag.x, -dDiag.y)).rgb * 2.0f;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2( dDiag.x, -dDiag.y)).rgb * 2.0f;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2(-dDiag.x,  dDiag.y)).rgb * 2.0f;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2( dDiag.x,  dDiag.y)).rgb * 2.0f;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2(0.0f, -dAxial.y)).rgb;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2(0.0f,  dAxial.y)).rgb;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2(-dAxial.x, 0.0f)).rgb;
    upsampledGlow += gInputTex.Sample(gLinearSampler, input.outUV + float2( dAxial.x, 0.0f)).rgb;
    upsampledGlow /= 16.0f;
    
    float3 higherResLayer = gSourceMipTex.Sample(gSourceMipSampler, input.outUV).rgb;
    
    float weight = clamp((gGlowRadius / 30.0f) * 0.7f, 0.15f, 0.75f);
    return float4(lerp(higherResLayer, upsampledGlow, weight), 1.0f);
}

// --- PASS 4: SCENE COMPOSITE OVERLAY ---
float4 PS_FinalComposite(VS_To_PS input) : SV_Target
{
    float4 baseScene = gSourceMipTex.Sample(gSourceMipSampler, input.outUV);
    float3 fullGlowPyramid = gInputTex.Sample(gLinearSampler, input.outUV).rgb;
    float3 cleanGlow = max(fullGlowPyramid, float3(0.0f, 0.0f, 0.0f));
    
    float radiusVolumeBoost = 1.0f + (gGlowRadius * 0.04f);
    float3 finalColor = baseScene.rgb + (cleanGlow * gBloomIntensity * radiusVolumeBoost);
    
    return float4(finalColor, baseScene.a);
}

// --- TECHNIQUES ---
technique11 ThresholdDownsample
{
    pass p0 {
        SetVertexShader(CompileShader(vs_5_0, VS_Main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_ThresholdDownsample()));
    }
}

technique11 StandardDownsample
{
    pass p0 {
        SetVertexShader(CompileShader(vs_5_0, VS_Main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_StandardDownsample()));
    }
}

technique11 UpsampleWide
{
    pass p0 {
        SetVertexShader(CompileShader(vs_5_0, VS_Main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_UpsampleWide()));
    }
}

technique11 UpsampleMedium
{
    pass p0 {
        SetVertexShader(CompileShader(vs_5_0, VS_Main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_UpsampleMedium()));
    }
}

technique11 UpsampleTight
{
    pass p0 {
        SetVertexShader(CompileShader(vs_5_0, VS_Main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_UpsampleTight()));
    }
}

technique11 FinalComposite
{
    pass p0 {
        SetVertexShader(CompileShader(vs_5_0, VS_Main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_FinalComposite()));
    }
}