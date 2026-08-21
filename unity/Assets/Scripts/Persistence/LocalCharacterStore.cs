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
        public static string BackupPath => FilePath + ".bak";
        public static string TempPath => FilePath + ".tmp";

        public static bool Exists()
        {
            return File.Exists(FilePath) || File.Exists(BackupPath);
        }

        public static void Save(CharacterCreationState state)
        {
            if (state == null)
                throw new ArgumentNullException(nameof(state));

            state.NormalizeAfterLoad();
            if (!state.IsValidForPhaseZero())
                throw new InvalidOperationException("Character state is incomplete.");

            Directory.CreateDirectory(Application.persistentDataPath);
            string json = JsonUtility.ToJson(state, true);

            File.WriteAllText(TempPath, json);
            ValidateFile(TempPath);

            if (!File.Exists(FilePath))
            {
                File.Move(TempPath, FilePath);
                return;
            }

            try
            {
                File.Replace(TempPath, FilePath, BackupPath);
            }
            catch (PlatformNotSupportedException)
            {
                FallbackReplace();
            }
            catch (IOException)
            {
                FallbackReplace();
            }
        }

        public static CharacterCreationState Load()
        {
            CharacterCreationState primary = TryLoad(FilePath);
            if (primary != null)
                return primary;

            CharacterCreationState backup = TryLoad(BackupPath);
            if (backup == null)
                return null;

            try
            {
                string recoveredJson = JsonUtility.ToJson(backup, true);
                File.WriteAllText(TempPath, recoveredJson);
                if (File.Exists(FilePath))
                    File.Delete(FilePath);
                File.Move(TempPath, FilePath);
            }
            catch (Exception exc)
            {
                Debug.LogWarning($"Recovered backup loaded but primary save could not be restored: {exc.Message}");
            }

            return backup;
        }

        public static void Delete()
        {
            DeleteIfExists(FilePath);
            DeleteIfExists(BackupPath);
            DeleteIfExists(TempPath);
        }

        private static void FallbackReplace()
        {
            if (File.Exists(FilePath))
                File.Copy(FilePath, BackupPath, true);

            File.Delete(FilePath);
            File.Move(TempPath, FilePath);
        }

        private static CharacterCreationState TryLoad(string path)
        {
            if (!File.Exists(path))
                return null;

            try
            {
                string json = File.ReadAllText(path);
                CharacterCreationState state = JsonUtility.FromJson<CharacterCreationState>(json);
                if (state == null)
                    return null;
                state.NormalizeAfterLoad();
                return state.IsValidForPhaseZero() ? state : null;
            }
            catch (Exception exc)
            {
                Debug.LogWarning($"Character save is unreadable: {path}. {exc.Message}");
                return null;
            }
        }

        private static void ValidateFile(string path)
        {
            CharacterCreationState state = TryLoad(path);
            if (state == null)
                throw new InvalidDataException("Temporary character save failed validation.");
        }

        private static void DeleteIfExists(string path)
        {
            if (File.Exists(path))
                File.Delete(path);
        }
    }
}
