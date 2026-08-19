using System;

namespace MMORPG.Character
{
    [Serializable]
    public sealed class CharacterCreationState
    {
        public string characterName = string.Empty;
        public int nation;
        public int race;
        public int characterClass;
        public int face;
        public int hair;
        public int strength;
        public int stamina;
        public int dexterity;
        public int intelligence;
        public int magicAttack;
        public int level = 1;
        public long experience;
        public long gold;
        public int zoneId = 1;
        public float positionX;
        public float positionY;
        public float positionZ;

        public bool IsValidForPhaseZero()
        {
            return !string.IsNullOrWhiteSpace(characterName)
                   && race > 0
                   && characterClass > 0
                   && strength >= 0
                   && stamina >= 0
                   && dexterity >= 0
                   && intelligence >= 0
                   && magicAttack >= 0;
        }
    }
}
