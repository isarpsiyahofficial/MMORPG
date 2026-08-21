using MMORPG.Gameplay;
using MMORPG.Input;
using UnityEngine;

namespace MMORPG.Core
{
    public sealed class AndroidLifecycle : MonoBehaviour
    {
        private void Awake()
        {
            Application.lowMemory += OnLowMemory;
        }

        private void OnDestroy()
        {
            Application.lowMemory -= OnLowMemory;
        }

        private void OnApplicationPause(bool paused)
        {
            if (paused)
            {
                SavePlayer();
                MobileInputState.Reset();
                AudioListener.pause = true;
            }
            else
            {
                AudioListener.pause = false;
            }
        }

        private void OnApplicationFocus(bool focused)
        {
            if (!focused)
            {
                SavePlayer();
                MobileInputState.Reset();
            }
        }

        private static void OnLowMemory()
        {
            Resources.UnloadUnusedAssets();
            System.GC.Collect();
        }

        private static void SavePlayer()
        {
            KOPlayerController player = FindFirstObjectByType<KOPlayerController>();
            if (player != null)
                player.SaveNow();
        }
    }
}
