using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace MMORPG.UI
{
    public sealed class LegacyUiButtonVisual : MonoBehaviour,
        IPointerDownHandler, IPointerUpHandler, IPointerEnterHandler, IPointerExitHandler
    {
        // CN3UIButton::eBTN_STATE stored in each child image's reserved field.
        private const int Normal = 0;
        private const int Down = 1;
        private const int On = 2;
        private const int Disabled = 3;

        private readonly Dictionary<int, Graphic> states = new Dictionary<int, Graphic>();
        private bool pointerInside;

        public void RegisterState(int reservedState, Graphic graphic)
        {
            if (graphic == null)
                return;
            states[reservedState] = graphic;
            ShowState(Normal);
        }

        public void OnPointerDown(PointerEventData eventData)
        {
            ShowState(Down);
        }

        public void OnPointerUp(PointerEventData eventData)
        {
            ShowState(pointerInside ? On : Normal);
        }

        public void OnPointerEnter(PointerEventData eventData)
        {
            pointerInside = true;
            if (!eventData.dragging)
                ShowState(On);
        }

        public void OnPointerExit(PointerEventData eventData)
        {
            pointerInside = false;
            if (!eventData.dragging)
                ShowState(Normal);
        }

        public void SetInteractable(bool interactable)
        {
            ShowState(interactable ? Normal : Disabled);
        }

        private void ShowState(int requested)
        {
            if (states.Count == 0)
                return;

            int selected = states.ContainsKey(requested)
                ? requested
                : states.ContainsKey(Normal)
                    ? Normal
                    : FirstKey();

            foreach (KeyValuePair<int, Graphic> pair in states)
                pair.Value.enabled = pair.Key == selected;
        }

        private int FirstKey()
        {
            foreach (int key in states.Keys)
                return key;
            return Normal;
        }
    }
}
