#!/usr/bin/env ruby
# send_png_mail.rb
#
# Usage:
#  send_png_mail.rb [from=addr] [to=addr] [subject=text] file1.png [...]
#
# Sends PNG files by a multipart/mixed mail

require 'base64'
require 'securerandom'

user=`whoami`.strip
from=user
to=user
subject=nil
png_files=[]

ARGV.each do |arg|
  case arg
  when /\Afrom=(.+)\z/
    from=Regexp.last_match(1)
  when /\Ato=(.+)\z/
    to=Regexp.last_match(1)
  when /\Asubj(:?ect)?=(.+)\z/
    subject=Regexp.last_match(1)
  else
    png_files.push(arg)
  end
end
abort("no png file given") if png_files.empty?
subject |= File.basename(png_files.first)

boundary="----=_Part_#{SecureRandom.hex(16)}"

mail=[]
mail << "From: #{from}\r\n"
mail << "To: #{to}\r\n"
mail << "Subject: #{subject}\r\n"
mail << "MIME-Version: 1.0\r\n"
mail << "Content-Type: multipart/mixed; boundary=#{boundary}\r\n"
mail << "\r\n"

mail << "--#{boundary}\r\n"
mail << "Content-Type: text/plain; charset=UTF-8\r\n"
mail << "Content-Transfer-Encoding: 7bit\r\n"
mail << "\r\n"
mail << "Please find attched files.\r\n"
mail << "\r\n"

png_files.each do |file|
  filename = File.basename(file)
  encoded = Base64.encode64(File.binread(file))
  mail << "--#{boundary}\r\n"
  mail << "Content-Type: image/png; name=\"#{filename}\"\r\n"
  mail << "Content-Transfer-Encoding: base64\r\n"
  mail << "Content-Disposition: attachment; filename=\"#{filename}\"\r\n"
  mail << "\r\n"
  mail << encoded
  mail << "\r\n"
end
mail << "--#{boundary}--\r\n"

IO.popen("/usr/lib/sendmail -t", "w") do |sendmail|
  sendmail.write(mail.join)
end
