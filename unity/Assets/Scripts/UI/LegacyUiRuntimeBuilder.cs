using System;
using System.Collections.Generic;
using System.IO;
using MMORPG.Character;
using UnityEngine;
using UnityEngine.SceneManagement;
using UnityEngine.UI;

namespace MMORPG.UI
{
    [RequireComponent(typeof(Canvas))]
    public sealed class LegacyUiRuntimeBuilder : MonoBehaviour
    {
        private const string DocumentResource = "LegacyUI/character_create";
        private const string TextureIndexResource = "LegacyUI/texture_index";

        private readonly Dictionary<string, GameObject> objectsById =
            new Dictionary<string, GameObject>(StringComparer.OrdinalIgnoreCase);
        private readonly Dictionary<string, Text> textsById =
            new Dictionary<string, Text>(StringComparer.OrdinalIgnoreCase);
        private readonly Dictionary<string, Button> buttonsById =
            new Dictionary<string, Button>(StringComparer.OrdinalIgnoreCase);
        private readonly Dictionary<string, LegacyUiButtonVisual> buttonVisualsById =
            new Dictionary<string, LegacyUiButtonVisual>(StringComparer.OrdinalIgnoreCase);
        private readonly Dictionary<string, string> resourcePathByLegacyPath =
            new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

        [SerializeField] private CharacterCreateController controller;
        [SerializeField] private bool buildOnStart = true;

        private LegacyUiDocument document;
        private RectTransform safeAreaRoot;
        private Font fallbackFont;

        private void Awake()
        {
            if (controller == null)
                controller = FindFirstObjectByType<CharacterCreateController>();

            fallbackFont = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            ConfigureCanvas();
        }

        private void Start()
        {
            if (buildOnStart)
                Build();
        }

        public void Build()
        {
            if (controller == null)
                throw new InvalidOperationException("CharacterCreateController is missing from CharacterCreate scene.");

            TextAsset documentJson = Resources.Load<TextAsset>(DocumentResource);
            if (documentJson == null)
                throw new InvalidOperationException(
                    "Converted original KO CharacterCreate UIF is missing at Resources/LegacyUI/character_create.json."
                );

            document = JsonUtility.FromJson<LegacyUiDocument>(documentJson.text);
            if (document?.root == null || !document.root.HasRegion)
                throw new InvalidDataException("Converted CharacterCreate UIF document is invalid.");

            LoadTextureIndex();
            ClearPreviousBuild();
            CreateSafeAreaRoot();

            GameObject root = BuildNode(document.root, safeAreaRoot, null, true, null);
            root.name = "KO_CharacterCreate_Original";

            BindCharacterCreateControls();
            controller.StateChanged += RefreshDynamicState;
            controller.ValidationFailed += OnValidationFailed;
            RefreshDynamicState();
        }

        private void OnDestroy()
        {
            if (controller != null)
            {
                controller.StateChanged -= RefreshDynamicState;
                controller.ValidationFailed -= OnValidationFailed;
            }
        }

        private void ConfigureCanvas()
        {
            Canvas canvas = GetComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            canvas.pixelPerfect = false;

            CanvasScaler scaler = GetComponent<CanvasScaler>();
            if (scaler == null)
                scaler = gameObject.AddComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(1280f, 720f);
            scaler.screenMatchMode = CanvasScaler.ScreenMatchMode.MatchWidthOrHeight;
            scaler.matchWidthOrHeight = 1f;

            if (GetComponent<GraphicRaycaster>() == null)
                gameObject.AddComponent<GraphicRaycaster>();
        }

        private void LoadTextureIndex()
        {
            resourcePathByLegacyPath.Clear();
            TextAsset indexJson = Resources.Load<TextAsset>(TextureIndexResource);
            if (indexJson == null)
                throw new InvalidOperationException("Legacy UI texture index is missing.");

            LegacyUiTextureIndex index = JsonUtility.FromJson<LegacyUiTextureIndex>(indexJson.text);
            if (index?.entries == null || index.entries.Length == 0)
                throw new InvalidDataException("Legacy UI texture index is empty.");

            foreach (LegacyUiTextureIndexEntry entry in index.entries)
            {
                if (entry == null || string.IsNullOrWhiteSpace(entry.legacyPath) || string.IsNullOrWhiteSpace(entry.resourcePath))
                    continue;
                resourcePathByLegacyPath[NormalizeLegacyPath(entry.legacyPath)] = entry.resourcePath;
            }
        }

