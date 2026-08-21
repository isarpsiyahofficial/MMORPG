using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.Rendering;

namespace MMORPG.Legacy
{
    public static class KoN3RuntimeBuilder
    {
        private const uint RenderAlphaBlend = 0x001;
        private const uint RenderDoubleSided = 0x004;
        private const uint RenderPointSampling = 0x010;
        private const uint RenderNoLight = 0x040;
        private const uint RenderNoZWrite = 0x100;
        private const uint RenderUvClamp = 0x200;

        public static async Task<GameObject> BuildCharacterAsync(string characterPath, Transform parent = null)
        {
            string resolvedCharacter = RequirePath(characterPath);
            KoCharacterData character = KoN3Reader.ReadCharacter(await KoRuntime.Files.ReadBytesAsync(resolvedCharacter));

            GameObject root = new GameObject(string.IsNullOrWhiteSpace(character.transform.name)
                ? Path.GetFileNameWithoutExtension(resolvedCharacter)
                : character.transform.name);
            if (parent != null)
                root.transform.SetParent(parent, false);
            ApplyTransform(root.transform, character.transform);

            string jointPath = ResolveReference(resolvedCharacter, character.jointPath);
            if (string.IsNullOrWhiteSpace(jointPath))
                throw new InvalidDataException($"KO character skeleton is missing: {resolvedCharacter} -> {character.jointPath}");

            KoJointNode jointRoot = KoN3Reader.ReadJoint(await KoRuntime.Files.ReadBytesAsync(jointPath));
            GameObject skeletonObject = new GameObject("Skeleton");
            skeletonObject.transform.SetParent(root.transform, false);
            List<Transform> bones = new List<Transform>();
            BuildJointHierarchy(jointRoot, skeletonObject.transform, bones);

            foreach (string storedPartPath in character.partPaths ?? Array.Empty<string>())
            {
                string partPath = ResolveReference(resolvedCharacter, storedPartPath);
                if (string.IsNullOrWhiteSpace(partPath))
                    throw new InvalidDataException($"KO character part is missing: {resolvedCharacter} -> {storedPartPath}");
                await BuildCharacterPartAsync(partPath, root.transform, bones.ToArray());
            }

            foreach (string storedPlugPath in character.plugPaths ?? Array.Empty<string>())
            {
                string plugPath = ResolveReference(resolvedCharacter, storedPlugPath);
                if (string.IsNullOrWhiteSpace(plugPath))
                    throw new InvalidDataException($"KO character plug is missing: {resolvedCharacter} -> {storedPlugPath}");
                await BuildCharacterPlugAsync(plugPath, root.transform, bones.ToArray());
            }

            KoAnimationControlData animationControl = null;
            if (!string.IsNullOrWhiteSpace(character.animationPath))
            {
                string animationPath = ResolveReference(resolvedCharacter, character.animationPath);
                if (string.IsNullOrWhiteSpace(animationPath))
                    throw new InvalidDataException($"KO character animation file is missing: {character.animationPath}");
                animationControl = KoN3Reader.ReadAnimationControl(await KoRuntime.Files.ReadBytesAsync(animationPath));
            }

            if (animationControl != null && animationControl.animations.Length > 0)
            {
                KoSkeletonAnimator animator = root.AddComponent<KoSkeletonAnimator>();
                animator.Initialize(jointRoot, bones.ToArray(), animationControl);
            }

            return root;
        }

        public static async Task<GameObject> BuildShapeAsync(string shapePath, Transform parent = null)
        {
            string resolvedShape = RequirePath(shapePath);
            KoShapeData shape = KoN3Reader.ReadShape(await KoRuntime.Files.ReadBytesAsync(resolvedShape));

            GameObject root = new GameObject(string.IsNullOrWhiteSpace(shape.transform.name)
                ? Path.GetFileNameWithoutExtension(resolvedShape)
                : shape.transform.name);
            if (parent != null)
                root.transform.SetParent(parent, false);
            ApplyTransform(root.transform, shape.transform);

            for (int i = 0; i < shape.parts.Length; i++)
            {
                KoShapePartData part = shape.parts[i];
                string meshPath = ResolveReference(resolvedShape, part.meshPath);
                if (string.IsNullOrWhiteSpace(meshPath))
                    throw new InvalidDataException($"KO shape mesh is missing: {resolvedShape} -> {part.meshPath}");

                KoProgressiveMeshData pmesh = KoN3Reader.ReadProgressiveMesh(await KoRuntime.Files.ReadBytesAsync(meshPath));
                GameObject partObject = new GameObject($"Part_{i}_{Path.GetFileNameWithoutExtension(meshPath)}");
                partObject.transform.SetParent(root.transform, false);
                partObject.transform.localPosition = part.pivot;

                MeshFilter filter = partObject.AddComponent<MeshFilter>();
                MeshRenderer renderer = partObject.AddComponent<MeshRenderer>();
                filter.sharedMesh = BuildStaticMesh(pmesh);

                string texturePath = part.texturePaths != null && part.texturePaths.Length > 0
                    ? ResolveReference(resolvedShape, part.texturePaths[0])
                    : string.Empty;
                renderer.sharedMaterial = await BuildMaterialAsync(part.material, texturePath);
            }

            return root;
        }

