using MMORPG.Input;
using UnityEngine;

namespace MMORPG.Gameplay
{
    public sealed class OrbitCameraController : MonoBehaviour
    {
        [SerializeField] private Transform target;
        [SerializeField] private float distance = 5.5f;
        [SerializeField] private float minDistance = 2.5f;
        [SerializeField] private float maxDistance = 9f;
        [SerializeField] private float yaw = 180f;
        [SerializeField] private float pitch = 18f;
        [SerializeField] private float minPitch = -10f;
        [SerializeField] private float maxPitch = 55f;
        [SerializeField] private float lookSensitivity = 0.18f;
        [SerializeField] private Vector3 targetOffset = new Vector3(0f, 1.35f, 0f);
        [SerializeField] private LayerMask collisionMask = ~0;
        [SerializeField] private float collisionRadius = 0.2f;

        private GameInputActions actions;

        public void SetTarget(Transform value)
        {
            target = value;
        }

        private void OnEnable()
        {
            actions = new GameInputActions();
            actions.Enable();
        }

        private void LateUpdate()
        {
            if (target == null || actions == null)
                return;

            Vector2 look = actions.ConsumeLookDelta();
            yaw += look.x * lookSensitivity;
            pitch = Mathf.Clamp(pitch - look.y * lookSensitivity, minPitch, maxPitch);

            Quaternion rotation = Quaternion.Euler(pitch, yaw, 0f);
            Vector3 pivot = target.position + targetOffset;
            Vector3 desired = pivot - rotation * Vector3.forward * distance;

            Vector3 direction = desired - pivot;
            float desiredDistance = direction.magnitude;
            if (desiredDistance > 0.001f)
            {
                direction /= desiredDistance;
                if (Physics.SphereCast(pivot, collisionRadius, direction, out RaycastHit hit, desiredDistance, collisionMask, QueryTriggerInteraction.Ignore))
                    desired = pivot + direction * Mathf.Max(0.1f, hit.distance - collisionRadius);
            }

            transform.SetPositionAndRotation(desired, rotation);
        }

        public void AddZoom(float delta)
        {
            distance = Mathf.Clamp(distance - delta, minDistance, maxDistance);
        }

        private void OnDisable()
        {
            actions?.Dispose();
            actions = null;
        }
    }
}
