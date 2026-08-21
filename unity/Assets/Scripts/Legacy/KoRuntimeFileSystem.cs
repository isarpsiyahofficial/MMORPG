using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.Networking;

namespace MMORPG.Legacy
{
    [Serializable]
    public sealed class KoRuntimePackManifest
    {
        public int schema;
        public string sourceCommit = string.Empty;
        public int sourceFiles;
        public int embeddedFiles;
        public int platformExcludedFiles;
        public long embeddedBytes;
        public KoRuntimePackEntry[] files = Array.Empty<KoRuntimePackEntry>();
    }

    [Serializable]
    public sealed class KoRuntimePackEntry
    {
        public string path = string.Empty;
        public long bytes;
        public string sha256 = string.Empty;
        public string extension = string.Empty;
        public string category = string.Empty;
        public string strategy = string.Empty;
        public string status = string.Empty;
        public string runtimePath = string.Empty;
        public string reason = string.Empty;
    }

    public sealed class KoRuntimeFileSystem
    {
        public const string ManifestRelativePath = "KO/runtime-pack.json";

        private readonly Dictionary<string, KoRuntimePackEntry> entries =
            new Dictionary<string, KoRuntimePackEntry>(StringComparer.OrdinalIgnoreCase);

        public KoRuntimePackManifest Manifest { get; private set; }
        public bool IsReady => Manifest != null;

        public async Task InitializeAsync()
        {
            byte[] manifestBytes = await ReadStreamingAssetBytesAsync(ManifestRelativePath);
            string json = System.Text.Encoding.UTF8.GetString(manifestBytes);
            Manifest = JsonUtility.FromJson<KoRuntimePackManifest>(json);
            if (Manifest == null || Manifest.files == null)
                throw new InvalidOperationException("KO Android runtime pack manifest is invalid.");

            entries.Clear();
            foreach (KoRuntimePackEntry entry in Manifest.files)
            {
                if (entry == null || string.IsNullOrWhiteSpace(entry.path))
                    continue;
                entries[Normalize(entry.path)] = entry;
            }

            if (entries.Count != Manifest.sourceFiles)
                throw new InvalidOperationException(
                    $"KO runtime index mismatch: indexed={entries.Count}, source={Manifest.sourceFiles}"
                );
        }

        public bool TryGetEntry(string legacyPath, out KoRuntimePackEntry entry)
        {
            return entries.TryGetValue(Normalize(legacyPath), out entry);
        }

        public async Task<byte[]> ReadBytesAsync(string legacyPath)
        {
            if (!IsReady)
                throw new InvalidOperationException("KO runtime file system is not initialized.");
            if (!TryGetEntry(legacyPath, out KoRuntimePackEntry entry))
                throw new KeyNotFoundException($"KO runtime file is not indexed: {legacyPath}");
            if (!string.Equals(entry.status, "embedded-exact", StringComparison.OrdinalIgnoreCase))
                throw new InvalidOperationException(
                    $"KO runtime file cannot be opened on Android: {legacyPath} ({entry.status})"
                );

            return await ReadStreamingAssetBytesAsync(entry.runtimePath);
        }

        public IEnumerable<KoRuntimePackEntry> EnumerateCategory(string category)
        {
            foreach (KoRuntimePackEntry entry in entries.Values)
                if (string.Equals(entry.category, category, StringComparison.OrdinalIgnoreCase))
                    yield return entry;
        }

        private static async Task<byte[]> ReadStreamingAssetBytesAsync(string relativePath)
        {
            string path = Application.streamingAssetsPath.TrimEnd('/', '\\') + "/" + relativePath.Replace('\\', '/');
            using UnityWebRequest request = UnityWebRequest.Get(path);
            UnityWebRequestAsyncOperation operation = request.SendWebRequest();
            while (!operation.isDone)
                await Task.Yield();

            if (request.result != UnityWebRequest.Result.Success)
                throw new InvalidOperationException(
                    $"Android StreamingAssets read failed: {relativePath}: {request.error}"
                );

            return request.downloadHandler.data;
        }

        private static string Normalize(string path)
        {
            return (path ?? string.Empty)
                .Replace('\\', '/')
                .TrimStart('.', '/')
                .ToLowerInvariant();
        }
    }

    public static class KoRuntime
    {
        public static KoRuntimeFileSystem Files { get; } = new KoRuntimeFileSystem();
    }
}