        private void ClearPreviousBuild()
        {
            objectsById.Clear();
            textsById.Clear();
            buttonsById.Clear();
            buttonVisualsById.Clear();

            Transform existing = transform.Find("KO_SafeArea");
            if (existing != null)
                Destroy(existing.gameObject);
        }

        private void CreateSafeAreaRoot()
        {
            GameObject safe = new GameObject("KO_SafeArea", typeof(RectTransform), typeof(SafeAreaFitter));
            safe.transform.SetParent(transform, false);
            safeAreaRoot = (RectTransform)safe.transform;
            safeAreaRoot.anchorMin = Vector2.zero;
            safeAreaRoot.anchorMax = Vector2.one;
            safeAreaRoot.offsetMin = Vector2.zero;
            safeAreaRoot.offsetMax = Vector2.zero;
        }

        private GameObject BuildNode(
            LegacyUiNode node,
            RectTransform parent,
            int[] parentRegion,
            bool isRoot,
            LegacyUiButtonVisual parentButtonVisual)
        {
            string objectName = !string.IsNullOrWhiteSpace(node.id)
                ? node.id
                : !string.IsNullOrWhiteSpace(node.name) ? node.name : node.type;

            GameObject go = new GameObject(objectName, typeof(RectTransform));
            RectTransform rect = (RectTransform)go.transform;
            rect.SetParent(parent, false);
            ApplyRect(rect, node.region, parentRegion, isRoot);

            if (!string.IsNullOrWhiteSpace(node.id))
                objectsById[node.id] = go;

            LegacyUiButtonVisual ownButtonVisual = null;
            Graphic graphic = null;

            switch (node.typeId)
            {
                case 1:
                    ownButtonVisual = go.AddComponent<LegacyUiButtonVisual>();
                    Button button = go.AddComponent<Button>();
                    button.transition = Selectable.Transition.None;
                    button.navigation = new Navigation { mode = Navigation.Mode.None };
                    if (!string.IsNullOrWhiteSpace(node.id))
                    {
                        buttonsById[node.id] = button;
                        buttonVisualsById[node.id] = ownButtonVisual;
                    }
                    break;

                case 4:
                    graphic = CreateImage(go, node);
                    if (parentButtonVisual != null)
                        parentButtonVisual.RegisterState((int)node.reserved, graphic);
                    break;

                case 6:
                    Text text = CreateText(go, node);
                    graphic = text;
                    if (!string.IsNullOrWhiteSpace(node.id))
                        textsById[node.id] = text;
                    break;

                case 8:
                    // InputField is configured after its legacy child image/string nodes exist.
                    break;
            }

            if (node.children != null)
            {
                foreach (LegacyUiNode child in node.children)
                {
                    if (child == null)
                        continue;
                    BuildNode(child, rect, node.region, false, ownButtonVisual);
                }
            }

            if (node.typeId == 1)
            {
                Button button = go.GetComponent<Button>();
                Graphic target = FindFirstDirectGraphic(rect);
                button.targetGraphic = target;
            }
            else if (node.typeId == 8)
            {
                ConfigureEdit(go, rect, node);
            }

            return go;
        }

