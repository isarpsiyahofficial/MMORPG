using UnityEngine;
using UnityEngine.EventSystems;

namespace MMORPG.Input
{
    public enum MobileActionButtonKind
    {
        TargetNearest,
        AutoAttack,
        Sit,
        WalkRun,
        AutoRun,
        Inventory,
        Skill,
        State,
        Map,
        HotbarSlot,
        HotbarPage,
    }

    public sealed class MobileActionButton : MonoBehaviour, IPointerDownHandler
    {
        [SerializeField] private MobileActionButtonKind kind;
        [SerializeField, Range(0, 7)] private int index;

        public void Configure(MobileActionButtonKind actionKind, int actionIndex = 0)
        {
            kind = actionKind;
            index = Mathf.Clamp(actionIndex, 0, 7);
        }

        public void OnPointerDown(PointerEventData eventData)
        {
            switch (kind)
            {
                case MobileActionButtonKind.TargetNearest:
                    MobileInputState.Press(MobileGameAction.TargetNearest);
                    break;
                case MobileActionButtonKind.AutoAttack:
                    MobileInputState.Press(MobileGameAction.AutoAttack);
                    break;
                case MobileActionButtonKind.Sit:
                    MobileInputState.Press(MobileGameAction.Sit);
                    break;
                case MobileActionButtonKind.WalkRun:
                    MobileInputState.Press(MobileGameAction.WalkRun);
                    break;
                case MobileActionButtonKind.AutoRun:
                    MobileInputState.Press(MobileGameAction.AutoRun);
                    break;
                case MobileActionButtonKind.Inventory:
                    MobileInputState.Press(MobileGameAction.Inventory);
                    break;
                case MobileActionButtonKind.Skill:
                    MobileInputState.Press(MobileGameAction.Skill);
                    break;
                case MobileActionButtonKind.State:
                    MobileInputState.Press(MobileGameAction.State);
                    break;
                case MobileActionButtonKind.Map:
                    MobileInputState.Press(MobileGameAction.Map);
                    break;
                case MobileActionButtonKind.HotbarSlot:
                    MobileInputState.PressHotbarSlot(index);
                    break;
                case MobileActionButtonKind.HotbarPage:
                    MobileInputState.RequestHotbarPage(index);
                    break;
            }
        }
    }
}
