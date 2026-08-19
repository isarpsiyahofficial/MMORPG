#if UNITY_EDITOR
using System;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;
using UnityEngine;

namespace MMORPG.EditorTools
{
    public sealed class PhaseZeroBuildGuard : IPreprocessBuildWithReport
    {
        public int callbackOrder => -1000;

        public void OnPreprocessBuild(BuildReport report)
        {
            if (report.summary.platform != BuildTarget.Android)
                throw new BuildFailedException("Phase-0 only permits Android APK/AAB builds.");

            RequireFile("Resources/Data/new_character_values.json", "Original KO character-start values are missing.");
            RequireFile("Resources/Data/player_looks.json", "Original KO player appearance definitions are missing.");
            RequireConvertedPlayer();
            RequireBuildScene("Boot");
            RequireBuildScene("CharacterCreate");
            RequireBuildScene("World");
        }

        private static void RequireFile(string assetsRelativePath, string message)
        {
            string fullPath = Path.Combine(Application.dataPath, assetsRelativePath.Replace('/', Path.DirectorySeparatorChar));
            if (!File.Exists(fullPath))
                throw new BuildFailedException(message + " Expected: Assets/" + assetsRelativePath);
        }

        private static void RequireConvertedPlayer()
        {
            string folder = Path.Combine(Application.dataPath, "LegacyConverted", "Players");
            bool found = Directory.Exists(folder)
                         && Directory.EnumerateFiles(folder, "*.fbx", SearchOption.AllDirectories).Any();
            if (!found)
                throw new BuildFailedException(
                    "No original KO player has been converted yet. A verified player FBX is required before APK build."
                );
        }

        private static void RequireBuildScene(string sceneName)
        {
            bool found = EditorBuildSettings.scenes.Any(
                scene => scene.enabled
                         && string.Equals(Path.GetFileNameWithoutExtension(scene.path), sceneName, StringComparison.Ordinal)
            );
            if (!found)
                throw new BuildFailedException($"Required phase-0 scene is not enabled in Build Settings: {sceneName}");
        }
    }
}
#endif
