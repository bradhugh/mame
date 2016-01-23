// FXC command: fxc /T ps_4_0_level_9_1 /Vn d3d11_PixelShader /Fh "D:\Archive\Sources\bradhugh.mame\mame\src\osd\modules\render\d3d11\PixelShader.h" "D:\Archive\Sources\bradhugh.mame\mame\src\osd\modules\render\d3d11\PixelShader.hlsl"

Texture2D tex2D : register(t0);
SamplerState DiffuseSampler : register(s0);

struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float rhw : RHW0;
	float4 Color : COLOR0;
	float2 TexCoord : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
	float4 baseTexel = tex2D.Sample(DiffuseSampler, input.TexCoord);
	return baseTexel * input.Color;
}