        public static Mesh BuildStaticMesh(KoProgressiveMeshData data)
        {
            if (data == null)
                throw new ArgumentNullException(nameof(data));

            Vector3[] vertices = new Vector3[data.vertices.Length];
            Vector3[] normals = new Vector3[data.vertices.Length];
            Vector2[] uv = new Vector2[data.vertices.Length];
            for (int i = 0; i < data.vertices.Length; i++)
            {
                vertices[i] = data.vertices[i].position;
                normals[i] = data.vertices[i].normal;
                uv[i] = data.vertices[i].uv;
            }

            Mesh mesh = new Mesh { name = string.IsNullOrWhiteSpace(data.name) ? "KO_N3PMesh" : data.name };
            if (vertices.Length > ushort.MaxValue)
                mesh.indexFormat = IndexFormat.UInt32;
            mesh.vertices = vertices;
            mesh.normals = normals;
            mesh.uv = uv;
            mesh.triangles = data.indices;
            mesh.RecalculateBounds();
            if (normals.Length == 0)
                mesh.RecalculateNormals();
            return mesh;
        }

        private static async Task BuildCharacterPartAsync(string partPath, Transform characterRoot, Transform[] bones)
        {
            KoCharacterPartData part = KoN3Reader.ReadCharacterPart(await KoRuntime.Files.ReadBytesAsync(partPath));
            string skinsPath = ResolveReference(partPath, part.skinsPath);
            if (string.IsNullOrWhiteSpace(skinsPath))
                throw new InvalidDataException($"KO character skin file is missing: {partPath} -> {part.skinsPath}");

            KoCharacterSkinsData skins = KoN3Reader.ReadCharacterSkins(await KoRuntime.Files.ReadBytesAsync(skinsPath));
            KoSkinData skin = skins.lods?.FirstOrDefault(candidate => candidate != null && candidate.vertexCount > 0);
            if (skin == null)
                throw new InvalidDataException($"KO character part has no usable skin LOD: {partPath}");

            GameObject partObject = new GameObject(string.IsNullOrWhiteSpace(part.name)
                ? Path.GetFileNameWithoutExtension(partPath)
                : part.name);
            partObject.transform.SetParent(characterRoot, false);
            SkinnedMeshRenderer renderer = partObject.AddComponent<SkinnedMeshRenderer>();
            renderer.bones = bones;
            renderer.rootBone = bones.Length > 0 ? bones[0] : characterRoot;
            renderer.sharedMesh = BuildSkinnedMesh(skin, bones, characterRoot);

            string texturePath = ResolveReference(partPath, part.texturePath);
            renderer.sharedMaterial = await BuildMaterialAsync(part.material, texturePath);
        }

        private static async Task BuildCharacterPlugAsync(string plugPath, Transform characterRoot, Transform[] bones)
        {
            KoCharacterPlugData plug = KoN3Reader.ReadCharacterPlug(await KoRuntime.Files.ReadBytesAsync(plugPath));
            if (plug.useVirtualMesh != 0)
                throw new NotSupportedException($"KO VirtualMesh plug is not ported yet: {plugPath}");

            string meshPath = ResolveReference(plugPath, plug.meshPath);
            if (string.IsNullOrWhiteSpace(meshPath))
                throw new InvalidDataException($"KO plug mesh is missing: {plugPath} -> {plug.meshPath}");
            if (!meshPath.EndsWith(".n3pmesh", StringComparison.OrdinalIgnoreCase))
                throw new NotSupportedException($"KO plug mesh format is not ported yet: {meshPath}");

            KoProgressiveMeshData meshData = KoN3Reader.ReadProgressiveMesh(await KoRuntime.Files.ReadBytesAsync(meshPath));
            Transform attach = plug.jointIndex >= 0 && plug.jointIndex < bones.Length ? bones[plug.jointIndex] : characterRoot;

            GameObject item = new GameObject(string.IsNullOrWhiteSpace(plug.name)
                ? Path.GetFileNameWithoutExtension(plugPath)
                : plug.name);
            item.transform.SetParent(attach, false);
            item.transform.localPosition = plug.position;
            item.transform.localScale = plug.scale;
            item.transform.localRotation = QuaternionFromMatrix(plug.rotationMatrix);

            MeshFilter filter = item.AddComponent<MeshFilter>();
            MeshRenderer renderer = item.AddComponent<MeshRenderer>();
            filter.sharedMesh = BuildStaticMesh(meshData);
            string texturePath = ResolveReference(plugPath, plug.texturePath);
            renderer.sharedMaterial = await BuildMaterialAsync(plug.material, texturePath);
        }

