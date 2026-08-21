using System;
using System.Collections.Generic;
using UnityEngine;

namespace MMORPG.Legacy
{
    public sealed class KoSkeletonAnimator : MonoBehaviour
    {
        private KoJointNode[] joints = Array.Empty<KoJointNode>();
        private Transform[] bones = Array.Empty<Transform>();
        private KoAnimationControlData control;
        private int animationIndex = -1;
        private float frame;
        private bool loop = true;

        public int AnimationIndex => animationIndex;
        public float Frame => frame;

        public void Initialize(KoJointNode rootJoint, Transform[] depthFirstBones, KoAnimationControlData animationControl)
        {
            List<KoJointNode> flattened = new List<KoJointNode>();
            Flatten(rootJoint, flattened);
            if (depthFirstBones == null || flattened.Count != depthFirstBones.Length)
                throw new InvalidOperationException(
                    $"KO skeleton mapping mismatch: joints={flattened.Count}, bones={depthFirstBones?.Length ?? 0}"
                );

            joints = flattened.ToArray();
            bones = depthFirstBones;
            control = animationControl;
            Play(0, true, true);
        }

        public void Play(int index, bool shouldLoop = true, bool restart = false)
        {
            if (control?.animations == null || index < 0 || index >= control.animations.Length)
                return;
            if (!restart && animationIndex == index)
                return;

            animationIndex = index;
            loop = shouldLoop;
            frame = control.animations[index].frameStart;
            ApplyFrame(frame);
        }

        private void Update()
        {
            if (control?.animations == null || animationIndex < 0 || animationIndex >= control.animations.Length)
                return;

            KoAnimationMeta clip = control.animations[animationIndex];
            float fps = clip.framesPerSecond > 0f ? clip.framesPerSecond : 30f;
            frame += Time.deltaTime * fps;

            if (frame > clip.frameEnd)
            {
                if (loop)
                {
                    float length = Mathf.Max(0.001f, clip.frameEnd - clip.frameStart);
                    frame = clip.frameStart + Mathf.Repeat(frame - clip.frameStart, length);
                }
                else
                {
                    frame = clip.frameEnd;
                }
            }

            ApplyFrame(frame);
        }

        public void ApplyFrame(float absoluteFrame)
        {
            int count = Mathf.Min(joints.Length, bones.Length);
            for (int i = 0; i < count; i++)
            {
                KoJointNode joint = joints[i];
                Transform bone = bones[i];
                if (joint == null || bone == null)
                    continue;

                Vector3 position = SampleVector(joint.positionKeys, absoluteFrame, joint.position);
                Quaternion rotation = SampleQuaternion(joint.rotationKeys, absoluteFrame, joint.rotation);
                Vector3 scale = SampleVector(joint.scaleKeys, absoluteFrame, joint.scale);
                if (joint.orientKeys != null && joint.orientKeys.count > 0)
                    rotation *= SampleQuaternion(joint.orientKeys, absoluteFrame, Quaternion.identity);

                bone.localPosition = position;
                bone.localRotation = rotation;
                bone.localScale = scale;
            }
        }

        private static Vector3 SampleVector(KoAnimKeyData key, float frame, Vector3 fallback)
        {
            if (key == null || key.count <= 0 || key.vectors == null || key.vectors.Length == 0)
                return fallback;

            SamplePosition(key, frame, out int index, out float delta);
            if (index < 0 || index >= key.vectors.Length)
                return fallback;
            int next = Mathf.Min(index + 1, key.vectors.Length - 1);
            return delta > 0f ? Vector3.LerpUnclamped(key.vectors[index], key.vectors[next], delta) : key.vectors[index];
        }

        private static Quaternion SampleQuaternion(KoAnimKeyData key, float frame, Quaternion fallback)
        {
            if (key == null || key.count <= 0 || key.quaternions == null || key.quaternions.Length == 0)
                return fallback;

            SamplePosition(key, frame, out int index, out float delta);
            if (index < 0 || index >= key.quaternions.Length)
                return fallback;
            int next = Mathf.Min(index + 1, key.quaternions.Length - 1);
            return delta > 0f
                ? Quaternion.SlerpUnclamped(key.quaternions[index], key.quaternions[next], delta)
                : key.quaternions[index];
        }

        private static void SamplePosition(KoAnimKeyData key, float frame, out int index, out float delta)
        {
            float sampling = key.samplingRate > 0f ? key.samplingRate : 30f;
            index = Mathf.FloorToInt(frame * (sampling / 30f));
            if (index < 0)
            {
                index = 0;
                delta = 0f;
                return;
            }
            if (index >= key.count)
            {
                index = key.count - 1;
                delta = 0f;
                return;
            }

            float frameDistance = 30f / sampling;
            delta = frameDistance > 0f ? (frame - index * frameDistance) / frameDistance : 0f;
            delta = Mathf.Clamp01(delta);
        }

        private static void Flatten(KoJointNode joint, List<KoJointNode> output)
        {
            if (joint == null)
                return;
            output.Add(joint);
            if (joint.children == null)
                return;
            foreach (KoJointNode child in joint.children)
                Flatten(child, output);
        }
    }
}
