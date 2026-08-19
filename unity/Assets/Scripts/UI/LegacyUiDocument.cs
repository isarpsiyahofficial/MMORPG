using System;

namespace MMORPG.UI
{
    [Serializable]
    public sealed class LegacyUiDocument
    {
        public int schema;
        public int formatVersion;
        public string sourceFile = string.Empty;
        public string sourcePath = string.Empty;
        public int byteLength;
        public string[] textures = Array.Empty<string>();
        public string[] ids = Array.Empty<string>();
        public LegacyUiNode root;
    }

    [Serializable]
    public sealed class LegacyUiNode
    {
        public string type = string.Empty;
        public int typeId;
        public string name = string.Empty;
        public string id = string.Empty;
        public int[] region = Array.Empty<int>();
        public int[] movable = Array.Empty<int>();
        public long style;
        public long reserved;
        public string tooltip = string.Empty;
        public string soundOpen = string.Empty;
        public string soundClose = string.Empty;
        public LegacyUiNode[] children = Array.Empty<LegacyUiNode>();

        public string texture = string.Empty;
        public float[] uv = Array.Empty<float>();
        public float animationFps;

        public string fontName = string.Empty;
        public int fontHeight;
        public long fontFlags;
        public long color;
        public string text = string.Empty;
        public int lineSpacing;

        public int[] clickRegion = Array.Empty<int>();
        public string soundOn = string.Empty;
        public string soundClick = string.Empty;
        public string soundTyping = string.Empty;
        public int areaType;

        public bool HasRegion => region != null && region.Length == 4;
    }

    [Serializable]
    public sealed class LegacyUiTextureIndex
    {
        public int schema;
        public LegacyUiTextureIndexEntry[] entries = Array.Empty<LegacyUiTextureIndexEntry>();
    }

    [Serializable]
    public sealed class LegacyUiTextureIndexEntry
    {
        public string legacyPath = string.Empty;
        public string resourcePath = string.Empty;
    }
}
