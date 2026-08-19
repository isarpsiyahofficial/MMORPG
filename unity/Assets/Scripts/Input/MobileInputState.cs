using UnityEngine;

namespace MMORPG.Input
{
    public static class MobileInputState
    {
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

        public static void Reset()
        {
            Move = Vector2.zero;
            LookDelta = Vector2.zero;
        }
    }
}
