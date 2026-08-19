using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace MMORPG.UI
{
    public sealed class LegacyUiButtonVisual : MonoBehaviour,
        IPointerDownHandler, IPointerUpHandler, IPointerEnterHandler, IPointerExitHandler
    {
        private readonly Dictionary<int, Graphic> states = new Dictionary<int, Graphic>();
        private bool pointerInside;

        public void RegisterState(int reservedState, Graphic graphic)
        {
            if (graphic == null)
                return;
            states[reservedState] = graphic;
            ShowState(2); // UI_STATE_BUTTON_NORMAL
        }

        public void OnPointerDown(PointerEventData eventData)
        {
            ShowState(3); // UI_STATE_BUTTON_DOWN
        }

        public void OnPointerUp(PointerEventData eventData)
        {
            ShowState(pointerInside ? 6 : 2); // ON : NORMAL
        }

        public void OnPointerEnter(PointerEventData eventData)
        {
            pointerInside = true;
            if (!eventData.dragging)
                ShowState(6); // UI_STATE_BUTTON_ON
        }

        public void OnPointerExit(PointerEventData eventData)
        {
            pointerInside = false;
            if (!eventData.dragging)
                ShowState(2);
        }

        public void SetInteractable(bool interactable)
        {
            ShowState(interactable ? 2 : 7); // NORMAL : DISABLE
        }

        private void ShowState(int requested)
        {
            if (states.Count == 0)
                return;

            int selected = states.ContainsKey(requested)
                ? requested
                : states.ContainsKey(2)
                    ? 2
                    : FirstKey();

            foreach (KeyValuePair<int, Graphic> pair in states)
                pair.Value.enabled = pair.Key == selected;
        }

        private int FirstKey()
        {
            foreach (int key in states.Keys)
                return key;
            return 0;
        }
    }
}
