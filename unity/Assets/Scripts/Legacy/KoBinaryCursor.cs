using System;
using System.IO;
using System.Text;
using UnityEngine;

namespace MMORPG.Legacy
{
    public sealed class KoBinaryCursor
    {
        private readonly byte[] data;
        private int offset;

        public KoBinaryCursor(byte[] bytes)
        {
            data = bytes ?? throw new ArgumentNullException(nameof(bytes));
        }

        public int Position => offset;
        public int Remaining => data.Length - offset;
        public int Length => data.Length;

        public void Skip(int count)
        {
            Require(count);
            offset += count;
        }

        public byte ReadUInt8()
        {
            Require(1);
            return data[offset++];
        }

        public short ReadInt16()
        {
            Require(2);
            short value = (short)(data[offset] | (data[offset + 1] << 8));
            offset += 2;
            return value;
        }

        public ushort ReadUInt16()
        {
            return unchecked((ushort)ReadInt16());
        }

        public int ReadInt32()
        {
            Require(4);
            int value = data[offset]
                        | (data[offset + 1] << 8)
                        | (data[offset + 2] << 16)
                        | (data[offset + 3] << 24);
            offset += 4;
            return value;
        }

        public uint ReadUInt32()
        {
            return unchecked((uint)ReadInt32());
        }

        public float ReadSingle()
        {
            Require(4);
            if (!BitConverter.IsLittleEndian)
            {
                byte[] temp = ReadBytes(4);
                Array.Reverse(temp);
                return BitConverter.ToSingle(temp, 0);
            }
            float value = BitConverter.ToSingle(data, offset);
            offset += 4;
            return value;
        }

        public Vector2 ReadUvDirectXToUnity()
        {
            float u = ReadSingle();
            float v = ReadSingle();
            return new Vector2(u, 1f - v);
        }

        public Vector3 ReadVector3()
        {
            return new Vector3(ReadSingle(), ReadSingle(), ReadSingle());
        }

        public Quaternion ReadQuaternion()
        {
            return new Quaternion(ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle());
        }

        public Matrix4x4 ReadMatrix4x4RowMajor()
        {
            Matrix4x4 matrix = new Matrix4x4();
            for (int row = 0; row < 4; row++)
                for (int column = 0; column < 4; column++)
                    matrix[row, column] = ReadSingle();
            return matrix;
        }

        public Color ReadD3DColor()
        {
            return new Color(ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle());
        }

        public byte[] ReadBytes(int count)
        {
            Require(count);
            byte[] result = new byte[count];
            Buffer.BlockCopy(data, offset, result, 0, count);
            offset += count;
            return result;
        }

        public string ReadString()
        {
            int length = ReadInt32();
            if (length <= 0)
                return string.Empty;
            if (length > 1_048_576)
                throw new InvalidDataException($"KO string length is invalid: {length} at {offset - 4}");
            byte[] raw = ReadBytes(length);
            return DecodeLegacyString(raw);
        }

        private static string DecodeLegacyString(byte[] raw)
        {
            try
            {
                return new UTF8Encoding(false, true).GetString(raw);
            }
            catch (DecoderFallbackException)
            {
                try
                {
                    Encoding.RegisterProvider(CodePagesEncodingProvider.Instance);
                    return Encoding.GetEncoding(949).GetString(raw);
                }
                catch
                {
                    return Encoding.GetEncoding(1252).GetString(raw);
                }
            }
        }

        private void Require(int count)
        {
            if (count < 0 || offset + count > data.Length)
                throw new EndOfStreamException(
                    $"KO binary read exceeded file bounds: offset={offset}, need={count}, length={data.Length}"
                );
        }
    }
}
