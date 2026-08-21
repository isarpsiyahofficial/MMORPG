using System;
using System.IO;
using UnityEngine;

namespace MMORPG.Legacy
{
    public static class KoN3AdditionalReader
    {
        private const int MaxReasonableCount = 2_000_000;

        public static KoIndexedMeshData ReadIndexedMesh(byte[] bytes)
        {
            KoBinaryCursor r = new KoBinaryCursor(bytes);
            KoIndexedMeshData mesh = ReadIndexedMesh(r);
            if (r.Remaining != 0)
                throw new InvalidDataException($"N3 indexed mesh has {r.Remaining} unread bytes.");
            return mesh;
        }

        public static KoSkinData ReadSingularSkin(byte[] bytes)
        {
            KoBinaryCursor r = new KoBinaryCursor(bytes);
            KoSkinData skin = ReadSkin(r);
            if (r.Remaining != 0)
                throw new InvalidDataException($"N3 singular skin has {r.Remaining} unread bytes.");
            return skin;
        }

        public static KoVectorMeshData ReadVectorMesh(byte[] bytes)
        {
            KoBinaryCursor r = new KoBinaryCursor(bytes);
            string name = r.ReadString();
            int vertexCount = CheckedCount(r.ReadInt32(), "vector mesh vertex count");
            Vector3[] vertices = new Vector3[vertexCount];
            for (int i = 0; i < vertexCount; i++)
                vertices[i] = r.ReadVector3();

            int indexCount = CheckedCount(r.ReadInt32(), "vector mesh index count");
            int[] indices = new int[indexCount];
            for (int i = 0; i < indexCount; i++)
                indices[i] = r.ReadUInt16();

            if (r.Remaining != 0)
                throw new InvalidDataException($"N3 vector mesh has {r.Remaining} unread bytes.");

            return new KoVectorMeshData
            {
                name = name,
                vertices = vertices,
                indices = indices,
            };
        }

        private static KoIndexedMeshData ReadIndexedMesh(KoBinaryCursor r)
        {
            string name = r.ReadString();
            int faceCount = CheckedCount(r.ReadInt32(), "indexed mesh face count");
            int vertexCount = CheckedCount(r.ReadInt32(), "indexed mesh vertex count");
            int uvCount = CheckedCount(r.ReadInt32(), "indexed mesh UV count");

            KoIndexedMeshData mesh = new KoIndexedMeshData
            {
                name = name,
                faceCount = faceCount,
                vertexCount = vertexCount,
                uvCount = uvCount,
                positions = new Vector3[vertexCount],
                normals = new Vector3[vertexCount],
                indices = new int[faceCount * 3],
                uvs = new Vector2[uvCount],
                uvIndices = new int[uvCount > 0 ? faceCount * 3 : 0],
            };

            if (faceCount > 0 && vertexCount > 0)
            {
                for (int i = 0; i < vertexCount; i++)
                {
                    mesh.positions[i] = r.ReadVector3();
                    mesh.normals[i] = r.ReadVector3();
                }
                for (int i = 0; i < mesh.indices.Length; i++)
                    mesh.indices[i] = r.ReadUInt16();
            }

            if (uvCount > 0)
            {
                for (int i = 0; i < uvCount; i++)
                    mesh.uvs[i] = r.ReadUvDirectXToUnity();
                for (int i = 0; i < mesh.uvIndices.Length; i++)
                    mesh.uvIndices[i] = r.ReadUInt16();
            }
            return mesh;
        }

        private static KoSkinData ReadSkin(KoBinaryCursor r)
        {
            KoIndexedMeshData indexed = ReadIndexedMesh(r);
            KoSkinData skin = new KoSkinData
            {
                name = indexed.name,
                faceCount = indexed.faceCount,
                vertexCount = indexed.vertexCount,
                uvCount = indexed.uvCount,
                positions = indexed.positions,
                normals = indexed.normals,
                indices = indexed.indices,
                uvs = indexed.uvs,
                uvIndices = indexed.uvIndices,
                skinVertices = new KoSkinInfluence[indexed.vertexCount],
            };

            for (int i = 0; i < indexed.vertexCount; i++)
            {
                KoSkinInfluence influence = new KoSkinInfluence { origin = r.ReadVector3() };
                int affected = CheckedCount(r.ReadInt32(), "singular skin influence count");
                r.Skip(8); // serialized 32-bit pointer fields

                influence.joints = new int[affected];
                influence.weights = new float[affected];
                if (affected > 1)
                {
                    for (int j = 0; j < affected; j++)
                        influence.joints[j] = r.ReadInt32();
                    for (int j = 0; j < affected; j++)
                        influence.weights[j] = r.ReadSingle();
                }
                else if (affected == 1)
                {
                    influence.joints[0] = r.ReadInt32();
                    influence.weights[0] = 1f;
                }
                skin.skinVertices[i] = influence;
            }
            return skin;
        }

        private static int CheckedCount(int value, string label)
        {
            if (value < 0 || value > MaxReasonableCount)
                throw new InvalidDataException($"Invalid KO {label}: {value}");
            return value;
        }
    }
}
