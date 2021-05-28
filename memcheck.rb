mem = Hash.new
keep = Hash.new
for line in ARGF
  case line
  when /^#mymalloc (\w+) (0x\w+)/ then
    size, ptr = $1, $2
    keep[ptr] = mem[ptr] = size
  when /^#myfree (0x\w+)/ then
    ptr = $1
    puts "bad free #{ptr}" unless mem.include?(ptr)
    mem.delete(ptr)
  end
end

