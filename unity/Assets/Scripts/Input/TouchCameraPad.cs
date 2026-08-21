using UnityEngine;
using UnityEngine.EventSystems;

namespace MMORPG.Input
{
    public sealed class TouchCameraPad : MonoBehaviour, IDragHandler
    {
        [SerializeField] private float sensitivity = 0.12f;

        public void Configure(float value)
        {
            sensitivity = Mathf.Max(0.001f, value);
        }

        public void OnDrag(PointerEventData eventData)
        {
            MobileInputState.AddLookDelta(eventData.delta * sensitivity);
        }
    }
}
