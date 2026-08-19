using UnityEngine;
using UnityEngine.EventSystems;

namespace MMORPG.Input
{
    public sealed class TouchJoystick : MonoBehaviour, IPointerDownHandler, IDragHandler, IPointerUpHandler
    {
        [SerializeField] private RectTransform background;
        [SerializeField] private RectTransform handle;
        [SerializeField, Range(0.2f, 1f)] private float handleRange = 0.6f;

        public void Configure(RectTransform backgroundRect, RectTransform handleRect, float range = 0.6f)
        {
            background = backgroundRect;
            handle = handleRect;
            handleRange = Mathf.Clamp(range, 0.2f, 1f);
        }

        public void OnPointerDown(PointerEventData eventData)
        {
            OnDrag(eventData);
        }

        public void OnDrag(PointerEventData eventData)
        {
            if (background == null)
                return;

            if (!RectTransformUtility.ScreenPointToLocalPointInRectangle(
                    background,
                    eventData.position,
                    eventData.pressEventCamera,
                    out Vector2 localPoint))
                return;

            Rect rect = background.rect;
            Vector2 halfSize = rect.size * 0.5f;
            if (halfSize.x <= 0f || halfSize.y <= 0f)
                return;

            Vector2 normalized = new Vector2(localPoint.x / halfSize.x, localPoint.y / halfSize.y);
            normalized = Vector2.ClampMagnitude(normalized, 1f);
            MobileInputState.SetMove(normalized);

            if (handle != null)
                handle.anchoredPosition = new Vector2(normalized.x * halfSize.x, normalized.y * halfSize.y) * handleRange;
        }

        public void OnPointerUp(PointerEventData eventData)
        {
            MobileInputState.SetMove(Vector2.zero);
            if (handle != null)
                handle.anchoredPosition = Vector2.zero;
        }

        private void OnDisable()
        {
            MobileInputState.SetMove(Vector2.zero);
            if (handle != null)
                handle.anchoredPosition = Vector2.zero;
        }
    }
}
