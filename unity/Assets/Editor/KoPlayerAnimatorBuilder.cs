#if UNITY_EDITOR
using System;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEditor.Animations;
using UnityEngine;

namespace MMORPG.EditorTools
{
    public static class KoPlayerAnimatorBuilder
    {
        [Serializable]
        private sealed class AnimationManifest
        {
            public int schema;
            public int race;
            public int clipCount;
            public AnimationManifestClip[] clips = Array.Empty<AnimationManifestClip>();
        }

        [Serializable]
        private sealed class AnimationManifestClip
        {
            public int index;
            public string name = string.Empty;
            public float frameStart;
            public float frameEnd;
        }

        public static void BuildController(string modelPath, string manifestPath, string controllerPath)
        {
            TextAsset manifestAsset = AssetDatabase.LoadAssetAtPath<TextAsset>(manifestPath);
            ModelImporter importer = AssetImporter.GetAtPath(modelPath) as ModelImporter;
            if (manifestAsset == null)
                throw new InvalidOperationException($"KO animation manifest is missing: {manifestPath}");
            if (importer == null)
                throw new InvalidOperationException($"KO FBX does not have a ModelImporter: {modelPath}");

            AnimationManifest manifest = JsonUtility.FromJson<AnimationManifest>(manifestAsset.text);
            if (manifest?.clips == null || manifest.clips.Length < 16)
                throw new InvalidDataException("KO animation manifest is incomplete; at least the base movement set is required.");

            ConfigureModelImporter(importer, manifest);
            Dictionary<int, AnimationClip> clipsByIndex = ResolveClips(modelPath, manifest);

            RequireClip(clipsByIndex, 0, "ANI_BREATH");
            RequireClip(clipsByIndex, 1, "ANI_WALK");
            RequireClip(clipsByIndex, 2, "ANI_RUN");
            RequireClip(clipsByIndex, 12, "ANI_SITDOWN_BREATH");

            string directory = Path.GetDirectoryName(controllerPath)?.Replace('\\', '/') ?? "Assets";
            EnsureAssetFolder(directory);
            if (AssetDatabase.LoadAssetAtPath<RuntimeAnimatorController>(controllerPath) != null)
                AssetDatabase.DeleteAsset(controllerPath);

            AnimatorController controller = AnimatorController.CreateAnimatorControllerAtPath(controllerPath);
            controller.AddParameter("Speed", AnimatorControllerParameterType.Float);
            controller.AddParameter("Running", AnimatorControllerParameterType.Bool);
            controller.AddParameter("Sitting", AnimatorControllerParameterType.Bool);
            controller.AddParameter("AutoAttack", AnimatorControllerParameterType.Bool);

            AnimatorStateMachine stateMachine = controller.layers[0].stateMachine;
            AnimatorState locomotion = stateMachine.AddState("Locomotion");
            stateMachine.defaultState = locomotion;

            BlendTree locomotionTree = new BlendTree
            {
                name = "KO_Locomotion",
                blendType = BlendTreeType.Simple1D,
                blendParameter = "Speed",
                useAutomaticThresholds = false,
            };
            AssetDatabase.AddObjectToAsset(locomotionTree, controller);
            locomotionTree.AddChild(clipsByIndex[0], 0f);
            locomotionTree.AddChild(clipsByIndex[1], 0.5f);
            locomotionTree.AddChild(clipsByIndex[2], 1f);
            locomotion.motion = locomotionTree;

            AnimatorState sitting = stateMachine.AddState("Sitting");
            sitting.motion = clipsByIndex[12];
            AnimatorStateTransition toSit = locomotion.AddTransition(sitting);
            ConfigureTransition(toSit, "Sitting", AnimatorConditionMode.If);
            AnimatorStateTransition fromSit = sitting.AddTransition(locomotion);
            ConfigureTransition(fromSit, "Sitting", AnimatorConditionMode.IfNot);

            int attackIndex = clipsByIndex.ContainsKey(87) ? 87 : 15;
            RequireClip(clipsByIndex, attackIndex, "default attack");
            AnimatorState attack = stateMachine.AddState("AutoAttack");
            attack.motion = clipsByIndex[attackIndex];
            AnimatorStateTransition toAttack = locomotion.AddTransition(attack);
            ConfigureTransition(toAttack, "AutoAttack", AnimatorConditionMode.If);
            AnimatorStateTransition fromAttack = attack.AddTransition(locomotion);
            ConfigureTransition(fromAttack, "AutoAttack", AnimatorConditionMode.IfNot);

            EditorUtility.SetDirty(controller);
            AssetDatabase.SaveAssets();
            AssetDatabase.ImportAsset(controllerPath, ImportAssetOptions.ForceUpdate);

            Debug.Log(
                $"KO animator generated from exact N3 animation indices: race={manifest.race}, " +
                $"clips={manifest.clipCount}, controller={controllerPath}"
            );
        }

