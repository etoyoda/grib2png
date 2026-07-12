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
  when /\Asubject=(.+)\z/
    subject=Regexp.last_match(1)
  else
    png_files.push(arg)
  end
end
abort("no png file given") if png_files.empty?
unless subject
  subject = File.basename(png_files.first)
end

boundary="----=_Part_#{SecureRandom.hex(16)}"
altbound="----=_Part_#{SecureRandom.hex(16)}"

mail=[]
mail << "From: #{from}"
mail << "To: #{to}"
mail << "Subject: #{subject}"
mail << "MIME-Version: 1.0"
mail << "Content-Type: multipart/mixed; boundary=\"#{boundary}\""
mail << ""

mail << "--#{boundary}"
mail << "Content-Type: multipart/alternative; boundary=\"#{altbound}\""
mail << ""

mail << "--#{altbound}"
mail << "Content-Type: text/plain; charset=UTF-8"
mail << "Content-Transfer-Encoding: 7bit"
mail << ""
mail << "Please find attched files:"
png_files.each do |f|
  mail << "* #{File.basename(f)}"
end
mail << ""

mail << "--#{altbound}"
mail << "Content-Type: text/html; charset=UTF-8"
mail << "Content-Transfer-Encoding: 7bit"
mail << ""
mail << "<html><head><title>PNG sender</title></head><body>"
mail << "<p>Please find attched files:</p>"
png_files.size.times do |i|
  mail << "<p><img src=\"cid:image#{i}\" alt=\"#{File.basename(png_files[i])}\" /></p>"
end
mail << "</body></html>"
mail << ""

mail << "--#{altbound}--"

cidno=0
png_files.each do |file|
  filename = File.basename(file)
  encoded = Base64.encode64(File.binread(file))
  mail << "--#{boundary}"
  mail << "Content-Type: image/png; name=\"#{filename}\""
  mail << "Content-Transfer-Encoding: base64"
  mail << "Content-Disposition: inline; filename=\"#{filename}\""
  mail << "Content-ID: <image#{cidno}>"
  mail << "X-Attachment-Id: image#{cidno}"
  mail << ""
  mail << encoded
  mail << ""
  cidno=cidno.succ
end
mail << "--#{boundary}--"

IO.popen("/usr/lib/sendmail -t", "w") do |sendmail|
  sendmail.write(mail.join("\r\n"))
end
