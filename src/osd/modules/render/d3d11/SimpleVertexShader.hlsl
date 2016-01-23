// // FXC command: fxc /T vs_4_0_level_9_1 /Vn d3d11_SimpleVertexShader /Fh "D:\Archive\Sources\bradhugh.mame\mame\src\osd\modules\render\d3d11\SimpleVertexShader.h" "D:\Archive\Sources\bradhugh.mame\mame\src\osd\modules\render\d3d11\SimpleVertexShader.hlsl"

cbuffer VertexShaderSettingsConstantBuffer
{
	float TargetWidth;
	float3 Pad1;
	float TargetHeight;
	float3 Pad2;
	float PostPass;
	float3 Pad3;
};

//-----------------------------------------------------------------------------
// Vertex Definitions
//-----------------------------------------------------------------------------

struct VS_INPUT
{
	float3 Position : POSITION0;
	float rhw : RHW0;
	//uint Color : COLOR0;
	float4 Color : COLOR0;
	float2 TexCoord : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float rhw : RHW0;
	float4 Color : COLOR0;
	float2 TexCoord : TEXCOORD0;
};

//-----------------------------------------------------------------------------
// Simple Vertex Shader
//-----------------------------------------------------------------------------

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output = (VS_OUTPUT)0;
	
	Output.Position = float4(Input.Position.xyz, 1.0f);
	Output.Position.x /= TargetWidth;
	Output.Position.y /= TargetHeight;
	Output.Position.y = 1.0f - Output.Position.y;
	Output.Position.x -= 0.5f;
	Output.Position.y -= 0.5f;

	Output.Position *= float4(2.0f, 2.0f, 1.0f, 1.0f);

	Output.rhw = Input.rhw;
	Output.Color = Input.Color;
	Output.TexCoord = lerp(Input.TexCoord, Input.Position.xy / float2(TargetWidth, TargetHeight), PostPass);

	return Output;
}