        private static void ConfigureModelImporter(ModelImporter importer, AnimationManifest manifest)
        {
            bool changed = false;
            if (!importer.importAnimation)
            {
                importer.importAnimation = true;
                changed = true;
            }
            if (importer.animationType != ModelImporterAnimationType.Generic)
            {
                importer.animationType = ModelImporterAnimationType.Generic;
                changed = true;
            }

            ModelImporterClipAnimation[] clips = importer.defaultClipAnimations;
            HashSet<string> loopNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (int index in new[] { 0, 1, 2, 3, 12, 86, 87 })
            {
                AnimationManifestClip entry = FindManifestClip(manifest, index);
                if (entry != null && !string.IsNullOrWhiteSpace(entry.name))
                    loopNames.Add(entry.name);
            }

            foreach (ModelImporterClipAnimation clip in clips)
            {
                bool shouldLoop = MatchesAny(clip.name, loopNames) || MatchesAny(clip.takeName, loopNames);
                if (clip.loopTime != shouldLoop)
                {
                    clip.loopTime = shouldLoop;
                    clip.loopPose = shouldLoop;
                    changed = true;
                }
            }

            if (changed)
            {
                importer.clipAnimations = clips;
                importer.SaveAndReimport();
            }
        }

        private static Dictionary<int, AnimationClip> ResolveClips(string modelPath, AnimationManifest manifest)
        {
            List<AnimationClip> clipList = new List<AnimationClip>();
            foreach (UnityEngine.Object asset in AssetDatabase.LoadAllAssetsAtPath(modelPath))
            {
                if (asset is AnimationClip clip && !clip.name.StartsWith("__preview__", StringComparison.OrdinalIgnoreCase))
                    clipList.Add(clip);
            }

            Dictionary<int, AnimationClip> result = new Dictionary<int, AnimationClip>();
            foreach (AnimationManifestClip entry in manifest.clips)
            {
                AnimationClip clip = FindImportedClip(clipList, entry.name);
                if (clip != null)
                    result[entry.index] = clip;
            }
            return result;
        }

        private static AnimationClip FindImportedClip(List<AnimationClip> clips, string expectedName)
        {
            if (string.IsNullOrWhiteSpace(expectedName))
                return null;

            foreach (AnimationClip clip in clips)
                if (string.Equals(clip.name, expectedName, StringComparison.OrdinalIgnoreCase))
                    return clip;

            foreach (AnimationClip clip in clips)
            {
                if (clip.name.EndsWith("|" + expectedName, StringComparison.OrdinalIgnoreCase)
                    || clip.name.EndsWith("_" + expectedName, StringComparison.OrdinalIgnoreCase))
                    return clip;
            }
            return null;
        }

        private static AnimationManifestClip FindManifestClip(AnimationManifest manifest, int index)
        {
            foreach (AnimationManifestClip clip in manifest.clips)
                if (clip.index == index)
                    return clip;
            return null;
        }

        private static bool MatchesAny(string value, HashSet<string> names)
        {
            if (string.IsNullOrWhiteSpace(value))
                return false;
            foreach (string name in names)
            {
                if (string.Equals(value, name, StringComparison.OrdinalIgnoreCase)
                    || value.EndsWith("|" + name, StringComparison.OrdinalIgnoreCase)
                    || value.EndsWith("_" + name, StringComparison.OrdinalIgnoreCase))
                    return true;
            }
            return false;
        }

        private static void ConfigureTransition(
            AnimatorStateTransition transition,
            string parameter,
            AnimatorConditionMode mode)
        {
            transition.hasExitTime = false;
            transition.hasFixedDuration = true;
            transition.duration = 0.08f;
            transition.AddCondition(mode, 0f, parameter);
        }

        private static void RequireClip(Dictionary<int, AnimationClip> clips, int index, string purpose)
        {
            if (!clips.ContainsKey(index) || clips[index] == null)
                throw new InvalidOperationException($"KO animation index {index} ({purpose}) was not imported into the FBX.");
        }

        private static void EnsureAssetFolder(string path)
        {
            string[] parts = path.Split('/');
            string current = parts[0];
            for (int i = 1; i < parts.Length; i++)
            {
                string next = current + "/" + parts[i];
                if (!AssetDatabase.IsValidFolder(next))
                    AssetDatabase.CreateFolder(current, parts[i]);
                current = next;
            }
        }
    }
}
#endif
