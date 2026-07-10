// Render target texture provided by Maya's pipeline
Texture2D gInputTex : register(t0);

// In FX files, define SamplerStates using an initialization block
SamplerState gLinearSampler 
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
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

// Pixel Shader processing grayscale conversion
float4 PS_Grayscale(VS_Output input) : SV_Target {
    // Correctly sample using the FX sampler state block
    float4 color = gInputTex.Sample(gLinearSampler, input.UV);
    
    // Standard luminosity coefficients (NTSC weights)
    float gray = dot(color.rgb, float3(0.299, 0.587, 0.114));
    
    return float4(gray, gray, gray, color.a);
    // return float4(1.0, 0.0, 0.0, 1.0);
}

technique11 GrayscaleTech {
    pass p0 {
        SetVertexShader(CompileShader(vs_5_0, VS_Main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_Grayscale()));
    }
}