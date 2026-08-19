using System;
using UnityEngine;

namespace MMORPG.Core
{
    [Serializable]
    public sealed class OfflineBootstrapProfile
    {
        public int nation = 2;
        public string internalAccountId = "offline-mobile";
        public int characterSlot;
        public int initialZoneId = 1;

        public void Validate()
        {
            if (nation != 1 && nation != 2)
                nation = 2;
            if (characterSlot < 0 || characterSlot > 2)
                characterSlot = 0;
            if (initialZoneId <= 0)
                initialZoneId = 1;
            if (string.IsNullOrWhiteSpace(internalAccountId))
                internalAccountId = "offline-mobile";
        }
    }

    public static class OfflineBootstrapSession
    {
        public static OfflineBootstrapProfile Profile { get; private set; } = new OfflineBootstrapProfile();

        public static void Set(OfflineBootstrapProfile profile)
        {
            Profile = profile ?? new OfflineBootstrapProfile();
            Profile.Validate();
        }
    }
}
