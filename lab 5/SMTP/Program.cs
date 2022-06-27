using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net;
using System.Net.Mail;
using System.IO;

namespace SMTP
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.Write("Введите вашу gmail почту: ");
            string fromMail = Console.ReadLine();

            Console.Write("\nВведите имя отправителя: ");
            string fromName = Console.ReadLine();

            Console.Write("\nВведите адрес получателя: ");
            string toMail = Console.ReadLine();

            Console.Write("\nВведите имя получателя: ");
            string toName = Console.ReadLine();

            var fromAddress = new MailAddress(fromMail, fromName);
            var toAddress = new MailAddress(toMail, toName);

            Console.Write("\nВведите пароль от почты или сгенерированный пароль от двухфакторной аутентификации: ");
            string fromPassword = Console.ReadLine();

            Console.Write("\nВведите тему письма: ");
            string subject = Console.ReadLine();

            Console.WriteLine("\nВведите тело письма:");
            string body = Console.ReadLine();

            //Дополнительное задание
            //Вариант II
            DirectoryInfo directory = new DirectoryInfo(Directory.GetCurrentDirectory() + "/tests");
            FileInfo[] Files = directory.GetFiles("*.txt");

            Console.WriteLine("\nТекущая директория содержит файлы: ");
            foreach (FileInfo file in Files)
            {
                Console.WriteLine(file.Name);
            }

            Console.Write("\nВыберете по какому ключевому слову будет поиск файла: ");
            string keyWord = Console.ReadLine();

            string filePath = "";
            foreach (FileInfo file in Files)
            {
                if (file.Name.ToLower().Contains(keyWord.ToLower()))
                {
                    filePath = directory + "/" + file.Name;
                }
            }

            System.Net.Mail.Attachment attachment;
            attachment = new System.Net.Mail.Attachment(filePath);

            var smtp = new SmtpClient
            {
                Host = "smtp.gmail.com",
                Port = 587,
                EnableSsl = true,
                DeliveryMethod = SmtpDeliveryMethod.Network,
                UseDefaultCredentials = false,
                Credentials = new NetworkCredential(fromAddress.Address, fromPassword)
            };

            using (var message = new MailMessage(fromAddress, toAddress) { Subject = subject, Body = body })
            {
                message.Attachments.Add(attachment);
                smtp.Send(message);
            }

            Console.WriteLine("\nОтправлено!");
            Console.ReadLine();
        }
    }
}




