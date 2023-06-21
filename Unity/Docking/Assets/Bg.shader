Shader "Unlit/Bg"
{
    Properties
    {
        _MainTex ("Texture", 2D) = "white" {}
        _Resolution ("Resolution", Vector) = (1, 1, 1, 1)
        _Color1 ("Color1", Color) = (1, 1, 1, 1)
        _Color2 ("Color2", Color) = (1, 1, 1, 1)
        _Color3 ("Color3", Color) = (1, 1, 1, 1)
        _Index ("Index", float) = 1
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 100

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            // make fog work
            #pragma multi_compile_fog

            #include "Packages/jp.keijiro.noiseshader/Shader/SimplexNoise3D.hlsl"
            #include "UnityCG.cginc"

            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv : TEXCOORD0;
            };

            struct v2f
            {
                float2 uv : TEXCOORD0;
                UNITY_FOG_COORDS(1)
                float4 vertex : SV_POSITION;
            };

            sampler2D _MainTex;
            float4 _MainTex_ST;
            float2 _Resolution;
            float3 _Color1;
            float3 _Color2;
            float3 _Color3;
            float _Index;
            float _UtcTime;

            v2f vert (appdata v)
            {
                v2f o;
                o.vertex = UnityObjectToClipPos(v.vertex);
                o.uv = TRANSFORM_TEX(v.uv, _MainTex);
                UNITY_TRANSFER_FOG(o,o.vertex);
                return o;
            }

            fixed4 frag (v2f i) : SV_Target
            {
                float SCALE = .3;
                float2 uv = i.uv;
                float t = _UtcTime * .1;
                uv *= float2(_Resolution.x / _Resolution.x, _Resolution.y / _Resolution.x);
                uv.x += _Index;

                fixed4 color = fixed4(1, 1, 1, 1);

                fixed2 q = fixed2(0, 0);
                for(int i = 0; i < 5; i++) {
                    q.x += SimplexNoise(fixed3(uv * SCALE, t)) * .4;
                    q.y += SimplexNoise(fixed3(uv * SCALE + fixed2(10, 10), t)) * .4;
                }
                q = (q + fixed2(1, 1)) * .5;

                fixed2 r = fixed2(0, 0);

                for(int i = 0; i < 5; i++) {
                    r.x += SimplexNoise(fixed3(uv * SCALE * q, t)) * .4;
                    r.y += SimplexNoise(fixed3(uv * SCALE * q + fixed2(10, 10), t)) * .4;
                }

                float v1 = SimplexNoise(fixed3(r, 0));
                v1 = (v1 + 1) * .5;

                int index;
                fixed3 _Colors[] = {_Color1, _Color2, _Color3};

                v1 *= 2;
                index = int(v1);
                color.rgb *= lerp(_Colors[index], _Colors[index + 1], frac(v1));

                return color;
            }
            ENDCG
        }
    }
}