        private void ApplyRect(RectTransform rect, int[] region, int[] parentRegion, bool isRoot)
        {
            if (region == null || region.Length != 4)
                return;

            float width = Mathf.Max(1f, region[2] - region[0]);
            float height = Mathf.Max(1f, region[3] - region[1]);

            if (isRoot)
            {
                rect.anchorMin = new Vector2(0.5f, 0.5f);
                rect.anchorMax = new Vector2(0.5f, 0.5f);
                rect.pivot = new Vector2(0.5f, 0.5f);
                rect.sizeDelta = new Vector2(width, height);
                rect.anchoredPosition = Vector2.zero;
                return;
            }

            int parentLeft = parentRegion != null && parentRegion.Length == 4 ? parentRegion[0] : 0;
            int parentTop = parentRegion != null && parentRegion.Length == 4 ? parentRegion[1] : 0;

            rect.anchorMin = new Vector2(0f, 1f);
            rect.anchorMax = new Vector2(0f, 1f);
            rect.pivot = new Vector2(0f, 1f);
            rect.sizeDelta = new Vector2(width, height);
            rect.anchoredPosition = new Vector2(region[0] - parentLeft, -(region[1] - parentTop));
        }

        private Graphic CreateImage(GameObject go, LegacyUiNode node)
        {
            RawImage raw = go.AddComponent<RawImage>();
            raw.raycastTarget = false;

            Sprite sprite = ResolveSprite(node.texture);
            if (sprite == null)
            {
                raw.enabled = false;
                return raw;
            }

            raw.texture = sprite.texture;
            raw.uvRect = ConvertUv(node.uv);
            raw.color = Color.white;
            return raw;
        }

        private Text CreateText(GameObject go, LegacyUiNode node)
        {
            Text text = go.AddComponent<Text>();
            text.raycastTarget = false;
            text.font = fallbackFont;
            text.fontSize = Mathf.Clamp(node.fontHeight > 0 ? node.fontHeight : 12, 6, 96);
            text.fontStyle = (node.fontFlags & 1L) != 0 ? FontStyle.Bold : FontStyle.Normal;
            text.color = DecodeArgb(node.color);
            text.text = node.text ?? string.Empty;
            text.alignment = DecodeTextAnchor(node.style);
            text.horizontalOverflow = HorizontalWrapMode.Wrap;
            text.verticalOverflow = VerticalWrapMode.Truncate;
            return text;
        }

        private void ConfigureEdit(GameObject go, RectTransform rect, LegacyUiNode node)
        {
            InputField input = go.AddComponent<InputField>();
            input.transition = Selectable.Transition.None;
            input.lineType = InputField.LineType.SingleLine;
            input.contentType = InputField.ContentType.Standard;
            input.navigation = new Navigation { mode = Navigation.Mode.None };

            Text text = FindFirstChildText(rect);
            if (text == null)
            {
                GameObject textGo = new GameObject("RuntimeText", typeof(RectTransform), typeof(Text));
                RectTransform textRect = (RectTransform)textGo.transform;
                textRect.SetParent(rect, false);
                textRect.anchorMin = Vector2.zero;
                textRect.anchorMax = Vector2.one;
                textRect.offsetMin = new Vector2(2f, 0f);
                textRect.offsetMax = new Vector2(-2f, 0f);
                text = textGo.GetComponent<Text>();
                text.font = fallbackFont;
                text.fontSize = 14;
                text.color = Color.white;
                text.alignment = TextAnchor.MiddleLeft;
            }
            text.raycastTarget = false;
            input.textComponent = text;
            input.targetGraphic = FindFirstDirectGraphic(rect);

            if (string.Equals(node.id, "edit_name", StringComparison.OrdinalIgnoreCase))
            {
                input.onValueChanged.AddListener(controller.SetName);
                input.text = controller.State.characterName;
            }
        }