        private static Mesh BuildSkinnedMesh(KoSkinData skin, Transform[] bones, Transform characterRoot)
        {
            int cornerCount = checked(skin.faceCount * 3);
            Vector3[] vertices = new Vector3[cornerCount];
            Vector3[] normals = new Vector3[cornerCount];
            Vector2[] uvs = new Vector2[cornerCount];
            int[] triangles = new int[cornerCount];
            BoneWeight[] weights = new BoneWeight[cornerCount];

            for (int corner = 0; corner < cornerCount; corner++)
            {
                int sourceVertex = skin.indices[corner];
                if (sourceVertex < 0 || sourceVertex >= skin.vertexCount)
                    throw new InvalidDataException($"KO skin vertex index out of range: {sourceVertex}/{skin.vertexCount}");

                KoSkinInfluence influence = skin.skinVertices[sourceVertex];
                vertices[corner] = influence != null ? influence.origin : skin.positions[sourceVertex];
                normals[corner] = skin.normals[sourceVertex];
                if (skin.uvCount > 0 && skin.uvIndices.Length > corner)
                {
                    int uvIndex = skin.uvIndices[corner];
                    if (uvIndex >= 0 && uvIndex < skin.uvs.Length)
                        uvs[corner] = skin.uvs[uvIndex];
                }
                triangles[corner] = corner;
                weights[corner] = ToBoneWeight(influence, bones.Length);
            }

            Mesh mesh = new Mesh { name = string.IsNullOrWhiteSpace(skin.name) ? "KO_N3Skin" : skin.name };
            if (cornerCount > ushort.MaxValue)
                mesh.indexFormat = IndexFormat.UInt32;
            mesh.vertices = vertices;
            mesh.normals = normals;
            mesh.uv = uvs;
            mesh.triangles = triangles;
            mesh.boneWeights = weights;

            Matrix4x4[] bindposes = new Matrix4x4[bones.Length];
            for (int i = 0; i < bones.Length; i++)
                bindposes[i] = bones[i].worldToLocalMatrix * characterRoot.localToWorldMatrix;
            mesh.bindposes = bindposes;
            mesh.RecalculateBounds();
            return mesh;
        }

        private static BoneWeight ToBoneWeight(KoSkinInfluence influence, int boneCount)
        {
            if (influence?.joints == null || influence.joints.Length == 0)
                return new BoneWeight { boneIndex0 = 0, weight0 = 1f };

            List<(int joint, float weight)> entries = new List<(int joint, float weight)>();
            for (int i = 0; i < influence.joints.Length; i++)
            {
                int joint = influence.joints[i];
                if (joint < 0 || joint >= boneCount)
                    continue;
                float weight = influence.weights != null && i < influence.weights.Length ? influence.weights[i] : 1f;
                if (weight > 0f)
                    entries.Add((joint, weight));
            }
            if (entries.Count == 0)
                return new BoneWeight { boneIndex0 = 0, weight0 = 1f };

            entries.Sort((a, b) => b.weight.CompareTo(a.weight));
            if (entries.Count > 4)
                entries.RemoveRange(4, entries.Count - 4);
            float sum = entries.Sum(entry => entry.weight);
            if (sum <= 0f)
                sum = 1f;

            BoneWeight result = new BoneWeight();
            for (int i = 0; i < entries.Count; i++)
            {
                float normalized = entries[i].weight / sum;
                switch (i)
                {
                    case 0: result.boneIndex0 = entries[i].joint; result.weight0 = normalized; break;
                    case 1: result.boneIndex1 = entries[i].joint; result.weight1 = normalized; break;
                    case 2: result.boneIndex2 = entries[i].joint; result.weight2 = normalized; break;
                    case 3: result.boneIndex3 = entries[i].joint; result.weight3 = normalized; break;
                }
            }
            return result;
        }

        private static void BuildJointHierarchy(KoJointNode joint, Transform parent, List<Transform> depthFirst)
        {
            GameObject boneObject = new GameObject(string.IsNullOrWhiteSpace(joint.name)
                ? $"Joint_{depthFirst.Count}"
                : joint.name);
            Transform bone = boneObject.transform;
            bone.SetParent(parent, false);
            bone.localPosition = joint.position;
            bone.localRotation = joint.rotation;
            bone.localScale = joint.scale;
            depthFirst.Add(bone);

            if (joint.children == null)
                return;
            foreach (KoJointNode child in joint.children)
                BuildJointHierarchy(child, bone, depthFirst);
        }

