using MMORPG.Persistence;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace MMORPG.Core
{
    public sealed class OfflineBootstrap : MonoBehaviour
    {
        [SerializeField] private string characterCreateScene = "CharacterCreate";
        [SerializeField] private string worldScene = "World";
        [SerializeField] private int nation = 2;
        [SerializeField] private string internalAccountId = "offline-mobile";
        [SerializeField] private int characterSlot;
        [SerializeField] private int initialZoneId = 2;
        [SerializeField] private bool openExistingCharacterDirectly = true;

        private void Start()
        {
            OfflineBootstrapProfile profile = new OfflineBootstrapProfile
            {
                nation = nation,
                internalAccountId = internalAccountId,
                characterSlot = characterSlot,
                initialZoneId = initialZoneId,
            };
            OfflineBootstrapSession.Set(profile);

            if (openExistingCharacterDirectly && LocalCharacterStore.Load() != null)
            {
                Load(worldScene);
                return;
            }

            Load(characterCreateScene);
        }

        private static void Load(string sceneName)
        {
            if (string.IsNullOrWhiteSpace(sceneName))
            {
                Debug.LogError("Bootstrap scene name is not configured.");
                return;
            }
            SceneManager.LoadScene(sceneName, LoadSceneMode.Single);
        }
    }
}
