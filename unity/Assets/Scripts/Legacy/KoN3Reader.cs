using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

namespace MMORPG.Legacy
{
    public static class KoN3Reader
    {
        private const int MaxReasonableCount = 2_000_000;

        public static KoProgressiveMeshData ReadProgressiveMesh(byte[] bytes)
        {
            KoBinaryCursor r = new KoBinaryCursor(bytes);
            string name = r.ReadString();
            int collapseCount = CheckedCount(r.ReadInt32(), "collapse count");
            int totalIndexChanges = CheckedCount(r.ReadInt32(), "index-change count");
            int maxVertices = CheckedCount(r.ReadInt32(), "vertex count");
            int maxIndices = CheckedCount(r.ReadInt32(), "index count");
            int minVertices = r.ReadInt32();
            int minIndices = r.ReadInt32();

            KoPmeshVertex[] vertices = new KoPmeshVertex[maxVertices];
            for (int i = 0; i < maxVertices; i++)
            {
                vertices[i] = new KoPmeshVertex
                {
                    position = r.ReadVector3(),
                    normal = r.ReadVector3(),
                    uv = r.ReadUvDirectXToUnity(),
                };
            }

            int[] indices = new int[maxIndices];
            for (int i = 0; i < maxIndices; i++)
                indices[i] = r.ReadUInt16();

            r.Skip(checked(collapseCount * 24));
            r.Skip(checked(totalIndexChanges * 4));

            int lodCount = CheckedCount(r.ReadInt32(), "LOD count");
            KoLodControl[] lod = new KoLodControl[lodCount];
            for (int i = 0; i < lodCount; i++)
            {
                lod[i] = new KoLodControl
                {
                    distance = r.ReadSingle(),
                    vertexCount = r.ReadInt32(),
                };
            }

            return new KoProgressiveMeshData
            {
                name = name,
                minVertexCount = minVertices,
                minIndexCount = minIndices,
                vertices = vertices,
                indices = indices,
                lod = lod,
            };
        }

        public static KoCharacterSkinsData ReadCharacterSkins(byte[] bytes)
        {
            KoBinaryCursor r = new KoBinaryCursor(bytes);
            string name = r.ReadString();
            KoSkinData[] lods = new KoSkinData[4];
            for (int lod = 0; lod < lods.Length; lod++)
                lods[lod] = ReadSkin(r);
            return new KoCharacterSkinsData { name = name, lods = lods };
        }

        public static KoCharacterPartData ReadCharacterPart(byte[] bytes)
        {
            KoBinaryCursor r = new KoBinaryCursor(bytes);
            string name = r.ReadString();
            int version = r.ReadInt32();
            KoMaterialData material = ReadMaterial(r);
            string texture = r.ReadString();
            string diffuseTexture = version == 1 ? r.ReadString() : string.Empty;
            string skins = r.ReadString();
            return new KoCharacterPartData
            {
                name = name,
                version = version,
                material = material,
                texturePath = texture,
                diffuseTexturePath = diffuseTexture,
                skinsPath = skins,
            };
        }

        public static KoCharacterPlugData ReadCharacterPlug(byte[] bytes)
        {
            KoBinaryCursor r = new KoBinaryCursor(bytes);
            KoCharacterPlugData plug = new KoCharacterPlugData
            {
                name = r.ReadString(),
                plugType = r.ReadInt32(),
                jointIndex = r.ReadInt32(),
                position = r.ReadVector3(),
                rotationMatrix = r.ReadMatrix4x4RowMajor(),
                scale = r.ReadVector3(),
                material = ReadMaterial(r),
                meshPath = r.ReadString(),
                texturePath = r.ReadString(),
            };

            if (r.Remaining >= 4)
            {
                plug.traceStep = r.ReadInt32();
                if (plug.traceStep > 0)
                {
                    plug.traceColor = r.ReadUInt32();
                    plug.trace0 = r.ReadSingle();
                    plug.trace1 = r.ReadSingle();
                }
            }
            if (r.Remaining >= 4)
                plug.useVirtualMesh = r.ReadInt32();

            return plug;
        }

        public static KoJointNode ReadJoint(byte[] bytes)
        {
            KoBinaryCursor r = new KoBinaryCursor(bytes);
            KoJointNode root = ReadJointNode(r, 0);
            if (r.Remaining != 0)
                throw new InvalidDataException($"N3Joint has {r.Remaining} unread bytes.");
            return root;
        }

