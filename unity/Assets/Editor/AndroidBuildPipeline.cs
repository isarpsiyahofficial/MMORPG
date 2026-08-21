#if UNITY_EDITOR
using System;
using System.IO;
using MMORPG.Character;
using MMORPG.Core;
using MMORPG.Gameplay;
using MMORPG.Input;
using MMORPG.UI;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem.UI;
using UnityEngine.Rendering;
using UnityEngine.SceneManagement;
using UnityEngine.UI;

namespace MMORPG.EditorTools
{
    public static class AndroidBuildPipeline
    {
        public const string BootScenePath = "Assets/Scenes/Boot.unity";
        public const string CharacterCreateScenePath = "Assets/Scenes/CharacterCreate.unity";
        public const string WorldScenePath = "Assets/Scenes/World.unity";
        public const string PlayerModelPath = "Assets/LegacyConverted/Players/ko_player_race_12.fbx";
        public const string PlayerAnimatorPath = "Assets/LegacyConverted/Players/ko_player_race_12.controller";
        public const string StartZonePrefabPath = "Assets/LegacyConverted/Zones/zone_1.prefab";
        public const string CharacterCreateJsonPath = "Assets/Resources/LegacyUI/character_create.json";
        public const string UiTextureIndexPath = "Assets/Resources/LegacyUI/texture_index.json";
        public const string DefaultApkPath = "Builds/Android/MMORPG-debug.apk";

        [MenuItem("MMORPG/Android/Prepare scenes")]
        public static void PrepareAndroidScenes()
        {
            ValidateConvertedRuntimeInputs();
            EnsureFolder("Assets/Scenes");
            ConfigureAndroidPlayerSettings();
            CreateBootScene();
            CreateCharacterCreateScene();
            CreateWorldScene();

            EditorBuildSettings.scenes = new[]
            {
                new EditorBuildSettingsScene(BootScenePath, true),
                new EditorBuildSettingsScene(CharacterCreateScenePath, true),
                new EditorBuildSettingsScene(WorldScenePath, true),
            };
            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh();
        }

        [MenuItem("MMORPG/Android/Build strict debug APK")]
        public static void BuildDebugApk()
        {
            PrepareAndroidScenes();

            if (EditorUserBuildSettings.activeBuildTarget != BuildTarget.Android)
            {
                if (!EditorUserBuildSettings.SwitchActiveBuildTarget(BuildTargetGroup.Android, BuildTarget.Android))
                    throw new BuildFailedException("Unity could not switch the active build target to Android.");
            }

            string output = Path.GetFullPath(DefaultApkPath);
            Directory.CreateDirectory(Path.GetDirectoryName(output) ?? throw new InvalidOperationException());
            if (File.Exists(output))
                File.Delete(output);

            EditorUserBuildSettings.buildAppBundle = false;
            EditorUserBuildSettings.androidBuildSystem = AndroidBuildSystem.Gradle;

            BuildPlayerOptions options = new BuildPlayerOptions
            {
                scenes = new[] { BootScenePath, CharacterCreateScenePath, WorldScenePath },
                locationPathName = output,
                target = BuildTarget.Android,
                options = BuildOptions.Development | BuildOptions.AllowDebugging,
            };

            BuildReport report = BuildPipeline.BuildPlayer(options);
            BuildSummary summary = report.summary;
            if (summary.result != BuildResult.Succeeded)
            {
                throw new BuildFailedException(
                    $"Android APK build failed: result={summary.result}, errors={summary.totalErrors}, warnings={summary.totalWarnings}"
                );
            }
            if (!File.Exists(output) || new FileInfo(output).Length <= 0)
                throw new BuildFailedException($"Unity reported success but APK is missing/empty: {output}");

            Debug.Log(
                $"STRICT ANDROID APK PASS: {output} ({new FileInfo(output).Length} bytes, " +
                $"warnings={summary.totalWarnings})"
            );
        }

