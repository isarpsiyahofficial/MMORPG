#if UNITY_EDITOR
using UnityEditor;

namespace MMORPG.EditorTools
{
    public sealed class LegacyUiTextureImporter : AssetPostprocessor
    {
        private const string UiRoot = "Assets/Resources/LegacyUI/Textures/";

        private void OnPreprocessTexture()
        {
            if (!assetPath.StartsWith(UiRoot, System.StringComparison.OrdinalIgnoreCase))
                return;

            TextureImporter importer = (TextureImporter)assetImporter;
            importer.textureType = TextureImporterType.Sprite;
            importer.spriteImportMode = SpriteImportMode.Single;
            importer.alphaIsTransparency = true;
            importer.mipmapEnabled = false;
            importer.isReadable = false;
            importer.npotScale = TextureImporterNPOTScale.None;

            TextureImporterPlatformSettings android = importer.GetPlatformTextureSettings("Android");
            android.name = "Android";
            android.overridden = true;
            android.format = TextureImporterFormat.ETC2_RGBA8;
            android.textureCompression = TextureImporterCompression.Compressed;
            android.compressionQuality = 100;
            importer.SetPlatformTextureSettings(android);
        }
    }
}
#endif