        public static KoAnimationControlData ReadAnimationControl(byte[] bytes)
        {
            KoBinaryCursor r = new KoBinaryCursor(bytes);
            int count = CheckedCount(r.ReadInt32(), "animation count");
            KoAnimationMeta[] animations = new KoAnimationMeta[count];
            for (int i = 0; i < count; i++)
            {
                animations[i] = new KoAnimationMeta
                {
                    reserved = r.ReadInt32(),
                    frameStart = r.ReadSingle(),
                    frameEnd = r.ReadSingle(),
                    framesPerSecond = r.ReadSingle(),
                    plugTraceStart = r.ReadSingle(),
                    plugTraceEnd = r.ReadSingle(),
                    soundFrame0 = r.ReadSingle(),
                    soundFrame1 = r.ReadSingle(),
                    blendTime = r.ReadSingle(),
                    blendFlags = r.ReadInt32(),
                    strikeFrame0 = r.ReadSingle(),
                    strikeFrame1 = r.ReadSingle(),
                    name = r.ReadString(),
                };
            }
            return new KoAnimationControlData { animations = animations };
        }

        public static KoCharacterData ReadCharacter(byte[] bytes)
        {
            KoBinaryCursor r = new KoBinaryCursor(bytes);
            KoCharacterData character = new KoCharacterData
            {
                transform = ReadTransform(r),
                collisionMeshPath = r.ReadString(),
                climbMeshPath = r.ReadString(),
                jointPath = r.ReadString(),
            };

            int partCount = CheckedCount(r.ReadInt32(), "character part count");
            character.partPaths = ReadStrings(r, partCount);

            int plugCount = CheckedCount(r.ReadInt32(), "character plug count");
            character.plugPaths = ReadStrings(r, plugCount);
            character.animationPath = r.ReadString();

            character.jointPartStarts = new[] { r.ReadInt32(), r.ReadInt32() };
            character.jointPartEnds = new[] { r.ReadInt32(), r.ReadInt32() };

            if (r.Remaining >= 4)
                character.fxPlugPath = r.ReadString();
            if (r.Remaining >= 4)
                character.collisionSkinPath = r.ReadString();

            return character;
        }

        public static KoShapeData ReadShape(byte[] bytes)
        {
            KoBinaryCursor r = new KoBinaryCursor(bytes);
            KoShapeData shape = new KoShapeData
            {
                transform = ReadTransform(r),
                collisionMeshPath = r.ReadString(),
                climbMeshPath = r.ReadString(),
            };

            int partCount = CheckedCount(r.ReadInt32(), "shape part count");
            shape.parts = new KoShapePartData[partCount];
            for (int i = 0; i < partCount; i++)
            {
                KoShapePartData part = new KoShapePartData
                {
                    pivot = r.ReadVector3(),
                    meshPath = r.ReadString(),
                    material = ReadMaterial(r),
                };
                int textureCount = CheckedCount(r.ReadInt32(), "shape texture count");
                part.textureFps = r.ReadSingle();
                part.texturePaths = ReadStrings(r, textureCount);
                shape.parts[i] = part;
            }

            shape.belongId = r.ReadInt32();
            shape.eventId = r.ReadInt32();
            shape.eventType = r.ReadInt32();
            shape.npcId = r.ReadInt32();
            shape.npcStatus = r.ReadInt32();
            return shape;
        }

        public static KoTransformData ReadTransform(KoBinaryCursor r)
        {
            return new KoTransformData
            {
                name = r.ReadString(),
                position = r.ReadVector3(),
                rotation = r.ReadQuaternion(),
                scale = r.ReadVector3(),
                positionKeys = ReadAnimKey(r),
                rotationKeys = ReadAnimKey(r),
                scaleKeys = ReadAnimKey(r),
            };
        }

