#if UNITY_EDITOR
using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using UnityEditor;
using UnityEngine;

namespace MMORPG.EditorTools
{
    public static class BaselineManifestGenerator
    {
        private const string DefaultSourceFolder = "unity/LegacySource";
        private const string DefaultOutputFile = "unity/Assets/StreamingAssets/Baseline/ko-source.generated.sha256";

        [MenuItem("MMORPG/Baseline/Generate KO Source SHA-256 Manifest")]
        public static void Generate()
        {
            DirectoryInfo assetsParent = Directory.GetParent(Application.dataPath);
            DirectoryInfo repoDirectory = assetsParent?.Parent;
            if (repoDirectory == null)
                throw new InvalidOperationException("Repository root could not be resolved.");

            string repoRoot = repoDirectory.FullName;
            string sourceRoot = Path.Combine(repoRoot, DefaultSourceFolder.Replace('/', Path.DirectorySeparatorChar));
            if (!Directory.Exists(sourceRoot))
            {
                Debug.LogError($"KO source folder was not found: {sourceRoot}");
                return;
            }

            string outputPath = Path.Combine(repoRoot, DefaultOutputFile.Replace('/', Path.DirectorySeparatorChar));
            string outputDirectory = Path.GetDirectoryName(outputPath);
            if (string.IsNullOrEmpty(outputDirectory))
                throw new InvalidOperationException("Manifest output directory could not be resolved.");

            Directory.CreateDirectory(outputDirectory);

            var lines = new List<string>();
            foreach (string file in Directory.EnumerateFiles(sourceRoot, "*", SearchOption.AllDirectories))
            {
                if (Path.GetFileName(file).Equals("README.md", StringComparison.OrdinalIgnoreCase))
                    continue;

                string relative = file.Substring(sourceRoot.Length)
                    .TrimStart(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)
                    .Replace('\\', '/');
                string hash = ComputeSha256(file);
                lines.Add($"{hash}  {relative}");
            }

            lines.Sort(StringComparer.Ordinal);
            File.WriteAllLines(outputPath, lines.ToArray(), new UTF8Encoding(false));
            AssetDatabase.Refresh();
            Debug.Log($"KO source manifest generated with {lines.Count} files: {outputPath}");
        }

        private static string ComputeSha256(string filePath)
        {
            using (SHA256 sha = SHA256.Create())
            using (FileStream stream = File.OpenRead(filePath))
            {
                byte[] hash = sha.ComputeHash(stream);
                return BitConverter.ToString(hash).Replace("-", string.Empty).ToLowerInvariant();
            }
        }
    }
}
#endif