        private void BindCharacterCreateControls()
        {
            Bind("btn_create", () => controller.CreateCharacter());
            Bind("btn_cancel", () => SceneManager.LoadScene("CharacterCreate", LoadSceneMode.Single));
            Bind("btn_face_left", controller.FaceLeft);
            Bind("btn_face_right", controller.FaceRight);
            Bind("btn_hair_left", controller.HairLeft);
            Bind("btn_hair_right", controller.HairRight);

            Bind("btn_race_ka_at", () => controller.SelectRace(1));
            Bind("btn_race_ka_tu", () => controller.SelectRace(2));
            Bind("btn_race_ka_wt", () => controller.SelectRace(3));
            Bind("btn_race_ka_pt", () => controller.SelectRace(4));
            Bind("btn_race_el_ba", () => controller.SelectRace(11));
            Bind("btn_race_el_rm", () => controller.SelectRace(12));
            Bind("btn_race_el_rf", () => controller.SelectRace(13));

            int baseClass = controller.State.nation == 1 ? 100 : 200;
            Bind("btn_class_warrior", () => controller.SelectClass(baseClass + 1));
            Bind("btn_class_rogue", () => controller.SelectClass(baseClass + 2));
            Bind("btn_class_mage", () => controller.SelectClass(baseClass + 3));
            Bind("btn_class_priest", () => controller.SelectClass(baseClass + 4));

            string[] statNames = { "str", "sta", "dex", "int", "map" };
            for (int i = 0; i < statNames.Length; i++)
            {
                int statIndex = i;
                Bind($"btn_{statNames[i]}_right", () => controller.AddStatPoint(statIndex));
                Bind($"btn_{statNames[i]}_left", () => controller.RemoveStatPoint(statIndex));
            }

            if (objectsById.TryGetValue("area_character", out GameObject previewArea))
            {
                CharacterPreviewDrag drag = previewArea.GetComponent<CharacterPreviewDrag>();
                if (drag == null)
                    drag = previewArea.AddComponent<CharacterPreviewDrag>();
                drag.Configure(controller);
                Image raycastSurface = previewArea.GetComponent<Image>();
                if (raycastSurface == null)
                {
                    raycastSurface = previewArea.AddComponent<Image>();
                    raycastSurface.color = new Color(0f, 0f, 0f, 0f);
                }
                raycastSurface.raycastTarget = true;
            }
        }

        private void Bind(string id, UnityEngine.Events.UnityAction action)
        {
            if (buttonsById.TryGetValue(id, out Button button))
                button.onClick.AddListener(action);
        }

        private void RefreshDynamicState()
        {
            CharacterCreationState state = controller.State;
            SetText("text_str", state.strength.ToString());
            SetText("text_sta", state.stamina.ToString());
            SetText("text_dex", state.dexterity.ToString());
            SetText("text_int", state.intelligence.ToString());
            SetText("text_map", state.magicAttack.ToString());
            SetText("text_bonus", controller.BonusPoints.ToString());

            int baseClass = state.nation == 1 ? 100 : 200;
            UpdateClassAvailability("btn_class_warrior", "img_warrior", baseClass + 1);
            UpdateClassAvailability("btn_class_rogue", "img_rogue", baseClass + 2);
            UpdateClassAvailability("btn_class_mage", "img_mage", baseClass + 3);
            UpdateClassAvailability("btn_class_priest", "img_priest", baseClass + 4);

            SetStatLabelVisible(0, state.characterClass % 100 == 1 || state.characterClass % 100 == 4);
            SetStatLabelVisible(1, state.characterClass % 100 == 1 || state.characterClass % 100 == 2);
            SetStatLabelVisible(2, state.characterClass % 100 == 2);
            SetStatLabelVisible(3, state.characterClass % 100 == 3 || state.characterClass % 100 == 4);
            SetStatLabelVisible(4, state.characterClass % 100 == 3);
        }

        private void UpdateClassAvailability(string buttonId, string disabledImageId, int classId)
        {
            bool available = controller.IsClassAvailable(classId);
            if (buttonsById.TryGetValue(buttonId, out Button button))
            {
                button.gameObject.SetActive(available);
                button.interactable = available;
            }
            if (buttonVisualsById.TryGetValue(buttonId, out LegacyUiButtonVisual visual))
                visual.SetInteractable(available);
            if (objectsById.TryGetValue(disabledImageId, out GameObject disabledImage))
                disabledImage.SetActive(!available);
        }

        private void SetStatLabelVisible(int statIndex, bool visible)
        {
            string[] ids = { "img_str", "img_sta", "img_dex", "img_int", "img_map" };
            if (statIndex >= 0 && statIndex < ids.Length && objectsById.TryGetValue(ids[statIndex], out GameObject obj))
                obj.SetActive(visible);
        }

