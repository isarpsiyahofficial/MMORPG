using System;
using UnityEngine;
using UnityEngine.InputSystem;

namespace MMORPG.Input
{
    public sealed class GameInputActions : IDisposable
    {
        public InputActionMap Gameplay { get; }
        public InputAction Move { get; }
        public InputAction CameraLook { get; }
        public InputAction TargetNearest { get; }
        public InputAction AutoAttack { get; }
        public InputAction Sit { get; }
        public InputAction WalkRun { get; }
        public InputAction AutoRun { get; }
        public InputAction Inventory { get; }
        public InputAction Skill { get; }
        public InputAction State { get; }
        public InputAction Map { get; }
        public InputAction[] Hotbar { get; } = new InputAction[8];
        public InputAction[] HotbarPage { get; } = new InputAction[8];

        public GameInputActions()
        {
            Gameplay = new InputActionMap("Gameplay");

            Move = Gameplay.AddAction("Move", InputActionType.Value, expectedControlType: "Vector2");
            Move.AddCompositeBinding("2DVector")
                .With("Up", "<Keyboard>/w")
                .With("Down", "<Keyboard>/s")
                .With("Left", "<Keyboard>/a")
                .With("Right", "<Keyboard>/d");
            Move.AddBinding("<Gamepad>/leftStick");

            CameraLook = Gameplay.AddAction("CameraLook", InputActionType.Value, expectedControlType: "Vector2");
            CameraLook.AddBinding("<Mouse>/delta");
            CameraLook.AddBinding("<Gamepad>/rightStick");

            TargetNearest = AddButton("TargetNearest", "<Keyboard>/tab", "<Gamepad>/rightShoulder");
            AutoAttack = AddButton("AutoAttack", "<Keyboard>/r", "<Gamepad>/buttonWest");
            Sit = AddButton("Sit", "<Keyboard>/c", "<Gamepad>/dpad/down");
            WalkRun = AddButton("WalkRun", "<Keyboard>/x", "<Gamepad>/leftStickPress");
            AutoRun = AddButton("AutoRun", "<Keyboard>/numLock", "<Gamepad>/dpad/up");
            Inventory = AddButton("Inventory", "<Keyboard>/i", "<Gamepad>/select");
            Skill = AddButton("Skill", "<Keyboard>/k", "<Gamepad>/buttonNorth");
            State = AddButton("State", "<Keyboard>/u", "<Gamepad>/buttonEast");
            Map = AddButton("Map", "<Keyboard>/m", "<Gamepad>/start");

            string[] digitKeys = { "1", "2", "3", "4", "5", "6", "7", "8" };
            string[] functionKeys = { "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8" };
            for (int i = 0; i < 8; i++)
            {
                Hotbar[i] = AddButton($"Hotbar{i + 1}", $"<Keyboard>/{digitKeys[i]}");
                HotbarPage[i] = AddButton($"HotbarPage{i + 1}", $"<Keyboard>/{functionKeys[i]}");
            }
        }

        public void Enable() => Gameplay.Enable();
        public void Disable() => Gameplay.Disable();

        public Vector2 ReadMove()
        {
            Vector2 hardware = Move.ReadValue<Vector2>();
            Vector2 touch = MobileInputState.Move;
            return touch.sqrMagnitude > hardware.sqrMagnitude ? touch : hardware;
        }

        public Vector2 ConsumeLookDelta(float gamepadScale = 12f)
        {
            Vector2 value = MobileInputState.ConsumeLookDelta();
            Vector2 hardware = CameraLook.ReadValue<Vector2>();
            if (Gamepad.current != null && CameraLook.activeControl?.device is Gamepad)
                hardware *= gamepadScale * Time.unscaledDeltaTime;
            return value + hardware;
        }

        public bool ConsumeTargetNearest() => TargetNearest.WasPressedThisFrame() || MobileInputState.Consume(MobileGameAction.TargetNearest);
        public bool ConsumeAutoAttack() => AutoAttack.WasPressedThisFrame() || MobileInputState.Consume(MobileGameAction.AutoAttack);
        public bool ConsumeSit() => Sit.WasPressedThisFrame() || MobileInputState.Consume(MobileGameAction.Sit);
        public bool ConsumeWalkRun() => WalkRun.WasPressedThisFrame() || MobileInputState.Consume(MobileGameAction.WalkRun);
        public bool ConsumeAutoRun() => AutoRun.WasPressedThisFrame() || MobileInputState.Consume(MobileGameAction.AutoRun);
        public bool ConsumeInventory() => Inventory.WasPressedThisFrame() || MobileInputState.Consume(MobileGameAction.Inventory);
        public bool ConsumeSkill() => Skill.WasPressedThisFrame() || MobileInputState.Consume(MobileGameAction.Skill);
        public bool ConsumeState() => State.WasPressedThisFrame() || MobileInputState.Consume(MobileGameAction.State);
        public bool ConsumeMap() => Map.WasPressedThisFrame() || MobileInputState.Consume(MobileGameAction.Map);

        public int ConsumeHotbarSlot()
        {
            int mobile = MobileInputState.ConsumeHotbarSlot();
            if (mobile >= 0)
                return mobile;
            for (int i = 0; i < Hotbar.Length; i++)
                if (Hotbar[i].WasPressedThisFrame())
                    return i;
            return -1;
        }

        public int ConsumeHotbarPage()
        {
            int mobile = MobileInputState.ConsumeHotbarPage();
            if (mobile >= 0)
                return mobile;
            for (int i = 0; i < HotbarPage.Length; i++)
                if (HotbarPage[i].WasPressedThisFrame())
                    return i;
            return -1;
        }

        public void Dispose()
        {
            Disable();
            Gameplay.Dispose();
        }

        private InputAction AddButton(string name, params string[] bindings)
        {
            InputAction action = Gameplay.AddAction(name, InputActionType.Button);
            foreach (string binding in bindings)
                action.AddBinding(binding);
            return action;
        }
    }
}
