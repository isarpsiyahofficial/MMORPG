using UnityEngine;
using UnityEngine.InputSystem.UI;
using UnityEngine.SceneManagement;

namespace MMORPG.Input
{
    public static class InputSystemUiBootstrap
    {
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void InitializeCurrentScene()
        {
            ConfigureModules();
            SceneManager.sceneLoaded -= OnSceneLoaded;
            SceneManager.sceneLoaded += OnSceneLoaded;
        }

        private static void OnSceneLoaded(Scene scene, LoadSceneMode mode)
        {
            ConfigureModules();
        }

        private static void ConfigureModules()
        {
            InputSystemUIInputModule[] modules = Object.FindObjectsByType<InputSystemUIInputModule>(
                FindObjectsInactive.Include,
                FindObjectsSortMode.None
            );

            foreach (InputSystemUIInputModule module in modules)
            {
                if (module == null)
                    continue;

                if (module.actionsAsset == null)
                    module.AssignDefaultActions();
            }
        }
    }
}
