using System;

namespace MMORPG.Character
{
    [Serializable]
    public sealed class CharacterCreationState
    {
        public const int CurrentSchemaVersion = 2;

        public int schemaVersion = CurrentSchemaVersion;
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
        public int currentHp;
        public int currentMp;
        public int zoneId = 1;
        public float positionX;
        public float positionY;
        public float positionZ;
        public float rotationY;
        public bool isRunning = true;
        public InventorySlotState[] inventory = Array.Empty<InventorySlotState>();
        public EquipmentSlotState[] equipment = Array.Empty<EquipmentSlotState>();
        public LearnedSkillState[] skills = Array.Empty<LearnedSkillState>();
        public HotbarSlotState[] hotbar = Array.Empty<HotbarSlotState>();
        public int activeHotbarPage;

        public bool IsValidForPhaseZero()
        {
            return CharacterNameRules.IsValid(characterName)
                   && (nation == 1 || nation == 2)
                   && race > 0
                   && characterClass > 0
                   && strength >= 0
                   && stamina >= 0
                   && dexterity >= 0
                   && intelligence >= 0
                   && magicAttack >= 0
                   && level >= 1
                   && zoneId > 0
                   && activeHotbarPage >= 0
                   && activeHotbarPage < 8;
        }

        public void NormalizeAfterLoad()
        {
            if (schemaVersion <= 0)
                schemaVersion = 1;
            inventory ??= Array.Empty<InventorySlotState>();
            equipment ??= Array.Empty<EquipmentSlotState>();
            skills ??= Array.Empty<LearnedSkillState>();
            hotbar ??= Array.Empty<HotbarSlotState>();
            if (activeHotbarPage < 0 || activeHotbarPage > 7)
                activeHotbarPage = 0;
            schemaVersion = CurrentSchemaVersion;
        }
    }

    [Serializable]
    public sealed class InventorySlotState
    {
        public int slot;
        public int itemId;
        public int count = 1;
        public int durability;
        public long serial;
    }

    [Serializable]
    public sealed class EquipmentSlotState
    {
        public int slot;
        public int itemId;
        public int durability;
        public long serial;
    }

    [Serializable]
    public sealed class LearnedSkillState
    {
        public int skillId;
        public int rank;
    }

    [Serializable]
    public sealed class HotbarSlotState
    {
        public int page;
        public int slot;
        public string kind = string.Empty;
        public int referenceId;
    }
}
