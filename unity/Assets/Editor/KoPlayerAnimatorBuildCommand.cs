#if UNITY_EDITOR
using UnityEditor;
using UnityEngine;

namespace MMORPG.EditorTools
{
    public static class KoPlayerAnimatorBuildCommand
    {
        public const string ModelPath = "Assets/LegacyConverted/Players/ko_player_race_12.fbx";
        public const string ManifestPath = "Assets/LegacyConverted/Players/ko_player_race_12.animations.json";
        public const string ControllerPath = "Assets/LegacyConverted/Players/ko_player_race_12.controller";

        [MenuItem("MMORPG/Android/Build race 12 animator")]
        public static void BuildRace12()
        {
            AssetDatabase.Refresh();
            KoPlayerAnimatorBuilder.BuildController(ModelPath, ManifestPath, ControllerPath);
            RuntimeAnimatorController controller = AssetDatabase.LoadAssetAtPath<RuntimeAnimatorController>(ControllerPath);
            if (controller == null)
                throw new UnityEditor.Build.BuildFailedException($"Animator controller was not generated: {ControllerPath}");
            Debug.Log($"KO RACE 12 ANIMATOR PASS: {ControllerPath}");
        }
    }
}
#endif
