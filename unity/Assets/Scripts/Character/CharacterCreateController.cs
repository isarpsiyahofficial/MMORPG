using MMORPG.Core;
using UnityEngine;

namespace MMORPG.Character
{
    public sealed class CharacterCreateController : MonoBehaviour
    {
        [SerializeField] private CharacterCreationFlow flow;
        [SerializeField] private Transform previewRoot;

        private CharacterCreationState state;
        private int bonusPoints;
        private int maxBonusPoints;

        public CharacterCreationState State => state;
        public int BonusPoints => bonusPoints;
        public int MaxBonusPoints => maxBonusPoints;

        public event System.Action StateChanged;
        public event System.Action<string> ValidationFailed;

        private void Awake()
        {
            if (flow == null)
                flow = GetComponent<CharacterCreationFlow>();

            int nation = OfflineBootstrapSession.Profile.nation;
            state = new CharacterCreationState
            {
                nation = nation,
                zoneId = OfflineBootstrapSession.Profile.initialZoneId,
                face = 0,
                hair = 0,
            };
        }

        public int[] GetRaceOptions()
        {
            return state.nation == 1
                ? new[] { 1, 2, 3, 4 }
                : new[] { 11, 12, 13 };
        }

        public int[] GetClassOptions()
        {
            return state.nation == 1
                ? new[] { 101, 102, 103, 104 }
                : new[] { 201, 202, 203, 204 };
        }

        public bool IsClassAvailable(int characterClass)
        {
            if (state.race <= 0 || !Contains(GetClassOptions(), characterClass))
                return false;

            int kind = characterClass % 100;
            return state.race switch
            {
                11 => kind == 1,                              // El Morad barbarian: warrior
                12 => kind >= 1 && kind <= 4,                // El Morad male: all base classes
                13 => kind >= 1 && kind <= 4,                // El Morad female: all base classes
                1 => kind == 1,                               // Karus Arktuarek: warrior
                2 => kind == 2 || kind == 4,                 // Karus Tuarek: rogue/priest
                3 => kind == 3,                               // Karus Wrinkle Tuarek: mage
                4 => kind == 4,                               // Karus Puri Tuarek: priest
                _ => false,
            };
        }

        public bool SelectRace(int race)
        {
            if (!Contains(GetRaceOptions(), race))
                return Reject("Selected race is not valid for this nation.");

            if (state.race == race)
                return true;

            state.race = race;
            state.characterClass = 0;
            ResetStats();
            Notify();
            return true;
        }

        public bool SelectClass(int characterClass)
        {
            if (state.race <= 0)
                return Reject("Select a race first.");
            if (!IsClassAvailable(characterClass))
                return Reject("This class is disabled for the selected race in the original KO rules.");
            if (!CharacterDefaultsCatalog.TryGet(state.race, characterClass, out CharacterDefaultEntry defaults))
                return Reject("Original KO NewChrValue data does not contain this race/class combination.");

            state.characterClass = characterClass;
            ApplyDefaults(defaults);
            Notify();
            return true;
        }

        public void SetName(string value)
        {
            state.characterName = value?.Trim() ?? string.Empty;
            Notify();
        }

        public void FaceLeft() => SetFace(state.face - 1);
        public void FaceRight() => SetFace(state.face + 1);
        public void HairLeft() => SetHair(state.hair - 1);
        public void HairRight() => SetHair(state.hair + 1);

        public bool AddStatPoint(int statIndex)
        {
            if (bonusPoints <= 0)
                return false;

            switch (statIndex)
            {
                case 0: state.strength++; break;
                case 1: state.stamina++; break;
                case 2: state.dexterity++; break;
                case 3: state.intelligence++; break;
                case 4: state.magicAttack++; break;
                default: return false;
            }

            bonusPoints--;
            Notify();
            return true;
        }

        public bool RemoveStatPoint(int statIndex)
        {
            if (!CharacterDefaultsCatalog.TryGet(state.race, state.characterClass, out CharacterDefaultEntry defaults))
                return false;

            bool changed = statIndex switch
            {
                0 when state.strength > defaults.strength => --state.strength >= 0,
                1 when state.stamina > defaults.stamina => --state.stamina >= 0,
                2 when state.dexterity > defaults.dexterity => --state.dexterity >= 0,
                3 when state.intelligence > defaults.intelligence => --state.intelligence >= 0,
                4 when state.magicAttack > defaults.magicAttack => --state.magicAttack >= 0,
                _ => false,
            };

            if (!changed)
                return false;

            bonusPoints = Mathf.Min(maxBonusPoints, bonusPoints + 1);
            Notify();
            return true;
        }

        public bool CreateCharacter()
        {
            if (!CharacterNameRules.IsValid(state.characterName))
                return Reject("Character name contains a forbidden character or is empty.");
            if (state.race <= 0)
                return Reject("Race is not selected.");
            if (state.characterClass <= 0 || !IsClassAvailable(state.characterClass))
                return Reject("Class is not selected or is invalid for this race.");
            if (bonusPoints > 0)
                return Reject("Spend all bonus stat points before creating the character.");
            if (!state.IsValidForPhaseZero())
                return Reject("Character state is incomplete.");
            if (flow == null)
                return Reject("Character creation flow is not configured.");

            flow.Complete(state);
            return true;
        }

        public void RotatePreview(float deltaDegrees)
        {
            if (previewRoot != null)
                previewRoot.Rotate(0f, -deltaDegrees, 0f, Space.World);
        }

        public void SetPreviewRoot(Transform value)
        {
            previewRoot = value;
        }

        private void SetFace(int value)
        {
            int clamped = Mathf.Clamp(value, 0, 3);
            if (clamped == state.face)
                return;
            state.face = clamped;
            Notify();
        }

        private void SetHair(int value)
        {
            int clamped = Mathf.Clamp(value, 0, 2);
            if (clamped == state.hair)
                return;
            state.hair = clamped;
            Notify();
        }

        private void ApplyDefaults(CharacterDefaultEntry defaults)
        {
            state.strength = defaults.strength;
            state.stamina = defaults.stamina;
            state.dexterity = defaults.dexterity;
            state.intelligence = defaults.intelligence;
            state.magicAttack = defaults.magicAttack;
            bonusPoints = defaults.bonus;
            maxBonusPoints = defaults.bonus;
        }

        private void ResetStats()
        {
            state.strength = 0;
            state.stamina = 0;
            state.dexterity = 0;
            state.intelligence = 0;
            state.magicAttack = 0;
            bonusPoints = 0;
            maxBonusPoints = 0;
        }

        private bool Reject(string reason)
        {
            ValidationFailed?.Invoke(reason);
            return false;
        }

        private void Notify()
        {
            StateChanged?.Invoke();
        }

        private static bool Contains(int[] values, int value)
        {
            foreach (int candidate in values)
                if (candidate == value)
                    return true;
            return false;
        }
    }
}
