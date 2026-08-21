using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.Networking;

namespace MMORPG.Legacy
{
    [Serializable]
    public sealed class KoRuntimeTextureIndex
    {
        public int schema;
        public KoRuntimeTextureIndexEntry[] entries = Array.Empty<KoRuntimeTextureIndexEntry>();
    }

    [Serializable]
    public sealed class KoRuntimeTextureIndexEntry
    {
        public string legacyPath = string.Empty;
        public string runtimePath = string.Empty;
    }

    public sealed class KoRuntimeTextureStore
    {
        private const string IndexPath = "KOConverted/texture-index.json";

        private readonly Dictionary<string, string> runtimePathByLegacy =
            new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        private readonly Dictionary<string, Texture2D> cache =
            new Dictionary<string, Texture2D>(StringComparer.OrdinalIgnoreCase);

        public bool IsReady { get; private set; }

        public async Task InitializeAsync()
        {
            byte[] bytes = await ReadStreamingAssetBytesAsync(IndexPath);
            string json = System.Text.Encoding.UTF8.GetString(bytes);
            KoRuntimeTextureIndex index = JsonUtility.FromJson<KoRuntimeTextureIndex>(json);
            if (index?.entries == null || index.entries.Length == 0)
                throw new InvalidOperationException("KO runtime texture index is missing or empty.");

            runtimePathByLegacy.Clear();
            foreach (KoRuntimeTextureIndexEntry entry in index.entries)
            {
                if (entry == null || string.IsNullOrWhiteSpace(entry.legacyPath) || string.IsNullOrWhiteSpace(entry.runtimePath))
                    continue;
                runtimePathByLegacy[Normalize(entry.legacyPath)] = entry.runtimePath;
            }
            IsReady = true;
        }

        public async Task<Texture2D> LoadAsync(string legacyPath, bool pointSampling = false)
        {
            if (!IsReady)
                throw new InvalidOperationException("KO runtime texture store is not initialized.");

            string key = Normalize(legacyPath);
            if (cache.TryGetValue(key, out Texture2D cached) && cached != null)
                return cached;

            if (!TryResolveRuntimePath(key, out string runtimePath))
                throw new KeyNotFoundException($"KO texture is not indexed for Android: {legacyPath}");

            byte[] bytes = await ReadStreamingAssetBytesAsync(runtimePath);
            Texture2D texture = new Texture2D(2, 2, TextureFormat.RGBA32, false, false)
            {
                name = System.IO.Path.GetFileNameWithoutExtension(legacyPath),
                wrapMode = TextureWrapMode.Repeat,
                filterMode = pointSampling ? FilterMode.Point : FilterMode.Bilinear,
            };
            if (!ImageConversion.LoadImage(texture, bytes, true))
            {
                UnityEngine.Object.Destroy(texture);
                throw new InvalidOperationException($"Converted KO PNG could not be decoded: {legacyPath}");
            }

            cache[key] = texture;
            return texture;
        }

        public void Clear()
        {
            foreach (Texture2D texture in cache.Values)
                if (texture != null)
                    UnityEngine.Object.Destroy(texture);
            cache.Clear();
        }

        private bool TryResolveRuntimePath(string normalizedLegacyPath, out string runtimePath)
        {
            if (runtimePathByLegacy.TryGetValue(normalizedLegacyPath, out runtimePath))
                return true;

            string basename = System.IO.Path.GetFileName(normalizedLegacyPath);
            string found = null;
            foreach (KeyValuePair<string, string> pair in runtimePathByLegacy)
            {
                if (!string.Equals(System.IO.Path.GetFileName(pair.Key), basename, StringComparison.OrdinalIgnoreCase))
                    continue;
                if (found != null && !string.Equals(found, pair.Value, StringComparison.OrdinalIgnoreCase))
                {
                    runtimePath = null;
                    return false;
                }
                found = pair.Value;
            }
            runtimePath = found;
            return found != null;
        }

        private static async Task<byte[]> ReadStreamingAssetBytesAsync(string relativePath)
        {
            string path = Application.streamingAssetsPath.TrimEnd('/', '\\') + "/" + relativePath.Replace('\\', '/');
            using UnityWebRequest request = UnityWebRequest.Get(path);
            UnityWebRequestAsyncOperation operation = request.SendWebRequest();
            while (!operation.isDone)
                await Task.Yield();
            if (request.result != UnityWebRequest.Result.Success)
                throw new InvalidOperationException($"StreamingAssets read failed: {relativePath}: {request.error}");
            return request.downloadHandler.data;
        }

        private static string Normalize(string path)
        {
            return (path ?? string.Empty).Replace('\\', '/').TrimStart('.', '/').ToLowerInvariant();
        }
    }

    public static class KoTextures
    {
        public static KoRuntimeTextureStore Store { get; } = new KoRuntimeTextureStore();
    }
}
