using UnityEngine;
using UnityEngine.SceneManagement;

namespace MMORPG.Core
{
    /// <summary>
    /// Phase-0 startup path. The Android test build intentionally bypasses
    /// account login and server selection and opens the character creation scene.
    /// </summary>
    public sealed class OfflineBootstrap : MonoBehaviour
    {
        [SerializeField] private string characterCreateScene = "CharacterCreate";

        private void Start()
        {
            if (string.IsNullOrWhiteSpace(characterCreateScene))
            {
                Debug.LogError("CharacterCreate scene name is not configured.");
                return;
            }

            SceneManager.LoadScene(characterCreateScene, LoadSceneMode.Single);
        }
    }
}
