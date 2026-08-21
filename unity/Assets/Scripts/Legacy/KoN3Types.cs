using System;
using UnityEngine;

namespace MMORPG.Legacy
{
    [Serializable]
    public sealed class KoMaterialData
    {
        public Color diffuse;
        public Color ambient;
        public Color specular;
        public Color emissive;
        public float power;
        public uint colorOp;
        public uint colorArg1;
        public uint colorArg2;
        public uint renderFlags;
        public uint sourceBlend;
        public uint destinationBlend;
    }

    [Serializable]
    public sealed class KoAnimKeyData
    {
        public int count;
        public int type;
        public float samplingRate;
        public Vector3[] vectors = Array.Empty<Vector3>();
        public Quaternion[] quaternions = Array.Empty<Quaternion>();
    }

    [Serializable]
    public sealed class KoTransformData
    {
        public string name = string.Empty;
        public Vector3 position;
        public Quaternion rotation = Quaternion.identity;
        public Vector3 scale = Vector3.one;
        public KoAnimKeyData positionKeys = new KoAnimKeyData();
        public KoAnimKeyData rotationKeys = new KoAnimKeyData();
        public KoAnimKeyData scaleKeys = new KoAnimKeyData();
    }

    [Serializable]
    public sealed class KoPmeshVertex
    {
        public Vector3 position;
        public Vector3 normal;
        public Vector2 uv;
    }

    [Serializable]
    public sealed class KoLodControl
    {
        public float distance;
        public int vertexCount;
    }

    [Serializable]
    public sealed class KoProgressiveMeshData
    {
        public string name = string.Empty;
        public int minVertexCount;
        public int minIndexCount;
        public KoPmeshVertex[] vertices = Array.Empty<KoPmeshVertex>();
        public int[] indices = Array.Empty<int>();
        public KoLodControl[] lod = Array.Empty<KoLodControl>();
    }

    [Serializable]
    public sealed class KoIndexedMeshData
    {
        public string name = string.Empty;
        public int faceCount;
        public int vertexCount;
        public int uvCount;
        public Vector3[] positions = Array.Empty<Vector3>();
        public Vector3[] normals = Array.Empty<Vector3>();
        public int[] indices = Array.Empty<int>();
        public Vector2[] uvs = Array.Empty<Vector2>();
        public int[] uvIndices = Array.Empty<int>();
    }

    [Serializable]
    public sealed class KoVectorMeshData
    {
        public string name = string.Empty;
        public Vector3[] vertices = Array.Empty<Vector3>();
        public int[] indices = Array.Empty<int>();
    }

    [Serializable]
    public sealed class KoSkinInfluence
    {
        public Vector3 origin;
        public int[] joints = Array.Empty<int>();
        public float[] weights = Array.Empty<float>();
    }

    [Serializable]
    public sealed class KoSkinData
    {
        public string name = string.Empty;
        public int faceCount;
        public int vertexCount;
        public int uvCount;
        public Vector3[] positions = Array.Empty<Vector3>();
        public Vector3[] normals = Array.Empty<Vector3>();
        public int[] indices = Array.Empty<int>();
        public Vector2[] uvs = Array.Empty<Vector2>();
        public int[] uvIndices = Array.Empty<int>();
        public KoSkinInfluence[] skinVertices = Array.Empty<KoSkinInfluence>();
    }

    [Serializable]
    public sealed class KoCharacterSkinsData
    {
        public string name = string.Empty;
        public KoSkinData[] lods = Array.Empty<KoSkinData>();
    }

    [Serializable]
    public sealed class KoCharacterPartData
    {
        public string name = string.Empty;
        public int version;
        public KoMaterialData material = new KoMaterialData();
        public string texturePath = string.Empty;
        public string diffuseTexturePath = string.Empty;
        public string skinsPath = string.Empty;
    }

    [Serializable]
    public sealed class KoCharacterPlugData
    {
        public string name = string.Empty;
        public int plugType;
        public int jointIndex;
        public Vector3 position;
        public Matrix4x4 rotationMatrix = Matrix4x4.identity;
        public Vector3 scale = Vector3.one;
        public KoMaterialData material = new KoMaterialData();
        public string meshPath = string.Empty;
        public string texturePath = string.Empty;
        public int traceStep;
        public uint traceColor;
        public float trace0;
        public float trace1;
        public int useVirtualMesh;
    }

    [Serializable]
    public sealed class KoJointNode
    {
        public string name = string.Empty;
        public Vector3 position;
        public Quaternion rotation = Quaternion.identity;
        public Vector3 scale = Vector3.one;
        public KoAnimKeyData positionKeys = new KoAnimKeyData();
        public KoAnimKeyData rotationKeys = new KoAnimKeyData();
        public KoAnimKeyData scaleKeys = new KoAnimKeyData();
        public KoAnimKeyData orientKeys = new KoAnimKeyData();
        public KoJointNode[] children = Array.Empty<KoJointNode>();
    }

    [Serializable]
    public sealed class KoAnimationMeta
    {
        public int reserved;
        public float frameStart;
        public float frameEnd;
        public float framesPerSecond;
        public float plugTraceStart;
        public float plugTraceEnd;
        public float soundFrame0;
        public float soundFrame1;
        public float blendTime;
        public int blendFlags;
        public float strikeFrame0;
        public float strikeFrame1;
        public string name = string.Empty;
    }

    [Serializable]
    public sealed class KoAnimationControlData
    {
        public KoAnimationMeta[] animations = Array.Empty<KoAnimationMeta>();
    }

    [Serializable]
    public sealed class KoCharacterData
    {
        public KoTransformData transform = new KoTransformData();
        public string collisionMeshPath = string.Empty;
        public string climbMeshPath = string.Empty;
        public string jointPath = string.Empty;
        public string[] partPaths = Array.Empty<string>();
        public string[] plugPaths = Array.Empty<string>();
        public string animationPath = string.Empty;
        public int[] jointPartStarts = Array.Empty<int>();
        public int[] jointPartEnds = Array.Empty<int>();
        public string fxPlugPath = string.Empty;
        public string collisionSkinPath = string.Empty;
    }

    [Serializable]
    public sealed class KoShapePartData
    {
        public Vector3 pivot;
        public string meshPath = string.Empty;
        public KoMaterialData material = new KoMaterialData();
        public float textureFps;
        public string[] texturePaths = Array.Empty<string>();
    }

    [Serializable]
    public sealed class KoShapeData
    {
        public KoTransformData transform = new KoTransformData();
        public string collisionMeshPath = string.Empty;
        public string climbMeshPath = string.Empty;
        public KoShapePartData[] parts = Array.Empty<KoShapePartData>();
        public int belongId;
        public int eventId;
        public int eventType;
        public int npcId;
        public int npcStatus;
    }
}
