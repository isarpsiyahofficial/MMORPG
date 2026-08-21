namespace MMORPG.Character
{
    public static class CharacterNameRules
    {
        private const string ForbiddenCharacters = "~`!@#$%^&*()-+=|\\<>,.?/{}[]\"' ";

        public static bool IsValid(string characterName)
        {
            if (string.IsNullOrEmpty(characterName))
                return false;

            foreach (char value in characterName)
            {
                if (ForbiddenCharacters.IndexOf(value) >= 0)
                    return false;
            }

            return true;
        }
    }
}
