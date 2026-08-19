using MMORPG.Character;
using MMORPG.Input;
using MMORPG.Persistence;
using UnityEngine;

namespace MMORPG.Gameplay
{
    [RequireComponent(typeof(CharacterController))]
    public sealed class KOPlayerController : MonoBehaviour
    {
        [SerializeField] private Transform cameraTransform;
        [SerializeField] private Animator animator;
        [SerializeField] private float walkSpeed = 2.4f;
        [SerializeField] private float runSpeed = 4.8f;
        [SerializeField] private float gravity = -22f;
        [SerializeField] private float turnSpeed = 12f;
        [SerializeField] private float saveIntervalSeconds = 5f;

        private CharacterController controller;
        private GameInputActions actions;
        private CharacterCreationState state;
        private float verticalVelocity;
        private float saveTimer;
        private bool autoRun;
        private bool sitting;
        private bool autoAttack;

        public CharacterCreationState State => state;

        public void Configure(Transform gameplayCamera, Animator characterAnimator = null)
        {
            cameraTransform = gameplayCamera;
            if (characterAnimator != null)
                animator = characterAnimator;
        }

        private void Awake()
        {
            controller = GetComponent<CharacterController>();
            if (animator == null)
                animator = GetComponentInChildren<Animator>(true);
            if (cameraTransform == null && Camera.main != null)
                cameraTransform = Camera.main.transform;

            state = LocalCharacterStore.Load();
            if (state == null)
            {
                Debug.LogError("World scene opened without a saved character.");
                enabled = false;
                return;
            }

            transform.SetPositionAndRotation(
                new Vector3(state.positionX, state.positionY, state.positionZ),
                Quaternion.Euler(0f, state.rotationY, 0f));

            actions = new GameInputActions();
            actions.Enable();
            ApplyAnimatorState(false);
        }

        private void Update()
        {
            if (state == null || actions == null)
                return;

            HandleToggles();
            HandleMovement();
            HandleHotbar();
            HandlePanels();
            HandlePeriodicSave();
        }

        private void HandleToggles()
        {
            if (actions.ConsumeWalkRun())
                state.isRunning = !state.isRunning;

            if (actions.ConsumeAutoRun())
                autoRun = !autoRun;

            if (actions.ConsumeSit())
            {
                sitting = !sitting;
                if (sitting)
                    autoRun = false;
            }

            if (actions.ConsumeAutoAttack())
                autoAttack = !autoAttack;

            if (actions.ConsumeTargetNearest())
                SendMessage("TargetNearest", SendMessageOptions.DontRequireReceiver);
        }

        private void HandleMovement()
        {
            Vector2 input = actions.ReadMove();
            if (autoRun && input.sqrMagnitude < 0.01f)
                input = Vector2.up;
            if (sitting)
                input = Vector2.zero;

            Vector3 forward = cameraTransform != null ? cameraTransform.forward : Vector3.forward;
            Vector3 right = cameraTransform != null ? cameraTransform.right : Vector3.right;
            forward.y = 0f;
            right.y = 0f;
            forward.Normalize();
            right.Normalize();

            Vector3 desired = forward * input.y + right * input.x;
            if (desired.sqrMagnitude > 1f)
                desired.Normalize();

            if (desired.sqrMagnitude > 0.001f)
            {
                Quaternion targetRotation = Quaternion.LookRotation(desired, Vector3.up);
                transform.rotation = Quaternion.Slerp(transform.rotation, targetRotation, turnSpeed * Time.deltaTime);
            }

            float speed = state.isRunning ? runSpeed : walkSpeed;
            if (controller.isGrounded && verticalVelocity < 0f)
                verticalVelocity = -2f;
            verticalVelocity += gravity * Time.deltaTime;

            Vector3 velocity = desired * speed;
            velocity.y = verticalVelocity;
            controller.Move(velocity * Time.deltaTime);

            ApplyAnimatorState(desired.sqrMagnitude > 0.001f);
        }

        private void HandleHotbar()
        {
            int page = actions.ConsumeHotbarPage();
            if (page >= 0)
                state.activeHotbarPage = page;

            int slot = actions.ConsumeHotbarSlot();
            if (slot < 0)
                return;

            HotbarSlotState entry = FindHotbar(state.activeHotbarPage, slot);
            if (entry == null || entry.referenceId <= 0)
                return;

            SendMessage("ActivateHotbarEntry", entry, SendMessageOptions.DontRequireReceiver);
        }

        private void HandlePanels()
        {
            if (actions.ConsumeInventory())
                SendMessage("ToggleInventory", SendMessageOptions.DontRequireReceiver);
            if (actions.ConsumeSkill())
                SendMessage("ToggleSkill", SendMessageOptions.DontRequireReceiver);
            if (actions.ConsumeState())
                SendMessage("ToggleState", SendMessageOptions.DontRequireReceiver);
            if (actions.ConsumeMap())
                SendMessage("ToggleMap", SendMessageOptions.DontRequireReceiver);
        }

        private void HandlePeriodicSave()
        {
            saveTimer += Time.unscaledDeltaTime;
            if (saveTimer < saveIntervalSeconds)
                return;
            saveTimer = 0f;
            SaveNow();
        }

        public void SaveNow()
        {
            if (state == null)
                return;

            Vector3 position = transform.position;
            state.positionX = position.x;
            state.positionY = position.y;
            state.positionZ = position.z;
            state.rotationY = transform.eulerAngles.y;
            LocalCharacterStore.Save(state);
        }

        private void ApplyAnimatorState(bool moving)
        {
            if (animator == null)
                return;

            animator.SetFloat("Speed", moving ? (state != null && state.isRunning ? 1f : 0.5f) : 0f, 0.08f, Time.deltaTime);
            animator.SetBool("Running", state != null && state.isRunning);
            animator.SetBool("Sitting", sitting);
            animator.SetBool("AutoAttack", autoAttack);
        }

        private HotbarSlotState FindHotbar(int page, int slot)
        {
            if (state?.hotbar == null)
                return null;
            foreach (HotbarSlotState entry in state.hotbar)
                if (entry != null && entry.page == page && entry.slot == slot)
                    return entry;
            return null;
        }

        private void OnApplicationPause(bool paused)
        {
            if (paused)
                SaveNow();
        }

        private void OnApplicationFocus(bool focused)
        {
            if (!focused)
                SaveNow();
        }

        private void OnDisable()
        {
            SaveNow();
            actions?.Dispose();
            actions = null;
            MobileInputState.Reset();
        }
    }
}
