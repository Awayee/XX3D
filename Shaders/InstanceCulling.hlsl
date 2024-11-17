#define NUM_THREAD_PER_GROUP 256
#define NUM_INSTANCE_PER_CLUSTER 16

#define TEST_RESULT_OUTER 0
#define TEST_RESULT_INTERSECTED 1
#define TEST_RESULT_INNER 2

struct CameraFrustum {
    float4 uNear;
    float4 uFar;
    float4 uLeft;
    float4 uRight;
    float4 uBottom;
    float4 uTop;
};

struct CameraInfo {
    CameraFrustum Frustum;
    float4x4 ViewProj;
    float4 HZBSizeScale;
};

struct AABBData {
    float4 AABBMin; // xyz: aabbMin, w: instance_start
    float4 AABBMax; // xyz: aabbMax, w: instance_end
};

struct ClusterData {
    uint ClusterIndex;
    uint SrcInstanceOffset;
    uint DstInstanceOffset;
    uint PrimitiveStart;
    uint PrimitiveCount;
};

struct IndirectDrawBuffer{
    uint IndexCount;
    uint InstanceCount;
    uint FirstIndex;
    uint VertexOffset;
    uint FirstInstance;
};

[[vk::binding(0, 0)]] ConstantBuffer<CameraInfo> uParams;
[[vk::binding(1, 0)]] StructuredBuffer<AABBData> uInstanceAABBs;
[[vk::binding(2, 0)]] StructuredBuffer<AABBData> uClusterAABBs;
[[vk::binding(3, 0)]] StructuredBuffer<ClusterData> uClusterData;
[[vk::binding(4, 0)]] RWStructuredBuffer<IndirectDrawBuffer> uIndirectDrawCmds;
//[[vk::binding(5, 0)]] RWByteAddressBuffer uInstanceIDs;
[[vk::binding(5, 0)]] RWStructuredBuffer<uint> uInstanceIDs;
#if defined(OCCLUSION_TEST)
[[vk::binding(6, 0)]] Texture2D<float> uHZB;
[[vk::binding(7, 0)]] SamplerState uSampler;
#endif

inline bool IsAABBOnOrForwardPlane(float4 plane, float3 aabbCenter, float3 aabbExtent)
{
    float3 planeNormal = plane.xyz;
    float planeDistance = plane.w;

    float signedDistance = dot(planeNormal, aabbCenter) - planeDistance;
    float3 planeNormalAbs = abs(planeNormal);
    float r = dot(aabbExtent, planeNormalAbs);
    return signedDistance >= -r;
}

inline uint PlaneTestAABB(float4 plane, float3 aabbCenter, float3 aabbExtent)
{
    float3 planeNormal = plane.xyz;
    float planeDistance = plane.w;

    float signedDistance = dot(planeNormal, aabbCenter) - planeDistance;
    float3 planeNormalAbs = abs(planeNormal);
    float r = dot(aabbExtent, planeNormalAbs);
    if (signedDistance < r && signedDistance > -r)
    {
        return TEST_RESULT_INTERSECTED;
    }
    if (signedDistance >= r)
    {
        return TEST_RESULT_INNER;
    }
    return TEST_RESULT_OUTER;
}

uint FrustumTestAABB(CameraFrustum frustum, float3 aabbMin, float3 aabbMax) {
    const float3 aabbCenter = 0.5f * (aabbMax + aabbMin);
    const float3 aabbExtent = 0.5f * (aabbMax - aabbMin);
    uint testResult = TEST_RESULT_INNER;
#define PLANE_AABB_TEST(plane) {\
    uint planeTestResult = PlaneTestAABB(plane, aabbCenter, aabbExtent);\
    if(TEST_RESULT_OUTER == planeTestResult){\
		return TEST_RESULT_OUTER;\
    }\
    testResult = min(testResult, planeTestResult);\
}
    PLANE_AABB_TEST(frustum.uNear);
    PLANE_AABB_TEST(frustum.uFar);
    PLANE_AABB_TEST(frustum.uLeft);
    PLANE_AABB_TEST(frustum.uRight);
    PLANE_AABB_TEST(frustum.uBottom);
    PLANE_AABB_TEST(frustum.uTop);

#undef PLANE_AABB_TEST
    return testResult;
}

#if defined(OCCLUSION_TEST)

#include "Common.hlsli"

// return ndc position, xyz in [0, 1]
float4 TransformToNDC(float4 pos)
{
    float4 ndcPos = mul(uParams.ViewProj, pos);
    ndcPos.xyz /= ndcPos.w;
    ndcPos.xy = ndcPos.xy * 0.5f + 0.5f;
    return ndcPos;
}