        private void SetText(string id, string value)
        {
            if (textsById.TryGetValue(id, out Text text))
                text.text = value;
        }

        private void OnValidationFailed(string message)
        {
            Debug.LogWarning($"CharacterCreate validation: {message}");
            SetText("text_desc", message);
        }

        private Sprite ResolveSprite(string legacyPath)
        {
            if (string.IsNullOrWhiteSpace(legacyPath))
                return null;

            string key = NormalizeLegacyPath(legacyPath);
            if (!resourcePathByLegacyPath.TryGetValue(key, out string resourcePath))
            {
                // Some UIFs omit the UI/UI_US prefix. Fall back only when basename is unique.
                string basename = Path.GetFileNameWithoutExtension(key);
                string unique = null;
                foreach (KeyValuePair<string, string> pair in resourcePathByLegacyPath)
                {
                    if (!string.Equals(Path.GetFileNameWithoutExtension(pair.Key), basename, StringComparison.OrdinalIgnoreCase))
                        continue;
                    if (unique != null && !string.Equals(unique, pair.Value, StringComparison.OrdinalIgnoreCase))
                        return MissingTexture(legacyPath);
                    unique = pair.Value;
                }
                resourcePath = unique;
            }

            if (string.IsNullOrWhiteSpace(resourcePath))
                return MissingTexture(legacyPath);

            Sprite sprite = Resources.Load<Sprite>(resourcePath);
            if (sprite == null)
                Debug.LogError($"Converted KO UI texture is indexed but cannot be loaded: {legacyPath} -> {resourcePath}");
            return sprite;
        }

        private static Sprite MissingTexture(string legacyPath)
        {
            Debug.LogError($"Original KO UIF texture is not present in the Android runtime index: {legacyPath}");
            return null;
        }

        private static string NormalizeLegacyPath(string path)
        {
            return path.Replace('\\', '/').TrimStart('.', '/').ToLowerInvariant();
        }

        private static Rect ConvertUv(float[] uv)
        {
            if (uv == null || uv.Length != 4)
                return new Rect(0f, 0f, 1f, 1f);

            float width = uv[2] - uv[0];
            float height = uv[3] - uv[1];
            if (width <= 0f || height <= 0f)
                return new Rect(0f, 0f, 1f, 1f);

            // N3/D3 UI data uses top-left texture V; Unity RawImage uses bottom-left V.
            return new Rect(uv[0], 1f - uv[3], width, height);
        }

        private static Color DecodeArgb(long raw)
        {
            uint value = unchecked((uint)raw);
            byte a = (byte)((value >> 24) & 0xFF);
            byte r = (byte)((value >> 16) & 0xFF);
            byte g = (byte)((value >> 8) & 0xFF);
            byte b = (byte)(value & 0xFF);
            return new Color32(r, g, b, a);
        }

        private static TextAnchor DecodeTextAnchor(long style)
        {
            const long Right = 0x00400000;
            const long Center = 0x00800000;
            const long Bottom = 0x02000000;
            const long VCenter = 0x04000000;

            bool right = (style & Right) != 0;
            bool center = (style & Center) != 0;
            bool bottom = (style & Bottom) != 0;
            bool middle = (style & VCenter) != 0;

            if (bottom)
                return right ? TextAnchor.LowerRight : center ? TextAnchor.LowerCenter : TextAnchor.LowerLeft;
            if (middle)
                return right ? TextAnchor.MiddleRight : center ? TextAnchor.MiddleCenter : TextAnchor.MiddleLeft;
            return right ? TextAnchor.UpperRight : center ? TextAnchor.UpperCenter : TextAnchor.UpperLeft;
        }

        private static Graphic FindFirstDirectGraphic(RectTransform parent)
        {
            for (int i = 0; i < parent.childCount; i++)
            {
                Graphic graphic = parent.GetChild(i).GetComponent<Graphic>();
                if (graphic != null)
                    return graphic;
            }
            return parent.GetComponent<Graphic>();
        }

        private static Text FindFirstChildText(RectTransform parent)
        {
            return parent.GetComponentInChildren<Text>(true);
        }
    }
}
