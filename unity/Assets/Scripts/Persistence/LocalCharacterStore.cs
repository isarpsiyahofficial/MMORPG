using System;
using System.IO;
using MMORPG.Character;
using UnityEngine;

namespace MMORPG.Persistence
{
    public static class LocalCharacterStore
    {
        private const string FileName = "phase0-character.json";

        public static string FilePath => Path.Combine(Application.persistentDataPath, FileName);

        public static bool Exists()
        {
            return File.Exists(FilePath);
        }

        public static void Save(CharacterCreationState state)
        {
            if (state == null)
                throw new ArgumentNullException(nameof(state));

            if (!state.IsValidForPhaseZero())
                throw new InvalidOperationException("Character state is incomplete.");

            Directory.CreateDirectory(Application.persistentDataPath);

            string json = JsonUtility.ToJson(state, true);
            string tempPath = FilePath + ".tmp";

            File.WriteAllText(tempPath, json);

            if (File.Exists(FilePath))
                File.Delete(FilePath);

            File.Move(tempPath, FilePath);
        }

        public static CharacterCreationState Load()
        {
            if (!Exists())
                return null;

            string json = File.ReadAllText(FilePath);
            CharacterCreationState state = JsonUtility.FromJson<CharacterCreationState>(json);

            if (state == null || !state.IsValidForPhaseZero())
                throw new InvalidDataException("Saved character file is invalid.");

            return state;
        }

        public static void Delete()
        {
            if (Exists())
                File.Delete(FilePath);
        }
    }
}
