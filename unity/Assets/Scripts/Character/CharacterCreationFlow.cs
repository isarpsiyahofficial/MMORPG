using MMORPG.Persistence;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace MMORPG.Character
{
    /// <summary>
    /// Receives the values selected on the KO-style character creation screen,
    /// saves them locally without SQL, then enters the test world.
    /// </summary>
    public sealed class CharacterCreationFlow : MonoBehaviour
    {
        [SerializeField] private string worldScene = "World";

        public void Complete(CharacterCreationState state)
        {
            if (state == null || !state.IsValidForPhaseZero())
            {
                Debug.LogError("Character creation is incomplete.");
                return;
            }

            LocalCharacterStore.Save(state);
            SceneManager.LoadScene(worldScene, LoadSceneMode.Single);
        }
    }
}
