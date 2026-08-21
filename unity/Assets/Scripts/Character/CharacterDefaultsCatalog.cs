using System;
using UnityEngine;

namespace MMORPG.Character
{
    [Serializable]
    public sealed class CharacterDefaultEntry
    {
        public int id;
        public string name = string.Empty;
        public int strength;
        public int stamina;
        public int dexterity;
        public int intelligence;
        public int magicAttack;
        public int bonus;
        public int race;
        public int classId;
    }

    [Serializable]
    internal sealed class CharacterDefaultsPayload
    {
        public CharacterDefaultEntry[] entries = Array.Empty<CharacterDefaultEntry>();
    }

    public static class CharacterDefaultsCatalog
    {
        private const string ResourcePath = "Data/new_character_values";
        private static CharacterDefaultsPayload cached;

        public static bool TryGet(int race, int characterClass, out CharacterDefaultEntry entry)
        {
            EnsureLoaded();
            int id = race * 10000 + characterClass;

            foreach (CharacterDefaultEntry candidate in cached.entries)
            {
                if (candidate != null && candidate.id == id)
                {
                    entry = candidate;
                    return true;
                }
            }

            entry = null;
            return false;
        }

        private static void EnsureLoaded()
        {
            if (cached != null)
                return;

            TextAsset json = Resources.Load<TextAsset>(ResourcePath);
            if (json == null)
                throw new InvalidOperationException(
                    "Original KO NewChrValue data is missing. Generate Assets/Resources/Data/new_character_values.json before building."
                );

            cached = JsonUtility.FromJson<CharacterDefaultsPayload>(json.text);
            if (cached == null || cached.entries == null || cached.entries.Length == 0)
                throw new InvalidOperationException("Original KO NewChrValue data is empty or invalid.");
        }
    }
}
