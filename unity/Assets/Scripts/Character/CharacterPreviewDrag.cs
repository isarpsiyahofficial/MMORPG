using UnityEngine;
using UnityEngine.EventSystems;

namespace MMORPG.Character
{
    public sealed class CharacterPreviewDrag : MonoBehaviour, IDragHandler
    {
        [SerializeField] private CharacterCreateController controller;
        [SerializeField] private float degreesPerPixel = 0.25f;

        public void Configure(CharacterCreateController value)
        {
            controller = value;
        }

        public void OnDrag(PointerEventData eventData)
        {
            if (controller != null)
                controller.RotatePreview(eventData.delta.x * degreesPerPixel);
        }
    }
}