        private static void ValidateConvertedRuntimeInputs()
        {
            RequireAsset(CharacterCreateJsonPath, "original KO CharacterCreate UIF conversion");
            RequireAsset(UiTextureIndexPath, "original KO UI texture index");
            RequireAsset(PlayerModelPath, "converted KO player model");
            RequireAsset(PlayerAnimatorPath, "KO player animation controller");
            RequireAsset(StartZonePrefabPath, "converted KO starting zone");

            TextAsset uiDocument = AssetDatabase.LoadAssetAtPath<TextAsset>(CharacterCreateJsonPath);
            TextAsset textureIndex = AssetDatabase.LoadAssetAtPath<TextAsset>(UiTextureIndexPath);
            if (uiDocument == null || string.IsNullOrWhiteSpace(uiDocument.text))
                throw new BuildFailedException("CharacterCreate UIF JSON exists but Unity cannot load it as TextAsset.");
            if (textureIndex == null || string.IsNullOrWhiteSpace(textureIndex.text))
                throw new BuildFailedException("Legacy UI texture index exists but Unity cannot load it as TextAsset.");

            LegacyUiDocument doc = JsonUtility.FromJson<LegacyUiDocument>(uiDocument.text);
            LegacyUiTextureIndex index = JsonUtility.FromJson<LegacyUiTextureIndex>(textureIndex.text);
            if (doc?.root == null || doc.ids == null || doc.ids.Length == 0)
                throw new BuildFailedException("CharacterCreate UIF JSON is structurally invalid.");
            if (index?.entries == null || index.entries.Length == 0)
                throw new BuildFailedException("Legacy UI texture index contains no entries.");

            string[] requiredIds =
            {
                "edit_name", "area_character", "btn_create", "btn_face_left", "btn_face_right",
                "btn_hair_left", "btn_hair_right", "text_bonus", "btn_class_warrior",
                "btn_class_rogue", "btn_class_mage", "btn_class_priest",
            };
            foreach (string id in requiredIds)
            {
                if (Array.IndexOf(doc.ids, id) < 0)
                    throw new BuildFailedException($"Original CharacterCreate UIF is missing required control id: {id}");
            }
        }

        private static void ConfigureAndroidPlayerSettings()
        {
            PlayerSettings.companyName = "isarpsiyahofficial";
            PlayerSettings.productName = "MMORPG";
            PlayerSettings.applicationIdentifier = "com.isarpsiyahofficial.mmorpg";
            PlayerSettings.bundleVersion = "0.1.0";
            PlayerSettings.Android.bundleVersionCode = 1;
            PlayerSettings.defaultInterfaceOrientation = UIOrientation.LandscapeLeft;
            PlayerSettings.colorSpace = ColorSpace.Linear;
            PlayerSettings.Android.targetArchitectures = AndroidArchitecture.ARM64;
            PlayerSettings.SetScriptingBackend(NamedBuildTarget.Android, ScriptingImplementation.IL2CPP);
            PlayerSettings.Android.minSdkVersion = AndroidSdkVersions.AndroidApiLevel26;
            PlayerSettings.Android.targetSdkVersion = AndroidSdkVersions.AndroidApiLevelAuto;
            PlayerSettings.Android.forceInternetPermission = true;
            PlayerSettings.stripEngineCode = true;

            PlayerSettings.SetUseDefaultGraphicsAPIs(BuildTarget.Android, false);
            PlayerSettings.SetGraphicsAPIs(
                BuildTarget.Android,
                new[] { GraphicsDeviceType.Vulkan, GraphicsDeviceType.OpenGLES3 }
            );
        }

        private static void CreateBootScene()
        {
            Scene scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
            GameObject app = new GameObject("AppRoot");
            app.AddComponent<OfflineBootstrap>();
            app.AddComponent<AndroidLifecycle>();
            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene, BootScenePath);
        }

