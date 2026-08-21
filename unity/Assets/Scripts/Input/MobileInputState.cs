using System;
using UnityEngine;

namespace MMORPG.Input
{
    [Flags]
    public enum MobileGameAction
    {
        None = 0,
        TargetNearest = 1 << 0,
        AutoAttack = 1 << 1,
        Sit = 1 << 2,
        WalkRun = 1 << 3,
        AutoRun = 1 << 4,
        Inventory = 1 << 5,
        Skill = 1 << 6,
        State = 1 << 7,
        Map = 1 << 8,
    }

    public static class MobileInputState
    {
        private static MobileGameAction pressedActions;
        private static int pressedHotbarSlot = -1;
        private static int requestedHotbarPage = -1;

        public static Vector2 Move { get; private set; }
        public static Vector2 LookDelta { get; private set; }

        public static void SetMove(Vector2 value)
        {
            Move = Vector2.ClampMagnitude(value, 1f);
        }

        public static void AddLookDelta(Vector2 value)
        {
            LookDelta += value;
        }

        public static Vector2 ConsumeLookDelta()
        {
            Vector2 value = LookDelta;
            LookDelta = Vector2.zero;
            return value;
        }

        public static void Press(MobileGameAction action)
        {
            pressedActions |= action;
        }

        public static bool Consume(MobileGameAction action)
        {
            bool pressed = (pressedActions & action) != 0;
            pressedActions &= ~action;
            return pressed;
        }

        public static void PressHotbarSlot(int slot)
        {
            if (slot >= 0 && slot < 8)
                pressedHotbarSlot = slot;
        }

        public static int ConsumeHotbarSlot()
        {
            int value = pressedHotbarSlot;
            pressedHotbarSlot = -1;
            return value;
        }

        public static void RequestHotbarPage(int page)
        {
            if (page >= 0 && page < 8)
                requestedHotbarPage = page;
        }

        public static int ConsumeHotbarPage()
        {
            int value = requestedHotbarPage;
            requestedHotbarPage = -1;
            return value;
        }

        public static void Reset()
        {
            Move = Vector2.zero;
            LookDelta = Vector2.zero;
            pressedActions = MobileGameAction.None;
            pressedHotbarSlot = -1;
            requestedHotbarPage = -1;
        }
    }
}
