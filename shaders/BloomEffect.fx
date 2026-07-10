// Render target texture provided by Maya's pipeline
Texture2D gInputTex : register(t0);

// Global uniform parameters mapped dynamically from C++
float gBloomIntensity = 10.5; 
float gGlowTrail = 6.8; 

// Clamp mapping to prevent texel bleed artifacts at edges
SamplerState gLinearSampler 
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

struct VS_Input {
    float4 Pos : POSITION;
    float2 UV  : TEXCOORD0;
};

struct VS_Output {
    float4 Pos : SV_POSITION;
    float2 UV  : TEXCOORD0;
};

// Simple pass-through Vertex Shader for full-screen quad
VS_Output VS_Main(VS_Input input) {
    VS_Output output;
    output.Pos = input.Pos;
    output.UV = input.UV;
    return output;
}

// Extract bright pixels above a low threshold for standard LDR viewports
float3 ExtractBright(float3 color, float threshold) {
    // Standard luminosity coefficients (NTSC weights)
    float brightness = dot(color, float3(0.2126, 0.7152, 0.0722));
    if (brightness > threshold) {
        // Soft knee curve to smoothly pass lower highlights instead of a hard clip
        return color * pow((brightness - threshold) / (1.0 - threshold), 2.0);
    }
    return float3(0.0, 0.0, 0.0);
}

// Pixel Shader processing a wide-radius multi-tiered blur
float4 PS_Bloom(VS_Output input) : SV_Target {
    float4 baseColor = gInputTex.Sample(gLinearSampler, input.UV);
    
    // Lower threshold ensures the object's body colors contribute to the glow radius
    float threshold = 0.15; 
    
    uint width, height;
    gInputTex.GetDimensions(width, height);
    float2 texelSize = float2(1.0 / width, 1.0 / height);
    
    float3 bloomAccumulator = float3(0.0, 0.0, 0.0);
    float totalWeight = 0.0;
    
    // Define an optimized 13-tap pattern spanning multiple distance concentric rings
    float2 sampleOffsets[13] = {
        float2( 0.0,  0.0),
        // Inner Ring (Close blur core)
        float2(-1.5, -1.5), float2( 1.5, -1.5), float2(-1.5,  1.5), float2( 1.5,  1.5),
        // Mid Ring (Extending the thickness)
        float2( 0.0, -3.5), float2( 0.0,  3.5), float2(-3.5,  0.0), float2( 3.5,  0.0),
        // Outer Ring (Maximizes the wide soft glow tail)
        float2(-6.0, -6.0), float2( 6.0, -6.0), float2(-6.0,  6.0), float2( 6.0,  6.0)
    };
    
    // Corresponding Gaussian-style distribution weights for the 13 taps
    float sampleWeights[13] = {
        0.16,
        0.12, 0.12, 0.12, 0.12,  // Inner ring weights
        0.07, 0.07, 0.07, 0.07,  // Mid ring weights
        0.02, 0.02, 0.02, 0.02   // Outer ring weights
    };
    
    [unroll]
    for (int i = 0; i < 13; i++) {
        // Multiply texel size offsets by the gGlowTrail parameter to expand/tighten the trail width dynamically
        float2 offset = sampleOffsets[i] * texelSize * gGlowTrail;
        float3 texelColor = gInputTex.Sample(gLinearSampler, input.UV + offset).rgb;
        
        float3 brightColor = ExtractBright(texelColor, threshold);
        
        bloomAccumulator += brightColor * sampleWeights[i];
        totalWeight += sampleWeights[i];
    }
    
    if (totalWeight > 0.0) {
        bloomAccumulator /= totalWeight;
    }
    
    // Composite (Additive Blend) using your dynamic intensity parameter
    float3 finalColor = baseColor.rgb + (bloomAccumulator * gBloomIntensity);
    return float4(finalColor, baseColor.a);
}

technique11 BloomTech {
    pass p0 {
        SetVertexShader(CompileShader(vs_5_0, VS_Main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_Bloom()));
    }
}