        public static KoAnimKeyData ReadAnimKey(KoBinaryCursor r)
        {
            int count = CheckedCount(r.ReadInt32(), "animation key count");
            KoAnimKeyData key = new KoAnimKeyData { count = count };
            if (count == 0)
                return key;

            key.type = r.ReadInt32();
            key.samplingRate = r.ReadSingle();
            if (key.type == 0)
            {
                key.vectors = new Vector3[count];
                for (int i = 0; i < count; i++)
                    key.vectors[i] = r.ReadVector3();
            }
            else if (key.type == 1)
            {
                key.quaternions = new Quaternion[count];
                for (int i = 0; i < count; i++)
                    key.quaternions[i] = r.ReadQuaternion();
            }
            else
            {
                throw new InvalidDataException($"Unsupported KO animation key type: {key.type}");
            }
            return key;
        }

        public static KoMaterialData ReadMaterial(KoBinaryCursor r)
        {
            return new KoMaterialData
            {
                diffuse = r.ReadD3DColor(),
                ambient = r.ReadD3DColor(),
                specular = r.ReadD3DColor(),
                emissive = r.ReadD3DColor(),
                power = r.ReadSingle(),
                colorOp = r.ReadUInt32(),
                colorArg1 = r.ReadUInt32(),
                colorArg2 = r.ReadUInt32(),
                renderFlags = r.ReadUInt32(),
                sourceBlend = r.ReadUInt32(),
                destinationBlend = r.ReadUInt32(),
            };
        }

        private static KoSkinData ReadSkin(KoBinaryCursor r)
        {
            KoSkinData skin = new KoSkinData
            {
                name = r.ReadString(),
                faceCount = CheckedCount(r.ReadInt32(), "skin face count"),
                vertexCount = CheckedCount(r.ReadInt32(), "skin vertex count"),
                uvCount = CheckedCount(r.ReadInt32(), "skin UV count"),
            };

            skin.positions = new Vector3[skin.vertexCount];
            skin.normals = new Vector3[skin.vertexCount];
            if (skin.faceCount > 0 && skin.vertexCount > 0)
            {
                for (int i = 0; i < skin.vertexCount; i++)
                {
                    skin.positions[i] = r.ReadVector3();
                    skin.normals[i] = r.ReadVector3();
                }
                skin.indices = new int[skin.faceCount * 3];
                for (int i = 0; i < skin.indices.Length; i++)
                    skin.indices[i] = r.ReadUInt16();
            }

            if (skin.uvCount > 0)
            {
                skin.uvs = new Vector2[skin.uvCount];
                for (int i = 0; i < skin.uvs.Length; i++)
                    skin.uvs[i] = r.ReadUvDirectXToUnity();
                skin.uvIndices = new int[skin.faceCount * 3];
                for (int i = 0; i < skin.uvIndices.Length; i++)
                    skin.uvIndices[i] = r.ReadUInt16();
            }

            skin.skinVertices = new KoSkinInfluence[skin.vertexCount];
            for (int i = 0; i < skin.vertexCount; i++)
            {
                KoSkinInfluence influence = new KoSkinInfluence { origin = r.ReadVector3() };
                int affected = CheckedCount(r.ReadInt32(), "skin influence count");
                r.ReadInt32(); // serialized joint pointer
                r.ReadInt32(); // serialized weight pointer

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

        private static KoJointNode ReadJointNode(KoBinaryCursor r, int depth)
        {
            if (depth > 512)
                throw new InvalidDataException("KO skeleton recursion exceeded 512 joints.");

            KoJointNode joint = new KoJointNode
            {
                name = r.ReadString(),
                position = r.ReadVector3(),
                rotation = r.ReadQuaternion(),
                scale = r.ReadVector3(),
                positionKeys = ReadAnimKey(r),
                rotationKeys = ReadAnimKey(r),
                scaleKeys = ReadAnimKey(r),
                orientKeys = ReadAnimKey(r),
            };
            int childCount = CheckedCount(r.ReadInt32(), "joint child count");
            joint.children = new KoJointNode[childCount];
            for (int i = 0; i < childCount; i++)
                joint.children[i] = ReadJointNode(r, depth + 1);
            return joint;
        }

        private static string[] ReadStrings(KoBinaryCursor r, int count)
        {
            string[] values = new string[count];
            for (int i = 0; i < count; i++)
                values[i] = r.ReadString();
            return values;
        }

        private static int CheckedCount(int value, string label)
        {
            if (value < 0 || value > MaxReasonableCount)
                throw new InvalidDataException($"Invalid KO {label}: {value}");
            return value;
        }
    }
}