        private static void CreateCharacterCreateScene()
        {
            Scene scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
            CreateEventSystem();

            Camera camera = CreateCamera("CharacterCreateCamera");
            camera.transform.SetPositionAndRotation(new Vector3(0f, 1.35f, -4.4f), Quaternion.Euler(4f, 0f, 0f));
            CreateDirectionalLight();

            GameObject runtime = new GameObject("CharacterCreateRuntime");
            runtime.AddComponent<AndroidLifecycle>();
            runtime.AddComponent<CharacterCreationFlow>();
            CharacterCreateController controller = runtime.AddComponent<CharacterCreateController>();

            GameObject previewRoot = new GameObject("CharacterPreviewRoot");
            previewRoot.transform.position = new Vector3(0f, 0f, 0f);
            GameObject playerAsset = AssetDatabase.LoadAssetAtPath<GameObject>(PlayerModelPath);
            GameObject preview = (GameObject)PrefabUtility.InstantiatePrefab(playerAsset, scene);
            preview.name = "KO_PlayerPreview_Race12";
            preview.transform.SetParent(previewRoot.transform, false);
            controller.SetPreviewRoot(previewRoot.transform);
            EditorUtility.SetDirty(controller);

            GameObject canvasGo = new GameObject(
                "CharacterCreateCanvas",
                typeof(RectTransform),
                typeof(Canvas),
                typeof(CanvasScaler),
                typeof(GraphicRaycaster),
                typeof(LegacyUiRuntimeBuilder)
            );
            Canvas canvas = canvasGo.GetComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            canvas.sortingOrder = 100;

            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene, CharacterCreateScenePath);
        }

        private static void CreateWorldScene()
        {
            Scene scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
            CreateEventSystem();
            CreateDirectionalLight();

            GameObject zoneAsset = AssetDatabase.LoadAssetAtPath<GameObject>(StartZonePrefabPath);
            GameObject zone = (GameObject)PrefabUtility.InstantiatePrefab(zoneAsset, scene);
            zone.name = "KO_Zone_1";

            Camera camera = CreateCamera("GameplayCamera");
            camera.transform.SetPositionAndRotation(new Vector3(0f, 3f, -5.5f), Quaternion.Euler(18f, 0f, 0f));

            GameObject playerRoot = new GameObject("Player");
            CharacterController characterController = playerRoot.AddComponent<CharacterController>();
            characterController.center = new Vector3(0f, 1f, 0f);
            characterController.height = 2f;
            characterController.radius = 0.42f;
            characterController.stepOffset = 0.35f;

            GameObject playerAsset = AssetDatabase.LoadAssetAtPath<GameObject>(PlayerModelPath);
            GameObject model = (GameObject)PrefabUtility.InstantiatePrefab(playerAsset, scene);
            model.name = "KO_PlayerModel_Race12";
            model.transform.SetParent(playerRoot.transform, false);

            RuntimeAnimatorController animatorController =
                AssetDatabase.LoadAssetAtPath<RuntimeAnimatorController>(PlayerAnimatorPath);
            Animator animator = model.GetComponentInChildren<Animator>(true);
            if (animator == null)
                animator = model.AddComponent<Animator>();
            animator.runtimeAnimatorController = animatorController;

            KOPlayerController player = playerRoot.AddComponent<KOPlayerController>();
            player.Configure(camera.transform, animator);
            EditorUtility.SetDirty(player);

            OrbitCameraController orbit = camera.gameObject.AddComponent<OrbitCameraController>();
            orbit.SetTarget(playerRoot.transform);
            EditorUtility.SetDirty(orbit);

            GameObject lifecycle = new GameObject("AndroidLifecycle");
            lifecycle.AddComponent<AndroidLifecycle>();

            CreateMobileHud();

            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene, WorldScenePath);
        }

        private static void CreateEventSystem()
        {
            GameObject go = new GameObject("EventSystem", typeof(EventSystem));
            go.AddComponent<InputSystemUIInputModule>();
        }

        private static Camera CreateCamera(string name)
        {
            GameObject go = new GameObject(name, typeof(Camera), typeof(AudioListener));
            go.tag = "MainCamera";
            Camera camera = go.GetComponent<Camera>();
            camera.nearClipPlane = 0.05f;
            camera.farClipPlane = 2000f;
            camera.fieldOfView = 55f;
            return camera;
        }

        private static void CreateDirectionalLight()
        {
            GameObject go = new GameObject("Sun", typeof(Light));
            Light light = go.GetComponent<Light>();
            light.type = LightType.Directional;
            light.intensity = 1f;
            go.transform.rotation = Quaternion.Euler(48f, -32f, 0f);
        }

        private static void CreateMobileHud()
        {
            GameObject canvasGo = new GameObject(
                "MobileHUD",
                typeof(RectTransform),
                typeof(Canvas),
                typeof(CanvasScaler),
                typeof(GraphicRaycaster)
            );
            Canvas canvas = canvasGo.GetComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            canvas.sortingOrder = 200;

            CanvasScaler scaler = canvasGo.GetComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(1920f, 1080f);
            scaler.screenMatchMode = CanvasScaler.ScreenMatchMode.MatchWidthOrHeight;
            scaler.matchWidthOrHeight = 1f;

            GameObject safeGo = new GameObject("SafeArea", typeof(RectTransform), typeof(SafeAreaFitter));
            RectTransform safe = (RectTransform)safeGo.transform;
            safe.SetParent(canvasGo.transform, false);
            safe.anchorMin = Vector2.zero;
            safe.anchorMax = Vector2.one;
            safe.offsetMin = Vector2.zero;
            safe.offsetMax = Vector2.zero;

            CreateJoystick(safe);
            CreateCameraPad(safe);
            CreateHotbar(safe);
            CreateActionCluster(safe);
        }

        private static void CreateJoystick(RectTransform parent)
        {
            RectTransform background = CreatePanel("MoveJoystick", parent, new Vector2(170f, 170f));
            background.anchorMin = background.anchorMax = new Vector2(0f, 0f);
            background.pivot = new Vector2(0.5f, 0.5f);
            background.anchoredPosition = new Vector2(155f, 165f);
            background.GetComponent<Image>().color = new Color(0f, 0f, 0f, 0.28f);

            RectTransform handle = CreatePanel("Handle", background, new Vector2(72f, 72f));
            handle.anchorMin = handle.anchorMax = new Vector2(0.5f, 0.5f);
            handle.pivot = new Vector2(0.5f, 0.5f);
            handle.anchoredPosition = Vector2.zero;
            handle.GetComponent<Image>().color = new Color(1f, 1f, 1f, 0.42f);

            TouchJoystick joystick = background.gameObject.AddComponent<TouchJoystick>();
            joystick.Configure(background, handle, 0.62f);
            EditorUtility.SetDirty(joystick);
        }

        private static void CreateCameraPad(RectTransform parent)
        {
            RectTransform pad = CreatePanel("CameraTouchArea", parent, Vector2.zero);
            pad.anchorMin = new Vector2(0.42f, 0f);
            pad.anchorMax = new Vector2(1f, 1f);
            pad.offsetMin = Vector2.zero;
            pad.offsetMax = Vector2.zero;
            pad.GetComponent<Image>().color = new Color(0f, 0f, 0f, 0.001f);
            TouchCameraPad cameraPad = pad.gameObject.AddComponent<TouchCameraPad>();
            cameraPad.Configure(1f);
            EditorUtility.SetDirty(cameraPad);
        }

        private static void CreateHotbar(RectTransform parent)
        {
            GameObject bar = new GameObject("Hotbar", typeof(RectTransform));
            RectTransform rect = (RectTransform)bar.transform;
            rect.SetParent(parent, false);
            rect.anchorMin = rect.anchorMax = new Vector2(0.5f, 0f);
            rect.pivot = new Vector2(0.5f, 0f);
            rect.sizeDelta = new Vector2(840f, 150f);
            rect.anchoredPosition = new Vector2(0f, 20f);

            for (int i = 0; i < 8; i++)
            {
                RectTransform slot = CreateButton(
                    $"HotbarSlot_{i + 1}",
                    rect,
                    (i + 1).ToString(),
                    MobileActionButtonKind.HotbarSlot,
                    i,
                    new Vector2(88f, 88f)
                );
                slot.anchorMin = slot.anchorMax = new Vector2(0f, 0f);
                slot.pivot = new Vector2(0f, 0f);
                slot.anchoredPosition = new Vector2(i * 100f, 0f);
            }

            for (int i = 0; i < 8; i++)
            {
                RectTransform page = CreateButton(
                    $"HotbarPage_{i + 1}",
                    rect,
                    $"F{i + 1}",
                    MobileActionButtonKind.HotbarPage,
                    i,
                    new Vector2(72f, 42f)
                );
                page.anchorMin = page.anchorMax = new Vector2(0f, 0f);
                page.pivot = new Vector2(0f, 0f);
                page.anchoredPosition = new Vector2(i * 100f + 8f, 98f);
            }
        }

        private static void CreateActionCluster(RectTransform parent)
        {
            (string label, MobileActionButtonKind kind)[] actions =
            {
                ("TAB", MobileActionButtonKind.TargetNearest),
                ("R", MobileActionButtonKind.AutoAttack),
                ("RUN", MobileActionButtonKind.WalkRun),
                ("AUTO", MobileActionButtonKind.AutoRun),
                ("SIT", MobileActionButtonKind.Sit),
                ("INV", MobileActionButtonKind.Inventory),
                ("SKILL", MobileActionButtonKind.Skill),
                ("STATE", MobileActionButtonKind.State),
                ("MAP", MobileActionButtonKind.Map),
            };

            for (int i = 0; i < actions.Length; i++)
            {
                RectTransform button = CreateButton(
                    $"Action_{actions[i].kind}",
                    parent,
                    actions[i].label,
                    actions[i].kind,
                    0,
                    new Vector2(96f, 64f)
                );
                int column = i % 3;
                int row = i / 3;
                button.anchorMin = button.anchorMax = new Vector2(1f, 0f);
                button.pivot = new Vector2(1f, 0f);
                button.anchoredPosition = new Vector2(-25f - column * 108f, 150f + row * 76f);
            }
        }

        private static RectTransform CreateButton(
            string name,
            RectTransform parent,
            string label,
            MobileActionButtonKind kind,
            int index,
            Vector2 size)
        {
            RectTransform rect = CreatePanel(name, parent, size);
            Image image = rect.GetComponent<Image>();
            image.color = new Color(0.05f, 0.05f, 0.05f, 0.62f);
            MobileActionButton action = rect.gameObject.AddComponent<MobileActionButton>();
            action.Configure(kind, index);
            EditorUtility.SetDirty(action);

            GameObject labelGo = new GameObject("Label", typeof(RectTransform), typeof(Text));
            RectTransform labelRect = (RectTransform)labelGo.transform;
            labelRect.SetParent(rect, false);
            labelRect.anchorMin = Vector2.zero;
            labelRect.anchorMax = Vector2.one;
            labelRect.offsetMin = Vector2.zero;
            labelRect.offsetMax = Vector2.zero;
            Text text = labelGo.GetComponent<Text>();
            text.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            text.fontSize = 22;
            text.alignment = TextAnchor.MiddleCenter;
            text.color = Color.white;
            text.raycastTarget = false;
            text.text = label;
            return rect;
        }

        private static RectTransform CreatePanel(string name, RectTransform parent, Vector2 size)
        {
            GameObject go = new GameObject(name, typeof(RectTransform), typeof(Image));
            RectTransform rect = (RectTransform)go.transform;
            rect.SetParent(parent, false);
            rect.sizeDelta = size;
            return rect;
        }

        private static void RequireAsset(string path, string purpose)
        {
            if (!File.Exists(Path.GetFullPath(path)) && AssetDatabase.LoadMainAssetAtPath(path) == null)
                throw new BuildFailedException($"Missing {purpose}: {path}");
        }

        private static void EnsureFolder(string path)
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