bool OcclusionTestAABB(float3 aabbMin, float3 aabbMax) {
    // occlusion test
    // Transform 8 corners to ndc space.
    float4 corners[8];
    corners[0] = float4(aabbMin, 1);
    corners[1] = float4(aabbMax, 1);
    corners[2] = float4(aabbMax.x, aabbMax.y, aabbMin.z, 1);
    corners[3] = float4(aabbMax.x, aabbMin.y, aabbMax.z, 1);
    corners[6] = float4(aabbMax.x, aabbMin.y, aabbMin.z, 1);
    corners[4] = float4(aabbMin.x, aabbMax.y, aabbMax.z, 1);
    corners[5] = float4(aabbMin.x, aabbMax.y, aabbMin.z, 1);
    corners[7] = float4(aabbMin.x, aabbMin.y, aabbMax.z, 1);

    float3 cornerNdcMin = float3(1.0f, 1.0f, 1.0f);
    float3 cornerNdcMax = float3(-1.0f, -1.0f, -1.0f);

    [unroll]
    for (uint i = 0; i < 8; ++i) {
        const float4 cornerNdc = TransformToNDC(corners[i]);
        cornerNdcMin = min(cornerNdcMin, cornerNdc.xyz);
        cornerNdcMax = max(cornerNdcMax, cornerNdc.xyz);
    }

    // y-down in texture
    cornerNdcMin.y = 1.0f - cornerNdcMin.y;
    cornerNdcMax.y = 1.0f - cornerNdcMax.y;
    // scale to actual size
    cornerNdcMin.xy *= uParams.HZBSizeScale.xy;
    cornerNdcMax.xy *= uParams.HZBSizeScale.xy;

    float2 rectSize = cornerNdcMax - cornerNdcMin;
    uint2 hzbSize;
    uint hzbMipSize;
    uHZB.GetDimensions(0, hzbSize.x, hzbSize.y, hzbMipSize);
    hzbSize.xy *= uParams.HZBSizeScale.xy;

    // too big to cull
    if (max(rectSize.x, rectSize.y) > 1.0f){
	    return true;
    }
    
    // select mip level
    float2 numRectPixelsXY = hzbSize.xy * rectSize.xy;
    float numRectPixels = max(numRectPixelsXY.x, numRectPixelsXY.y);
    float mipLevel = ceil(log2(numRectPixels));
    mipLevel = min(mipLevel, float(hzbMipSize));

    // depth of center are compared with hzb
    float closerDepth = CloserDepth(cornerNdcMin.z, cornerNdcMax.z);
    float2 texCoord = 0.5f * (cornerNdcMin.xy + cornerNdcMax.xy);
    float hzbDepth = uHZB.SampleLevel(uSampler, texCoord, mipLevel);
    return IsDepthCloserOrEqual(closerDepth, hzbDepth);
}
#endif

// HZB test, aabbMin and aabbMax are in 0
uint VisibleTestAABB(float3 aabbMin, float3 aabbMax){
    // frustum test
    uint testResult = FrustumTestAABB(uParams.Frustum, aabbMin, aabbMax);
#if defined(OCCLUSION_TEST)
    if(TEST_RESULT_OUTER != testResult) {
    	if(!OcclusionTestAABB(aabbMin, aabbMax)) {
            testResult = TEST_RESULT_OUTER;
        }
    }
#endif
    return testResult;
}

[numthreads(NUM_THREAD_PER_GROUP, 1, 1)]
void MainCS(uint3 tid : SV_DispatchThreadID){
    uint tIndex = tid.x;
    ClusterData cluster = uClusterData[tIndex];

    AABBData clusterRange = uClusterAABBs[cluster.ClusterIndex];
    uint instanceStart = (uint)clusterRange.AABBMin.w;
    uint instanceEnd = (uint) clusterRange.AABBMax.w;

    // test for cluster
    {
        //const uint testResult = FrustumTestAABB(uParams.Frustum, cluster.AABBMin, cluster.AABBMax);
        const uint testResult = VisibleTestAABB(clusterRange.AABBMin.xyz, clusterRange.AABBMax.xyz);
        // out of frustum
        if(TEST_RESULT_OUTER == testResult) {
            return;
        }

        // cluster in frustum, write all instances in the cluster to result
        if(TEST_RESULT_INNER == testResult) {
            const uint instanceCount = instanceEnd - instanceStart;
            uint instanceWriteIndex = 0;

        	[loop]
            for (uint i = 0; i < cluster.PrimitiveCount; ++i) {
                const uint primitiveIndex = cluster.PrimitiveStart + i;
                InterlockedAdd(uIndirectDrawCmds[primitiveIndex].InstanceCount, instanceCount, instanceWriteIndex);
            }

            [loop]
            for (uint i = 0; i < instanceCount; ++i) {
                uInstanceIDs[cluster.DstInstanceOffset + instanceWriteIndex + i] = instanceStart + i;
                //uInstanceIDs.Store(cache.InstanceStart + instanceWriteIndex + i, cluster.InstanceStart + i);
            }
            return;
        }
    }

	// intersected, test each instance
	[unroll]
    for (uint i = 0; i < NUM_INSTANCE_PER_CLUSTER; ++i){
        const uint instanceIndex = instanceStart + i;
        if (instanceIndex > instanceEnd){
            return;
        }
        const uint globalInstanceIndex = instanceIndex + cluster.SrcInstanceOffset;
        const AABBData instance = uInstanceAABBs[globalInstanceIndex];
        const uint testResult = VisibleTestAABB(instance.AABBMin.xyz, instance.AABBMax.xyz);

        if (TEST_RESULT_OUTER != testResult) {
            uint instanceWriteIndex = 0;
			[loop]
            for (uint j = 0; j < cluster.PrimitiveCount; ++j){
                const uint primitiveIndex = cluster.PrimitiveStart + j;
                InterlockedAdd(uIndirectDrawCmds[primitiveIndex].InstanceCount, 1, instanceWriteIndex);
            }
            uInstanceIDs[cluster.DstInstanceOffset + instanceWriteIndex] = instanceIndex;
            //uInstanceIDs.Store(cache.InstanceStart + instanceWriteIndex, instanceIndex);
        }
    }
}
