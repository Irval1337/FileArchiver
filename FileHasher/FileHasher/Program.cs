using System;
using System.IO;
using System.Security.Cryptography;

namespace FileHasher
{
    internal class Program
    {
        static void Main(string[] args)
        {
            while (true)
            {
                string path = Console.ReadLine();
                using (var md5 = MD5.Create())
                {
                    using (var stream = File.OpenRead(path))
                    {
                        var hash = BitConverter.ToString(md5.ComputeHash(stream)).Replace("-", "").ToLowerInvariant();
                        Console.WriteLine("MD5: " + hash);
                    }
                }
            }
        }
    }
}