        private static async Task<Material> BuildMaterialAsync(KoMaterialData source, string texturePath)
        {
            bool unlit = source != null && (source.renderFlags & RenderNoLight) != 0;
            Shader shader = Shader.Find(unlit ? "Unlit/Texture" : "Standard") ?? Shader.Find("Sprites/Default");
            Material material = new Material(shader)
            {
                name = "KO_Material",
                color = source?.diffuse ?? Color.white,
            };

            if (!string.IsNullOrWhiteSpace(texturePath))
            {
                bool pointSampling = source != null && (source.renderFlags & RenderPointSampling) != 0;
                Texture2D texture = await KoTextures.Store.LoadAsync(texturePath, pointSampling);
                if (source != null && (source.renderFlags & RenderUvClamp) != 0)
                    texture.wrapMode = TextureWrapMode.Clamp;
                material.mainTexture = texture;
            }

            if (source != null)
            {
                if ((source.renderFlags & RenderDoubleSided) != 0 && material.HasProperty("_Cull"))
                    material.SetInt("_Cull", (int)CullMode.Off);
                if ((source.renderFlags & RenderNoZWrite) != 0 && material.HasProperty("_ZWrite"))
                    material.SetInt("_ZWrite", 0);
                if ((source.renderFlags & RenderAlphaBlend) != 0)
                    ConfigureAlphaBlend(material);
            }
            return material;
        }

        private static void ConfigureAlphaBlend(Material material)
        {
            if (material.HasProperty("_Mode"))
                material.SetFloat("_Mode", 3f);
            if (material.HasProperty("_SrcBlend"))
                material.SetInt("_SrcBlend", (int)BlendMode.SrcAlpha);
            if (material.HasProperty("_DstBlend"))
                material.SetInt("_DstBlend", (int)BlendMode.OneMinusSrcAlpha);
            if (material.HasProperty("_ZWrite"))
                material.SetInt("_ZWrite", 0);
            material.DisableKeyword("_ALPHATEST_ON");
            material.EnableKeyword("_ALPHABLEND_ON");
            material.DisableKeyword("_ALPHAPREMULTIPLY_ON");
            material.renderQueue = (int)RenderQueue.Transparent;
        }

        private static void ApplyTransform(Transform target, KoTransformData source)
        {
            target.localPosition = source.position;
            target.localRotation = source.rotation;
            target.localScale = source.scale;
        }

        private static Quaternion QuaternionFromMatrix(Matrix4x4 matrix)
        {
            Vector3 forward = new Vector3(matrix.m02, matrix.m12, matrix.m22);
            Vector3 upwards = new Vector3(matrix.m01, matrix.m11, matrix.m21);
            if (forward.sqrMagnitude < 0.000001f || upwards.sqrMagnitude < 0.000001f)
                return Quaternion.identity;
            return Quaternion.LookRotation(forward, upwards);
        }

        public static string ResolveReference(string basePath, string storedPath)
        {
            if (string.IsNullOrWhiteSpace(storedPath))
                return string.Empty;

            string stored = Normalize(storedPath);
            string baseNormalized = Normalize(basePath);
            string baseDir = DirectoryName(baseNormalized);
            string parentDir = DirectoryName(baseDir);
            string basename = FileName(stored);

            string[] candidates =
            {
                Combine(baseDir, basename),
                Combine(baseDir, stored),
                Combine(parentDir, stored),
                stored,
            };

            foreach (string candidate in candidates)
                if (KoRuntime.Files.TryGetEntry(candidate, out KoRuntimePackEntry entry)
                    && string.Equals(entry.status, "embedded-exact", StringComparison.OrdinalIgnoreCase))
                    return entry.path;

            return string.Empty;
        }

        private static string RequirePath(string path)
        {
            if (!KoRuntime.Files.TryGetEntry(path, out KoRuntimePackEntry entry)
                || !string.Equals(entry.status, "embedded-exact", StringComparison.OrdinalIgnoreCase))
                throw new FileNotFoundException($"KO runtime asset is not available on Android: {path}");
            return entry.path;
        }

        private static string Normalize(string value)
        {
            return (value ?? string.Empty).Replace('\\', '/').TrimStart('.', '/');
        }

        private static string DirectoryName(string value)
        {
            int index = value.LastIndexOf('/');
            return index > 0 ? value.Substring(0, index) : string.Empty;
        }

        private static string FileName(string value)
        {
            int index = value.LastIndexOf('/');
            return index >= 0 ? value.Substring(index + 1) : value;
        }

        private static string Combine(string left, string right)
        {
            if (string.IsNullOrWhiteSpace(left))
                return right.TrimStart('/');
            if (string.IsNullOrWhiteSpace(right))
                return left.TrimEnd('/');
            return left.TrimEnd('/') + "/" + right.TrimStart('/');
        }
    }
}
